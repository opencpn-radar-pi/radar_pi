/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
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

#include "GarminxHDControl.h"

PLUGIN_BEGIN_NAMESPACE

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

#pragma pack(pop)

GarminxHDControl::GarminxHDControl(NetworkAddress sendAddress) {
  m_addr = sendAddress.GetSockAddrIn();  // addr part overwritten by actual radar addr

  m_radar_socket = INVALID_SOCKET;
  m_name = wxT("Navico radar");

  m_pi = 0;
  m_ri = 0;
  m_name = wxT("GarminxHD");
}

GarminxHDControl::~GarminxHDControl() {
  if (m_radar_socket != INVALID_SOCKET) {
    closesocket(m_radar_socket);
    LOG_TRANSMIT(wxT("%s transmit socket closed"), m_name.c_str());
  }
}

bool GarminxHDControl::Init(radar_pi *pi, RadarInfo *ri, NetworkAddress &ifadr, NetworkAddress &radaradr) {
  int r;
  int one = 1;

  m_addr.sin_addr = radaradr.addr;

  m_pi = pi;
  m_ri = ri;
  m_name = ri->m_name;

  if (m_radar_socket != INVALID_SOCKET) {
    closesocket(m_radar_socket);
  }
  m_radar_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (m_radar_socket == INVALID_SOCKET) {
    r = -1;
  } else {
    r = setsockopt(m_radar_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
  }

  if (!r) {
    struct sockaddr_in s = ifadr.GetSockAddrIn();

    r = ::bind(m_radar_socket, (struct sockaddr *)&s, sizeof(s));
  }

  if (r) {
    wxLogError(wxT("Unable to create UDP sending socket"));
    // Might as well give up now
    return false;
  }

  LOG_TRANSMIT(wxT("%s transmit socket open"), m_name);
  return true;
}

void GarminxHDControl::logBinaryData(const wxString &what, const void *data, int size) {
  wxString explain;
  const uint8_t *d = (const uint8_t *)data;
  int i = 0;

  explain.Alloc(size * 3 + 50);
  explain += wxT("") + m_name + wxT(" ");
  explain += what;
  explain += wxString::Format(wxT(" %d bytes: "), size);
  for (i = 0; i < size; i++) {
    explain += wxString::Format(wxT(" %02X"), d[i]);
  }
  LOG_TRANSMIT(explain);
}

bool GarminxHDControl::TransmitCmd(const void *msg, int size) {
  if (m_radar_socket == INVALID_SOCKET) {
    wxLogError(wxT("Unable to transmit command to unknown radar"));
    return false;
  }
  if (sendto(m_radar_socket, (char *)msg, size, 0, (struct sockaddr *)&m_addr, sizeof(m_addr)) < size) {
    wxLogError(wxT("Unable to transmit command to %s: %s"), m_name.c_str(), SOCKETERRSTR);
    return false;
  }
  IF_LOG_AT(LOGLEVEL_TRANSMIT, logBinaryData(wxString::Format(wxT("%s transmit"), m_name), msg, size));
  return true;
}

void GarminxHDControl::RadarTxOff() {
  IF_LOG_AT(LOGLEVEL_VERBOSE | LOGLEVEL_TRANSMIT, wxLogMessage(wxT("%s transmit: turn off"), m_name));

  rad_ctl_pkt_9 packet;
  packet.packet_type = 0x919;
  packet.len1 = sizeof(packet.parm1);
  packet.parm1 = 0;  // 0 for "off"

  TransmitCmd(&packet, sizeof(packet));
}

void GarminxHDControl::RadarTxOn() {
  IF_LOG_AT(LOGLEVEL_VERBOSE | LOGLEVEL_TRANSMIT, wxLogMessage(wxT("%s transmit: turn on"), m_name));

  rad_ctl_pkt_9 packet;
  packet.packet_type = 0x919;
  packet.len1 = sizeof(packet.parm1);
  packet.parm1 = 1;  // 1 for "on"

  TransmitCmd(&packet, sizeof(packet));
}

bool GarminxHDControl::RadarStayAlive() {
  // Garmin radars don't need a ping
  return true;
}

bool GarminxHDControl::SetRange(int meters) {
  if (meters >= 200 && meters <= 48 * 1852) {
    rad_ctl_pkt_12 packet;

    packet.packet_type = 0x91e;
    packet.len1 = sizeof(packet.parm1);
    packet.parm1 = meters;
    LOG_VERBOSE(wxT("%s transmit: range %d meters"), m_name.c_str(), meters);
    return TransmitCmd(&packet, sizeof(packet));
  }
  return false;
}

bool GarminxHDControl::SetControlValue(ControlType controlType, RadarControlItem &item, RadarControlButton *button) {
  bool r = false;
  int value = item.GetValue();
  RadarControlState state = item.GetState();

  rad_ctl_pkt_9 pck_9;
  rad_ctl_pkt_10 pck_10;
  rad_ctl_pkt_12 pck_12;

  pck_9.len1 = sizeof(pck_9.parm1);
  pck_10.len1 = sizeof(pck_10.parm1);
  pck_12.len1 = sizeof(pck_12.parm1);

  switch (controlType) {
    // The following are settings that are not radar commands. Made them explicit so the
    // compiler can catch missing control types.
    case CT_ACCENT_LIGHT:
    case CT_ANTENNA_SIZE:
    case CT_PARKING_ANGLE:
    case CT_ALL_TO_AUTO:
    case CT_ANTENNA_FORWARD:
    case CT_ANTENNA_STARBOARD:
    case CT_CENTER_VIEW:
    case CT_COLOR_GAIN:
    case CT_MAIN_BANG_SIZE:
    case CT_MAX:
    case CT_MODE:
    case CT_NONE:
    case CT_ORIENTATION:
    case CT_OVERLAY_CANVAS:
    case CT_RANGE:
    case CT_RANGE_ADJUSTMENT:
    case CT_REFRESHRATE:
    case CT_TARGET_ON_PPI:
    case CT_TARGET_TRAILS:
    case CT_THRESHOLD:
    case CT_TRAILS_MOTION:
    case CT_TRANSPARENCY:

    // The following are settings not supported by Garmin xHD.
    case CT_ANTENNA_HEIGHT:
    case CT_AUTOTTRACKDOPPLER:
    case CT_DISPLAY_TIMING:
    case CT_DOPPLER:
    case CT_DOPPLER_THRESHOLD:
    case CT_FTC:
    case CT_LOCAL_INTERFERENCE_REJECTION:
    case CT_MAIN_BANG_SUPPRESSION:
    case CT_NOISE_REJECTION:
    case CT_SEA_STATE:
    case CT_SIDE_LOBE_SUPPRESSION:
    case CT_STC:
    case CT_STC_CURVE:
    case CT_TARGET_BOOST:
    case CT_TARGET_EXPANSION:
    case CT_TARGET_SEPARATION:
    case CT_TUNE_COARSE:
    case CT_TUNE_FINE:
    case CT_NO_TRANSMIT_END_2:
    case CT_NO_TRANSMIT_END_3:
    case CT_NO_TRANSMIT_END_4:
    case CT_NO_TRANSMIT_START_2:
    case CT_NO_TRANSMIT_START_3:
    case CT_NO_TRANSMIT_START_4:

      break;

      // Ordering the radar commands by the first byte value.
      // Some interesting holes here, seems there could be more commands!

    case CT_BEARING_ALIGNMENT: {
      pck_12.packet_type = 0x930;
      pck_12.parm1 = value << 5;

      LOG_VERBOSE(wxT("%s Bearing alignment: %d"), m_name.c_str(), value);
      r = TransmitCmd(&pck_12, sizeof(pck_12));
      break;
    }

    case CT_NO_TRANSMIT_START_1: {
      // value is already in range -180 .. +180 which is what I think radar wants...
      if (state == RCS_OFF) {  // OFF
        pck_9.packet_type = 0x93f;
        pck_9.parm1 = 0;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else {
        pck_9.packet_type = 0x93f;
        pck_9.parm1 = 1;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_12.packet_type = 0x940;
        pck_12.parm1 = value * 32;
        r = TransmitCmd(&pck_12, sizeof(pck_12));
        m_ri->m_no_transmit_start[0].Update(value);  // necessary because we hacked "off" as auto value
      }
      LOG_VERBOSE(wxT("%s No Transmit Start: value=%d state=%d"), m_name.c_str(), value, (int)state);
      break;
    }

    case CT_NO_TRANSMIT_END_1: {
      // value is already in range -180 .. +180 which is what I think radar wants...
      if (state == RCS_OFF) {  // OFF
        pck_9.packet_type = 0x93f;
        pck_9.parm1 = 0;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else {
        pck_9.packet_type = 0x93f;
        pck_9.parm1 = 1;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_12.packet_type = 0x941;
        pck_12.parm1 = value * 32;
        r = TransmitCmd(&pck_12, sizeof(pck_12));
        m_ri->m_no_transmit_end[0].Update(value);  // necessary because we hacked "off" as auto value
      }
      LOG_VERBOSE(wxT("%s No Transmit End: value=%d state=%d"), m_name.c_str(), value, (int)state);
      break;
    }

    case CT_GAIN: {
      LOG_VERBOSE(wxT("%s Gain: value=%d state=%d"), m_name.c_str(), value, (int)state);

      if (state >= RCS_AUTO_1) {
        pck_9.packet_type = 0x924;
        pck_9.parm1 = 2;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_9.packet_type = 0x91d;
        pck_9.parm1 = (state == RCS_AUTO_1) ? 0 : 1;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else if (state == RCS_MANUAL) {
        pck_9.packet_type = 0x924;
        pck_9.parm1 = 0;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_10.packet_type = 0x925;
        pck_10.parm1 = value * 100;
        r = TransmitCmd(&pck_10, sizeof(pck_10));
      }
      break;
    }

    case CT_SEA: {
      LOG_VERBOSE(wxT("%s Sea: value=%d state=%d"), m_name.c_str(), value, (int)state);

      if (state >= RCS_AUTO_1) {
        pck_9.packet_type = 0x939;
        pck_9.parm1 = 2;  // auto
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_9.packet_type = 0x93b;
        pck_9.parm1 = state - RCS_AUTO_1;
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else if (state == RCS_OFF) {
        pck_9.packet_type = 0x939;
        pck_9.parm1 = 0;  // off
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else if (state == RCS_MANUAL) {
        pck_9.packet_type = 0x939;
        pck_9.parm1 = 1;  // manual
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_10.packet_type = 0x93a;
        pck_10.parm1 = value * 100;
        r = TransmitCmd(&pck_10, sizeof(pck_10));
      }
      break;
    }

    case CT_RAIN: {  // Rain Clutter - Manual. Range is 0x01 to 0x50
      LOG_VERBOSE(wxT("%s Rain: value=%d state=%d"), m_name.c_str(), value, (int)state);

      if (state == RCS_OFF) {
        pck_9.packet_type = 0x933;
        pck_9.parm1 = 0;  // off
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else if (state == RCS_MANUAL) {
        pck_9.packet_type = 0x933;
        pck_9.parm1 = 1;  // manual
        r = TransmitCmd(&pck_9, sizeof(pck_9));
        pck_10.packet_type = 0x934;
        pck_10.parm1 = value * 100;
        r = TransmitCmd(&pck_10, sizeof(pck_10));
      }
      break;
    }

    case CT_INTERFERENCE_REJECTION: {
      LOG_VERBOSE(wxT("%s Interference Rejection / Crosstalk: %d"), m_name.c_str(), value);
      pck_9.parm1 = value;

      pck_9.packet_type = 0x91b;
      r = TransmitCmd(&pck_9, sizeof(pck_9));

      pck_9.packet_type = 0x932;
      r = TransmitCmd(&pck_9, sizeof(pck_9));

      pck_9.packet_type = 0x2b9;
      r = TransmitCmd(&pck_9, sizeof(pck_9));
      break;
    }

    case CT_SCAN_SPEED: {
      LOG_VERBOSE(wxT("%s Scan speed: %d"), m_name.c_str(), value);
      pck_9.packet_type = 0x916;
      pck_9.parm1 = value * 2;

      r = TransmitCmd(&pck_9, sizeof(pck_9));
      break;
    }

    case CT_TIMED_IDLE: {
      LOG_VERBOSE(wxT("%s Timed idle: value=%d state=%d"), m_name.c_str(), value, (int)state);
      if (state == RCS_OFF) {
        pck_9.packet_type = 0x942;
        pck_9.parm1 = 0;  // off
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      } else if (state == RCS_MANUAL) {
        pck_10.packet_type = 0x943;
        pck_10.parm1 = value * 60;
        r = TransmitCmd(&pck_10, sizeof(pck_10));
        pck_9.packet_type = 0x942;
        pck_9.parm1 = 1;  // manual
        r = TransmitCmd(&pck_9, sizeof(pck_9));
      }
      break;
    }

    case CT_TIMED_RUN: {
      LOG_VERBOSE(wxT("%s Timed run: %d"), m_name.c_str(), value);
      pck_10.packet_type = 0x944;
      pck_10.parm1 = value * 60;
      r = TransmitCmd(&pck_10, sizeof(pck_10));
      break;
    }
  }

  return r;
}

PLUGIN_END_NAMESPACE
