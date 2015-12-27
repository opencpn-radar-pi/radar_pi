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
 *   Copyright (C) 2012-2013 by Kees Verruijt         canboat@verruijt.net *
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

#ifndef _GUARDZONE_H_
#define _GUARDZONE_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class GuardZone {
 public:
  GuardZoneType type;
  SpokeBearing start_bearing;
  SpokeBearing end_bearing;
  int inner_range;  // now in meters
  int outer_range;  // now in meters
  int multi_sweep_filter;

  /*
   * Check if data is in this GuardZone, if so update bogeyCount
   */
  void ProcessSpoke(SpokeBearing angle, UINT8 *data, UINT8 *hist, size_t len, int range);

  /*
   * Return total bogeyCount over all spokes
   */
  int GetBogeyCount(SpokeBearing current_hdt);

  /*
   * Reset the bogey count for all spokes
   */
  void ResetBogeys();

  GuardZone(br24radar_pi *pi) {
    m_pi = pi;

    type = GZ_OFF;
    start_bearing = 0;
    end_bearing = 0;
    inner_range = 0;
    outer_range = 0;
    multi_sweep_filter = 0;

    ResetBogeys();
  }

 private:
  br24radar_pi *m_pi;
  int bogeyCount[LINES_PER_ROTATION];
};

PLUGIN_END_NAMESPACE

#endif /* _GUARDZONE_H_ */
