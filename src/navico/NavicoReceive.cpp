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

#include "NavicoReceive.h"

#include "MessageBox.h"
#include "NavicoControl.h"

PLUGIN_BEGIN_NAMESPACE

/*
 * This file not only contains the radar receive threads, it is also
 * the only unit that understands what the radar returned data looks like.
 * The rest of the plugin uses a (slightly) abstract definition of the radar.
 */

// On HALO we need to send data every 50 ms or so, so sleep is short ...
#define MILLIS_PER_SELECT 50
#define SECONDS_SELECT(x) ((x)*MILLISECONDS_PER_SECOND / MILLIS_PER_SELECT)
#define IS_HALO (m_ri->m_radar_type == RT_HaloA || m_ri->m_radar_type == RT_HaloB)

//
// Navico radars use an internal spoke ID that has range [0..4096> but they
// only send half of them
//
#define SPOKES (4096)
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double)DEGREES_PER_ROTATION / SPOKES)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)SPOKES / DEGREES_PER_ROTATION))

// A marker that uniquely identifies BR24 generation scanners, as opposed to 4G(eneration)
// Note that 3G scanners are BR24's with better power, so they are more BR24+ than 4G-.
// As far as we know they 3G's use exactly the same command set.

/*
 Heading on radar. Observed in field:
 - Hakan: BR24, no RI: 0x9234 = negative, with recognisable 1234 in hex?
 - Marcus: 3G, RI, true heading: 0x45be
 - Kees: 4G, RI, mag heading: 0x07d6 = 2006 = 176,6 deg
 - Kees: 4G, RI, no heading: 0x8000 = -1 = negative
 - Kees: Halo, true heading: 0x4xxx => true
 Known values for heading value:
*/
#define HEADING_TRUE_FLAG 0x4000
#define HEADING_MASK (SPOKES - 1)
#define HEADING_VALID(x) (((x) & ~(HEADING_TRUE_FLAG | HEADING_MASK)) == 0)

// One MFD or OpenCPN shall send the radar timing and heading info
// Without this the Doppler function doesn't work
static const NetworkAddress haloInfoAddress(239, 238, 55, 73, 7527);

SOCKET g_HaloInfoSocket = INVALID_SOCKET;  // Only _one_ radar is able to create this socket at a time.
wxCriticalSection g_HaloInfoSocketLock;

#pragma pack(push, 1)

struct halo_heading_packet {
  char marker[4];    // 4 bytes containing 'NKOE'
  uint8_t u00[4];    // 4 bytes containing '00 01 90 02'
  uint16_t counter;  // 2 byte counter incrementing by 1 every transmission, in BigEndian
  // 10
  uint8_t u01[26];  // 25 bytes of unknown stuff that doesn't seem to vary
  // 36
  uint8_t u02[2];  // 2 bytes containing '12 f1'
  uint8_t u03[2];  // 2 bytes containing '01 00'
  // 40
  wxLongLong epoch;  // 8 bytes containing millis since 1970
  // 48
  uint64_t u04;  // 8 bytes containing 2
  // 56
  uint32_t u05a;  // 4 bytes containing some fixed data, could be position?
  // 60
  uint32_t u05b;  // 4 bytes containing some fixed data, could be position?
  // 64
  uint8_t u06[1];  // 1 byte containing counter or 0xff
  // 65
  uint16_t heading;  // 2 bytes containing heading
  // 67
  uint8_t u07[5];  // 5 bytes containing varying unknown data
  // 72
};

struct halo_mystery_packet {
  char marker[4];    // 4 bytes containing 'NKOE'
  uint8_t u00[4];    // 4 bytes containing '00 01 90 02'
  uint16_t counter;  // 2 byte counter incrementing by 1 every transmission, in BigEndian
  // 10
  uint8_t u01[26];  // 25 bytes of unknown stuff that doesn't seem to vary
  // 36
  uint8_t u02[2];  // 2 bytes containing '02 f8'...
  uint8_t u03[2];  // 2 bytes containing '01 00'
  // 40
  wxLongLong epoch;  // 8 bytes containing millis since 1970
  // 48
  uint64_t u04;  // 8 bytes containing 2
  // 56
  uint32_t u05a;  // 4 bytes containing some fixed data, could be position?
  // 60
  uint32_t u05b;  // 4 bytes containing some fixed data, could be position?
  // 64
  uint8_t u06[1];  // 1 byte containing counter or 0xff
  // 65
  uint8_t u07[1];  // 1 byte containing 0xfc
  // 66
  uint16_t mystery1;  // 2 bytes containing some varying field
  // 68
  uint16_t mystery2;  // 2 bytes containing some varying field
  // 70
  uint8_t u08[2];  // 2 bytes containg 0xff 0xff
  // 72
};

#define SCAN_MAX (256)

struct common_header {
  uint8_t headerLen;    // 1 bytes
  uint8_t status;       // 1 bytes
  uint8_t scan_number;  // 1 byte
  uint8_t u00[1];       // 1 byte, 2nd byte of scan_number on 4G and older
  uint8_t u01[4];       // 4 bytes
  uint8_t angle[2];     // 2 bytes
  uint8_t heading[2];   // 2 bytes heading with RI-10/11. See bitmask explanation above.
};

struct br24_header {
  uint8_t headerLen;       // 1 bytes
  uint8_t status;          // 1 bytes
  uint8_t scan_number[2];  // 2 bytes, 0-4095
  uint8_t mark[4];         // 4 bytes 0x00, 0x44, 0x0d, 0x0e
  uint8_t angle[2];        // 2 bytes
  uint8_t heading[2];      // 2 bytes heading with RI-10/11. See bitmask explanation above.
  uint8_t range[4];        // 4 bytes
  uint8_t u01[2];          // 2 bytes blank
  uint8_t u02[2];          // 2 bytes
  uint8_t u03[4];          // 4 bytes blank
};                         /* total size = 24 */

struct br4g_header {
  uint8_t headerLen;       // 1 bytes
  uint8_t status;          // 1 bytes
  uint8_t scan_number[2];  // 2 bytes, 0-4095
  uint8_t u00[2];          // Always 0x4400 (integer)
  uint8_t largerange[2];   // 2 bytes or -1
  uint8_t angle[2];        // 2 bytes
  uint8_t heading[2];      // 2 bytes heading with RI-10/11 or -1. See bitmask explanation above.
  uint8_t smallrange[2];   // 2 bytes or -1
  uint8_t rotation[2];     // 2 bytes, rotation/angle
  uint8_t u02[4];          // 4 bytes signed integer, always -1
  uint8_t u03[4];          // 4 bytes signed integer, mostly -1 (0x80 in last byte) or 0xa0 in last byte
};                         /* total size = 24 */

