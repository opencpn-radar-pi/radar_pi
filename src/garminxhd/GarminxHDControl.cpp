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

GarminxHDControl::GarminxHDControl(NetworkAddress sendMultiCastAddress) {
  m_addr.sin_family = AF_INET;
  m_addr.sin_addr = sendMultiCastAddress.addr;
  m_addr.sin_port = sendMultiCastAddress.port;

  m_radar_socket = INVALID_SOCKET;
  m_name = wxT("Navico radar");

  m_pi = 0;
  m_ri = 0;
  m_name = wxT("GarminxHD");
}

GarminxHDControl::~GarminxHDControl() {}

bool GarminxHDControl::Init(radar_pi *pi, RadarInfo *ri, NetworkAddress &ifadr, NetworkAddress &radaradr) {
  m_pi = pi;
  m_ri = ri;
  m_name = ri->m_name;

  return true;
}

void GarminxHDControl::RadarTxOff() { m_ri->m_state.Update(RADAR_STANDBY); }

void GarminxHDControl::RadarTxOn() {
  if (m_ri) {
    m_ri->m_state.Update(RADAR_TRANSMIT);
  }
}

bool GarminxHDControl::RadarStayAlive() { return true; }

bool GarminxHDControl::SetRange(int meters) { return false; }

bool GarminxHDControl::SetControlValue(ControlType controlType, int value, int autoValue) {  // sends the command to the radar
  bool r = false;

  return r;
}

PLUGIN_END_NAMESPACE
