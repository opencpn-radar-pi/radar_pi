/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 *           Martin Hassellov: testing the Raymarine radar
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

#ifndef _RM_CONTROL_H_
#define _RM_CONTROL_H_

#include "RadarInfo.h"
#include "pi_common.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

class RME120Control : public RadarControl {
 public:
  RME120Control(radar_pi *pi, RadarInfo *ri);
  ~RME120Control();

  bool Init(radar_pi *pi, RadarInfo *ri, NetworkAddress &ifadr, NetworkAddress &radaradr);

  void RadarTxOff();
  void RadarTxOn();
  bool RadarStayAlive();
  bool SetRange(int meters);
  bool SetControlValue(ControlType controlType, RadarControlItem &item, RadarControlButton *button);

 private:
  radar_pi *m_pi;
  RadarInfo *m_ri;

  SOCKET m_radar_socket;
  wxString m_name;

  bool TransmitCmd(const uint8_t *msg, int size);
  void logBinaryData(const wxString &what, const uint8_t *data, int size);
  void SetRangeIndex(size_t index);
};

PLUGIN_END_NAMESPACE

#endif /* _RM_CONTROL_H_ */