struct radar_line {
  union {
    common_header common;
    br24_header br24;
    br4g_header br4g;
  };
  uint8_t data[NAVICO_SPOKE_LEN / 2];
};

/* Normally the packets are have 32 spokes, or scan lines, but we assume nothing
 * so we take up to 120 spokes. This is the nearest round figure without going over
 * 64kB.
 */

struct radar_frame_pkt {
  uint8_t frame_hdr[8];
  radar_line line[120];  //  scan lines, or spokes
};
#pragma pack(pop)

enum LookupSpokeEnum {
  LOOKUP_SPOKE_LOW_NORMAL,
  LOOKUP_SPOKE_LOW_BOTH,
  LOOKUP_SPOKE_LOW_APPROACHING,
  LOOKUP_SPOKE_HIGH_NORMAL,
  LOOKUP_SPOKE_HIGH_BOTH,
  LOOKUP_SPOKE_HIGH_APPROACHING
};

static uint8_t lookupData[6][256];

// Make space for BLOB_HISTORY_COLORS
static const uint8_t lookupNibbleToByte[16] = {
    0,     // 0
    0x32,  // 1
    0x40,  // 2
    0x4e,  // 3
    0x5c,  // 4
    0x6a,  // 5
    0x78,  // 6
    0x86,  // 7
    0x94,  // 8
    0xa2,  // 9
    0xb0,  // a
    0xbe,  // b
    0xcc,  // c
    0xda,  // d
    0xe8,  // e
    0xf4,  // f
};

void NavicoReceive::InitializeLookupData() {
  if (lookupData[5][255] == 0) {
    for (int j = 0; j <= UINT8_MAX; j++) {
      uint8_t low = lookupNibbleToByte[(j & 0x0f)];
      uint8_t high = lookupNibbleToByte[(j & 0xf0) >> 4];

      lookupData[LOOKUP_SPOKE_LOW_NORMAL][j] = (uint8_t)low;
      lookupData[LOOKUP_SPOKE_HIGH_NORMAL][j] = (uint8_t)high;

      switch (low) {
        case 0xf4:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = 0xff;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = 0xff;
          break;

        case 0xe8:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = 0xfe;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = (uint8_t)low;
          break;

        default:
          lookupData[LOOKUP_SPOKE_LOW_BOTH][j] = (uint8_t)low;
          lookupData[LOOKUP_SPOKE_LOW_APPROACHING][j] = (uint8_t)low;
      }

      switch (high) {
        case 0xf4:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = 0xff;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = 0xff;
          break;

        case 0xe8:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = 0xfe;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = (uint8_t)high;
          break;

        default:
          lookupData[LOOKUP_SPOKE_HIGH_BOTH][j] = (uint8_t)high;
          lookupData[LOOKUP_SPOKE_HIGH_APPROACHING][j] = (uint8_t)high;
      }
    }
  }
}

// ProcessFrame
// ------------
// Process one radar frame packet, which can contain up to 32 'spokes' or lines extending outwards
// from the radar up to the range indicated in the packet.
//
void NavicoReceive::ProcessFrame(const uint8_t *data, size_t len) {
  time_t now = time(0);

  // log_line.time_rec = wxGetUTCTimeMillis();
  wxLongLong time_rec = wxGetUTCTimeMillis();

  radar_frame_pkt *packet = (radar_frame_pkt *)data;

  wxCriticalSectionLocker lock(m_ri->m_exclusive);

  m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;
  m_ri->m_data_timeout = now + DATA_TIMEOUT;
  m_ri->m_state.Update(RADAR_TRANSMIT);

  m_ri->m_statistics.packets++;
  if (len < sizeof(packet->frame_hdr)) {
    // The packet is so small it contains no scan_lines, quit!
    m_ri->m_statistics.broken_packets++;
    return;
  }
  size_t scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(radar_line);
  if (scanlines_in_packet != 32) {
    m_ri->m_statistics.broken_packets++;
  }

  if (m_first_receive) {
    m_first_receive = false;
    wxLongLong startup_elapsed = wxGetUTCTimeMillis() - m_pi->GetBootMillis();
    LOG_INFO(wxT("%s first radar spoke received after %llu ms\n"), m_ri->m_name.c_str(), startup_elapsed);
  }

  for (size_t scanline = 0; scanline < scanlines_in_packet; scanline++) {
    radar_line *line = &packet->line[scanline];

    // Validate the spoke
    uint8_t scan = line->common.scan_number;
    m_ri->m_statistics.spokes++;
    if (line->common.headerLen != 0x18) {
      LOG_RECEIVE(wxT("%s strange header length %d"), m_ri->m_name.c_str(), line->common.headerLen);
      // Do not draw something with this...
      m_ri->m_statistics.broken_spokes++;
      m_next_scan = scan + 1;  // We use automatic rollover of uint8_t here
      continue;
    }
    if (line->common.status != 0x02 && line->common.status != 0x12) {
      LOG_RECEIVE(wxT("%s strange status %02x"), m_ri->m_name.c_str(), line->common.status);
      m_ri->m_statistics.broken_spokes++;
    }

    LOG_RECEIVE(wxT("%s scan=%d m_next_scan=%d"), m_ri->m_name.c_str(), scan, m_next_scan);
    if (m_next_scan >= 0 && scan != m_next_scan) {
      if (scan > m_next_scan) {
        m_ri->m_statistics.missing_spokes += scan - m_next_scan;
      } else {
        m_ri->m_statistics.missing_spokes += SCAN_MAX + scan - m_next_scan;
      }
    }
    m_next_scan = scan + 1;  // We use automatic rollover of uint8_t here

    int range_raw = 0;
    int angle_raw = 0;
    short int heading_raw = 0;
    int range_meters = 0;

    heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];

    switch (m_ri->m_radar_type) {
      case RT_BR24: {
        range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
        angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
        range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
        break;
      }

      case RT_3G:
      case RT_4GA:
      case RT_4GB: {
        short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
        short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
        angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
        if (large_range == 0x80) {
          if (small_range == -1) {
            range_meters = 0;  // Invalid range received
          } else {
            range_meters = small_range / 4;
          }
        } else {
          range_meters = large_range * 64;
        }
        break;
      }

      case RT_HaloA:
      case RT_HaloB: {
        uint16_t large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
        uint16_t small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
        angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
        if (large_range == 0x80) {
          if (small_range == 0xffff) {
            range_meters = 0;  // Invalid range received
          } else {
            range_meters = small_range / 4;
          }
        } else {
          range_meters = large_range * small_range / 512;
        }
        break;
      }

      default:
        return;
    }

    /*
        LOG_BINARY_RECEIVE(wxString::Format(wxT("display=%d range=%d, angle=%d hdg=%d"), m_ri->GetDisplayRange(), range_meters,
                                            angle_raw, heading_raw),
                           (uint8_t *)&line->br24, sizeof(line->br24));
    */

    bool radar_heading_valid = HEADING_VALID(heading_raw);
    bool radar_heading_true = (heading_raw & HEADING_TRUE_FLAG) != 0;
    double heading;
    int bearing_raw;

    if (radar_heading_valid && !m_pi->m_settings.ignore_radar_heading) {
      // On HALO, we can be the ones that are sending heading to the radar;
      // in that case do NOT pass it back down to avoid feedback loop!
      if (!IS_HALO) {
        heading = MOD_DEGREES_FLOAT(SCALE_RAW_TO_DEGREES(heading_raw));
        m_pi->SetRadarHeading(heading, radar_heading_true);
      }
    } else {
      m_pi->SetRadarHeading();
      heading = m_pi->GetHeadingTrue();
      radar_heading_true = true;
      heading_raw = SCALE_DEGREES_TO_RAW(heading);
    }
    // Guess the heading for the spoke. This is updated much less frequently than the
    // data from the radar (which is accurate 10x per second), likely once per second.
    bearing_raw = angle_raw + heading_raw;
    // until here all is based on 4096 (SPOKES) scanlines

    SpokeBearing a = MOD_SPOKES(angle_raw / 2);    // divide by 2 to map on 2048 scanlines
    SpokeBearing b = MOD_SPOKES(bearing_raw / 2);  // divide by 2 to map on 2048 scanlines
    size_t len = NAVICO_SPOKE_LEN;
    uint8_t data_highres[NAVICO_SPOKE_LEN];

    int doppler = m_ri->m_doppler.GetValue();
    if (doppler < 0 || doppler > 2) {
      doppler = 0;
    }
    uint8_t *lookup_low = lookupData[LOOKUP_SPOKE_LOW_NORMAL + doppler];
    uint8_t *lookup_high = lookupData[LOOKUP_SPOKE_HIGH_NORMAL + doppler];
    for (int i = 0; i < NAVICO_SPOKE_LEN / 2; i++) {
      data_highres[2 * i] = lookup_low[line->data[i]];
      data_highres[2 * i + 1] = lookup_high[line->data[i]];
    }
    m_ri->ProcessRadarSpoke(a, b, data_highres, len, range_meters, time_rec);
  }
}

