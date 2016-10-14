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

// class Polar {
// public:
//     Polar(RadarInfo* ri);
//  int angle;
//  int r;
//  bool FindNearestContour(int dist);
//  bool FindContourFromInside();
//  bool Pix();
//  RadarInfo* m_ri;
////  br24radar_pi* m_pi;
//  Polar Pos2Polar(Position p, Position own_ship);
//  Position Polar2Pos(Position own_ship);
//};

struct polar {
  int angle;
  int r;
};

// class Position {
// public:
//  double lat;
//  double lon;
//  RadarInfo* m_ri;
//};

enum target_status {
  lost,
  aquire0,  // under aquisition, first seen, no contour yet
  aquire1,  // under aquisition, contour found, first position in log
  aquire2,  // under aquisition, speed and course taken
  aquire3,  // under aquisition, speed and course verified, next time active
  loosing,  // lost but not yet communicated to OCPN
  miss_1,   // missed 1 sweep
};

// class LogEntry{
// public:
//    LogEntry();
// //   LogEntry(RadarInfo* ri);
//    wxLongLong time;  // wxGetUTCTimeMillis
//    Position pos;
//    double speed;
//    double heading;
//
//};

#define SIZE_OF_LOG (5)

class Position {
 public:
  double lat;
  double lon;
};

class MarpaTarget {
 public:
  MarpaTarget(br24radar_pi* pi, RadarInfo* ri);
  MarpaTarget();

  class Polar {
   public:
    Polar();
    int angle;
    int r;
    bool FindNearestContour(int dist);
    bool FindContourFromInside();
    bool Pix();
    Polar Pos2Polar(Position p, Position own_ship);
    Position Polar2Pos(Position own_ship);
  };

  class LogEntry {
   public:
    LogEntry();
    //   LogEntry(RadarInfo* ri);
    wxLongLong time;  // wxGetUTCTimeMillis
    Position pos;
    double speed;
    double heading;
  };
  static int xxx;
  static RadarInfo* m_ri;
  static br24radar_pi* m_pi;
  LogEntry* logbook;
  Polar pol;
  // wxLongLong time;  // wxGetUTCTimeMillis
  target_status status;
  Polar contour[MAX_CONTOUR_LENGTH + 1];
  int contour_length;
  Polar max_angle, min_angle, max_r, min_r;
  void PushLogbook();
  void Aquire1NewTarget();
  void Aquire0NewTarget(Position p);
  int GetContour();
  void set(br24radar_pi* pi, RadarInfo* ri);
  int NextEmptyTarget();
};

class RadarMarpa {
 public:
  RadarMarpa(br24radar_pi* pi, RadarInfo* ri);
  ~RadarMarpa();

  int GetTargetWidth(int angle, int rad);
  int GetTargetHeight(int angle, int rad);
  void CalculateCentroid(MarpaTarget* t);
  void DrawContour(MarpaTarget t);
  void DrawMarpaTargets();
  void RefreshMarpaTargets();
  MarpaTarget* m_targets;
  br24radar_pi* m_pi;
  RadarInfo* m_ri;
};

PLUGIN_END_NAMESPACE

#endif
