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

#include "RME120Control.h"

PLUGIN_BEGIN_NAMESPACE

bool RME120Control::Init(radar_pi *pi, RadarInfo *ri, NetworkAddress &ifadr, NetworkAddress &radaradr) {
  int r;
  int one = 1;

  // The radar IP address is not used for Navico BR/Halo radars
  if (radaradr.port != 0) {
    // Null
  }

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
    LOG_INFO(wxT(" tx socketerror "));
    // Might as well give up now
    return false;
  }
  if (m_radar_socket == INVALID_SOCKET) {  // just another check...
    wxLogError(wxT("INVALID_SOCKET Unable to create UDP sending socket"));
    // Might as well give up now
    return false;
  }
  LOG_TRANSMIT(wxT("%s transmit socket open"), m_name.c_str());
  return true;
}

void RME120Control::logBinaryData(const wxString &what, const uint8_t *data, int size) {
  wxString explain;
  int i = 0;

  explain.Alloc(size * 3 + 50);
  explain += wxT("") + m_name.c_str() + wxT(" ");
  explain += what;
  explain += wxString::Format(wxT(" %d bytes: "), size);
  for (i = 0; i < size; i++) {
    explain += wxString::Format(wxT(" %02X"), data[i]);
  }
  LOG_TRANSMIT(explain);
}

bool RME120Control::TransmitCmd(const uint8_t *msg, int size) {
  if (m_send_address.IsNull()) {
    wxLogError(wxT("%s Unable to transmit command to unknown radar"), m_name.c_str());
    IF_LOG_AT(LOGLEVEL_TRANSMIT, logBinaryData(wxT("not transmitted"), msg, size));
    return false;
  }
  if (m_radar_socket == INVALID_SOCKET) {
    wxLogError(wxT("%s INVALID_SOCKET, Unable to transmit command to unknown radar"), m_name.c_str());
    return false;
  }

  struct sockaddr_in send_sock_addr = m_send_address.GetSockAddrIn();

  int sendlen = sendto(m_radar_socket, (char *)msg, size, 0, (struct sockaddr *)&send_sock_addr, sizeof(send_sock_addr));
  if (sendlen < size) {
    wxLogError(wxT("%s Unable to transmit command: %s"), m_name.c_str(), SOCKETERRSTR);
    IF_LOG_AT(LOGLEVEL_TRANSMIT, logBinaryData(wxT("transmit"), msg, size));
    return false;
  }
  IF_LOG_AT(LOGLEVEL_TRANSMIT, logBinaryData(wxT("transmit"), msg, size));
  return true;
}

static uint8_t rd_msg_tx_control[] = {0x01, 0x80, 0x01, 0x00,
                                      0x00,  // Control value at offset 4 : 0 - off, 1 - on
                                      0x00, 0x00, 0x00};

void RME120Control::RadarTxOff() {
  rd_msg_tx_control[4] = 0;
  TransmitCmd(rd_msg_tx_control, sizeof(rd_msg_tx_control));
}

void RME120Control::RadarTxOn() {
  rd_msg_tx_control[4] = 1;
  TransmitCmd(rd_msg_tx_control, sizeof(rd_msg_tx_control));
};