SOCKET NavicoReceive::PickNextEthernetCard() {
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
  socket = GetNewReportSocket();
  return socket;
}

SOCKET NavicoReceive::GetNewReportSocket() {
  SOCKET socket;
  wxString error = wxT(" ");
  wxString s = wxT(" ");
  RadarLocationInfo current_info = m_ri->GetRadarLocationInfo();

  if (!(m_info == current_info)) {  // initial values or NavicoLocate modified the info
    m_info = current_info;
    m_interface_addr = m_ri->GetRadarInterfaceAddress();
    LOG_INFO(wxT("%s Locator found radar at IP %s [%s]"), m_ri->m_name.c_str(), m_ri->m_radar_address.FormatNetworkAddressPort(),
             m_info.to_string());
  };

  if (m_interface_addr.IsNull()) {
    LOG_RECEIVE(wxT("%s no interface address to listen on"), m_ri->m_name.c_str());
    wxMilliSleep(200);  // don't make the log too large
    return INVALID_SOCKET;
  }
  if (m_info.report_addr.IsNull()) {
    LOG_RECEIVE(wxT("%s no report address to listen on"), m_ri->m_name.c_str());
    wxMilliSleep(200);
    return INVALID_SOCKET;
  }

  if (RadarOrder[m_ri->m_radar_type] >= RO_PRIMARY) {
    if (!m_info.serialNr.IsNull()) {
      s << _("Serial #") << m_info.serialNr << wxT("\n");
    }
  }

  socket = startUDPMulticastReceiveSocket(m_interface_addr, m_info.report_addr, error);

  if (socket != INVALID_SOCKET) {
    wxString addr = m_interface_addr.FormatNetworkAddress();
    wxString rep_addr = m_info.report_addr.FormatNetworkAddressPort();

    LOG_RECEIVE(wxT("%s listening on interface %s for reports from %s"), m_ri->m_name.c_str(), addr.c_str(), rep_addr.c_str());

    s << _("Scanning interface") << wxT(" ") << addr;
    SetInfoStatus(s);
  } else {
    s << error;
    SetInfoStatus(s);
    wxLogError(wxT("%s Unable to listen to socket: %s"), m_ri->m_name.c_str(), error.c_str());
  }
  return socket;
}

SOCKET NavicoReceive::GetNewDataSocket() {
  SOCKET socket;
  wxString error;

  if (m_interface_addr.addr.s_addr == 0) {
    return INVALID_SOCKET;
  }

  error.Printf(wxT("%s data: "), m_ri->m_name.c_str());
  socket = startUDPMulticastReceiveSocket(m_interface_addr, m_info.spoke_data_addr, error);
  if (socket != INVALID_SOCKET) {
    wxString addr = m_interface_addr.FormatNetworkAddress();
    wxString rep_addr = m_info.spoke_data_addr.FormatNetworkAddressPort();

    LOG_RECEIVE(wxT("%s listening for data on %s from %s"), m_ri->m_name.c_str(), addr.c_str(), rep_addr.c_str());
  } else {
    SetInfoStatus(error);
    wxLogError(wxT("Unable to listen to socket: %s"), error.c_str());
  }
  return socket;
}

/*
 * Get a socket to the multicast address where MFDs send the HALO the course and time
 * info that was previously done via the RI-10/11.
 */
SOCKET NavicoReceive::GetNewInfoSocket() {
  wxString error;

  // This is only necessary on HALO radars
  if (!IS_HALO) {
    LOG_RECEIVE(wxT("%s no halo info socket needed for radar type"), m_ri->m_name.c_str());
    return INVALID_SOCKET;
  }
  if (m_interface_addr.addr.s_addr == 0) {
    LOG_RECEIVE(wxT("%s no halo info socket needed when no radar address"), m_ri->m_name.c_str());
    return INVALID_SOCKET;
  }

  if (g_HaloInfoSocket != INVALID_SOCKET) {
    // Other thread already has a g_HaloInfoSocket, this thread should NOT use it
    return INVALID_SOCKET;
  }

  wxCriticalSectionLocker lock(g_HaloInfoSocketLock);

  error.Printf(wxT("%s info: "), m_ri->m_name.c_str());
  g_HaloInfoSocket = startUDPMulticastReceiveSocket(m_interface_addr, haloInfoAddress, error);
  if (g_HaloInfoSocket != INVALID_SOCKET) {
    wxString addr = m_interface_addr.FormatNetworkAddress();
    wxString rep_addr = haloInfoAddress.FormatNetworkAddressPort();

    LOG_RECEIVE(wxT("%s listening for halo info on %s"), m_ri->m_name.c_str(), addr.c_str());
  } else {
    SetInfoStatus(error);
    wxLogError(wxT("%s Unable to listen for halo info: %s"), m_ri->m_name.c_str(), error.c_str());
  }
  return g_HaloInfoSocket;
}

