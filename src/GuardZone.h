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

#ifndef _GUARDZONE_H_
#define _GUARDZONE_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class GuardZone {
 public:
  GuardZoneType type;
  SpokeBearing start_bearing;
  SpokeBearing end_bearing;
  int inner_range;  // start in meters
  int outer_range;  // end   in meters
  int multi_sweep_filter;

  void ResetBogeys() {
    m_bogey_count = 0;
    m_running_count = 0;
    m_last_in_guard_zone = false;
    m_last_angle = 0;
  };
  
  void SetType(GuardZoneType type) {
    this->type = type;
    ResetBogeys();
  };
  void SetStartBearing(SpokeBearing start_bearing) {
    this->start_bearing = start_bearing;
    ResetBogeys();
  };
  void SetEndBearing(SpokeBearing end_bearing) {
    this->end_bearing = end_bearing;
    ResetBogeys();
  };
  void SetInnerRange(int inner_range) {
    this->inner_range = inner_range;
    ResetBogeys();
  };
  void SetOuterRange(int outer_range) {
    this->outer_range = outer_range;
    ResetBogeys();
  };
  void SetMultiSweepFilter(int filter) {
    this->multi_sweep_filter = filter;
    ResetBogeys();
  };

  /*
   * Check if data is in this GuardZone, if so update bogeyCount
   */
  void ProcessSpoke(SpokeBearing angle, UINT8 *data, UINT8 *hist, size_t len, int range);

  int GetBogeyCount() {
    LOG_GUARD(wxT("BR24radar_pi: GUARD: reporting bogey_count=%d"), m_bogey_count);
    return m_bogey_count;
  };

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
  bool m_last_in_guard_zone;
  SpokeBearing m_last_angle;
  int m_bogey_count;    // complete cycle
  int m_running_count;  // current swipe

  void UpdateSettings();
};

PLUGIN_END_NAMESPACE

#endif /* _GUARDZONE_H_ */
