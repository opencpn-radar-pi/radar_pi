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

#ifndef _RADAR_MARPA_H_
#define _RADAR_MARPA_H_

#include "br24radar_pi.h"
//#include "RadarInfo.h"

PLUGIN_BEGIN_NAMESPACE

#define NUMBER_OF_TARGETS (20)
#define OFF_LOCATION (50)

#define MAX_CONTOUR_LENGTH (600)

class polar {
 public:
  int angle;
  int r;
};

class position {
 public:
  double lat;
  double lon;
};

enum target_status {
    lost,
    aquire0,   // under aquisition, first seen, no contour yet
    aquire1,   // under aquisition, contour found, first position in log
    aquire2,   // under aquisition, speed and course taken
    aquire3,   // under aquisition, speed and course verified, next time active
    loosing, // lost but not yet communicated to OCPN
    miss_1,  // missed 1 sweep
};

class LogEntry{
public:
    wxLongLong time;  // wxGetUTCTimeMillis
    position pos;
    double speed;
    double heading;
};

#define SIZE_OF_LOG (5)

class MarpaTarget {
 public:
  LogEntry logbook[SIZE_OF_LOG];
  polar pol;
 // wxLongLong time;  // wxGetUTCTimeMillis
  target_status status;
  polar contour[MAX_CONTOUR_LENGTH + 1];
  int contour_length;
  polar max_angle, min_angle, max_r, min_r;
  void PushLogbook();
};

class RadarMarpa {
 public:
  RadarMarpa(br24radar_pi* pi, RadarInfo* ri);

  ~RadarMarpa();
  position Polar2Pos(polar a, position own_ship);
  polar Pos2Polar(position p, position own_ship);
  int NextEmptyTarget();
  void Aquire0NewTarget(position p);
  void Aquire1NewTarget(MarpaTarget* t);
  int GetTargetWidth(int angle, int rad);
  int GetTargetHeight(int angle, int rad);
  int GetContour(polar* p, MarpaTarget* t);
  bool FindContourFromInside(polar* p);
  bool FindNearestContour(polar* p, int dist);
  ///*bool FindContour(polar* p);*/
  void CalculateCentroid(MarpaTarget* t);
  bool Pix(int a, int r);
  void DrawContour(MarpaTarget t);
  void DrawMarpaTargets();
  void RefreshMarpaTargets();
  MarpaTarget m_targets[NUMBER_OF_TARGETS];
//  bool PIX(polar* p, int aa, int rr);

 private:
  br24radar_pi* m_pi;
  RadarInfo* m_ri;
};

PLUGIN_END_NAMESPACE

#endif
