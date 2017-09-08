/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
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

#include "br24Transmit.h"

PLUGIN_BEGIN_NAMESPACE

br24Transmit::br24Transmit(br24radar_pi *pi, wxString name, int radar) {
  m_pi = pi;

  CLEAR_STRUCT(m_addr);
  m_addr.sin_family = AF_INET;

  static UINT8 radar_mcast_send_addr[2][4] = {{236, 6, 7, 10}, {236, 6, 7, 14}};

  static unsigned short radar_mcast_send_port[2] = {6680, 6658};

  memcpy(&m_addr.sin_addr.s_addr, radar_mcast_send_addr[radar % 2], sizeof(m_addr.sin_addr.s_addr));
  m_addr.sin_port = htons(radar_mcast_send_port[radar % 2]);
  m_name = name;
  m_radar_socket = INVALID_SOCKET;
}

br24Transmit::~br24Transmit() {
  if (m_radar_socket != INVALID_SOCKET) {
    closesocket(m_radar_socket);
    LOG_TRANSMIT(wxT("BR24radar_pi: %s transmit socket closed"), m_name.c_str());
  }
  LOG_VERBOSE(wxT("BR24radar_pi: %s transmit object destroyed"), m_name.c_str());
}

bool br24Transmit::Init(struct sockaddr_in *adr) {
  int r;
  int one = 1;

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
    r = bind(m_radar_socket, (struct sockaddr *)adr, sizeof(*adr));
  }

  if (r) {
    wxLogError(wxT("BR24radar_pi: Unable to create UDP sending socket"));
    // Might as well give up now
    return false;
  }

  LOG_TRANSMIT(wxT("BR24radar_pi: %s transmit socket open"), m_name);
  return true;
}

void br24Transmit::logBinaryData(const wxString &what, const UINT8 *data, int size) {
  wxString explain;
  int i = 0;

  explain.Alloc(size * 3 + 50);
  explain += wxT("BR24radar_pi: ") + m_name + wxT(" ");
  explain += what;
  explain += wxString::Format(wxT(" %d bytes: "), size);
  for (i = 0; i < size; i++) {
    explain += wxString::Format(wxT(" %02X"), data[i]);
  }
  LOG_TRANSMIT(explain);
}

bool br24Transmit::TransmitCmd(const UINT8 *msg, int size) {
  if (m_pi->m_settings.emulator_on) {
    wxLogError(wxT("BR24radar_pi: ignoring transmit command in emulator mode"));
    return false;
  }

  if (m_radar_socket == INVALID_SOCKET) {
    wxLogError(wxT("BR24radar_pi: Unable to transmit command to unknown radar"));
    return false;
  }
  if (sendto(m_radar_socket, (char *)msg, size, 0, (struct sockaddr *)&m_addr, sizeof(m_addr)) < size) {
    wxLogError(wxT("BR24radar_pi: Unable to transmit command to %s: %s"), m_name, SOCKETERRSTR);
    return false;
  }
  IF_LOG_AT(LOGLEVEL_TRANSMIT, logBinaryData(wxT("transmit"), msg, size));
  return true;
}

void br24Transmit::RadarTxOff() {
  IF_LOG_AT(LOGLEVEL_VERBOSE | LOGLEVEL_TRANSMIT, wxLogMessage(wxT("BR24radar_pi: %s transmit: turn Off"), m_name));
  TransmitCmd(COMMAND_TX_OFF_A, sizeof(COMMAND_TX_OFF_A));
  TransmitCmd(COMMAND_TX_OFF_B, sizeof(COMMAND_TX_OFF_B));
}

void br24Transmit::RadarTxOn() {
  IF_LOG_AT(LOGLEVEL_VERBOSE | LOGLEVEL_TRANSMIT, wxLogMessage(wxT("BR24radar_pi: %s transmit: turn on"), m_name));
  TransmitCmd(COMMAND_TX_ON_A, sizeof(COMMAND_TX_ON_A));
  TransmitCmd(COMMAND_TX_ON_B, sizeof(COMMAND_TX_ON_B));
}

bool br24Transmit::RadarStayAlive() {
  LOG_TRANSMIT(wxT("BR24radar_pi: %s transmit: stay alive"), m_name);

  TransmitCmd(COMMAND_STAY_ON_A, sizeof(COMMAND_STAY_ON_A));
  TransmitCmd(COMMAND_STAY_ON_B, sizeof(COMMAND_STAY_ON_B));
  TransmitCmd(COMMAND_STAY_ON_C, sizeof(COMMAND_STAY_ON_C));
  return TransmitCmd(COMMAND_STAY_ON_D, sizeof(COMMAND_STAY_ON_D));
}

