/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Hakan Svensson
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include "GarminxHDReceive.h"

PLUGIN_BEGIN_NAMESPACE

/*
 * This file not only contains the radar receive threads, it is also
 * the only unit that understands what the radar returned data looks like.
 * The rest of the plugin uses a (slightly) abstract definition of the radar.
 */

#define MILLIS_PER_SELECT 250
#define SECONDS_SELECT(x) ((x)*MILLISECONDS_PER_SECOND / MILLIS_PER_SELECT)

//
//
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double)DEGREES_PER_ROTATION / GARMIN_XHD_SPOKES)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)GARMIN_XHD_SPOKES / DEGREES_PER_ROTATION))

#pragma pack(push, 1)

struct radar_line {
  uint32_t packet_type;
  uint32_t len1;
  uint16_t fill_1;
  uint16_t scan_length;
  uint16_t angle;
  uint16_t fill_2;
  uint32_t range_meters;
  uint32_t display_meters;
  uint32_t fill_3[2];
  uint8_t line_data[GARMIN_XHD_MAX_SPOKE_LEN];
};

#pragma pack(pop)

// ProcessLine
// ------------
// Process one radar line, which contains exactly one line or spoke of data extending outwards
// from the radar up to the range indicated in the packet.
//
void GarminxHDReceive::ProcessFrame(const UINT8 *data, int len) {
  // log_line.time_rec = wxGetUTCTimeMillis();
  wxLongLong time_rec = wxGetUTCTimeMillis();
  time_t now = (time_t)(time_rec.GetValue() / MILLISECONDS_PER_SECOND);

  radar_line *packet = (radar_line *)data;

  wxCriticalSectionLocker lock(m_ri->m_exclusive);

  m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;
  m_ri->m_data_timeout = now + DATA_TIMEOUT;
  m_ri->m_state.Update(RADAR_TRANSMIT);

  m_ri->m_statistics.packets++;
  if (len < (int)sizeof(packet) - GARMIN_XHD_MAX_SPOKE_LEN + packet->scan_length) {
    // The packet is so small it contains no scan_lines, quit!
    m_ri->m_statistics.broken_packets++;
    return;
  }

  if (m_first_receive) {
    m_first_receive = false;
    wxLongLong startup_elapsed = wxGetUTCTimeMillis() - m_pi->GetBootMillis();
    LOG_INFO(wxT("radar_pi: %s first radar spoke received after %llu ms\n"), m_ri->m_name.c_str(), startup_elapsed);
  }

  int angle_raw = packet->angle / 8;
  int spoke = angle_raw;  // Garmin does not have radar heading, so there is no difference between spoke and angle
  m_ri->m_statistics.spokes++;
  if (m_next_spoke >= 0 && spoke != m_next_spoke) {
    if (spoke > m_next_spoke) {
      m_ri->m_statistics.missing_spokes += spoke - m_next_spoke;
    } else {
      m_ri->m_statistics.missing_spokes += GARMIN_XHD_SPOKES + spoke - m_next_spoke;
    }
  }

  m_next_spoke = (spoke + 1) % GARMIN_XHD_SPOKES;

  short int heading_raw = 0;
  double heading;
  int bearing_raw;

  heading_raw = SCALE_DEGREES_TO_RAW(m_pi->GetHeadingTrue());  // include variation
  bearing_raw = angle_raw + heading_raw;

  SpokeBearing a = MOD_SPOKES(angle_raw);
  SpokeBearing b = MOD_SPOKES(bearing_raw);

  m_ri->m_range.Update(packet->range_meters);
  m_ri->ProcessRadarSpoke(a, b, packet->line_data, packet->scan_length, packet->display_meters, time_rec);
}

