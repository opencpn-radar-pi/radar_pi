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

#ifndef _RADARCONTROL_H_
#define _RADARCONTROL_H_

#include "RadarInfo.h"
#include "pi_common.h"

PLUGIN_BEGIN_NAMESPACE

class RadarControl {
 public:
  RadarControl(radar_pi *pi, wxString name, int radar){};
  virtual ~RadarControl(){};

  RadarControl(){};

  /*
   * Initialize any local data structures.
   *
   * @param address     The address of the ethernet card on this machine
   */
  virtual bool Init(struct sockaddr_in *address) = 0;

  /*
   * Ask the radar to switch off.
   */
  virtual void RadarTxOff() = 0;

  /*
   * Ask the radar to switch on.
   */
  virtual void RadarTxOn() = 0;

  /*
   * Send a 'keepalive' message to the radar.
   * This can be a null implementation if the radar does not need this.
   *
   * @returns   true on success, false on failure.
   */
  virtual bool RadarStayAlive() { return true; };

  /*
   * Set the range to the given range in meters.
   *
   * @param     meters  Requested range in meters.
   * @returns   true on success, false on failure.
   */
  virtual bool SetRange(int meters) = 0;

  /*
   * Modify a radar setting.
   *
   * TODO: autoValue needs to be explained, and probably given a different interface.
   *
   * @param     controlType     Control such as CT_GAIN, etc.
   * @param     value           Requested value.
   * @param     autoValue       Requested auto-value.
   * @returns   true on success, false on failure.
   */
  virtual bool SetControlValue(ControlType controlType, int value, int autoValue) = 0;
};

PLUGIN_END_NAMESPACE

#endif /* _RADARCONTROL_H_ */
