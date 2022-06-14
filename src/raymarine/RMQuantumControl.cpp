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
 *           Martin Hassellov: testing the Raymarine radar
 *           Matt McShea: testing the Raymarine radar
 *           Al Grant for testing and decoding Quantum radar
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

#include "RMQuantumControl.h"

PLUGIN_BEGIN_NAMESPACE

bool RMQuantumControl::Init(radar_pi *pi, RadarInfo *ri, NetworkAddress &ifadr, NetworkAddress &radaradr) { return true; }

bool RMQuantumControl::TransmitCmd(const uint8_t *msg, int size) {
  if (m_ri->m_radar_location_info.send_command_addr.IsNull()) {
    wxLogError(wxT("%s Unable to transmit command to unknown radar"), m_name.c_str());
    IF_LOG_AT(LOGLEVEL_TRANSMIT, m_pi->logBinaryData(wxT("not transmitted"), msg, size));
    return false;
  } else {
    m_send_address = m_ri->m_radar_location_info.send_command_addr;
  }
  SOCKET tx_sock = INVALID_SOCKET;
  if (m_ri->m_receive != 0) {
    tx_sock = m_ri->m_receive->GetCommSocket();
  }
  if (tx_sock == INVALID_SOCKET) {
    wxLogError(wxT("%s INVALID_SOCKET, Unable to transmit command to unknown radar"), m_name.c_str());
    return false;
  }

  struct sockaddr_in send_sock_addr = m_send_address.GetSockAddrIn();

  int sendlen = sendto(tx_sock, (char *)msg, size, 0, (struct sockaddr *)&send_sock_addr, sizeof(send_sock_addr));
  if (sendlen < size) {
    wxLogError(wxT("%s Unable to transmit command: %s"), m_name.c_str(), SOCKETERRSTR);
    IF_LOG_AT(LOGLEVEL_TRANSMIT, m_pi->logBinaryData(wxT("TransmitCmd"), msg, size));
    return false;
  }
  IF_LOG_AT(LOGLEVEL_TRANSMIT, m_pi->logBinaryData(wxT("transmit"), msg, size));
  return true;
}

void RMQuantumControl::RadarTxOff() {
  uint8_t rd_msg_tx_control[] = {0x10, 0x00, 0x28, 0x00,
                                 0x00,  // Control value at offset 4 : 0 - off, 1 - on   //10 00 28 00 01 00 00 00
                                 0x00, 0x00, 0x00};
  rd_msg_tx_control[4] = 0;
  TransmitCmd(rd_msg_tx_control, sizeof(rd_msg_tx_control));
}

void RMQuantumControl::RadarTxOn() {
  uint8_t rd_msg_tx_control[] = {0x10, 0x00, 0x28, 0x00,
                                 0x01,  // Control value at offset 4 : 0 - off, 1 - on
                                 0x00, 0x00, 0x00};
  rd_msg_tx_control[4] = 1;
  TransmitCmd(rd_msg_tx_control, sizeof(rd_msg_tx_control));
};

static uint8_t stay_alive_1sec[] = {0x00, 0x00, 0x28, 0x00, 0x52, 0x61,
                                    0x64, 0x61, 0x72, 0x00, 0x00, 0x00};  // Quantum 1 sec stay alive