SOCKET GarminxHDReceive::PickNextEthernetCard() {
  SOCKET socket = INVALID_SOCKET;
  CLEAR_STRUCT(m_interface_addr);

  // Pick the next ethernet card
  // If set, we used this one last time. Go to the next card.
  if (m_interface) {
    m_interface = m_interface->ifa_next;
  }
  // Loop until card with a valid IPv4 address
  while (m_interface && !VALID_IPV4_ADDRESS(m_interface)) {
    m_interface = m_interface->ifa_next;
  }
  if (!m_interface) {
    if (m_interface_array) {
      freeifaddrs(m_interface_array);
      m_interface_array = 0;
    }
    if (!getifaddrs(&m_interface_array)) {
      m_interface = m_interface_array;
    }
    // Loop until card with a valid IPv4 address
    while (m_interface && !VALID_IPV4_ADDRESS(m_interface)) {
      m_interface = m_interface->ifa_next;
    }
  }
  if (m_interface && VALID_IPV4_ADDRESS(m_interface)) {
    m_interface_addr.addr = ((struct sockaddr_in *)m_interface->ifa_addr)->sin_addr;
    m_interface_addr.port = 0;
  }

  socket = GetNewReportSocket();

  return socket;
}

SOCKET GarminxHDReceive::GetNewReportSocket() {
  SOCKET socket;
  wxString error;

  if (m_interface_addr.addr.s_addr == 0) {
    return INVALID_SOCKET;
  }

  error.Printf(wxT("%s reports: "), m_ri->m_name.c_str());
  socket = startUDPMulticastReceiveSocket(m_interface_addr, m_report_addr, error);
  if (socket != INVALID_SOCKET) {
    wxString addr = FormatNetworkAddress(m_interface_addr);
    wxString rep_addr = FormatNetworkAddressPort(m_report_addr);

    LOG_RECEIVE(wxT("radar_pi: %s scanning interface %s for data from %s"), m_ri->m_name.c_str(), addr.c_str(), rep_addr.c_str());

    wxString s;
    s << m_ri->m_name << wxT(": ") << _("Scanning interface") << wxT(" ") << addr;
    SetInfoStatus(s);
  } else {
    SetInfoStatus(error);
    wxLogError(wxT("radar_pi: Unable to listen to socket: %s"), error.c_str());
  }
  return socket;
}

SOCKET GarminxHDReceive::GetNewDataSocket() {
  SOCKET socket;
  wxString error;

  if (m_interface_addr.addr.s_addr == 0) {
    return INVALID_SOCKET;
  }

  error.Printf(wxT("%s data: "), m_ri->m_name.c_str());
  socket = startUDPMulticastReceiveSocket(m_interface_addr, m_data_addr, error);
  if (socket != INVALID_SOCKET) {
    wxString addr = FormatNetworkAddress(m_interface_addr);
    wxString rep_addr = FormatNetworkAddressPort(m_data_addr);

    LOG_RECEIVE(wxT("radar_pi: %s listening for data on %s from %s"), m_ri->m_name.c_str(), addr.c_str(), rep_addr.c_str());
  } else {
    SetInfoStatus(error);
    wxLogError(wxT("radar_pi: Unable to listen to socket: %s"), error.c_str());
  }
  return socket;
}

/*
 * Entry
 *
 * Called by wxThread when the new thread is running.
 * It should remain running until Shutdown is called.
 */