static uint8_t rd_msg_5s[] = {
    0x03, 0x89, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // keeps alive for 5 seconds ?
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x01, 0x00, 0x00,
    0x9e, 0x03, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t rd_msg_1s[] = {
    // E120
    0x00, 0x80, 0x01, 0x00, 0x52, 0x41, 0x44, 0x41, 0x52, 0x00, 0x00, 0x00};

bool RME120Control::RadarStayAlive() {
  static int count = 4;

  if (count++ >= 4) {
    TransmitCmd(rd_msg_5s, sizeof(rd_msg_5s));
    count = 0;
  }
  TransmitCmd(rd_msg_1s, sizeof(rd_msg_1s));

  return true;
}

static uint8_t rd_msg_set_range[] = {0x01, 0x81, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                                     0x01,  // Range at offset 8 (0 - 1/8, 1 - 1/4, 2 - 1/2, 3 - 3/4, 4 - 1, 5 - 1.5, 6 - 3...)
                                     0x00, 0x00, 0x00};  // length == 12

bool RME120Control::SetRange(int meters) {
  LOG_INFO(wxT(" SetRangeMeters = %i"), meters);
  for (int i = 0; i < 11; i++) {
    if (meters <= m_ri->m_radar_ranges[i]) {
      SetRangeIndex(i);
      return true;
    }
  }
  return false;
}

void RME120Control::SetRangeIndex(size_t index) {
  LOG_VERBOSE(wxT(" SetRangeIndex index = %i"), index);
  rd_msg_set_range[8] = index;
  TransmitCmd(rd_msg_set_range, sizeof(rd_msg_set_range));
}

bool RME120Control::SetControlValue(ControlType controlType, RadarControlItem &item, RadarControlButton *button) {
  bool r = false;
  int value = item.GetValue();
  RadarControlState state = item.GetState();
  int autoValue = 0;
  if (state > RCS_MANUAL) {
    autoValue = state - RCS_MANUAL;
  }

  switch (controlType) {
    case CT_ACCENT_LIGHT:
    case CT_ALL_TO_AUTO:
    case CT_ANTENNA_FORWARD:
    case CT_ANTENNA_HEIGHT:
    case CT_ANTENNA_STARBOARD:
    case CT_AUTOTTRACKDOPPLER:
    case CT_CENTER_VIEW:
    case CT_COLOR_GAIN:
    case CT_DOPPLER:
    case CT_LOCAL_INTERFERENCE_REJECTION:
    case CT_MAIN_BANG_SIZE:
    case CT_MAX:
    case CT_MODE:
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
    case CT_TARGET_EXPANSION:
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

      // The above are not settings that are not radar commands or not supported by Raymarine E120 radar.
      // Made them explicit so the compiler can catch missing control types.
      break;

      // Ordering the radar commands by the first byte value.
      // Some interesting holes here, seems there could be more commands!

    case CT_BEARING_ALIGNMENT: {  // to be consistent with the local bearing alignment of the pi
                                  // this bearing alignment works opposite to the one an a Lowrance display
#if 0
      if (value < 0) {
        value += 360;
      }
#endif
      uint8_t rd_msg_bearing_offset[] = {0x07, 0x82, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00};
      rd_msg_bearing_offset[4] = value & 0xff;
      rd_msg_bearing_offset[5] = (value >> 8) & 0xff;
      rd_msg_bearing_offset[6] = (value >> 16) & 0xff;
      rd_msg_bearing_offset[7] = (value >> 24) & 0xff;
      LOG_VERBOSE(wxT("%s Bearing alignment: %d"), m_name.c_str(), value);
      r = TransmitCmd(rd_msg_bearing_offset, sizeof(rd_msg_bearing_offset));
      break;
    }

    case CT_GAIN: {  // tested OK by Martin
      uint8_t cmd[] = {0x01, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00,  // Gain value at offset 20
                       0x00, 0x00, 0x00};

      uint8_t cmd2[] = {0x01, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x01,  // Gain auto - 1, manual - 0 at offset 16
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      if (!autoValue) {
        cmd2[16] = 0;
        r = TransmitCmd(cmd2, sizeof(cmd2));        // set auto off
        cmd[20] = m_ri->m_gain.DeTransform(value);  // scaling for gain
        r = TransmitCmd(cmd, sizeof(cmd));
        LOG_TRANSMIT(wxT("send gain command gain value= %i, transmitted = %i"), value, cmd[20]);
      } else {  // auto on
        cmd2[16] = 1;
        r = TransmitCmd(cmd2, sizeof(cmd2));  // set auto on
      }
      LOG_VERBOSE(wxT("%s Gain: %d auto %d"), m_name.c_str(), value, autoValue);
      break;
    }

    case CT_SEA: {
      uint8_t rd_msg_set_sea[] = {0x02, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00,  // Sea value at offset 20
                                  0x00, 0x00, 0x00};

      uint8_t rd_msg_sea_auto[] = {0x02, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x01,  // Sea auto value at offset 16
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      if (!autoValue) {
        rd_msg_sea_auto[16] = 0;
        r = TransmitCmd(rd_msg_sea_auto, sizeof(rd_msg_sea_auto));  // set auto off
        rd_msg_set_sea[20] = m_ri->m_sea.DeTransform(value);        // scaling for gain
        LOG_TRANSMIT(wxT("send2 sea command sea value = %i, transmitted= %i"), value, rd_msg_set_sea[20]);
        r = TransmitCmd(rd_msg_set_sea, sizeof(rd_msg_set_sea));
      } else {
        LOG_TRANSMIT(wxT("sea auto clicked autoValue=%i"), autoValue);
        rd_msg_sea_auto[16] = autoValue;
        r = TransmitCmd(rd_msg_sea_auto, sizeof(rd_msg_sea_auto));
      }
      break;
    }

    case CT_RAIN: {  // Rain Clutter

      uint8_t rd_msg_rain_on[] = {0x03, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x01,  // Rain on at offset 16
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      uint8_t rd_msg_rain_set[] = {0x03, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x33,  // Rain value at offset 20
                                   0x00, 0x00, 0x00};

      if (state == RCS_OFF) {
        rd_msg_rain_on[16] = 0;  // rain off
        r = TransmitCmd(rd_msg_rain_on, sizeof(rd_msg_rain_on));
        LOG_TRANSMIT(wxT("rain state == RCS_OFF, value= %i"), value);
        break;
      } else {
        rd_msg_rain_on[16] = 1;  // rain on first
        r = TransmitCmd(rd_msg_rain_on, sizeof(rd_msg_rain_on));
        rd_msg_rain_set[20] = m_ri->m_rain.DeTransform(value);
        LOG_TRANSMIT(wxT("rainvalue= %i, transmitted=%i"), value, rd_msg_rain_set[20]);
        r = TransmitCmd(rd_msg_rain_set, sizeof(rd_msg_rain_set));
      }
      break;
    }

    case CT_FTC: {
      uint8_t rd_msg_ftc_on[] = {0x04, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x01,  // FTC on at offset 16
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      uint8_t rd_msg_ftc_set[] = {0x04, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x1a,  // FTC value at offset 20
                                  0x00, 0x00, 0x00};
      if (state == RCS_OFF) {
        rd_msg_ftc_on[16] = 0;  // FTC off
        r = TransmitCmd(rd_msg_ftc_on, sizeof(rd_msg_ftc_on));
        LOG_TRANSMIT(wxT("FTC state == RCS_OFF, value= %i"), value);
      } else {
        rd_msg_ftc_on[16] = 1;  // FTC on
        r = TransmitCmd(rd_msg_ftc_on, sizeof(rd_msg_ftc_on));
        LOG_TRANSMIT(wxT("FTC state != RCS_OFF, value= %i"), value);
        rd_msg_ftc_set[20] = m_ri->m_ftc.DeTransform(value);
        r = TransmitCmd(rd_msg_ftc_set, sizeof(rd_msg_ftc_set));
        LOG_TRANSMIT(wxT("send FTC command FTC value = %i, transmitted= %i"), value, rd_msg_ftc_set[20]);
      }
      break;
    }

    case CT_MAIN_BANG_SUPPRESSION: {  // Main bang suppression
      uint8_t rd_msg_mbs_control[] = {0x01, 0x82, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                                      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00,  // MBS Enable (1) at offset 16
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      if (state == RCS_OFF) {
        rd_msg_mbs_control[16] = 0;
      } else if (state == RCS_MANUAL) {
        rd_msg_mbs_control[16] = 1;
      }
      r = TransmitCmd(rd_msg_mbs_control, sizeof(rd_msg_mbs_control));
      break;
    }

    case CT_DISPLAY_TIMING: {
      uint8_t rd_msg_set_display_timing[] = {0x02, 0x82, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                                             0x6d,  // Display timing value at offset 8
                                             0x00, 0x00, 0x00};
      rd_msg_set_display_timing[8] = value;
      r = TransmitCmd(rd_msg_set_display_timing, sizeof(rd_msg_set_display_timing));
      break;
    }

      // case CT_STC: {
      //  uint8_t rd_msg_set_stc_preset[] = {0x03, 0x82, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
      //                                     0x74,  // STC preset value at offset 8
      //                                     0x00, 0x00, 0x00};
      //  rd_msg_set_stc_preset[16] = value;
      //  r = TransmitCmd(rd_msg_set_stc_preset, sizeof(rd_msg_set_stc_preset));
      //  break;

      //  break;
      //}

      // case CT_TUNE_COARSE: {  // coarse tuning
      //  uint8_t rd_msg_tune_coarse[] = {0x04, 0x82, 0x01, 0x00,
      //                                  0x00,  // Coarse tune at offset 4
      //                                  0x00, 0x00, 0x00};
      //  rd_msg_tune_coarse[4] = value;
      //  r = TransmitCmd(rd_msg_tune_coarse, sizeof(rd_msg_tune_coarse));
      //  break;
      //}

      // case CT_TUNE_FINE: {
      //  uint8_t rd_msg_tune_auto[] = {0x05, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      //                                       0x01,  // Enable at offset 12
      //                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      //  uint8_t rd_msg_tune_fine[] = {0x05, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      //                                       0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      //                                       0x00,  // Tune value at offset 16
      //                                       0x00, 0x00, 0x00};

      //  if (!autoValue) {
      //    rd_msg_tune_fine[16] = value;
      //    r = TransmitCmd(rd_msg_tune_fine, sizeof(rd_msg_tune_fine));
      //  } else {
      //    rd_msg_tune_auto[12] = 1;
      //    r = TransmitCmd(rd_msg_tune_auto, sizeof(rd_msg_tune_auto));
      //  }
      //  break;
      //}

    case CT_TARGET_BOOST: {
      uint8_t rd_msg_target_expansion[] = {0x06, 0x83, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                                           0x01,  // Expansion value at offset 8: 0 - disabled, 1 - low, 2 - high
                                           0x00, 0x00, 0x00};

      rd_msg_target_expansion[8] = value;
      r = TransmitCmd(rd_msg_target_expansion, sizeof(rd_msg_target_expansion));
      break;
    }

    case CT_INTERFERENCE_REJECTION: {
      uint8_t rd_msg_interference_rejection[] = {0x07, 0x83, 0x01, 0x00,
                                                 0x01,  // Interference rejection at offset 4, 0 - off, 1 - normal, 2 - high
                                                 0x00, 0x00, 0x00};

      rd_msg_interference_rejection[4] = value;
      r = TransmitCmd(rd_msg_interference_rejection, sizeof(rd_msg_interference_rejection));
      break;
    }

      // case CT_STC_CURVE: {
      //  uint8_t curve_values[] = {0, 1, 2, 4, 6, 8, 10, 13};
      //  uint8_t rd_msg_curve_select[] = {
      //      0x0a, 0x83, 0x01, 0x00,
      //      0x01  // Curve value at offset 4
      //  };
      //  rd_msg_curve_select[4] = curve_values[value - 1];
      //  r = TransmitCmd(rd_msg_curve_select, sizeof(rd_msg_curve_select));
      //  break;
      //}

      // case CT_SIDE_LOBE_SUPPRESSION: {
      //  int v = value * 256 / 100;
      //  if (v > 255) {
      //    v = 255;
      //  }
      //  uint8_t cmd[] = {0x6, 0xc1, 0x05, 0, 0, 0, (uint8_t)autoValue, 0, 0, 0, (uint8_t)v};
      //  LOG_VERBOSE(wxT("%s command Tx CT_SIDE_LOBE_SUPPRESSION: %d auto %d"), m_name.c_str(), value, autoValue);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}

      //  // What would command 7 be?

      //  // What would command b through d be?

      // case CT_SCAN_SPEED: {
      //  uint8_t cmd[] = {0x0f, 0xc1, (uint8_t)value};
      //  LOG_VERBOSE(wxT("%s Scan speed: %d"), m_name.c_str(), value);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}

      //  // What would command 10 through 20 be?

      // case CT_NOISE_REJECTION: {
      //  uint8_t cmd[] = {0x21, 0xc1, (uint8_t)value};
      //  LOG_VERBOSE(wxT("%s Noise rejection: %d"), m_name.c_str(), value);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}

      // case CT_TARGET_SEPARATION: {
      //  uint8_t cmd[] = {0x22, 0xc1, (uint8_t)value};
      //  LOG_VERBOSE(wxT("%s Target separation: %d"), m_name.c_str(), value);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}

      // case CT_DOPPLER: {
      //  uint8_t cmd[] = {0x23, 0xc1, (uint8_t)value};
      //  LOG_VERBOSE(wxT("%s Doppler state: %d"), m_name.c_str(), value);
      //  r = TransmitCmd(cmd, sizeof(cmd));
      //  break;
      //}

      //  // What would command 24 through 2f be?
  }

  return r;
}

PLUGIN_END_NAMESPACE