bool br24Transmit::SetRange(int meters) {
  if (meters >= 50 && meters <= 72704) {
    unsigned int decimeters = (unsigned int)meters * 10;
    UINT8 pck[] = {0x03,
                   0xc1,
                   (UINT8)((decimeters >> 0) & 0XFFL),
                   (UINT8)((decimeters >> 8) & 0XFFL),
                   (UINT8)((decimeters >> 16) & 0XFFL),
                   (UINT8)((decimeters >> 24) & 0XFFL)};
    LOG_VERBOSE(wxT("BR24radar_pi: %s transmit: range %d meters"), m_name, meters);
    return TransmitCmd(pck, sizeof(pck));
  }
  return false;
}

bool br24Transmit::SetControlValue(ControlType controlType, int value, int autoValue) {  // sends the command to the radar
  bool r = false;

  switch (controlType) {
    case CT_RANGE:
    case CT_TIMED_IDLE:
    case CT_TIMED_RUN:
    case CT_SCAN_AGE:
    case CT_TRANSPARENCY:
    case CT_REFRESHRATE:
    case CT_TARGET_TRAILS:
    case CT_TRAILS_MOTION:
    case CT_MAIN_BANG_SIZE:
    case CT_MAX:
    case CT_ANTENNA_FORWARD:
    case CT_ANTENNA_STARBOARD:
      // The above are not settings that are not radar commands. Made them explicit so the
      // compiler can catch missing control types.
      break;

    // Ordering the radar commands by the first byte value.
    // Some interesting holes here, seems there could be more commands!

    case CT_BEARING_ALIGNMENT: {  // to be consistent with the local bearing alignment of the pi
                                  // this bearing alignment works opposite to the one an a Lowrance display
      if (value < 0) {
        value += 360;
      }
      int v = value * 10;
      int v1 = v / 256;
      int v2 = v & 255;
      UINT8 cmd[4] = {0x05, 0xc1, (UINT8)v2, (UINT8)v1};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Bearing alignment: %d"), m_name, v);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_GAIN: {
      int v = (value + 1) * 255 / 100;
      if (v > 255) {
        v = 255;
      }
      UINT8 cmd[] = {0x06, 0xc1, 0, 0, 0, 0, (UINT8)autoValue, 0, 0, 0, (UINT8)v};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Gain: %d auto %d"), m_name, value, autoValue);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_SEA: {
      int v = (value + 1) * 255 / 100;
      if (v > 255) {
        v = 255;
      }
      UINT8 cmd[] = {0x06, 0xc1, 0x02, 0, 0, 0, autoValue, 0, 0, 0, (UINT8)v};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Sea: %d auto %d"), m_name, value, autoValue);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_RAIN: {  // Rain Clutter - Manual. Range is 0x01 to 0x50
      int v = (value + 1) * 255 / 100;
      if (v > 255) {
        v = 255;
      }
      UINT8 cmd[] = {0x06, 0xc1, 0x04, 0, 0, 0, 0, 0, 0, 0, (UINT8)v};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Rain: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_SIDE_LOBE_SUPPRESSION: {
      int v = value * 256 / 100;
      if (v > 255) {
        v = 255;
      }
      UINT8 cmd[] = {0x6, 0xc1, 0x05, 0, 0, 0, autoValue, 0, 0, 0, (UINT8)v};
      LOG_VERBOSE(wxT("BR24radar_pi: %s command Tx CT_SIDE_LOBE_SUPPRESSION: %d auto %d"), m_name, value, autoValue);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    // What would command 7 be?

    case CT_INTERFERENCE_REJECTION: {
      UINT8 cmd[] = {0x08, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Rejection: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_TARGET_EXPANSION: {
      UINT8 cmd[] = {0x09, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Target expansion: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_TARGET_BOOST: {
      UINT8 cmd[] = {0x0a, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Target boost: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    // What would command b through d be?

    case CT_LOCAL_INTERFERENCE_REJECTION: {
      if (value < 0) value = 0;
      if (value > 3) value = 3;
      UINT8 cmd[] = {0x0e, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s local interference rejection %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_SCAN_SPEED: {
      UINT8 cmd[] = {0x0f, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Scan speed: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    // What would command 10 through 20 be?

    case CT_NOISE_REJECTION: {
      UINT8 cmd[] = {0x21, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Noise rejection: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    case CT_TARGET_SEPARATION: {
      UINT8 cmd[] = {0x22, 0xc1, (UINT8)value};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Target separation: %d"), m_name, value);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }

    // What would command 23 through 2f be?

    case CT_ANTENNA_HEIGHT: {
      int v = value * 1000;  // radar wants millimeters, not meters :-)
      int v1 = v / 256;
      int v2 = v & 255;
      UINT8 cmd[10] = {0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0};
      LOG_VERBOSE(wxT("BR24radar_pi: %s Antenna height: %d"), m_name, v);
      r = TransmitCmd(cmd, sizeof(cmd));
      break;
    }
  }

  return r;
}

PLUGIN_END_NAMESPACE