void *GarminxHDReceive::Entry(void) {
  int r = 0;
  int no_data_timeout = 0;
  int no_spoke_timeout = 0;
  union {
    sockaddr_storage addr;
    sockaddr_in ipv4;
  } rx_addr;
  socklen_t rx_len;

  uint8_t data[sizeof(radar_line)];
  m_interface_array = 0;
  m_interface = 0;
  struct sockaddr_in radarFoundAddr;
  sockaddr_in *radar_addr = 0;

  SOCKET dataSocket = INVALID_SOCKET;
  SOCKET reportSocket = INVALID_SOCKET;

  LOG_VERBOSE(wxT("radar_pi: GarminxHDReceive thread %s starting"), m_ri->m_name.c_str());

  if (m_interface_addr.addr.s_addr == 0) {
    reportSocket = GetNewReportSocket();
  }

  while (m_receive_socket != INVALID_SOCKET) {
    if (reportSocket == INVALID_SOCKET) {
      reportSocket = PickNextEthernetCard();
      if (reportSocket != INVALID_SOCKET) {
        no_data_timeout = 0;
        no_spoke_timeout = 0;
      }
    }
    if (radar_addr) {
      // If we have detected a radar antenna at this address start opening more sockets.
      // We do this later for 2 reasons:
      // - Resource consumption
      // - Timing. If we start processing radar data before the rest of the system
      //           is initialized then we get ordering/race condition issues.
      if (dataSocket == INVALID_SOCKET) {
        dataSocket = GetNewDataSocket();
      }
    } else {
      if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
      }
    }

    struct timeval tv = {(long)0, (long)(MILLIS_PER_SELECT * 1000)};

    fd_set fdin;
    FD_ZERO(&fdin);

    int maxFd = INVALID_SOCKET;
    if (m_receive_socket != INVALID_SOCKET) {
      FD_SET(m_receive_socket, &fdin);
      maxFd = MAX(m_receive_socket, maxFd);
    }
    if (reportSocket != INVALID_SOCKET) {
      FD_SET(reportSocket, &fdin);
      maxFd = MAX(reportSocket, maxFd);
    }
    if (dataSocket != INVALID_SOCKET) {
      FD_SET(dataSocket, &fdin);
      maxFd = MAX(dataSocket, maxFd);
    }

    wxLongLong start = wxGetUTCTimeMillis();
    r = select(maxFd + 1, &fdin, 0, 0, &tv);
    LOG_VERBOSE(wxT("radar_pi: select maxFd=%d r=%d elapsed=%lld"), maxFd, r, wxGetUTCTimeMillis() - start);

    if (r > 0) {
      if (m_receive_socket != INVALID_SOCKET && FD_ISSET(m_receive_socket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(m_receive_socket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          LOG_VERBOSE(wxT("radar_pi: %s received stop instruction"), m_ri->m_name.c_str());
          break;
        }
      }

      if (dataSocket != INVALID_SOCKET && FD_ISSET(dataSocket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(dataSocket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          ProcessFrame(data, r);
          no_data_timeout = -15;
          no_spoke_timeout = -5;
        } else {
          closesocket(dataSocket);
          dataSocket = INVALID_SOCKET;
          wxLogError(wxT("radar_pi: %s illegal frame"), m_ri->m_name.c_str());
        }
      }

      if (reportSocket != INVALID_SOCKET && FD_ISSET(reportSocket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(reportSocket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          NetworkAddress radar_address;
          radar_address.addr = rx_addr.ipv4.sin_addr;
          radar_address.port = rx_addr.ipv4.sin_port;

          if (ProcessReport(data, r)) {
            if (!radar_addr) {
              wxCriticalSectionLocker lock(m_lock);
              m_ri->DetectedRadar(m_interface_addr, radar_address);  // enables transmit data

              // the dataSocket is opened in the next loop

              radarFoundAddr = rx_addr.ipv4;
              radar_addr = &radarFoundAddr;
              m_addr = FormatNetworkAddress(radar_address);

              if (m_ri->m_state.GetValue() == RADAR_OFF) {
                LOG_INFO(wxT("radar_pi: %s detected at %s"), m_ri->m_name.c_str(), m_addr.c_str());
                m_ri->m_state.Update(RADAR_STANDBY);
              }
            }
            no_data_timeout = SECONDS_SELECT(-15);
          }
        } else {
          wxLogError(wxT("radar_pi: %s illegal report"), m_ri->m_name.c_str());
          closesocket(reportSocket);
          reportSocket = INVALID_SOCKET;
        }
      }

    } else {  // no data received -> select timeout

      if (no_data_timeout >= SECONDS_SELECT(2)) {
        no_data_timeout = 0;
        if (reportSocket != INVALID_SOCKET) {
          closesocket(reportSocket);
          reportSocket = INVALID_SOCKET;
          m_ri->m_state.Update(RADAR_OFF);
          CLEAR_STRUCT(m_interface_addr);
          radar_addr = 0;
        }
      } else {
        no_data_timeout++;
      }

      if (no_spoke_timeout >= SECONDS_SELECT(2)) {
        no_spoke_timeout = 0;
        m_ri->ResetRadarImage();
      } else {
        no_spoke_timeout++;
      }
    }

    if (reportSocket == INVALID_SOCKET) {
      // If we closed the reportSocket then close the command and data socket
      if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
      }
    }

  }  // endless loop until thread destroy

  if (dataSocket != INVALID_SOCKET) {
    closesocket(dataSocket);
  }
  if (reportSocket != INVALID_SOCKET) {
    closesocket(reportSocket);
  }
  if (m_send_socket != INVALID_SOCKET) {
    closesocket(m_send_socket);
    m_send_socket = INVALID_SOCKET;
  }
  if (m_receive_socket != INVALID_SOCKET) {
    closesocket(m_receive_socket);
  }

  if (m_interface_array) {
    freeifaddrs(m_interface_array);
  }

#ifdef TEST_THREAD_RACES
  LOG_VERBOSE(wxT("radar_pi: %s receive thread sleeping"), m_ri->m_name.c_str());
  wxMilliSleep(1000);
#endif
  LOG_VERBOSE(wxT("radar_pi: %s receive thread stopping"), m_ri->m_name.c_str());
  m_is_shutdown = true;
  return 0;
}

/*
 RADAR REPORTS

 The radars send various reports.

 */

#pragma pack(push, 1)

typedef struct {
  uint32_t packet_type;
  uint32_t len1;
  uint8_t parm1;
} rad_ctl_pkt_9;

typedef struct {
  uint32_t packet_type;
  uint32_t len1;
  uint16_t parm1;
} rad_ctl_pkt_10;

typedef struct {
  uint32_t packet_type;
  uint32_t len1;
  uint32_t parm1;
} rad_ctl_pkt_12;

typedef struct {
  uint32_t packet_type;
  uint32_t len1;
  uint32_t parm1;
  uint32_t parm2;
  uint16_t parm3;
  uint16_t parm4;
  uint8_t parm5;
  uint8_t parm6;
  uint16_t parm7;
} rad_respond_pkt_16;

#pragma pack(pop)

bool GarminxHDReceive::UpdateScannerStatus(int status) {
  bool ret = true;

  if (status != m_radar_status) {
    m_radar_status = status;

    wxString stat;
    time_t now = time(0);

    switch (m_radar_status) {
      case 1:
        m_ri->m_state.Update(RADAR_WARMING_UP);
        LOG_VERBOSE(wxT("radar_pi: %s reports status WARMUP"), m_ri->m_name.c_str());
        stat = _("Warmup");
        break;
      case 3:
        m_ri->m_state.Update(RADAR_STANDBY);
        LOG_VERBOSE(wxT("radar_pi: %s reports status STANDBY"), m_ri->m_name.c_str());
        stat = _("Standby");
        break;
      case 4:
        m_ri->m_state.Update(RADAR_SPINNING_UP);
        m_ri->m_data_timeout = now + DATA_TIMEOUT;
        LOG_VERBOSE(wxT("radar_pi: %s reports status SPINNING UP"), m_ri->m_name.c_str());
        stat = _("Spinning up");
        break;
      case 5:
        m_ri->m_state.Update(RADAR_TRANSMIT);
        LOG_VERBOSE(wxT("radar_pi: %s reports status TRANSMIT"), m_ri->m_name.c_str());
        stat = _("Transmit");
        break;
      default:
        stat << _("Unknown status") << wxString::Format(wxT(" %d"), m_radar_status);
        ret = false;
        break;
    }
    SetInfoStatus(wxString::Format(wxT("%s IP %s %s"), m_ri->m_name.c_str(), m_addr.c_str(), stat.c_str()));
  }
  return ret;
}

bool GarminxHDReceive::ProcessReport(const UINT8 *report, int len) {
  LOG_BINARY_RECEIVE(wxT("ProcessReport"), report, len);

  time_t now = time(0);

  m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;

  if (len >= sizeof(rad_ctl_pkt_9)) {  //  sizeof(rad_respond_pkt_9)) {
    rad_ctl_pkt_9 *packet9 = (rad_ctl_pkt_9 *)report;
    rad_ctl_pkt_10 *packet10 = (rad_ctl_pkt_10 *)report;
    rad_ctl_pkt_12 *packet12 = (rad_ctl_pkt_12 *)report;
    uint16_t packet_type = packet9->packet_type;

    switch (packet_type) {
      case 0x0916:  // Dome Speed
        LOG_VERBOSE(wxT("scan speed %d"), packet9->parm1);
        m_ri->m_scan_speed.Update(packet9->parm1 >> 1);
        return true;

      case 0x0919:  // Standby/Transmit
                    // parm1 = 0 : Standby request
                    // parm1 = 1 : TX request
                    // Ignored, gxradar did nothing with this
        LOG_VERBOSE(wxT("standby/transmit %d"), packet9->parm1);
        return true;

      case 0x091d:  // Auto Gain Mode
        LOG_VERBOSE(wxT("auto-gain mode %d"), packet9->parm1);
        switch (packet9->parm1) {
          case 0:
            m_ri->m_gain.Update(AUTO_RANGE - 2);  // AUTO HIGH
            return true;

          case 1:
            m_ri->m_gain.Update(AUTO_RANGE - 1);  // AUTO LOW
            return true;

          default:
            break;
        }
        break;

      case 0x091e:                              // Range
        LOG_VERBOSE(wxT("range %d"), packet12->parm1);
        m_ri->m_range.Update(packet12->parm1);  // Range in meters
        return true;

      case 0x0925:  // Gain
        LOG_VERBOSE(wxT("gain %d"), packet10->parm1);
        m_ri->m_gain.Update(packet10->parm1 / 100);
        return true;

      case 0x0930:  // Dome offset, called bearing alignment here
        LOG_VERBOSE(wxT("bearing alignment %d"), packet12->parm1 / 32);
        m_ri->m_bearing_alignment.Update(packet12->parm1 / 32);
        return true;

      case 0x0932:  // Crosstalk reject, I guess this is the same as interference rejection?
        LOG_VERBOSE(wxT("crosstalk/interference rejection %d"), packet9->parm1);
        m_ri->m_interference_rejection.Update(packet9->parm1);
        return true;

      case 0x0933:  // Rain clutter mode
        LOG_VERBOSE(wxT("rain mode %d"), packet9->parm1);
        m_ri->m_rain.Update(AUTO_RANGE - packet9->parm1);
        return true;

      case 0x0934: {
        // Rain clutter level
        LOG_VERBOSE(wxT("rain clutter %d"), packet10->parm1);
        m_ri->m_rain.Update(packet10->parm1 / 100);
        return true;
      }

      case 0x0939: {
        // Sea Clutter On/Off
        LOG_VERBOSE(wxT("sea mode %d"), packet9->parm1);
        switch (packet9->parm1) {
          case 0: {
            // No sea clutter
            m_ri->m_sea.Update(0);
            return true;
          }
          case 1: {
            // Manual sea clutter, value set via report 0x093a
            return true;
          }
          case 2: {
            // Auto sea clutter
            m_ri->m_sea.Update(AUTO_RANGE - 1);
            return true;
          }
        }
        break;
      }

      case 0x093a: {
        // Sea Clutter level
        LOG_VERBOSE(wxT("sea clutter %d"), packet10->parm1);
        m_ri->m_sea.Update(packet10->parm1 / 100);
        return true;
      }

      // TODO no-xmit zone
        
      case 0x0992: {
        // Scanner state
        if (UpdateScannerStatus(packet9->parm1)) {
          return true;
        }
      }

      case 0x0993: {
        // Warmup
        m_ri->m_warmup.Update(packet9->parm1);
        return true;
      }


    }
  }

  if (m_pi->m_settings.verbose >= 2) {
    LOG_BINARY_RECEIVE(wxT("received unknown message"), report, len);
  }
  return false;
}

// Called from the main thread to stop this thread.
// We send a simple one byte message to the thread so that it awakens from the select() call with
// this message ready for it to be read on 'm_receive_socket'. See the constructor in GarminxHDReceive.h
// for the setup of these two sockets.

void GarminxHDReceive::Shutdown() {
  if (m_send_socket != INVALID_SOCKET) {
    m_shutdown_time_requested = wxGetUTCTimeMillis();
    if (send(m_send_socket, "!", 1, MSG_DONTROUTE) > 0) {
      LOG_VERBOSE(wxT("radar_pi: %s requested receive thread to stop"), m_ri->m_name.c_str());
      return;
    }
  }
  LOG_INFO(wxT("radar_pi: %s receive thread will take long time to stop"), m_ri->m_name.c_str());
}

wxString GarminxHDReceive::GetInfoStatus() {
  wxCriticalSectionLocker lock(m_lock);
  // Called on the UI thread, so be gentle

  return m_status;
}

PLUGIN_END_NAMESPACE