void NavicoReceive::ReleaseInfoSocket(void) {
  wxCriticalSectionLocker lock(g_HaloInfoSocketLock);
  if (g_HaloInfoSocket != INVALID_SOCKET) {
    closesocket(g_HaloInfoSocket);
    g_HaloInfoSocket = INVALID_SOCKET;
  }
}

static halo_heading_packet g_heading_msg = {
    {'N', 'K', 'O', 'E'},  // marker
    {0, 1, 0x90, 0x02},    // u00 bytes containing '00 01 90 02'
    0,                     // counter
    {0, 0, 0x10, 0, 0, 0x14, 0, 0, 4, 0, 0, 0, 0, 0, 5, 0x3C, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20},  // u01
    {0x12, 0xf1},                                                                                // u02
    {0x01, 0x00},                                                                                // u03
    0,                                                                                           // epoch
    2,                                                                                           // u04
    0,                                                                                           // u05a, likely position
    0,                                                                                           // u05b, likely position
    {0xff},                                                                                      // u06
    0,                                                                                           // heading
    {0xff, 0x7f, 0x79, 0xf8, 0xfc}                                                               // u07
};

static halo_mystery_packet g_mystery_msg = {
    {'N', 'K', 'O', 'E'},  // marker
    {0, 1, 0x90, 0x02},    // u00 bytes containing '00 01 90 02'
    0,                     // counter
    {0, 0, 0x10, 0, 0, 0x14, 0, 0, 4, 0, 0, 0, 0, 0, 5, 0x3C, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20},  // u01
    {0x02, 0xf8},                                                                                // u02
    {0x01, 0x00},                                                                                // u03
    0,                                                                                           // epoch
    2,                                                                                           // u04
    0,                                                                                           // u05a, likely position
    0,                                                                                           // u05b, likely position
    {0xff},                                                                                      // u06
    {0xfc},                                                                                      // u07
    0,                                                                                           // mystery1
    0,                                                                                           // mystery2
    {0xff, 0xff}                                                                                 // u08
};

static uint16_t g_counter;

void NavicoReceive::SendHeadingPacket() {
  NavicoControl *control = (NavicoControl *)m_ri->m_control;

  if (control != NULL) {
    g_counter++;
    g_heading_msg.counter = htons(g_counter);
    g_heading_msg.epoch = wxGetUTCTimeMillis();
    g_heading_msg.heading = (uint16_t)(m_pi->GetHeadingTrue() * 63488.0 / 360.0);

    LOG_TRANSMIT(wxT("%s SendHeadingPacket ctr=%u hdt=%g hdg=%u"), m_ri->m_name.c_str(), ntohs(g_heading_msg.counter),
                 m_pi->GetHeadingTrue(), g_heading_msg.heading);

    control->TransmitCmd(haloInfoAddress, (uint8_t *)&g_heading_msg, sizeof g_heading_msg);
  }
}

void NavicoReceive::SendMysteryPacket() {
  NavicoControl *control = (NavicoControl *)m_ri->m_control;

  if (control != NULL) {
    g_counter++;
    g_mystery_msg.counter = htons(g_counter);
    g_mystery_msg.epoch = wxGetUTCTimeMillis();
    g_mystery_msg.mystery1 = 0;
    g_mystery_msg.mystery2 = 0;

    LOG_TRANSMIT(wxT("%s SendMysteryPacket ctr=%u"), m_ri->m_name.c_str(), ntohs(g_mystery_msg.counter));

    control->TransmitCmd(haloInfoAddress, (uint8_t *)&g_mystery_msg, sizeof g_mystery_msg);
  }
}

/*
 * Entry
 *
 * Called by wxThread when the new thread is running.
 * It should remain running until Shutdown is called.
 */