static uint8_t rd_msg_5s[] = {
    0x03, 0x89, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Quantum stay alive 5 sec message, 36 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9e, 0x03, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool RMQuantumControl::RadarStayAlive() {
  static int counter = 0;

  TransmitCmd(stay_alive_1sec, sizeof(stay_alive_1sec));

  if (counter == 0) TransmitCmd(rd_msg_5s, sizeof(rd_msg_5s));

  if (counter++ > 4) counter = 0;

  return true;
}

static uint8_t rd_msg_set_range[] = {0x01, 0x01, 0x28, 0x00, 0x00,
                                     0x0f,  // Quantum range index at pos 5
                                     0x00, 0x00};

bool RMQuantumControl::SetRange(int meters) {
  LOG_VERBOSE(wxT(" SetRangeMeters = %i"), meters);
  for (int i = 0; i < 20; i++) {
    if (meters <= m_ri->m_radar_ranges[i]) {
      SetRangeIndex(i);
      return true;
    }
  }
  return false;
}

void RMQuantumControl::SetRangeIndex(size_t index) {
  LOG_VERBOSE(wxT("SetRangeIndex index = %i"), index);
  rd_msg_set_range[5] = index;
  TransmitCmd(rd_msg_set_range, sizeof(rd_msg_set_range));
}

// Overview of Quantum radar conmmands as far as known
// Ordering the radar commands by the first byte value.
/*
00 00 28       Stay alive 1 sec
01 01 28       Range
01 03 28 00 00 00 00 00 Gain to manual
01 03 28 00 00 01 00 00 Gain to auto
02 03 28 00 00 xx 7e a3    Gain
05 03 28 00 00 00 00 00 Sea to manual
05 03 28 00 00 01 00 00 Sea to auto
06 03 28 00 00 xx 00 00 Sea
0b 03 28 00 00 00 00 00 Rain to manual
0b 03 28 00 00 00 00 00 Rain to auto
0c 03 28       Rain
0f 03 28 00 00 01 00 00 Target expansion on, 00 on 5 == off
10 00 28 00 01 00 00 00 Radar transmit ON. Controlling value at pos 4!
10 00 28 00 00 00 00 00 Radar transmit OFF
14 03 28       Mode, harbor 0, coastal 1, offshore 2, weather 3

*/

bool RMQuantumControl::SetControlValue(ControlType controlType, RadarControlItem &item, RadarControlButton *button) {
  bool r = false;
  int value = item.GetValue();
  RadarControlState state = item.GetState();
  int autoValue = 0;
  if (state > RCS_MANUAL) {
    autoValue = state - RCS_MANUAL;
  }

  switch (controlType) {
    case CT_ACCENT_LIGHT:
    case CT_ANTENNA_FORWARD:
    case CT_ANTENNA_HEIGHT:
    case CT_ANTENNA_STARBOARD:
    case CT_AUTOTTRACKDOPPLER:
    case CT_CENTER_VIEW:
    case CT_DISPLAY_TIMING:
    case CT_DOPPLER:
    case CT_FTC:
    case CT_LOCAL_INTERFERENCE_REJECTION:
    case CT_MAIN_BANG_SIZE:
    case CT_MAIN_BANG_SUPPRESSION:
    case CT_MAX:
    case CT_NOISE_REJECTION:
    case CT_NONE:
    case CT_NO_TRANSMIT_END:
    case CT_NO_TRANSMIT_START:
    case CT_ORIENTATION:
    case CT_OVERLAY_CANVAS:
    case CT_RANGE:
    case CT_RANGE_ADJUSTMENT:
    case CT_REFRESHRATE:
    case CT_SCAN_SPEED:
    case CT_SIDE_LOBE_SUPPRESSION:
    case CT_STC:
    case CT_STC_CURVE:
    case CT_TARGET_BOOST:
    case CT_TARGET_ON_PPI:
    case CT_TARGET_SEPARATION:
    case CT_TARGET_TRAILS:
    case CT_THRESHOLD:
    case CT_TIMED_IDLE:
    case CT_TIMED_RUN:
    case CT_TRAILS_MOTION:
    case CT_TRANSPARENCY:
    case CT_TUNE_COARSE:
    case CT_TUNE_FINE:

      // The above are not settings that are not radar commands or not supported by Quantum radar.
      // Made them explicit so the compiler can catch missing control types.
      break;

    case CT_GAIN: {
      uint8_t command_gain_set[] = {0x02, 0x03, 0x28, 0x00, 0x00,
                                    0x28,  // Quantum gain value at pos 5
                                    0x00, 0x00};

      uint8_t command_gain_auto[] = {0x01, 0x03, 0x28, 0x00, 0x00,
                                     0x01,  // Gain auto - 1, manual - 0 at offset 5
                                     0x00, 0x00};

      if (!autoValue) {
        command_gain_auto[5] = 0;
        r = TransmitCmd(command_gain_auto, sizeof(command_gain_auto));  // set auto off
        command_gain_set[5] = value;
        r = TransmitCmd(command_gain_set, sizeof(command_gain_set));
        LOG_TRANSMIT(wxT("send gain command gain value= %i, transmitted = %i"), value, command_gain_set[5]);
      } else {  // auto on
        command_gain_auto[5] = 1;
        r = TransmitCmd(command_gain_auto, sizeof(command_gain_auto));  // set auto on
      }
      LOG_VERBOSE(wxT("%s Gain: %d auto %d"), m_name.c_str(), value, autoValue);
      break;
    }

    case CT_COLOR_GAIN: {
      uint8_t command_color_gain_auto[] = {0x03, 0x03, 0x28, 0x00, 0x00,
                                           0x00,  // Color gain auto - 1, manual - 0 @5
                                           0x00, 0x00};

      uint8_t command_color_gain_set[] = {0x04, 0x03, 0x28, 0x00, 0x00,
                                          0x37,  // Color gain value 0 - 100 @5
                                          0x00, 0x00};
      if (!autoValue) {
        command_color_gain_auto[5] = 0;
        r = TransmitCmd(command_color_gain_auto, sizeof(command_color_gain_auto));  // set auto off
        command_color_gain_set[5] = value;
        r = TransmitCmd(command_color_gain_set, sizeof(command_color_gain_set));
        LOG_TRANSMIT(wxT("send color gain command color gain value= %i, transmitted = %i"), value, command_color_gain_set[5]);
      } else {  // auto on
        command_color_gain_auto[5] = 1;
        r = TransmitCmd(command_color_gain_auto, sizeof(command_color_gain_auto));  // set auto on
      }
      LOG_VERBOSE(wxT("%s Color Gain: %d auto %d"), m_name.c_str(), value, autoValue);
      break;
    }

    case CT_SEA: {
      uint8_t command_sea_set[] = {0x06, 0x03, 0x28, 0x00, 0x00,
                                   0x28,  // Quantum, value at pos 5
                                   0x7e, 0xa3};

      uint8_t command_sea_auto[] = {0x05, 0x03, 0x28, 0x00, 0x00,
                                    0x01,  // Quantum, 0 = manual, 1 = auto
                                    0x00, 0x00};

      if (!autoValue) {
        command_sea_auto[5] = 0;
        r = TransmitCmd(command_sea_auto, sizeof(command_sea_auto));  // set auto off
        command_sea_set[5] = value;                                   // scaling for gain
        LOG_TRANSMIT(wxT("send2 sea command sea value = %i, transmitted= %i"), value, command_sea_set[5]);
        r = TransmitCmd(command_sea_set, sizeof(command_sea_set));
      } else {
        LOG_TRANSMIT(wxT("sea auto clicked autoValue=%i"), autoValue);  // has Quantum more auto values ?
        command_sea_auto[5] = autoValue;
        r = TransmitCmd(command_sea_auto, sizeof(command_sea_auto));
      }
      break;
    }

    case CT_RAIN: {  // Rain Clutter
      uint8_t command_rain_auto[] = {
          0x0b, 0x03, 0x28, 0x00, 0x00,
          0x01,  // Auto on at offset 5, 01 = manual, 00 = auto (different from the others!)// changed for test
          0x00, 0x00};

      uint8_t command_rain_set[] = {0x0c, 0x03, 0x28, 0x00, 0x00,
                                    0x28,  // Quantum value at pos 5
                                    0x00, 0x00};

      if (state >= RCS_MANUAL) {
        command_rain_auto[5] = 1;  // rain enabled
        r = TransmitCmd(command_rain_auto, sizeof(command_rain_auto));
        command_rain_set[5] = value;
        LOG_TRANSMIT(wxT("rainvalue= %i, transmitted=%i"), value, command_rain_set[5]);
        r = TransmitCmd(command_rain_set, sizeof(command_rain_set));
      } else {
        command_rain_auto[5] = 0;  // rain disabled
        r = TransmitCmd(command_rain_auto, sizeof(command_rain_auto));
        LOG_TRANSMIT(wxT("rain state == RCS_AUTO_1, value= %i"), value);
      }
      break;
    }

    case CT_MODE: {  // Mode , harbor 0, coastal 1, offshore 2, weather 3
      uint8_t command_mode_set[] = {0x14, 0x03, 0x28, 0x00, 0x00,
                                    0x00,  // mode value at pos 5
                                    0x00, 0x00};
      command_mode_set[5] = value;
      LOG_TRANSMIT(wxT("mode value= %i, transmitted=%i"), value, command_mode_set[5]);
      r = TransmitCmd(command_mode_set, sizeof(command_mode_set));
      break;
    }

    case CT_ALL_TO_AUTO: {  // Mode , harbor 0, coastal 1, offshore 2, weather 3
      uint8_t command_gain_auto[] = {0x01, 0x03, 0x28, 0x00, 0x00,
                                     0x01,  // Gain manual == 0, auto == 1 at offset 5
                                     0x00, 0x00};

      uint8_t command_sea_auto[] = {0x05, 0x03, 0x28, 0x00, 0x00,
                                    0x01,  //  0 = manual, 1 = auto
                                    0x00, 0x00};

      uint8_t command_rain_auto[] = {0x0b, 0x03, 0x28, 0x00, 0x00,
                                     0x00,  // Auto on at offset 5, manual == 1, auto == 0 (different from the others!)
                                     0x00, 0x00};

      // there is one unknown command lacking here, setting field 22 of QuantumRadarReport
      r = TransmitCmd(command_gain_auto, sizeof(command_gain_auto));
      r = TransmitCmd(command_sea_auto, sizeof(command_sea_auto));
      r = TransmitCmd(command_rain_auto, sizeof(command_rain_auto));
      break;
    }

    case CT_TARGET_EXPANSION: {
      uint8_t command_target_expansion[] = {0x0f, 0x03, 0x28, 0x00, 0x00,
                                            0x00,  // 0ff == 00, on == 01
                                            0x00, 0x00};

      command_target_expansion[5] = value;
      r = TransmitCmd(command_target_expansion, sizeof(command_target_expansion));
      break;
    }

    case CT_INTERFERENCE_REJECTION: {
      uint8_t rd_msg_interference_rejection[] = {0x11, 0x03, 0x28, 0x00,
                                                 0x01,  // Interference rejection at offset 4, 0 - off, 1 - 5
                                                 0x00, 0x00, 0x00};

      rd_msg_interference_rejection[4] = value;
      r = TransmitCmd(rd_msg_interference_rejection, sizeof(rd_msg_interference_rejection));
      break;
    }

    case CT_BEARING_ALIGNMENT: {  // to be consistent with the local bearing alignment of the pi
                                  // this bearing alignment works opposite to the one an a Lowrance display

      uint8_t rd_msg_bearing_offset[] = {0x01, 0x04, 0x28,
                                         0x00, 0x05, 0x00,  // Bearing offset in .1 degrees with .5 resolution [-179.5 - 180] @4
                                         0x00, 0x00};

      rd_msg_bearing_offset[4] = value & 0xff;
      rd_msg_bearing_offset[5] = (value >> 8) & 0xff;
      LOG_VERBOSE(wxT("%s Bearing alignment: %d"), m_name.c_str(), value);
      r = TransmitCmd(rd_msg_bearing_offset, sizeof(rd_msg_bearing_offset));
      break;
    }

      // case CT_DOPPLER: {
      //  uint8_t cmd[] = {0x23, 0xc1, (uint8_t)value};
      //  LOG_VERBOSE(wxT("%s Doppler state: %d"), m_name.c_str(), value);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}
  }

  return r;
}

PLUGIN_END_NAMESPACE
