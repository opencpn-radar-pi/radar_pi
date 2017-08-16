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

#ifndef _BR24TRANSMIT_H_
#define _BR24TRANSMIT_H_

#include "RadarInfo.h"
#include "pi_common.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

static const UINT8 COMMAND_TX_OFF_A[3] = {0x00, 0xc1, 0x01};  // OFF part 1, note same as TX ON part 1
static const UINT8 COMMAND_TX_OFF_B[3] = {0x01, 0xc1, 0x00};  // OFF part 1, note same as TX ON part 1

static const UINT8 COMMAND_TX_ON_A[3] = {0x00, 0xc1, 0x01};  // ON part 1
static const UINT8 COMMAND_TX_ON_B[3] = {0x01, 0xc1, 0x01};  // ON part 2

static const UINT8 COMMAND_STAY_ON_A[2] = {0xA0, 0xc1};
static const UINT8 COMMAND_STAY_ON_B[2] = {0x03, 0xc2};
static const UINT8 COMMAND_STAY_ON_C[2] = {0x04, 0xc2};
static const UINT8 COMMAND_STAY_ON_D[2] = {0x05, 0xc2};

class br24Transmit {
 public:
  br24Transmit(br24radar_pi *pi, wxString name, int radar);
  ~br24Transmit();

  bool Init(struct sockaddr_in *address);
  void RadarTxOff();
  void RadarTxOn();
  bool RadarStayAlive();
  bool SetRange(int meters);
  bool SetControlValue(ControlType controlType, int value, int autoValue);

 private:
  br24radar_pi *m_pi;
  struct sockaddr_in m_addr;
  SOCKET m_radar_socket;
  wxString m_name;

  bool TransmitCmd(const UINT8 *msg, int size);
  void logBinaryData(const wxString &what, const UINT8 *data, int size);
};

PLUGIN_END_NAMESPACE

#endif /* _BR24TRANSMIT_H_ */