void *NavicoReceive::Entry(void) {
  int r = 0;
  int no_data_timeout = 0;
  int no_spoke_timeout = 0;
  union {
    sockaddr_storage addr;
    sockaddr_in ipv4;
  } rx_addr;
  socklen_t rx_len;

  uint8_t data[sizeof(radar_frame_pkt)];
  m_interface_array = 0;
  m_interface = 0;
  NetworkAddress radar_address = NetworkAddress();

  SOCKET dataSocket = INVALID_SOCKET;
  SOCKET reportSocket = INVALID_SOCKET;
  SOCKET infoSocket = INVALID_SOCKET;

  LOG_VERBOSE(wxT("%s thread starting"), m_ri->m_name.c_str());
  reportSocket = GetNewReportSocket();  // Start using the same interface_addr as previous time

  while (m_receive_socket != INVALID_SOCKET) {
    if (reportSocket == INVALID_SOCKET) {
      reportSocket = PickNextEthernetCard();
      if (reportSocket != INVALID_SOCKET) {
        no_data_timeout = 0;
        no_spoke_timeout = 0;
      }
    }
    if (!radar_address.IsNull()) {
      // If we have detected a radar antenna at this address, start opening more sockets.
      // We do this later for 2 reasons:
      // - Resource consumption
      // - Timing. If we start processing radar data before the rest of the system
      //           is initialized then we get ordering/race condition issues.
      if (dataSocket == INVALID_SOCKET) {
        dataSocket = GetNewDataSocket();
      }
      if (infoSocket == INVALID_SOCKET) {
        // One of the two Halo radars will obtain an InfoSocket.
        infoSocket = GetNewInfoSocket();
      }
    } else {
      if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
      }
      if (infoSocket != INVALID_SOCKET) {
        ReleaseInfoSocket();
        infoSocket = INVALID_SOCKET;
      }
    }

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
    if (infoSocket != INVALID_SOCKET) {
      FD_SET(infoSocket, &fdin);
      maxFd = MAX(infoSocket, maxFd);
    }

    wxLongLong start = wxGetUTCTimeMillis();
    int64_t wait = MILLIS_PER_SELECT - (start.GetValue() % MILLIS_PER_SELECT);

    struct timeval tv = {0, (int)(wait * 1000)};
    r = select(maxFd + 1, &fdin, 0, 0, &tv);
    wxLongLong now = wxGetUTCTimeMillis();
    LOG_RECEIVE(wxT("%s select maxFd=%d r=%d elapsed=%lld"), m_ri->m_name.c_str(), maxFd, r, now - start);

    if (r > 0) {
      if (m_receive_socket != INVALID_SOCKET && FD_ISSET(m_receive_socket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(m_receive_socket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          LOG_VERBOSE(wxT("%s received stop instruction"), m_ri->m_name.c_str());
          break;
        }
      }

      if (dataSocket != INVALID_SOCKET && FD_ISSET(dataSocket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(dataSocket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          ProcessFrame(data, (size_t)r);
          no_data_timeout = -15;
          no_spoke_timeout = -5;
        } else {
          closesocket(dataSocket);
          dataSocket = INVALID_SOCKET;
          wxLogError(wxT("%s illegal frame"), m_ri->m_name.c_str());
        }
      }

      if (reportSocket != INVALID_SOCKET && FD_ISSET(reportSocket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(reportSocket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          if (ProcessReport(data, (size_t)r)) {
            if (radar_address.IsNull()) {
              radar_address.addr = rx_addr.ipv4.sin_addr;
              radar_address.port = rx_addr.ipv4.sin_port;
              wxCriticalSectionLocker lock(m_lock);
              m_ri->DetectedRadar(m_interface_addr, radar_address);  // enables transmit data
              DetectedRadar(radar_address);

              // the dataSocket is opened in the next loop

              if (m_ri->m_state.GetValue() == RADAR_OFF) {
                LOG_INFO(wxT("%s detected at %s"), m_ri->m_name.c_str(), radar_address.FormatNetworkAddress());
                m_ri->m_state.Update(RADAR_STANDBY);
              }
            }
            no_data_timeout = SECONDS_SELECT(-15);
          }
        } else {
          wxLogError(wxT("%s illegal report"), m_ri->m_name.c_str());
          closesocket(reportSocket);
          reportSocket = INVALID_SOCKET;
        }
      }

      if (infoSocket != INVALID_SOCKET && FD_ISSET(infoSocket, &fdin)) {
        rx_len = sizeof(rx_addr);
        r = recvfrom(infoSocket, (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
        if (r > 0) {
          NetworkAddress mfd_address;
          mfd_address.addr = rx_addr.ipv4.sin_addr;
          mfd_address.port = 0;
          if (m_interface_addr == mfd_address) {
            LOG_RECEIVE(wxT("%s active mfd detected at %s but that is us"), m_ri->m_name.c_str(),
                        mfd_address.FormatNetworkAddress());
          } else {
            LOG_RECEIVE(wxT("%s active mfd detected at %s"), m_ri->m_name.c_str(), mfd_address.FormatNetworkAddress());
            m_halo_received_info = wxGetUTCTimeMillis();
          }
          IF_LOG_AT(LOGLEVEL_RECEIVE, m_pi->logBinaryData(m_ri->m_name, data, r));

          halo_heading_packet *msg = (halo_heading_packet *)data;

          if (msg->u02[0] == 0x12 && msg->u02[1] == 0xf1) {
            double heading = (double)msg->heading * 360.0 / 63488.0;  // assume that this is a true heading ?
            if (m_pi->m_heading_source <= HEADING_FIX_COG || m_pi->m_heading_source >= HEADING_RADAR_HDM) {
              LOG_RECEIVE(wxT("Received and set radar_heading from network %f"), heading);
              m_pi->SetRadarHeading(heading, true);  // only set HEADING_RADAR_HDT if nothing better is available
            }

            LOG_RECEIVE(wxT("msg.counter = %u"), msg->counter);
            LOG_RECEIVE(wxT("msg.epoch   = %lld"), msg->epoch);
            LOG_RECEIVE(wxT("msg.heading = %u -> %f"), msg->heading, (double)msg->heading * 360.0 / 63488.0);
            LOG_RECEIVE(wxT("msg.u05a    = %x"), msg->u05a);
            LOG_RECEIVE(wxT("msg.u05b    = %x"), msg->u05b);
          } else {
            halo_mystery_packet *msg2 = (halo_mystery_packet *)data;
            LOG_RECEIVE(wxT("msg.counter = %u"), msg2->counter);
            LOG_RECEIVE(wxT("msg.epoch   = %lld"), msg2->epoch);
            LOG_RECEIVE(wxT("msg.mystery1 = %u"), msg2->mystery1);
            LOG_RECEIVE(wxT("msg.mystery2 = %u"), msg2->mystery2);
          }
        }
      }

    } else {  // no data received -> select timeout
      if (no_data_timeout >= SECONDS_SELECT(2)) {
        no_data_timeout = 0;
        if (reportSocket != INVALID_SOCKET) {
          closesocket(reportSocket);
          reportSocket = INVALID_SOCKET;
          m_ri->m_state.Update(RADAR_OFF);
          m_interface_addr = NetworkAddress();
          radar_address = NetworkAddress();
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

    if (m_pi->m_heading_source > HEADING_FIX_COG && m_pi->m_heading_source < HEADING_RADAR_HDM) {
      LOG_TRANSMIT(wxT("%s infoSocket=%d received=%lld sent=%lld\n"), m_ri->m_name.c_str(), infoSocket, now - m_halo_received_info,
                   now - m_halo_sent_heading);
      if (infoSocket != INVALID_SOCKET && m_halo_received_info + 10000 < now) {
        if (m_halo_sent_heading + 100 < now) {
          SendHeadingPacket();
          m_halo_sent_heading = now;
        }
        if (m_halo_sent_mystery + 250 < now) {
          SendMysteryPacket();
          m_halo_sent_mystery = now;
        }
      }
    }

    if (!(m_info == m_ri->GetRadarLocationInfo())) {
      // Navicolocate modified the RadarInfo in settings
      closesocket(reportSocket);
      reportSocket = INVALID_SOCKET;
    };

    if (reportSocket == INVALID_SOCKET) {
      // If we closed the reportSocket then close the command and data socket
      if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
        dataSocket = INVALID_SOCKET;
      }
      if (infoSocket != INVALID_SOCKET) {
        ReleaseInfoSocket();
        infoSocket = INVALID_SOCKET;
      }
    }

  }  // endless loop until thread destroy

  if (dataSocket != INVALID_SOCKET) {
    closesocket(dataSocket);
  }
  if (infoSocket != INVALID_SOCKET) {
    ReleaseInfoSocket();
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
  LOG_VERBOSE(wxT("%s receive thread sleeping"), m_ri->m_name.c_str());
  wxMilliSleep(1000);
#endif
  LOG_VERBOSE(wxT("%s receive thread stopping"), m_ri->m_name.c_str());
  m_is_shutdown = true;
  return 0;
}

/**
 * TODO: This function should really be removed, it is undesired to set RadarType on the fly, will create inconsistencies, (DF)
 */
void NavicoReceive::SetRadarType(RadarType t) {
  m_ri->m_radar_type = t;
  // m_pi->m_pMessageBox->SetRadarType(t);
}

void NavicoReceive::DetectedRadar(NetworkAddress &radar_address) {
  m_ri->DetectedRadar(m_interface_addr, radar_address);  // enables transmit data

  // The if tests should be superfluous, just be extra careful ...
  if (!m_info.send_command_addr.IsNull() && m_ri->m_control) {
    NavicoControl *control = (NavicoControl *)m_ri->m_control;
    control->SetSendAddress(m_info.send_command_addr);
  }
}

/*
 RADAR REPORTS

 The radars send various reports. The first 2 bytes indicate what the report type is.
 The types seen on a BR24 are:

 2nd byte C4:   01 02 03 04 05 07 08
 2nd byte F5:   08 0C 0D 0F 10 11 12 13 14

 Not definitive list for
 4G radars only send the C4 data.

*/

//
// The following is the received radar state. It sends this regularly
// but especially after something sends it a state change.
//
#pragma pack(push, 1)

struct RadarReport_01C4_18 {  // 01 C4 with length 18
  uint8_t what;               // 0   0x01
  uint8_t command;            // 1   0xC4
  uint8_t radar_status;       // 2
  uint8_t field3;             // 3
  uint8_t field4;             // 4
  uint8_t field5;             // 5
  uint16_t field6;            // 6-7
  uint16_t field8;            // 8-9
  uint16_t field10;           // 10-11
};

struct RadarReport_02C4_99 {       // length 99
  uint8_t what;                    // 0   0x02
  uint8_t command;                 // 1 0xC4
  uint32_t range;                  //  2-3   0x06 0x09
  uint8_t field4;                  // 6    0
  uint8_t mode;                    // 7    mode (0 = custom, 1 = harbor, 2 = offshore, 4 = bird, 5 = weather)
  uint32_t field8;                 // 8-11   1
  uint8_t gain;                    // 12
  uint8_t sea_auto;                // 13  0 = off, 1 = harbour, 2 = offshore
  uint8_t field14;                 // 14
  uint16_t field15;                // 15-16
  uint32_t sea;                    // 17-20   sea state (17)
  uint8_t field21;                 // 21
  uint8_t rain;                    // 22   rain clutter
  uint8_t field23;                 // 23
  uint32_t field24;                // 24-27
  uint32_t field28;                // 28-31
  uint8_t field32;                 // 32
  uint8_t field33;                 // 33
  uint8_t interference_rejection;  // 34
  uint8_t field35;                 // 35
  uint8_t field36;                 // 36
  uint8_t field37;                 // 37
  uint8_t target_expansion;        // 38
  uint8_t field39;                 // 39
  uint8_t field40;                 // 40
  uint8_t field41;                 // 41
  uint8_t target_boost;            // 42
};

#define REPORT_TYPE_BR24 (0x0f)
#define REPORT_TYPE_3G (0x08)
#define REPORT_TYPE_4G (0x01)
#define REPORT_TYPE_HALO (0x00)

struct RadarReport_03C4_129 {
  uint8_t what;
  uint8_t command;
  uint8_t radar_type;  // I hope! 01 = 4G and new 3G, 08 = 3G, 0F = BR24, 00 = HALO
  uint8_t u00[55];     // Lots of unknown
  uint16_t firmware_date[16];
  uint16_t firmware_time[16];
  uint8_t u01[7];
};

struct RadarReport_04C4_66 {   // 04 C4 with length 66
  uint8_t what;                // 0   0x04
  uint8_t command;             // 1   0xC4
  uint32_t field2;             // 2-5
  uint16_t bearing_alignment;  // 6-7
  uint16_t field8;             // 8-9
  uint16_t antenna_height;     // 10-11
  uint32_t field12;            // 12-15  0x00
  uint8_t field16[3];          // 16-18  0x00
  uint8_t accent_light;        // 19     accent light 0..3
};

struct RadarReport_08C4_18 {             // 08 c4  length 18
  uint8_t what;                          // 0  0x08
  uint8_t command;                       // 1  0xC4
  uint8_t field2;                        // 2
  uint8_t local_interference_rejection;  // 3
  uint8_t scan_speed;                    // 4
  uint8_t sls_auto;                      // 5 installation: sidelobe suppression auto
  uint8_t field6;                        // 6
  uint8_t field7;                        // 7
  uint8_t field8;                        // 8
  uint8_t side_lobe_suppression;         // 9 installation: sidelobe suppression
  uint16_t field10;                      // 10-11
  uint8_t noise_rejection;               // 12    noise rejection
  uint8_t target_sep;                    // 13
  uint8_t field11;                       // 14
  uint8_t field12;                       // 15
  uint8_t field13;                       // 16
  uint8_t field14;                       // 17
};

struct RadarReport_08C4_21 {
  RadarReport_08C4_18 old;
  uint8_t doppler_state;
  uint16_t doppler_speed;
};

struct RadarReport_12C4_66 {  // 12 C4 with length 66
  // Device Serial number is sent once upon network initialization only
  uint8_t what;          // 0   0x12
  uint8_t command;       // 1   0xC4
  uint8_t serialno[12];  // 2-13 Device serial number at 3G (All?)
};

struct RadarReport_01B2 {
  char serialno[16];  // ASCII serial number, zero terminated
  uint8_t u1[18];
  PackedAddress addr1;   // EC0608201970
  uint8_t u2[4];         // 11000000
  PackedAddress addr2;   // EC0607161A26
  uint8_t u3[10];        // 1F002001020010000000
  PackedAddress addr3;   // EC0608211971
  uint8_t u4[4];         // 11000000
  PackedAddress addr4;   // EC0608221972
  uint8_t u5[10];        // 10002001030010000000
  PackedAddress addr5;   // EC0608231973
  uint8_t u6[4];         // 11000000
  PackedAddress addr6;   // EC0608241974
  uint8_t u7[4];         // 12000000
  PackedAddress addr7;   // EC0608231975
  uint8_t u8[10];        // 10002002030010000000
  PackedAddress addr8;   // EC0608251976
  uint8_t u9[4];         // 11000000
  PackedAddress addr9;   // EC0608261977
  uint8_t u10[4];        // 12000000
  PackedAddress addr10;  // EC0608251978
  uint8_t u11[10];       // 12002001030010000000
  PackedAddress addr11;  // EC0608231979
  uint8_t u12[4];        // 11000000
  PackedAddress addr12;  // EC060827197A
  uint8_t u13[4];        // 12000000
  PackedAddress addr13;  // EC060823197B
  uint8_t u14[10];       // 12002002030010000000
  PackedAddress addr14;  // EC060825197C
  uint8_t u15[10];       // 11000000
  PackedAddress addr15;  // EC060828197D
  uint8_t u16[10];       // 12000000
  PackedAddress addr16;  // EC060825197E
};

#pragma pack(pop)

static void AppendChar16String(wxString &dest, uint16_t *src) {
  for (; *src; src++) {
    wchar_t wc = (wchar_t)*src;
    dest << wc;
  }
}

bool NavicoReceive::ProcessReport(const uint8_t *report, size_t len) {
  time_t now = time(0);

  m_ri->resetTimeout(now);

  LOG_BINARY_REPORTS(wxString::Format(wxT("%s report"), m_ri->m_name.c_str()), report, len);

  if (report[1] == 0xC4) {
    // Looks like a radar report. Is it a known one?
    switch ((len << 8) + report[0]) {
      case (18 << 8) + 0x01: {  //  length 18, 01 C4
        RadarReport_01C4_18 *s = (RadarReport_01C4_18 *)report;
        // Radar status in byte 2
        if (s->radar_status != m_radar_status) {
          m_radar_status = s->radar_status;
          wxString stat;

          switch (m_radar_status) {
            case 0x01:
              m_ri->m_state.Update(RADAR_STANDBY);
              LOG_VERBOSE(wxT("%s reports status STANDBY"), m_ri->m_name.c_str());
              stat = _("Standby");
              break;
            case 0x02:
              m_ri->m_state.Update(RADAR_TRANSMIT);
              LOG_VERBOSE(wxT("%s reports status TRANSMIT"), m_ri->m_name.c_str());
              stat = _("Transmit");
              break;
            case 0x05:
              m_ri->m_state.Update(RADAR_SPINNING_UP);
              LOG_VERBOSE(wxT("%s reports status SPINNING UP"), m_ri->m_name.c_str());
              stat = _("Waking up");
              break;
            default:
              LOG_BINARY_RECEIVE(wxT("received unknown radar status"), report, len);
              stat = _("Unknown status");
              break;
          }

          wxString s = wxString::Format(wxT("IP %s %s"), m_ri->m_radar_address.FormatNetworkAddress(), stat.c_str());
          if (RadarOrder[m_ri->m_radar_type] >= RO_PRIMARY) {
            RadarLocationInfo info = m_ri->GetRadarLocationInfo();
            s << wxT("\n") << _("Serial #") << info.serialNr;
          }
          SetInfoStatus(s);
        }
        break;
      }

      case (99 << 8) + 0x02: {  // length 99, 02 C4
        RadarReport_02C4_99 *s = (RadarReport_02C4_99 *)report;
        RadarControlState state;

        state = (s->field8 > 0) ? RCS_AUTO_1 : RCS_MANUAL;
        m_ri->m_gain.Update(s->gain * 100 / 255, state);

        m_ri->m_rain.Update(s->rain * 100 / 255);

        state = (RadarControlState)(RCS_MANUAL + s->sea_auto);
        m_ri->m_sea.Update(s->sea * 100 / 255, state);

        m_ri->m_mode.Update(s->mode);
        m_ri->m_target_boost.Update(s->target_boost);
        m_ri->m_interference_rejection.Update(s->interference_rejection);
        m_ri->m_target_expansion.Update(s->target_expansion);
        m_ri->m_range.Update(s->range / 10);

        LOG_RECEIVE(wxT("%s state range=%u gain=%u sea=%u rain=%u if_rejection=%u tgt_boost=%u tgt_expansion=%u"),
                    m_ri->m_name.c_str(), s->range, s->gain, s->sea, s->rain, s->interference_rejection, s->target_boost,
                    s->target_expansion);
        break;
      }

      case (129 << 8) + 0x03: {  // 129 bytes starting with 03 C4
        RadarReport_03C4_129 *s = (RadarReport_03C4_129 *)report;
        LOG_RECEIVE(wxT("%s RadarReport_03C4_129 radar_type=%u"), m_ri->m_name.c_str(), s->radar_type);

        switch (s->radar_type) {
          case REPORT_TYPE_BR24:
            if (m_ri->m_radar_type != RT_BR24) {
              LOG_INFO(wxT("%s radar report tells us this a Navico BR24"), m_ri->m_name.c_str());
              SetRadarType(RT_BR24);
            }
            break;
          case REPORT_TYPE_3G:
            if (m_ri->m_radar_type != RT_3G && m_ri->m_radar_type != RT_BR24) {
              LOG_INFO(wxT("%s radar report tells us this an old Navico 3G, use BR24 instead"), m_ri->m_name.c_str());
              SetRadarType(RT_BR24);
            }
            break;
          case REPORT_TYPE_4G:
            if (m_ri->m_radar_type != RT_4GA && m_ri->m_radar_type != RT_4GB && m_ri->m_radar_type != RT_3G) {
              LOG_INFO(wxT("%s radar report tells us this a Navico 4G or a modern 3G"), m_ri->m_name.c_str());
              if (m_ri->m_radar_type == RT_HaloB) {
                SetRadarType(RT_4GB);
              } else {
                SetRadarType(RT_4GA);
              }
            }
            break;
          case REPORT_TYPE_HALO:
            if (!IS_HALO) {
              LOG_INFO(wxT("%s radar report tells us this a Navico HALO"), m_ri->m_name.c_str());
              if (m_ri->m_radar_type == RT_4GB) {
                SetRadarType(RT_HaloB);
              } else {
                SetRadarType(RT_HaloA);
              }
            }
            break;
          default:
            LOG_INFO(wxT("%s: Unknown radar_type %u"), m_ri->m_name.c_str(), s->radar_type);
            return false;
        }

        wxString ts;

        ts << wxT("Firmware ");
        AppendChar16String(ts, s->firmware_date);
        ts << wxT(" ");
        AppendChar16String(ts, s->firmware_time);

        SetFirmware(ts);

        break;
      }

      case (66 << 8) + 0x04: {  // 66 bytes starting with 04 C4
        RadarReport_04C4_66 *data = (RadarReport_04C4_66 *)report;

        // bearing alignment
        int ba = (int)data->bearing_alignment / 10;
        if (ba > 180) {
          ba = ba - 360;
        }
        m_ri->m_bearing_alignment.Update(ba);

        // antenna height
        m_ri->m_antenna_height.Update(data->antenna_height / 1000);

        // accent light
        m_ri->m_accent_light.Update(data->accent_light);

        LOG_RECEIVE(wxT("%s bearing_alignment=%f antenna_height=%umm accent_light=%u"), m_ri->m_name.c_str(),
                    (int)data->bearing_alignment / 10.0, data->antenna_height, data->accent_light);
        break;
      }

#ifdef TODO
      case (564 << 8) + 0x05: {  // length 564, 05 C4
        // Content unknown, but we know that BR24 radomes send this
        LOG_RECEIVE(wxT("%s received familiar BR24 report"), m_ri->m_name.c_str());

        if (m_ri->m_radar_type == RT_UNKNOWN) {
          LOG_INFO(wxT("%s radar report tells us this a Navico BR24"), m_ri->m_name.c_str());
          m_ri->m_radar_type = RT_BR24;
          m_pi->m_pMessageBox->SetRadarType(RT_BR24);
        }
        break;
      }
#endif

      case (21 << 8) + 0x08: {  // length 21, 08 C4
                                // contains Doppler data in extra 3 bytes
        RadarReport_08C4_21 *s08 = (RadarReport_08C4_21 *)report;

        LOG_RECEIVE(wxT("%s %u 08C4: doppler=%d speed=%d, state=%d"), m_ri->m_name.c_str(), m_ri->m_radar, s08->doppler_state,
                    s08->doppler_speed, s08->doppler_state);
        // TODO: Doppler speed

        m_ri->m_doppler.Update(s08->doppler_state);
        m_ri->ComputeColourMap();
      }  // FALLTHRU to old length

      case (18 << 8) + 0x08: {  // length 18, 08 C4
        // contains scan speed, noise rejection and target_separation and sidelobe suppression
        RadarReport_08C4_18 *s08 = (RadarReport_08C4_18 *)report;

        LOG_RECEIVE(wxT("%s %u 08C4: scanspeed=%d noise=%u target_sep=%u"), m_ri->m_name.c_str(), m_ri->m_radar, s08->scan_speed,
                    s08->noise_rejection, s08->target_sep);
        LOG_RECEIVE(wxT("%s %u 08C4: f2=%u f6=%u f7=%u f8=%u f10=%u"), m_ri->m_name.c_str(), m_ri->m_radar, s08->field2,
                    s08->field6, s08->field7, s08->field8, s08->field10);
        LOG_RECEIVE(wxT("%s %u 08C4: f11=%u f12=%u f13=%u f14=%u"), m_ri->m_name.c_str(), m_ri->m_radar, s08->field11, s08->field12,
                    s08->field13, s08->field14);
        LOG_RECEIVE(wxT("%s %u 08C4: if=%u slsa=%u sls=%u"), m_ri->m_name.c_str(), m_ri->m_radar, s08->local_interference_rejection,
                    s08->sls_auto, s08->side_lobe_suppression);

        m_ri->m_scan_speed.Update(s08->scan_speed);
        m_ri->m_noise_rejection.Update(s08->noise_rejection);
        m_ri->m_target_separation.Update(s08->target_sep);
        m_ri->m_side_lobe_suppression.Update(s08->side_lobe_suppression * 100 / 255, s08->sls_auto ? RCS_AUTO_1 : RCS_MANUAL);
        m_ri->m_local_interference_rejection.Update(s08->local_interference_rejection);

        break;
      }

      case (66 << 8) + 0x12: {  // 66 bytes starting with 12 C4
        RadarReport_12C4_66 *s = (RadarReport_12C4_66 *)report;
        wxString sn = "#";
        sn << s->serialno;
        LOG_INFO(wxT("%s serial number is: %s"), m_ri->m_name.c_str(), sn);
        LOG_RECEIVE(wxT("%s RadarReport_12C4_66 serialno=%s"), m_ri->m_name.c_str(), sn);
        break;
      }

      default: {
        if (m_pi->m_settings.verbose >= 2) {
          wxString rep;

          rep << m_ri->m_name << wxT(" received unknown report");
        }
        break;
      }
    }
    return true;
  } else if (report[1] == 0xF5) {
#ifdef TODO
    // Looks like a radar report. Is it a known one?
    switch ((len << 8) + report[0]) {
      case (16 << 8) + 0x0f:
        if (m_ri->m_radar_type == RT_UNKNOWN) {
          LOG_INFO(wxT("%s radar report tells us this a Navico BR24"), m_ri->m_name.c_str());
          m_ri->m_radar_type = RT_BR24;
          m_pi->m_pMessageBox->SetRadarType(RT_BR24);
        }

        break;

      case (8 << 8) + 0x10:
      case (10 << 8) + 0x12:
      case (46 << 8) + 0x13:
        // Content unknown, but we know that BR24 radomes send this
        break;

      default:
        break;
    }
#endif
    return true;
  } else if (report[0] == 0x11 && report[1] == 0xC6) {
    LOG_RECEIVE(wxT("%s received heartbeat"), m_ri->m_name.c_str());
  } else if (report[0] == 01 && report[1] == 0xB2) {  // Common Navico message from 4G++
    RadarReport_01B2 *data = (RadarReport_01B2 *)report;

#define LOG_ADDR_N(n) LOG_RECEIVE(wxT("%s addr%d = %s"), m_ri->m_name.c_str(), n, FormatPackedAddress(data->addr##n));

    IF_LOG_AT_LEVEL(LOGLEVEL_RECEIVE) {
      LOG_ADDR_N(1);
      LOG_ADDR_N(2);
      LOG_ADDR_N(3);
      LOG_ADDR_N(4);
      LOG_ADDR_N(5);
      LOG_ADDR_N(6);
      LOG_ADDR_N(7);
      LOG_ADDR_N(8);
      LOG_ADDR_N(9);
      LOG_ADDR_N(10);
      LOG_ADDR_N(11);
      LOG_ADDR_N(12);
      LOG_ADDR_N(13);
      LOG_ADDR_N(14);
      LOG_ADDR_N(15);
      LOG_ADDR_N(16);
    }
  } else {
    LOG_BINARY_RECEIVE(wxT("received unknown message"), report, len);
  }
  return false;
}

// Called from the main thread to stop this thread.
// We send a simple one byte message to the thread so that it awakens from the select() call with
// this message ready for it to be read on 'm_receive_socket'. See the constructor in NavicoReceive.h
// for the setup of these two sockets.

void NavicoReceive::Shutdown() {
  if (m_send_socket != INVALID_SOCKET) {
    m_shutdown_time_requested = wxGetUTCTimeMillis();
    if (send(m_send_socket, "!", 1, MSG_DONTROUTE) > 0) {
      LOG_VERBOSE(wxT("%s requested receive thread to stop"), m_ri->m_name.c_str());
      return;
    }
  }
  LOG_INFO(wxT("%s receive thread will take long time to stop"), m_ri->m_name.c_str());
}

wxString NavicoReceive::GetInfoStatus() {
  wxString r;

  wxCriticalSectionLocker lock(m_lock);

  r << m_status;

  // Called on the UI thread, so be gentle
  if (m_firmware.length() > 0) {
    r << wxT("\n");
    r << m_firmware;
  }

  if (m_radar_status == 0 && m_pi->m_navico_locator) {
    m_pi->m_navico_locator->AppendErrors(r);
  }

  return r;
}

PLUGIN_END_NAMESPACE
