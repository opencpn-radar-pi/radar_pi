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
 *   Copyright (C) 2013-2016 by Douwe Fokkkema             df@percussion.nl*
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

//#include "pi_common.h"

//#include "br24radar_pi.h"
#include "Kalman.h"
#include "RadarInfo.h"
#include "Matrix.h"

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class Kalman_Filter;
class Position;
class Matrix;

#define NUMBER_OF_TARGETS (20)
#define OFF_LOCATION (30)
#define SIZE_OF_LOG (100)
#define MAX_CONTOUR_LENGTH (600)
#define MAX_LOST_COUNT (6)

struct polar {
  int angle;
  int r;
};

enum target_status {
  lost,
  aquire0,  // under aquisition, first seen, no contour yet
  aquire1,  // under aquisition, contour found, first position in log
  aquire2,  // under aquisition, speed and course taken
  aquire3,  // under aquisition, speed and course verified, next time active
  active
};


class Position {
 public:
  double lat;
  double lon;
  
  Position operator-(Position p) {
    Position q;
    q.lat = lat - p.lat;
    q.lon = lon - p.lon;
    return q;
  }
  Position operator+(Position p) {
    Position q;
    q.lat = lat + p.lat;
    q.lon = lon + p.lon;
    return q;
  }
};


class Polar {
 public:
  int angle;
  int r;
  void Pos2Polar(Position p, Position own_ship, int range);
};


class LogEntry {
 public:
  wxLongLong time;  // wxGetUTCTimeMillis
  Polar pp;
  Position pos;
  double speed;
  double course;
};

class MetricPoint {
public:
    double lat; // in meters
    double lon;  // in meters
    double mspeed;  // m / sec
    double course;  // degrees
    wxLongLong time;  // wxGetUTCTimeMillis
    Position Metric2Pos();
    MetricPoint operator+(MetricPoint p) {
        MetricPoint q;
        q.lat = lat + p.lat;
        q.lon = lon + p.lon;
        return q;
    }

    MetricPoint operator-(MetricPoint p) {
        MetricPoint q;
        q.lat = lat - p.lat;
        q.lon = lon - p.lon;
        return q;
    }
};


class ArpaTarget {
 public:
  ArpaTarget(br24radar_pi* pi, RadarInfo* ri);
  ArpaTarget();
  ~ArpaTarget();
  RadarInfo* m_ri;
  br24radar_pi* m_pi;
  int target_id;
  Position measured_pos;  // the most recently measured position of the target
  Kalman_Filter* m_kalman;
  MetricPoint xpos;  // holds actual metric position, metric speed, course (degrees)
  wxLongLong t_refresh;  // time of last refresh
  int nr_of_log_entries;
  LogEntry logbook[SIZE_OF_LOG];  // stores positions, time course and speed
  Polar pol;                      // temporary polarcoordinates of target
  double bearing;                 // only valid directly after calculation
  double distance;// only valid directly after calculation
  time_t last_O_update;
  target_status status;
  int lost_count;
  Polar contour[MAX_CONTOUR_LENGTH + 1];  // contour of target, only valid immediately after finding it
  Polar expected;                        
  int contour_length;
  Polar max_angle, min_angle, max_r, min_r;  // charasterictics of contour
  void PushLogbook();
  // void Aquire1NewTarget();
  int GetContour();
  void set(br24radar_pi* pi, RadarInfo* ri);
  bool FindNearestContour(int dist);
  bool FindContourFromInside();
  Position Polar2Pos(Polar pol, Position own_ship);
  bool Pix(int ang, int rad);
  void UpdatePolar();
  // void Aquire2NewTarget();
  void CalculateSpeedandCourse();
  bool GetTarget();
  void RefreshTarget();
  void PassARPAtoOCPN();
  void SetStatusLost();
};



class RadarArpa {
 public:
  RadarArpa(br24radar_pi* pi, RadarInfo* ri);
  ~RadarArpa();

  void PassARPATargetsToOCPN();
  int GetTargetWidth(int angle, int rad);
  int GetTargetHeight(int angle, int rad);
  ArpaTarget* m_targets;
  br24radar_pi* m_pi;
  RadarInfo* m_ri;  
  //  Polar Pos2Polar(Position p, Position own_ship);
  int NextEmptyTarget();

  void CalculateCentroid(ArpaTarget* t);
  void DrawContour(ArpaTarget t);
  void DrawArpaTargets();
  void RefreshArpaTargets();
  void Aquire0NewTarget(Position p);
};

  /*double Dist(MetricPoint p2) {
    double dist = sqrt((p2.lat - lat) * (p2.lat - lat) + (p2.lon - lon) * (p2.lon - lon));
    return dist;
  }
*/
  /*double Bearing(MetricPoint p2) {
    double bear = rad2deg(atan((p2.lon - lon) / (p2.lat - lat)));
    return bear;
  }*/

  


MetricPoint Pos2Metric(Position p);

PLUGIN_END_NAMESPACE

#endif
