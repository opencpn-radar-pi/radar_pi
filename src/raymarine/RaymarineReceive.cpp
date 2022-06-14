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
 *           Andrei Bankovs: Raymarine radars
 *           Martin Hassellov: testing the Raymarine radar
 *           Matt McShea: testing the Raymarine radar
 *           Al Grant: testing and decoding Quantum radar
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

#include "RaymarineReceive.h"

#include "MessageBox.h"
#include "RME120Control.h"

PLUGIN_BEGIN_NAMESPACE

/*
 * This file not only contains the radar receive threads, it is also
 * the only unit that understands what the radar returned data looks like.
 * The rest of the plugin uses a (slightly) abstract definition of the radar.
 */

#define MILLIS_PER_SELECT 250
#define SECONDS_SELECT(x) ((x)*MILLISECONDS_PER_SECOND / MILLIS_PER_SELECT)
#define MOD_ROTATION2048(raw) (((raw) + 2 * LINES_PER_ROTATION) % LINES_PER_ROTATION)
#define LINES_PER_ROTATION (2048)  // with lower ranges only 1024 lines used
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)SPOKES / DEGREES_PER_ROTATION))

//
// Raymarine radars use an internal spoke ID that has range [0..4096> but they
// only send half of them
//
#define SPOKES (4096)
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double)DEGREES_PER_ROTATION / SPOKES)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)SPOKES / DEGREES_PER_ROTATION))

#define HEADING_TRUE_FLAG 0x4000
#define HEADING_MASK (SPOKES - 1)
#define HEADING_VALID(x) (((x) & ~(HEADING_TRUE_FLAG | HEADING_MASK)) == 0)

#define IS_MULTICAST(x) (((x)&0xf0) == 224)

SOCKET RaymarineReceive::PickNextEthernetCard() {
  SOCKET socket = INVALID_SOCKET;
  m_interface_addr = NetworkAddress();

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

  if (m_ri->m_radar_type == RM_QUANTUM && !m_info.report_addr.IsNull() && !IS_MULTICAST(m_info.report_addr.addr.s_addr)) {
    LOG_INFO(wxT("%s Creating unicast socket for radar at IP %s [%s]"), m_ri->m_name,
             m_ri->m_radar_address.FormatNetworkAddressPort(), m_info.to_string());

    socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket != INVALID_SOCKET) {
      int one = 1;
      setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    }
  } else {
    socket = GetNewReportSocket();
    LOG_INFO(wxT("%s Creating multicast socket for radar at IP %s [%s] - %d"), m_ri->m_name,
             m_ri->m_radar_address.FormatNetworkAddressPort(), m_info.to_string(), socket);
  }
  return socket;
}

SOCKET RaymarineReceive::GetNewReportSocket() {
  SOCKET socket;
  wxString error = wxT(" ");
  wxString s = wxT(" ");

  if (!(m_info == m_ri->GetRadarLocationInfo())) {  // initial values or RaymarineLocate modified the info
    LOG_INFO(wxT("%s old radar address at IP %s [%s]"), m_ri->m_name, m_ri->m_radar_address.FormatNetworkAddressPort(),
             m_info.to_string());
    m_info = m_ri->GetRadarLocationInfo();

    m_interface_addr = m_ri->GetRadarInterfaceAddress();
    UpdateSendCommand();
    LOG_INFO(wxT("%s Locator found radar at IP %s [%s]"), m_ri->m_name, m_ri->m_radar_address.FormatNetworkAddressPort(),
             m_info.to_string());
  };

  if (m_interface_addr.IsNull()) {
    LOG_RECEIVE(wxT("%s no interface address to listen on"), m_ri->m_name);
    wxMilliSleep(1000);
    return INVALID_SOCKET;
  }
  if (m_info.report_addr.IsNull()) {
    LOG_RECEIVE(wxT("%s no report address to listen on"), m_ri->m_name);
    wxMilliSleep(1000);
    return INVALID_SOCKET;
  }

  if (RadarOrder[m_ri->m_radar_type] >= RO_PRIMARY) {
    if (!m_info.serialNr.IsNull()) {
      s << _("Serial #") << m_info.serialNr << wxT("\n");
    }
  }

  socket = startUDPMulticastReceiveSocket(m_interface_addr, m_info.report_addr, error);
  wxString addr = m_interface_addr.FormatNetworkAddress();
  wxString rep_addr = m_info.report_addr.FormatNetworkAddressPort();
  if (socket != INVALID_SOCKET) {
    LOG_RECEIVE(wxT("%s scanning interface %s for data from %s"), m_ri->m_name, addr.c_str(), rep_addr.c_str());

    s << _("Scanning interface") << wxT(" ") << addr;
    SetInfoStatus(s);
  } else {
    s << _("Interface ") << addr << " error: " << error;
    SetInfoStatus(s);
    wxLogError(wxT("%s Unable to listen to socket: %s"), m_ri->m_name, error.c_str());
    LOG_RECEIVE(wxT("%s invalid socket interface= %s for reportaddr= %s"), m_ri->m_name, addr.c_str(), rep_addr.c_str());
  }
  return socket;
}

/*
 * Entry
 *
 * Called by wxThread when the new thread is running.
 * It should remain running until Shutdown is called.
 */
void *RaymarineReceive::Entry(void) {
  int r = 0;
  int no_data_timeout = 0;
  int no_spoke_timeout = 0;
  union {
    sockaddr_storage addr;
    sockaddr_in ipv4;
  } rx_addr;
  socklen_t rx_len;

  uint8_t data[2048];  // largest packet seen so far from a Raymarine is 626
  m_interface_array = 0;
  m_interface = 0;
  struct sockaddr_in radarFoundAddr;
  sockaddr_in *radar_addr = 0;
  time_t last_keepalive = time(0);

  LOG_VERBOSE(wxT("RamarineReceive thread %s starting"), m_ri->m_name.c_str());
  if (!m_info.report_addr.IsNull() && (m_ri->m_radar_type != RM_QUANTUM || IS_MULTICAST(m_info.report_addr.addr.s_addr))) {
    LOG_VERBOSE(wxT("%s Creating multicast socket at the beginning %s"), m_ri->m_name.c_str(),
                m_info.report_addr.FormatNetworkAddressPort());
    m_comm_socket = GetNewReportSocket();  // Start using the same interface_addr as previous time
  }

  while (m_receive_socket != INVALID_SOCKET) {
    if (m_comm_socket == INVALID_SOCKET && !m_info.report_addr.IsNull()) {
      LOG_VERBOSE(wxT("%s Got report_addr %08x"), m_ri->m_name.c_str(), m_info.report_addr.addr.s_addr);
      if (m_ri->m_radar_type == RM_QUANTUM && !IS_MULTICAST(m_info.report_addr.addr.s_addr)) {
        LOG_INFO(wxT("Entry %s Creating unicast socket for radar at IP %s [%s]"), m_ri->m_name,
                 m_ri->m_radar_address.FormatNetworkAddressPort(), m_info.to_string());
        m_comm_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_comm_socket != INVALID_SOCKET) {
          int one = 1;
          setsockopt(m_comm_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
          m_ri->m_control->RadarStayAlive();
          last_keepalive = time(0);
        }
      } else {
        m_comm_socket = PickNextEthernetCard();
      }

      if (m_comm_socket != INVALID_SOCKET) {
        no_data_timeout = 0;
        no_spoke_timeout = 0;
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
    if (m_comm_socket != INVALID_SOCKET) {
      FD_SET(m_comm_socket, &fdin);
      maxFd = MAX(m_comm_socket, maxFd);
    }
    r = select(maxFd + 1, &fdin, 0, 0, &tv);
    if (r > 0) {
      if (m_receive_socket != INVALID_SOCKET && FD_ISSET(m_receive_socket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(m_receive_socket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          LOG_VERBOSE(wxT("%s received stop instruction"), m_ri->m_name.c_str());
          break;
        }
      }

      if (m_comm_socket != INVALID_SOCKET && FD_ISSET(m_comm_socket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(m_comm_socket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          NetworkAddress radar_address;
          radar_address.addr = rx_addr.ipv4.sin_addr;
          radar_address.port = rx_addr.ipv4.sin_port;

          ProcessFrame(data, (size_t)r);
          if (!radar_addr) {
            wxCriticalSectionLocker lock(m_lock);
            m_ri->DetectedRadar(m_interface_addr,
                                radar_address);  // enables transmit data, if radar multicast address is also known
            UpdateSendCommand();

            radarFoundAddr = rx_addr.ipv4;
            radar_addr = &radarFoundAddr;

            if (m_ri->m_state.GetValue() == RADAR_OFF) {
              LOG_INFO(wxT("%s detected at %s"), m_ri->m_name.c_str(), radar_address.FormatNetworkAddress());
              m_ri->m_state.Update(RADAR_STANDBY);
            }
          }
          no_data_timeout = SECONDS_SELECT(-15);
        }
      }

    } else {  // no data received -> select timeout
      LOG_INFO(wxT("%s RaymarineReceive receive timeout %d"), m_ri->m_name.c_str(), no_data_timeout);
      if (no_data_timeout >= SECONDS_SELECT(2)) {
        no_data_timeout = 0;
        if (m_comm_socket != INVALID_SOCKET) {
          if (m_ri->m_radar_type != RM_QUANTUM || IS_MULTICAST(m_info.report_addr.addr.s_addr)) {
            closesocket(m_comm_socket);
            m_comm_socket = INVALID_SOCKET;
            m_interface_addr = NetworkAddress();
            radar_addr = 0;
          }
          m_ri->m_state.Update(RADAR_OFF);
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

    if (!(m_info == m_ri->GetRadarLocationInfo())) {
      m_info = m_ri->GetRadarLocationInfo();
      LOG_INFO(wxT("%s RaymarineReceive updating radar location %s socket %d"), m_ri->m_name.c_str(), m_info.to_string(),
               m_comm_socket);
      if ((m_ri->m_radar_type != RM_QUANTUM || IS_MULTICAST(m_info.report_addr.addr.s_addr)) && m_comm_socket != INVALID_SOCKET) {
        closesocket(m_comm_socket);
        m_comm_socket = INVALID_SOCKET;
      } else {
        m_ri->m_control->RadarStayAlive();
        no_data_timeout = 0;
        no_spoke_timeout = 0;
      }
    }

    if (m_comm_socket != INVALID_SOCKET) {
      // If we closed the m_comm_socket then close the command socket
      if (time(0) > last_keepalive) {
        m_ri->m_control->RadarStayAlive();
        last_keepalive = time(0);
      }
    }
  }  // endless loop until thread destroy

  LOG_VERBOSE(wxT("%s received stop instruction, stopping"), m_ri->m_name.c_str());

  if (m_comm_socket != INVALID_SOCKET) {
    closesocket(m_comm_socket);
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
  LOG_VERBOSE(wxT("%s receive thread sleeping"), m_ri->m_name.c_str());
  wxMilliSleep(1000);
#endif
  m_is_shutdown = true;
  LOG_VERBOSE(wxT("%s received stop instruction, shutting down"), m_ri->m_name.c_str());

  return 0;
}

void RaymarineReceive::ProcessFrame(const UINT8 *data, size_t len) {  // This is the original ProcessFrame from RMradar_pi
  time_t now = time(0);
  wxString MOD_serial;
  wxString IF_serial;
  wxString s;
  RadarLocationInfo info;
  int status;
  wxString stat;
  // LOG_BINARY_RECEIVE(wxT("received frame"), data, len);
  m_ri->resetTimeout(now);
  m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;
  m_ri->m_statistics.packets++;
  if (len >= 4) {
    uint32_t msgId = 0;
    memcpy(&msgId, data, sizeof(msgId));
    switch (msgId) {
      case 0x00010001:
        ProcessRMReport(data, len);
        break;
      case 0x00010002:
        ProcessFixedReport(data, len);
        break;
      case 0x00010003:
        ProcessScanData(data, len);
        m_ri->m_data_timeout = now + DATA_TIMEOUT;
        break;
      case 0x00280003:
        ProcessQuantumScanData(data, len);
        m_ri->m_data_timeout = now + DATA_TIMEOUT;
        break;
      case 0x00280002:
        ProcessQuantumReport(data, len);
        m_ri->m_data_timeout = now + DATA_TIMEOUT;
        break;
      case 0x00280001:  // type and serial for Quantum radar
        LOG_BINARY_RECEIVE(wxT("received frame 0x00280001"), data, len);
        IF_serial = wxString::FromAscii(data + 10, 7);
        MOD_serial = wxString::FromAscii(data + 4, 6);
        if (MOD_serial == _("E70498")) {
          m_ri->m_quantum2type = true;
        }
        m_ri->m_radar_location_info.serialNr = IF_serial;
        status = m_ri->m_state.GetValue();

        switch (status) {
          case RADAR_OFF:
            LOG_VERBOSE(wxT("%s reports status RADAR_OFF"), m_ri->m_name.c_str());
            stat = _("Off");
            break;

          case RADAR_STANDBY:
            LOG_VERBOSE(wxT("%s reports status STANDBY"), m_ri->m_name.c_str());
            stat = _("Standby");
            break;

          case RADAR_WARMING_UP:
            LOG_VERBOSE(wxT("%s reports status RADAR_WARMING_UP"), m_ri->m_name.c_str());
            stat = _("Warming up");
            break;

          case RADAR_TRANSMIT:
            LOG_VERBOSE(wxT("%s reports status RADAR_TRANSMIT"), m_ri->m_name.c_str());
            stat = _("Transmit");
            break;

          default:
            // LOG_BINARY_RECEIVE(wxT("received unknown radar status"), report, len);
            stat = _("Unknown status");
            break;
        }

        s = wxString::Format(wxT("IP %s %s"), m_ri->m_radar_address.FormatNetworkAddress(), stat.c_str());
        info = m_ri->GetRadarLocationInfo();
        s << wxT("\n") << _("SKU ") << MOD_serial << _(" Serial #") << info.serialNr;
        SetInfoStatus(s);
        break;

      case 0x00010006:
        LOG_BINARY_RECEIVE(wxT("received frame 0x00010006"), data, len);
        IF_serial = wxString::FromAscii(data + 4, 7);
        MOD_serial = wxString::FromAscii(data + 20, 7);
        m_info = m_ri->GetRadarLocationInfo();
        m_ri->m_radar_location_info.serialNr = IF_serial;
        break;
      case 0x00018801:  // HD radar
        ProcessRMReport(data, len);
        break;
      case 0x00010007:
      case 0x00010008:
      case 0x00010009:
      case 0x00018942:
        LOG_BINARY_RECEIVE(wxT("received frame other"), data, len);
        ;
      default:
        LOG_BINARY_RECEIVE(wxT("received frame default"), data, len);
        break;
    }
  }
}

void RaymarineReceive::UpdateSendCommand() {
  if (!m_info.send_command_addr.IsNull() && m_ri->m_control) {
    RME120Control *control = (RME120Control *)m_ri->m_control;
    control->SetSendAddress(m_info.send_command_addr);
  }
}

#pragma pack(push, 1)

struct RMRadarReport {
  uint32_t field01;       // 0x010001  // 0-3
  uint32_t ranges[11];    // 4 - 47
  uint32_t fieldx_1[33];  // 48 -

  uint8_t status;       // 2 - warmup, 1 - transmit, 0 - standby, 6 - shutting down (warmup time - countdown), 3 - shutdown  // 180
  uint8_t fieldx_2[3];  // 181
  uint8_t warmup_time;  // 184
  uint8_t signal_strength;  // number of bars   // 185

  uint8_t fieldx_3[7];   // 186
  uint8_t range_id;      // 193
  uint8_t fieldx_4[2];   // 194
  uint8_t auto_gain;     // 196
  uint8_t fieldx_5[3];   // 197
  uint32_t gain;         // 200
  uint8_t auto_sea;      // 0 - disabled; 1 - harbour, 2 - offshore, 3 - coastal   // 204
  uint8_t fieldx_6[3];   // 205
  uint8_t sea_value;     // 208
  uint8_t rain_enabled;  // 209
  uint8_t fieldx_7[3];   // 210
  uint8_t rain_value;    // 213
  uint8_t ftc_enabled;   // 214
  uint8_t fieldx_8[3];   // 215
  uint8_t ftc_value;     // 218
  uint8_t auto_tune;
  uint8_t fieldx_9[3];
  uint8_t tune;
  int16_t bearing_offset;  // degrees * 10; left - negative, right - positive
  uint8_t interference_rejection;
  uint8_t fieldx_10[3];
  uint8_t target_expansion;
  uint8_t fieldx_11[13];
  uint8_t mbs_enabled;  // Main Bang Suppression enabled if 1
};

struct QuantumControls {
  uint8_t gain_auto;        // @ 0
  uint8_t gain;             // @ 1
  uint8_t color_gain_auto;  // @ 2
  uint8_t color_gain;       // @ 3
  uint8_t sea_auto;         // @ 4
  uint8_t sea;              // @ 5
  uint8_t rain_auto;        // @ 6
  uint8_t rain;             // @ 7
};

struct QuantumRadarReport {
  uint32_t type;                   // @0 0x280002
  uint8_t status;                  // @4 0 - standby ; 1 - transmitting
  uint8_t something_1[9];          // @5
  int16_t bearing_offset;          // @14
  uint8_t something_14;            // @16
  uint8_t interference_rejection;  // @17
  uint8_t something_13[2];         // @18
  uint8_t range_index;             // @20
  uint8_t mode;                    // @21 harbor - 0, coastal - 1, offshore - 2, weather - 3
  QuantumControls controls[4];     // @22 controls indexed by mode
  uint8_t target_expansion;        // @54
  uint8_t something_9;             // @55
  uint8_t something_10[3];         // @56
  uint8_t mbs_enabled;             // @59
  uint8_t something_11[88];        // @60
  uint32_t ranges[20];             // @148
  uint32_t something_12[8];        // @228
};

struct RMRadarFixedReport {
  uint32_t field01;           // 0x010002
  uint8_t fieldx_1[213];      // 221 - magnetron current; 233, 234 - rotation time ms (251 total) // 4
  uint16_t magnetron_time;    // 217
  uint8_t fieldx_2[6];        // 219
  uint8_t magnetron_current;  // 225
  uint8_t fieldx_3[11];       // 226
  uint16_t rotation_time;     // 237
  uint8_t fieldx_4[13];       // 239
  uint8_t fieldx_41;          // 252
  uint8_t fieldx_5[2];        // 253
  uint8_t fieldx_42[3];       // 255
  uint8_t fieldx_43[3];       // 0, 1, 2 - fine tune value for SP, MP, LP  // 258
  uint8_t fieldx_6[6];        // 261
  uint8_t display_timing;     // 267
  uint8_t fieldx_7[12];       // 268
  uint8_t fieldx_71;          // 280
  uint8_t fieldx_8[12];
  uint8_t gain_min;
  uint8_t gain_max;
  uint8_t sea_min;
  uint8_t sea_max;
  uint8_t rain_min;
  uint8_t rain_max;
  uint8_t ftc_min;
  uint8_t ftc_max;
  uint8_t fieldx_81;
  uint8_t fieldx_82;
  uint8_t fieldx_83;
  uint8_t fieldx_84;
  uint8_t signal_strength_value;
  uint8_t fieldx_9[2];
};

#pragma pack(pop)

static bool s_print_range = true;

void RaymarineReceive::ProcessQuantumReport(const UINT8 *data, int len) {
  QuantumRadarReport *bl_pter = (QuantumRadarReport *)data;
  wxString s;

  LOG_BINARY_REPORTS(wxString::Format(wxT("%s QuantumReport"), m_ri->m_name.c_str()), data, len);

  switch (bl_pter->status) {
    case 0:
      LOG_RECEIVE(wxT("%s received transmit off from %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
      m_ri->m_state.Update(RADAR_STANDBY);
      break;
    case 1:
      LOG_RECEIVE(wxT("%s received transmit on from %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
      m_ri->m_state.Update(RADAR_TRANSMIT);
      break;
    case 2:  // Warmup, but don't know if Quantum has these states
      LOG_RECEIVE(wxT("%s radar is warming up %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
      m_ri->m_state.Update(RADAR_WARMING_UP);
      break;
    case 3:  // Off
      LOG_RECEIVE(wxT("%s radar is off %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
      m_ri->m_state.Update(RADAR_OFF);
      break;
    default:
      m_ri->m_state.Update(RADAR_STANDBY);
      LOG_RECEIVE(wxT("%s radar is unknown state %s, state= %i"), m_ri->m_name.c_str(), bl_pter->status);
      break;
  }

  for (int i = 0; i < 20; i++) {
    m_ri->m_radar_ranges[i] = (int)(1.852 * (double)bl_pter->ranges[i]);
    if (s_print_range) {
      LOG_RECEIVE(wxT("received range= %i, radar_ranges=  %d "), bl_pter->ranges[i], m_ri->m_radar_ranges[i]);
    }
  }
  s_print_range = false;
  if (bl_pter->ranges[2] == 125) {
    M_SETTINGS.range_units = RANGE_NAUTIC;  // We don't know yet how Raymarine switches range units
  } else if (bl_pter->ranges[0] == 135) {   // Raymarine has no RANGE_MIXED
                                            // Ray marine alse has units statue miles, not supported by radar_pi
    M_SETTINGS.range_units = RANGE_METRIC;
  } else {
    LOG_INFO(wxT("Other range units found, bl_pter->ranges[0]= %i"), bl_pter->ranges[0]);
    M_SETTINGS.range_units = RANGE_NAUTIC;  // to be adapted to RANGE_MIXED if that exists with Raymarine
  }

  int range_id = bl_pter->range_index;
  LOG_VERBOSE(wxT("range index=%i, range = %i"), range_id, m_ri->m_radar_ranges[range_id]);
  if ((m_ri->m_radar_ranges[range_id] * 2) != m_range_meters) {
    if (m_pi->m_settings.verbose >= 1) {
      LOG_RECEIVE(wxT("%s now scanning with range %d meters (was %d meters)"), m_ri->m_name.c_str(),
                  m_ri->m_radar_ranges[range_id] * 2, m_range_meters);
    }
    if (m_ri->m_radar_ranges[range_id] > 0) {
      m_range_meters = m_ri->m_radar_ranges[range_id] * 2;  // displayed values are half of scanned values
      m_updated_range = true;
      m_ri->m_range.Update(m_range_meters / 2);  // RM MFD shows half of what is received
      LOG_VERBOSE(wxT("m_range updated to %i"), (m_range_meters / 2));
    } else {
      m_range_meters = 1;  // prevent accidents down the road
      return;
    }
  }

  int mode_idx = bl_pter->mode;
  m_ri->m_mode.Update(bl_pter->mode);
  LOG_RECEIVE(wxT("mode updated received= %i, displayed = %i"), bl_pter->mode, m_ri->m_mode.GetValue());

  if (mode_idx < 0 || mode_idx > 3) {
    LOG_RECEIVE(wxT("Invalid mode value %d, using 0 for control values"), bl_pter->mode);
    mode_idx = 0;
  }

  RadarControlState state;
  state = (RadarControlState)bl_pter->controls[mode_idx].gain_auto;
  m_ri->m_gain.Update(bl_pter->controls[mode_idx].gain);
  m_ri->m_gain.UpdateState(state);
  LOG_RECEIVE(wxT("gain updated received1= %i, displayed = %i state= %i"), bl_pter->controls[mode_idx].gain,
              m_ri->m_gain.GetValue(), state);

  state = (RadarControlState)bl_pter->controls[mode_idx].color_gain_auto == 0 ? RCS_MANUAL : RCS_AUTO_1;
  m_ri->m_color_gain.Update(bl_pter->controls[mode_idx].color_gain);
  m_ri->m_color_gain.UpdateState(state);
  LOG_RECEIVE(wxT("color gain updated received1= %i, displayed = %i state= %i"), bl_pter->controls[mode_idx].color_gain,
              m_ri->m_color_gain.GetValue(), state);

  state = (RadarControlState)bl_pter->controls[mode_idx].sea_auto;  // we don't know how many auto states there are....
  m_ri->m_sea.Update(bl_pter->controls[mode_idx].sea);
  m_ri->m_sea.UpdateState(state);
  LOG_RECEIVE(wxT("sea updated received= %i, displayed = %i, state=%i"), bl_pter->controls[mode_idx].sea, m_ri->m_sea.GetValue(),
              state);

  state = bl_pter->controls[mode_idx].rain_auto == 0 ? RCS_OFF : RCS_MANUAL;
  LOG_RECEIVE(wxT("rain state=%i bl_pter->rain_auto=%i"), state, bl_pter->controls[mode_idx].rain_auto);
  m_ri->m_rain.Update(bl_pter->controls[mode_idx].rain);
  m_ri->m_rain.UpdateState(state);
  LOG_RECEIVE(wxT("rain updated received= %i, displayed = %i state=%i"), bl_pter->controls[mode_idx].rain, m_ri->m_rain.GetValue(),
              state);

  if (bl_pter->controls[mode_idx].gain_auto && bl_pter->controls[mode_idx].sea_auto &&
      !bl_pter->controls[mode_idx].rain_auto) {  // all to auto
    m_ri->m_all_to_auto.Update(1);
  } else {
    m_ri->m_all_to_auto.Update(0);
  }

  m_target_expansion = (bl_pter->target_expansion != 0 ? true : false);
  m_ri->m_target_expansion.Update(bl_pter->target_expansion);
  LOG_RECEIVE(wxT("target_expansion updated received= %i, displayed = %i"), bl_pter->target_expansion,
              m_ri->m_target_expansion.GetValue());

  m_ri->m_interference_rejection.Update(bl_pter->interference_rejection);

  int ba = (int)bl_pter->bearing_offset;
  m_ri->m_bearing_alignment.Update(ba);

  state = (bl_pter->mbs_enabled > 0) ? RCS_MANUAL : RCS_OFF;
  m_ri->m_main_bang_suppression.Update(bl_pter->mbs_enabled, RCS_MANUAL);
}

void RaymarineReceive::ProcessRMReport(const UINT8 *data, int len) {
  RMRadarReport *bl_pter = (RMRadarReport *)data;
  wxString s;
  bool HDtype = bl_pter->field01 == 0x00018801;

  LOG_BINARY_REPORTS(wxString::Format(wxT("%s RMReport"), m_ri->m_name.c_str()), data, len);

  if (bl_pter->field01 == 0x00018801 || bl_pter->field01 == 0x010001) {  // HD radar or analog
    switch (bl_pter->status) {
      case 0:
        LOG_RECEIVE(wxT("%s received transmit off from %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
        m_ri->m_state.Update(RADAR_STANDBY);
        break;
      case 1:
        LOG_RECEIVE(wxT("%s received transmit on from %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
        m_ri->m_state.Update(RADAR_TRANSMIT);
        break;
      case 2:  // Warmup
        LOG_RECEIVE(wxT("%s radar is warming up %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
        m_ri->m_state.Update(RADAR_WARMING_UP);
        break;
      case 3:  // Off
        LOG_RECEIVE(wxT("%s radar is off %s"), m_ri->m_name.c_str(), "--" /*addr.c_str()*/);
        m_ri->m_state.Update(RADAR_OFF);
        break;
      default:
        m_ri->m_state.Update(RADAR_STANDBY);
        break;
    }
    if (bl_pter->ranges[0] == 125) {
      M_SETTINGS.range_units = RANGE_NAUTIC;  // We don't know yet how Raymarine switches range units
    } else if (bl_pter->ranges[0] == 135) {   // Raymarine has no RANGE_MIXED
                                              // Ray marine alse has units statue miles, not supported by radar_pi
      M_SETTINGS.range_units = RANGE_METRIC;
    } else {
      LOG_INFO(wxT("Other range units found, bl_pter->ranges[0]= %i"), bl_pter->ranges[0]);
      M_SETTINGS.range_units = RANGE_NAUTIC;  // to be adapted to RANGE_MIXED if that exists with Raymarine
    }

    for (int i = 0; i < 11; i++) {
      m_ri->m_radar_ranges[i] = (int)(1.852 * (double)bl_pter->ranges[i]);
      if (s_print_range) {
        LOG_RECEIVE(wxT("received range= %i, radar_ranges=  %d "), bl_pter->ranges[i], m_ri->m_radar_ranges[i]);
      }
    }
    s_print_range = false;
    int range_id;
    if (HDtype) {
      range_id = data[296];
    } else {
      range_id = bl_pter->range_id;
    }

    if ((m_ri->m_radar_ranges[range_id] * 2) != m_range_meters) {
      if (m_pi->m_settings.verbose >= 1) {
        LOG_RECEIVE(wxT("%s now scanning with range %d meters (was %d meters)"), m_ri->m_name.c_str(),
                    m_ri->m_radar_ranges[range_id] * 2, m_range_meters);
      }
      if (m_ri->m_radar_ranges[range_id] > 0) {
        m_range_meters = m_ri->m_radar_ranges[range_id] * 2;  // displayed values are half of scanned values
        m_updated_range = true;
        m_ri->m_range.Update(m_range_meters / 2);  // RM MFD shows half of what is received
        LOG_RECEIVE(wxT("m_range updated to %i"), (m_range_meters / 2));
      } else {
        m_range_meters = 1;  // prevent accidents down the road
        return;
      }
    }

    RadarControlState state;
    state = (bl_pter->auto_gain > 0) ? RCS_AUTO_1 : RCS_MANUAL;
    m_ri->m_gain.TransformAndUpdate(bl_pter->gain);
    m_ri->m_gain.UpdateState(state);
    LOG_RECEIVE(wxT("gain updated received1= %i, displayed = %i"), bl_pter->gain, m_ri->m_gain.GetValue());

    state = (RadarControlState)bl_pter->auto_sea;
    m_ri->m_sea.TransformAndUpdate(bl_pter->sea_value);
    m_ri->m_sea.UpdateState(state);
    LOG_RECEIVE(wxT("sea updated received= %i, displayed = %i, state=%i"), bl_pter->sea_value, m_ri->m_sea.GetValue(), state);

    state = (bl_pter->rain_enabled) ? RCS_MANUAL : RCS_OFF;
    LOG_RECEIVE(wxT("rain state=%i bl_pter->rain_enabled=%i"), state, bl_pter->rain_enabled);
    m_ri->m_rain.TransformAndUpdate(bl_pter->rain_value);
    m_ri->m_rain.UpdateState(state);
    LOG_RECEIVE(wxT("rain updated received= %i, displayed = %i state=%i"), bl_pter->rain_value, m_ri->m_rain.GetValue(), state);

    state = (bl_pter->ftc_enabled) ? RCS_MANUAL : RCS_OFF;
    m_ri->m_ftc.TransformAndUpdate(bl_pter->ftc_value);
    m_ri->m_ftc.UpdateState(state);
    LOG_RECEIVE(wxT("ftc updated received= %i, displayed = %i state=%i"), bl_pter->ftc_value, m_ri->m_ftc.GetValue(), state);

    m_ri->m_target_expansion.Update(bl_pter->target_expansion);
    m_ri->m_interference_rejection.Update(bl_pter->interference_rejection);

    int ba = (int)bl_pter->bearing_offset;
    m_ri->m_bearing_alignment.Update(ba);

    state = (bl_pter->auto_tune > 0) ? RCS_AUTO_1 : RCS_MANUAL;
    m_ri->m_tune_fine.Update(bl_pter->tune, state);

    state = (bl_pter->auto_tune > 0) ? RCS_AUTO_1 : RCS_MANUAL;

    m_ri->m_tune_coarse.UpdateState(state);

    state = (bl_pter->mbs_enabled > 0) ? RCS_MANUAL : RCS_OFF;
    m_ri->m_main_bang_suppression.Update(bl_pter->mbs_enabled, RCS_MANUAL);

    m_ri->m_warmup_time.Update(bl_pter->warmup_time);
    m_ri->m_signal_strength.Update(bl_pter->signal_strength);
  }

  int status = m_ri->m_state.GetValue();
  wxString stat;
  switch (status) {
    case RADAR_OFF:
      LOG_VERBOSE(wxT("%s reports status RADAR_OFF"), m_ri->m_name.c_str());
      stat = _("Off");
      break;

    case RADAR_STANDBY:
      LOG_VERBOSE(wxT("%s reports status STANDBY"), m_ri->m_name.c_str());
      stat = _("Standby");
      break;

    case RADAR_WARMING_UP:
      LOG_VERBOSE(wxT("%s reports status RADAR_WARMING_UP"), m_ri->m_name.c_str());
      stat = _("Warming up");
      break;

    case RADAR_TRANSMIT:
      LOG_VERBOSE(wxT("%s reports status RADAR_TRANSMIT"), m_ri->m_name.c_str());
      stat = _("Transmit");
      break;

    default:
      // LOG_BINARY_RECEIVE(wxT("received unknown radar status"), report, len);
      stat = _("Unknown status");
      break;
  }

  s = wxString::Format(wxT("IP %s %s"), m_ri->m_radar_address.FormatNetworkAddress(), stat.c_str());

  RadarLocationInfo info = m_ri->GetRadarLocationInfo();
  s << wxT("\n") << _("IF-Serial #") << info.serialNr;
  SetInfoStatus(s);
}

void RaymarineReceive::ProcessFixedReport(const UINT8 *data, int len) {
  LOG_BINARY_REPORTS(wxString::Format(wxT("%s FixedReport"), m_ri->m_name.c_str()), data, len);

  if (len == sizeof(RMRadarFixedReport)) {
    RMRadarFixedReport *bl_pter = (RMRadarFixedReport *)data;
    RadarControlState state;
    state = RCS_MANUAL;
    m_ri->m_display_timing.Update(bl_pter->display_timing, state);
    LOG_RECEIVE(wxT("bl_pter->gain_min=%i , bl_pter->gain_max=%i"), bl_pter->gain_min, bl_pter->gain_max);
    LOG_RECEIVE(wxT("bl_pter->sea_min=%i , bl_pter->sea_max=%i"), bl_pter->sea_min, bl_pter->sea_max);
    LOG_RECEIVE(wxT("bl_pter->rain_min=%i , bl_pter->rain_max=%i"), bl_pter->rain_min, bl_pter->rain_max);
    LOG_RECEIVE(wxT("bl_pter->ftc_min=%i , bl_pter->ftc_maxn=%i"), bl_pter->ftc_min, bl_pter->ftc_max);
    m_ri->m_gain.SetMin(bl_pter->gain_min);
    m_ri->m_gain.SetMax(bl_pter->gain_max);
    m_ri->m_sea.SetMin(bl_pter->sea_min);
    m_ri->m_sea.SetMax(bl_pter->sea_max);
    m_ri->m_rain.SetMin(bl_pter->rain_min);
    m_ri->m_rain.SetMax(bl_pter->rain_max);
    m_ri->m_ftc.SetMin(bl_pter->ftc_min);
    m_ri->m_ftc.SetMax(bl_pter->ftc_max);
    m_ri->m_magnetron_current.Update(bl_pter->magnetron_current);
    m_ri->m_magnetron_time.Update(bl_pter->magnetron_time);
    m_ri->m_rotation_period.Update(bl_pter->rotation_time);
  }
}

// Radar data

struct Header1 {
  uint32_t field01;  // 0x00010003
  uint32_t zero_1;
  uint32_t fieldx_1;     // 0x0000001c
  uint32_t nspokes;      // 0x00000008 - usually but changes
  uint32_t spoke_count;  // 0x00000000 in regular, counting in HD
  uint32_t zero_3;
  uint32_t fieldx_3;  // 0x00000001
  uint32_t fieldx_4;  // 0x00000000 or 0xffffffff in regular, 0x400 in HD
};

struct Header2 {
  uint32_t field01;
  uint32_t length;
  // ...
};

struct Header3 {
  uint32_t field01;  // 0x00000001
  uint32_t length;   // 0x00000028
  uint32_t azimuth;
  uint32_t fieldx_2;  // 0x00000001 - 0x03 - HD
  uint32_t fieldx_3;  // 0x00000002
  uint32_t fieldx_4;  // 0x00000001 - 0x03 - HD
  uint32_t fieldx_5;  // 0x00000001 - 0x00 - HD
  uint32_t fieldx_6;  // 0x000001f4 - 0x00 - HD
  uint32_t zero_1;
  uint32_t fieldx_7;  // 0x00000001
};

struct Header4 {     // No idea what is in there
  uint32_t field01;  // 0x00000002
  uint32_t length;   // 0x0000001c
  uint32_t zero_2[5];
};

struct SpokeData {
  uint32_t field01;  // 0x00000003
  uint32_t length;
  uint32_t data_len;
};

void RaymarineReceive::ProcessScanData(const UINT8 *data, int len) {
  if (m_range_meters == 1) {
    LOG_RECEIVE(wxT("Invalid range"));
    return;
  }
  m_pi->logBinaryData(wxT("Scandata"), data, len);
  if (len > (int)(sizeof(Header1) + sizeof(Header3))) {
    Header1 *pHeader = (Header1 *)data;
    bool HDtype = false;
    u_int returns_per_line;

    if (pHeader->field01 != 0x00010003 || pHeader->fieldx_1 != 0x0000001c || pHeader->fieldx_3 != 0x0000001) {
      LOG_INFO(wxT("ProcessScanData::Packet header mismatch %x, %x, %x, %x.\n"), pHeader->field01, pHeader->fieldx_1,
               pHeader->nspokes, pHeader->fieldx_3);
      return;
    }

    time_t now = time(0);
    m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;
    m_ri->m_data_timeout = now + DATA_TIMEOUT;
    m_ri->m_state.Update(RADAR_TRANSMIT);

    if (pHeader->fieldx_4 == 0x400) {
      LOG_RECEIVE(wxT(" different radar type found"));
    }
    wxLongLong nowMillis = wxGetLocalTimeMillis();
    int headerIdx = 0;
    int nextOffset = sizeof(Header1);

    while (nextOffset < len) {
      Header3 *sHeader = (Header3 *)(data + nextOffset);
      if (sHeader->field01 != 0x00000001 || sHeader->length != 0x00000028) {
        LOG_RECEIVE(wxT("ProcessScanData::Scan header #%d (%d) - %x, %x.\n"), headerIdx, nextOffset, sHeader->field01,
                    sHeader->length);
        break;
      }

      if (sHeader->fieldx_2 == 0x00000001 && sHeader->fieldx_3 == 0x00000002 && sHeader->fieldx_4 == 0x00000001 &&
          sHeader->fieldx_5 == 0x00000001 && sHeader->fieldx_6 == 0x000001f4 && sHeader->fieldx_7 == 0x00000001) {
        HDtype = false;
        returns_per_line = 512;
      } else if (sHeader->fieldx_2 == 3 && sHeader->fieldx_3 == 2 && sHeader->fieldx_4 == 3 && sHeader->fieldx_5 == 1 &&
                 sHeader->fieldx_6 == 0 && sHeader->fieldx_7 == 1) {
        HDtype = true;
        returns_per_line = 1024;
      } else {
        LOG_RECEIVE(wxT("ProcessScanData::Scan header #%d part 2 check failed.\n"), headerIdx);
        break;
      }

      nextOffset += sizeof(Header3);
      Header2 *nHeader = (Header2 *)(data + nextOffset);
      if (nHeader->field01 == 0x00000002) {
        if (nHeader->length != 0x0000001c) {
          LOG_RECEIVE(wxT("ProcessScanData::Opt header #%d part 2 check failed.\n"), headerIdx);
        }
        nextOffset += nHeader->length;
      }
      SpokeData *pSData = (SpokeData *)(data + nextOffset);
      if ((pSData->field01 & 0x7fffffff) != 0x00000003 || pSData->length < pSData->data_len + 8) {
        LOG_RECEIVE(wxT("ProcessScanData::Scan data header #%d check failed %x, %d, %d.\n"), headerIdx, pSData->field01,
                    pSData->length, pSData->data_len);
        break;
      }
      UINT8 unpacked_data[10240], *dataPtr = 0;

      uint8_t *dData = (uint8_t *)unpacked_data;
      uint8_t *sData = (uint8_t *)data + nextOffset + sizeof(SpokeData);

      unsigned int iS = 0;
      unsigned int iD = 0;
      // LOG_BINARY_RECEIVE(wxT("spoke data sData"), sData, pSData->data_len);
      while (iS < pSData->data_len) {
        if (HDtype) {
          if (iD >= 1024) {  // remove trailing zero's
            break;
          }
          if (*sData != 0x5c) {
            *dData++ = *sData;
            sData++;
            iS++;
            iD++;
          } else {
            uint8_t nFill = sData[1];  // number to be filled
            uint8_t cFill = sData[2];  // data to be filled
            for (unsigned int i = 0; i < nFill; i++) {
              *dData++ = cFill;
            }
            sData += 3;
            iS += 3;
            iD += nFill;
          }
        } else {  // not HDtype
          if (*sData != 0x5c) {
            *dData++ = (((*sData) & 0x0f) << 4) + 0x0f;
            *dData++ = ((*sData) & 0xf0) + 0x0f;
            sData++;
            iS++;
            iD += 2;
          } else {
            uint8_t nFill = sData[1];
            uint8_t cFill = sData[2];

            for (unsigned int i = 0; i < nFill; i++) {
              *dData++ = ((cFill & 0x0f) << 4) + 0x0f;
              *dData++ = (cFill & 0xf0) + 0x0f;
            }
            sData += 3;
            iS += 3;
            iD += nFill * 2;
          }
        }
      }
      if (iD != returns_per_line) {
        while (iS < pSData->length - 8 && iD <= returns_per_line) {
          if (HDtype) {
            *dData++ = *sData;
            sData++;
            iS++;
            iD++;
          } else {
            *dData++ = ((*sData) & 0x0f) << 4;
            *dData++ = (*sData) & 0xf0;
            sData++;
            iS++;
            iD += 2;
          }
        }
      }

      // LOG_BINARY_RECEIVE(wxT("spoke data dData"), unpacked_data, pSData->data_len);
      dataPtr = unpacked_data;

      nextOffset += pSData->length;
      m_ri->m_statistics.spokes++;
      unsigned int spoke = sHeader->azimuth;
      if (m_next_spoke >= 0 && (int)spoke != m_next_spoke) {
        if ((int)spoke > m_next_spoke) {
          m_ri->m_statistics.missing_spokes += spoke - m_next_spoke;
        } else {
          m_ri->m_statistics.missing_spokes += SPOKES + spoke - m_next_spoke;
        }
      }
      m_next_spoke = (spoke + 1) % 2048;

      if ((pSData->field01 & 0x80000000) != 0 && nextOffset < len) {
        // fprintf(stderr, "ProcessScanData::Last record %d (%d) in packet %d but still data to go %d:%d.\n",
        // 	headerIdx, scan_idx, packetIdx, nextOffset, len);
      }
      headerIdx++;

      m_pi->SetRadarHeading();

      int hdt_raw = SCALE_DEGREES_TO_RAW(m_pi->GetHeadingTrue() + m_pi->m_vp_rotation);

      int angle_raw = spoke * 2 + SCALE_DEGREES_TO_RAW(180);  // Compensate openGL rotation compared to North UP
      int bearing_raw = angle_raw + hdt_raw;

      SpokeBearing angle = MOD_ROTATION2048(angle_raw / 2);      // divide by 2 to map on 2048 scanlines
      SpokeBearing bearing = MOD_ROTATION2048(bearing_raw / 2);  // divide by 2 to map on 2048 scanlines
      bool spokes_1024;
      if (int(angle - m_previous_angle) == 2 || int(angle - m_previous_angle) == -2046) {
        spokes_1024 = true;
      } else {
        spokes_1024 = false;
      }
      m_previous_angle = angle;
      if (m_range_meters == 1) {
        LOG_INFO(wxT("Error range invalid"));
        return;
      }
      /*LOG_INFO(wxT("ProcessRadarSpoke a=%i, angle_raw=%i b=%i, bearing_raw=%i, returns_per_line=%i range=%i spokes=%i"), angle,
         angle_raw, bearing, bearing_raw, returns_per_line, m_range_meters, m_ri->m_spokes);*/
      m_ri->ProcessRadarSpoke(angle, bearing, dataPtr, returns_per_line, m_range_meters, nowMillis);
      // When te HD radar is transmitting in a mode with 1024 spokes, insert additional spokes to fill the image
      if (spokes_1024 && angle + 1 < (int)m_ri->m_spokes && bearing + 1 < (int)m_ri->m_spokes) {
        m_ri->ProcessRadarSpoke(angle + 1, bearing + 1, dataPtr, returns_per_line, m_range_meters, nowMillis);
      }
    }
  }
}

struct QuantumHeader {
  uint32_t field01;   //
  uint16_t counter1;  //
  uint16_t field02;
  uint32_t field03;
  uint32_t field04;   //
  uint16_t counter2;  //
  uint16_t data_len;
};

struct SQuantumScanDataHeader {
  uint32_t type;  // 0x00280003
  uint16_t seq_num;
  uint16_t something_1;        // 0x0101
  uint16_t scan_len;           // 0x002b
  uint16_t num_spokes;         // 0x00fa
  uint16_t something_3;        // 0x0008
  uint16_t returns_per_range;  // number of radar returns per range from the status
  uint16_t azimuth;
  uint16_t data_len;
};

void RaymarineReceive::ProcessQuantumScanData(const UINT8 *data, int len) {
  if (m_range_meters == 1) {
    LOG_RECEIVE(wxT("Invalid range"));
    return;
  }
  SQuantumScanDataHeader *qheader = (SQuantumScanDataHeader *)data;
  m_pi->logBinaryData(wxT("SQuantumScanDataHeader"), data, len);
  if (len > (int)(sizeof(SQuantumScanDataHeader))) {
    u_int returns_per_line;

    time_t now = time(0);
    m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;
    m_ri->m_data_timeout = now + DATA_TIMEOUT;
    m_ri->m_state.Update(RADAR_TRANSMIT);

    wxLongLong nowMillis = wxGetLocalTimeMillis();
    int headerIdx = 0;
    int nextOffset = sizeof(QuantumHeader);
    UINT8 unpacked_data[1024], *dataPtr = 0;

    uint8_t *dData = (uint8_t *)unpacked_data;
    uint8_t *sData = (uint8_t *)data + nextOffset;

    unsigned int iS = 0;
    unsigned int iD = 0;
    while (iS < qheader->data_len) {
      if (iD >= 1024) {  // remove trailing zero's
        break;
      }
      if (*sData != 0x5c) {
        *dData++ = *sData;
        sData++;
        iS++;
        iD++;
      } else {
        uint8_t nFill = sData[1];  // number to be filled
        uint8_t cFill = sData[2];  // data to be filled
        for (unsigned int i = 0; i < nFill; i++) {
          *dData++ = cFill;
        }
        sData += 3;
        iS += 3;
        iD += nFill;
      }
    }  // end of while, only one spoke per packet

    returns_per_line = qheader->scan_len;
    if (returns_per_line > 252) {
      LOG_VERBOSE(wxT("Error returns_per_line too large %i"), returns_per_line);
      returns_per_line = 252;
    }
    dataPtr = unpacked_data;
    m_ri->m_statistics.spokes++;
    unsigned int spoke = qheader->azimuth;
    if (m_next_spoke >= 0 && (int)spoke != m_next_spoke) {
      if ((int)spoke > m_next_spoke) {
        m_ri->m_statistics.missing_spokes += spoke - m_next_spoke;
      } else {
        m_ri->m_statistics.missing_spokes += SPOKES + spoke - m_next_spoke;
      }
    }
    m_next_spoke = spoke + 1;
    headerIdx++;
    m_pi->SetRadarHeading();
    int hdt_raw = qheader->num_spokes * (m_pi->GetHeadingTrue() + m_pi->m_vp_rotation) / 360.;

    int angle_raw = spoke;  // Compensate openGL rotation compared to North UP
    int bearing_raw = angle_raw + hdt_raw;

    SpokeBearing angle = angle_raw;
    SpokeBearing bearing = bearing_raw;
    angle += 125;
    bearing += 125;  // turn 180 degrees for Quantum

    while (angle >= 250) {
      angle -= 250;
    }
    while (angle < 0) {
      angle += 250;
    }
    while (bearing >= 250) {
      bearing -= 250;
    }
    while (bearing < 0) {
      bearing += 250;
    }

    if (m_range_meters == 1) {
      LOG_INFO(wxT("Error range invalid"));
      return;
    }
    m_ri->ProcessRadarSpoke(angle, bearing, dataPtr, returns_per_line,
                            m_range_meters * returns_per_line / qheader->returns_per_range / 2, nowMillis);
  }
}

void RaymarineReceive::Shutdown() {
  if (m_send_socket != INVALID_SOCKET) {
    m_shutdown_time_requested = wxGetUTCTimeMillis();
    if (send(m_send_socket, "!", 1, MSG_DONTROUTE) > 0) {
      return;
    }
  }
  LOG_INFO(wxT("%s receive thread will take long time to stop"), m_ri->m_name.c_str());
}

wxString RaymarineReceive::GetInfoStatus() {
  wxCriticalSectionLocker lock(m_lock);
  // Called on the UI thread, so be gentle
  if (m_firmware.length() > 0) {
    return m_status + wxT("\n") + m_firmware;
  }
  return m_status;
}

PLUGIN_END_NAMESPACE
