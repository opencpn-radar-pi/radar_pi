/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin, Arpa partition
 *           Target tracking
 * Authors:  Douwe Fokkema
 *           Kees Verruijt
 *           Håkan Svensson
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

#include "RadarMarpa.h"
#include "RadarInfo.h"
#include "br24radar_pi.h"
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

static int target_id_count = 0;

RadarArpa::RadarArpa(br24radar_pi* pi, RadarInfo* ri) {
  LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator call"));
  m_ri = ri;
  m_pi = pi;
  m_targets = new ArpaTarget[NUMBER_OF_TARGETS];
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    m_targets[i].set(pi, ri);
    m_targets[i].SetStatusLost();
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator ready"));
}

ArpaTarget::~ArpaTarget(){
   
}

void ArpaTarget::set(br24radar_pi* pi, RadarInfo* ri) {
  m_ri = ri;
  m_pi = pi;
  t_refresh = wxGetUTCTimeMillis();
}

RadarArpa::~RadarArpa() {}

Position Polar2Pos(Polar pol, Position own_ship, double range) {
  // The "own_ship" in the fumction call can be the position at an earlier time than the current position
  // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
  // based on the own ship position own_ship
  Position pos;
  pos.lat = own_ship.lat +
            (double)pol.r / (double)RETURNS_PER_LINE * range *
                cos(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / 60. / 1852.;
  pos.lon = own_ship.lon +
            (double)pol.r / (double)RETURNS_PER_LINE * range *
                sin(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / cos(deg2rad(own_ship.lat)) / 60. / 1852.;
  return pos;
}

Polar Pos2Polar(Position p, Position own_ship, int range) {
  // converts in a radar image a lat-lon position to angular data
    Polar pol;
  double dif_lat = p.lat;
  dif_lat -= own_ship.lat;
  double dif_lon = (p.lon - own_ship.lon) * cos(deg2rad(own_ship.lat));
  pol.r = (int)(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * (double)RETURNS_PER_LINE / (double)range + 1);
  pol.angle = (int)((atan2(dif_lon, dif_lat)) * (double)LINES_PER_ROTATION / (2. * PI) + 1);  // + 1 to minimize rounding errors
  return pol;
}

bool ArpaTarget::Pix(int ang, int rad) {
  if (rad < 1 || rad >= RETURNS_PER_LINE - 1) {  // $$$ avoid range ring
    return false;
  }
  return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
}

void RadarArpa::AquireNewTarget(Position target_pos, int status) {
  // aquires new target from mouse click
  // no contour taken yet
  // target status aquire0
  // returns in X metric coordinates of click
  // constructs Kalman filter

  int i_target = NextEmptyTarget();
  if (i_target == -1) {
    LOG_INFO(wxT("BR24radar_pi: RadarArpa:: max targets exceeded "));
    return;
  }
  Position own_pos;
  Polar pol;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pol = Pos2Polar(target_pos, own_pos, m_ri->m_range_meters);
  LOG_INFO(wxT("BR24radar_pi: $$$Aquire0NewTarget r %i a %i"), pol.r, pol.angle);
  LOG_INFO(wxT("BR24radar_pi: $$$Aquire0NewTarget lat %f lon %f"), target_pos.lat, target_pos. lon);


  m_targets[i_target].X = target_pos;  // Expected position
  pol = Pos2Polar(m_targets[i_target].X, own_pos, m_ri->m_range_meters);
  LOG_INFO(wxT("BR24radar_pi: $$$Aquire0NewTarget from metric r %i a %i"), pol.r, pol.angle);

  m_targets[i_target].X.time = 0;
  m_targets[i_target].X.dlat_dt = 0.;
  m_targets[i_target].X.dlon_dt = 0.;
  m_targets[i_target].status = status;
  target_id_count++;
  if (target_id_count >= 100) target_id_count = 1;
  m_targets[i_target].target_id = target_id_count;
  if (!m_targets[i_target].m_kalman) {
    m_targets[i_target].m_kalman = new Kalman_Filter();
  }
  return;
}

bool ArpaTarget::FindContourFromInside(Polar* pol) {  // moves pol to contour of blob
  // true if success
  // false when failed
  int ang = pol->angle;
  int rad = pol->r;
  if (rad > RETURNS_PER_LINE - 1) {
    return false;
  }
  if (rad < 3) {
    return false;
  }
  if (rad > 511) return false;
  if (!(Pix(ang, rad))) {
    return false;
  }
  while (Pix(ang, rad)) {
    if (ang < pol->angle - MAX_CONTOUR_LENGTH / 2) {
      return false;
    }
    ang--;
    if (rad > 511) return false;
  }
  ang++;
  pol->angle = ang;
  return true;
}

int ArpaTarget::GetContour(Polar* pol, Position* z) {  // sets the measured_pos if succesfull
  // pol must start on the contour of the blob
  // follows the contour in a clockwise direction
  // returns metric position of the blob in Z

  wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);
  // the 4 possible translations to move from a point on the contour to the next
  Polar transl[4] = {0, 1, 1, 0, 0, -1, -1, 0};  // NB structure polar, not class Polar
  int count = 0;
  Polar start = *pol;
  Polar current = *pol;
  int aa;
  int rr;
  bool succes = false;
  int index = 0;
  max_r = current;
  max_angle = current;
  min_r = current;
  min_angle = current;
  // check if p inside blob
  if (start.r > RETURNS_PER_LINE - 2) {
    return 1;  // return code 1, r too large
  }
  if (start.r < 4) {
    return 2;  // return code 2, r too small
  }
  if (start.r > 511) return 13;
  if (!Pix(start.angle, start.r)) {
    return 3;  // return code 3, starting point outside blob
  }
  // first find the orientation of border point p
  for (int i = 0; i < 4; i++) {
    index = i;
    aa = current.angle + transl[index].angle;
    rr = current.r + transl[index].r;
    if (rr > 511) return 13;  // r too large
    succes = !Pix(aa, rr);
    if (succes) break;
  }
  if (!succes) {
    return 4;  // return code 4, starting point not on contour
  }
  index += 1;  // determines starting direction
  if (index > 3) index -= 4;

  while (current.r != start.r || current.angle != start.angle || count == 0) {
    // try all translations to find the next point
    // start with the "left most" translation relative to the previous one
    index += 3;  // we will turn left all the time if possible
    for (int i = 0; i < 4; i++) {
      if (index > 3) index -= 4;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
      if (rr > RETURNS_PER_LINE - 2) {
        return 5;  // return code 5, getting outside image
      }
      if (rr < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarArpa::GetContour getting in origin"));
        return 6;  //  code 6, getting close to origin
      }
      if (rr > 511) return 10;  // return code 10 r too large
      succes = Pix(aa, rr);  // this is the case where the whole blob is followed
      // but we accept single pixel extensions of the blob
      if (succes) {
        // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      LOG_INFO(wxT("BR24radar_pi::RadarArpa::GetContour no next point found"));
      return 7;  // return code 7, no next point found
    }
    // next point found
    current.angle = aa;
    current.r = rr;
    contour[count] = current;
    count++;
    if (current.angle > max_angle.angle) {
      max_angle = current;
    }
    if (current.angle < min_angle.angle) {
      min_angle = current;
    }
    if (current.r > max_r.r) {
      max_r = current;
    }
    if (current.r < min_r.r) {
      min_r = current;
    }
    if (count >= MAX_CONTOUR_LENGTH) {
      // blob too large
      return 8;  // return code 8, Blob too large
    }
  }
  contour_length = count;
  //  CalculateCentroid(*target);   $$$
  pol->angle = (max_angle.angle + min_angle.angle) / 2;
  if (max_r.r >= 511 || min_r.r >= 511) {
    return 10;  // return code 10 r too large
  }
  if (max_r.r < 2 || min_r.r < 2) {
    return 11;  // return code 11 r too small
  }
  pol->r = (max_r.r + min_r.r) / 2;
  
  wxLongLong target_time = m_ri->m_history[MOD_ROTATION2048(pol->angle)].time;
  
  LOG_INFO(wxT("BR24radar_pi: $$$ blob found  target_time= %u"),  target_time.GetLo());
  
  Position p_own;
  p_own.lat = m_ri->m_history[MOD_ROTATION2048(pol->angle)].lat;  // get the position at receive time
  p_own.lon = m_ri->m_history[MOD_ROTATION2048(pol->angle)].lon;
  // Z is the measured metric position of the target
  *z = Polar2Pos(*pol, p_own, m_ri->m_range_meters);  // using own ship location from the time of reception
  z->time = target_time;
  return 0;  //  succes, blob found
}

int RadarArpa::NextEmptyTarget() {
  int index = 0;
  bool hit = false;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == LOST) {
      index = i;
      hit = true;
      break;
    }
  }
  if (!hit) {
    index = -1;
  }
  return index;
}

void RadarArpa::DrawContour(ArpaTarget target) {
    // should be improved using vertex arrays
  PolarToCartesianLookupTable* polarLookup;
  polarLookup = GetPolarToCartesianLookupTable();
  glColor4ub(40, 40, 100, 250);
  glLineWidth(3.0);
  glBegin(GL_LINES);

  for (int i = 0; i < target.contour_length; i++) {
    double xx;
    double yy;
    int angle = MOD_ROTATION2048(target.contour[i].angle - 512);
    int radius = target.contour[i].r;
    if (radius <= 0 || radius >= RETURNS_PER_LINE) {
      LOG_INFO(wxT("BR24radar_pi:RadarArpa::DrawContour r OUT OF RANGE"));
      return;
    }
    xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
    yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
    glVertex2f(xx, yy);
    int ii = i + 1;
    if (ii == target.contour_length) {
      ii = 0;  // start point again
    }
    if (radius <= 0 || radius >= RETURNS_PER_LINE) {
      LOG_INFO(wxT("BR24radar_pi:RadarArpa::DrawContour r OUT OF RANGE"));
      return;
    }
    angle = MOD_ROTATION2048(target.contour[ii].angle - 512);
    radius = target.contour[ii].r;
    xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
    yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
    glVertex2f(xx, yy);
  }
  // draw expected pos for test
  // may crash for unknown reason, but usefull in debugging
  int angle = MOD_ROTATION2048(target.expected.angle - 512);
  int radius = target.expected.r;

  // following displays expected position with crosses that indicate the size of the search area
  // for debugging only

   double xx;
   double yy;
   int dist_a = (int)(326. / (double)radius * OFF_LOCATION / 2.);
   int dist_r = (int)((double)OFF_LOCATION / 2.);
   glColor4ub(0, 250, 0, 250);
   if (radius < 511 - dist_r && radius > dist_r ){
       xx = polarLookup->x[MOD_ROTATION2048(angle)][radius - dist_r] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[MOD_ROTATION2048(angle)][radius - dist_r] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[MOD_ROTATION2048(angle)][radius + dist_r] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[MOD_ROTATION2048(angle)][radius + dist_r] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[MOD_ROTATION2048(angle - dist_a)][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[MOD_ROTATION2048(angle - dist_a)][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[MOD_ROTATION2048(angle + dist_a)][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[MOD_ROTATION2048(angle + dist_a)][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
   }

  glEnd();
}

void RadarArpa::DrawArpaTargets() {
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status != LOST) {
   //     LOG_INFO(wxT("BR24radar_pi: $$$ DrawArpaTargets  entered i= %i"), i);
      DrawContour(m_targets[i]);
      /* if (m_targets[i].nr_of_log_entries == 10){   // to find covariance of observations, use with steady ship on steady targets
           for (int j = 1; j < 10; j++){
               LOG_INFO(wxT("BR24radar_pi: $$$ logbook dist angle % i, %i"), m_targets[i].logbook[j].pp.r,
       m_targets[i].logbook[j].pp.angle);
           }
       }*/
    }
  }
}

void RadarArpa::RefreshArpaTargets() {
  //  LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets  entered"));
  int target_to_delete = -1;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == LOST) {
      continue;
    }
    //   LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets  entered, found i = %i"), i);
    m_targets[i].RefreshTarget();
    if (m_targets[i].status == FOR_DELETION) {
      target_to_delete = i;
      LOG_INFO(wxT("BR24radar_pi: $$$ FOR_DELETION i= %i"), i);
    }
  }
  if (target_to_delete != -1){
      // delete targets
      Polar pol = m_targets[target_to_delete].pol_z;
      for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
          if (i == target_to_delete) continue;
          if (m_targets[i].pol_z.r == pol.r && m_targets[i].pol_z.angle == pol.angle){
              // this is the target to be deleted
              m_targets[i].SetStatusLost();
              LOG_INFO(wxT("BR24radar_pi: $$$ deleted i= %i"), i);
              m_targets[target_to_delete].SetStatusLost();
          }
      }
      if (m_targets[target_to_delete].status == FOR_DELETION){
          // no target deleted
          m_targets[target_to_delete].SetStatusLost();
      }
  }
  
}

void ArpaTarget::RefreshTarget() {
  Position prev_X;
  Position prev2_X;
  Polar pol;
  Position own_pos;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pol = Pos2Polar(X, own_pos, m_ri->m_range_meters);
  wxLongLong time1 = m_ri->m_history[MOD_ROTATION2048(pol.angle)].time;
  wxLongLong time2 = m_ri->m_history[MOD_ROTATION2048(pol.angle + SCAN_MARGIN)].time;
  // check if target has been refreshed since last time
  // and if the beam has passed the target location with SCAN_MARGIN spokes
  if (time1 > (t_refresh + 1000) && time2 >= time1) {

      // set new refresh time
      t_refresh = time1;
      wxLongLong t_target = time1;  // estimated new target time
      prev2_X = prev_X;
      prev_X = X;  // save the previous target position

      // get estimate
      X.time = t_target;
      LOG_INFO(wxT("BR24radar_pi: $$$ before predict X.time %u, prevX.time %u"), X.time.GetLo(), prev_X.time.GetLo());
      unsigned int delta_t = (unsigned int)((X.time - prev_X.time).GetLo());
      LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets  before predict delta_t= %u"), delta_t);

      m_kalman->Predict(&X, delta_t);  // X is new estimated position vector

      // now set the polar to expected position
      pol = Pos2Polar(X, own_pos, m_ri->m_range_meters);
      // zooming and target movement may  cause r to be out of bounds
      if (pol.r >= RETURNS_PER_LINE || pol.r <= 0) {
          LOG_INFO(wxT("BR24radar_pi: $$$ after predict r too large or negative r = %i"), pol.r);
          SetStatusLost();
          LOG_INFO(wxT("BR24radar_pi: $$$ target lost"));
          return;
      }
      expected = pol;  // $$$ for test only
      LOG_INFO(wxT("BR24radar_pi: $$ expected r  %i, alfa %i"), pol.r, pol.angle);

      // get measurement
      Position z;

      if (GetTarget(&pol, &z)) {
          // target refreshed
          LOG_INFO(wxT("BR24radar_pi: $$$ ***Gettarget true estimated time %u, target time %u"), X.time.GetLo(), z.time.GetLo());
          // check if target has a new later time than previous target
          if (z.time <= prev_X.time) {
              // found old target again, reset what we have done   
              LOG_INFO(wxT("BR24radar_pi: $$$ Gettarget same time found"));
              X = prev_X;
              prev_X = prev2_X;
              return;
          }
          lost_count = 0;
          if (status == AQUIRE0 || status == FOR_DELETION) {
              X = z;  // as we have no history we move target to measured position
              LOG_INFO(wxT("BR24radar_pi: $$$ status 0 "));
              X.dlat_dt = 0.;
              X.dlon_dt = 0.;
              z.dlat_dt = 0.;
              z.dlon_dt = 0.;
          }
          else {  // set speed in z
              double delta_t = (double)((z.time - prev_X.time).GetLo()) / 1000.;  // seconds
              LOG_INFO(wxT("BR24radar_pi: $$$ delta t for speed %f"), delta_t);
              z.dlat_dt = (z.lat - prev_X.lat) / delta_t;  // degrees per second
              z.dlon_dt = (z.lon - prev_X.lon) / delta_t;  // degrees per second
          }
          if (status != FOR_DELETION) status++;
          LOG_INFO(wxT("BR24radar_pi: $$$ new status = %i"), status);
          double gain_p = 0.2;  // Kalman gain for position
          double gain_s = 0.1;  // Kalman gain for velocity
          if (status <= 5) { 
              gain_p = 0.5;
              gain_s = 0.5;
          }
          if (status <= 3) {
              gain_p = 0.8;
              gain_s = 0.8;
          }
          m_kalman->SetMeasurement(&z, &X, gain_p, gain_s);  // X is new estimated position, improved with measured position
          pol_z = Pos2Polar(z, own_pos, m_ri->m_range_meters);   // used for deletion of targets
      }
      else {
          // target not found
          if (status == AQUIRE0 || status == AQUIRE1 || status == FOR_DELETION) {
              SetStatusLost();
              //     LOG_INFO(wxT("BR24radar_pi: $$$ case aquire0 or 1 lost"));
              return;
          }
          else {
              lost_count++;
              if (lost_count >= MAX_LOST_COUNT) {
                  SetStatusLost();
                  //          LOG_INFO(wxT("BR24radar_pi: $$$ target lost"));
                  return;
              }
          }
          // target was not found but gets another chance
          LOG_INFO(wxT("BR24radar_pi: $$$ target not found but not lost, target time= %u"), X.time.GetLo());
      }
      // set refresh time to the time of the spoke where the target was found
      t_refresh = X.time;
      PushLogbook();
      logbook[0].time = X.time;
      
      logbook[0].pos = X;
      logbook[0].time = X.time;
      if (status >= 2) {
          CalculateSpeedandCourse();
          double speed_now;
          double s1 = X.dlat_dt * 60.;                       // nautical miles per second
          double s2 = X.dlon_dt * cos(deg2rad(X.lat)) * 60.; // nautical miles per second
          speed_now = (sqrt(s1 * s1 + s2 * s2)) * 3600.;   // and convert to nautical miles per hour
          LOG_INFO(wxT("BR24radar_pi: $$$ **before pass arpa X.time. %u, prev_X.time %u"), X.time.GetLo(), prev_X.time.GetLo());
          LOG_INFO(wxT("BR24radar_pi: $$$ *********Speed = %f,  s1= %f, s2= %f, Heading = %f"), speed_now, s1,
              s2, rad2deg(atan2(s2, s1)));
          // send target data to OCPN
          pol = Pos2Polar(X, own_pos, m_ri->m_range_meters);
          if (status >= 3) {
              OCPN_target_status s;
              if (status >= Q_NUM) s = Q;
              if (status > T_NUM) s = T;
              PassARPAtoOCPN(&pol, s);
          }
      }
  }
  return;
}

#define PIX(aa, rr)       \
  if (rr > 510) continue; \
  if (Pix(aa, rr)) {      \
    pol->angle = aa;       \
    pol->r = rr;           \
    return true;          \
  }

bool ArpaTarget::FindNearestContour(Polar* pol, int dist) {
  // $$$ to do: no single pixel targets
  // make a search pattern along a square
  // returns the position of the nearest blob found in pol
  int a = pol->angle;
  int r = pol->r;
  if (dist < 2) dist = 2;
  for (int j = 2; j <= dist; j++) {
    int dist_r = (int)((double)j / 2.);
    int dist_a = (int)(326. / (double)r * j / 2.);  // 326/r: conversion factor to make squares
    for (int i = a - dist_a; i < a + dist_a; i++) {  // "upper" side
      PIX(i, r + dist_r);
    }
    for (int i = r + dist_r; i > r - dist_r; i--) {  // "right hand" side
      PIX(a + dist_a, i);
    }
    for (int i = a + dist_a; i > a - dist_a; i--) {  // "lower" side
      PIX(i, r - dist_r);
    }
    for (int i = r - dist_r; i < r + dist_r; i++) {  // "left hand" side
      PIX(a - dist_a, i);
    }
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ FindNearestContour return false"));
  return false;
}

void RadarArpa::CalculateCentroid(ArpaTarget* target) {
  /* target->pol.angle = (target->max_angle.angle + target->min_angle.angle) / 2;
   target->pol.r = (target->max_r.r + target->min_r.r) / 2;*/
  // real calculation still to be done
}

void ArpaTarget::PushLogbook() {
  int num = sizeof(LogEntry) * (SIZE_OF_LOG - 1);
  memmove(&logbook[1], logbook, num);
  nr_of_log_entries++;
}

ArpaTarget::ArpaTarget(br24radar_pi* pi, RadarInfo* ri) {
  ArpaTarget::m_ri = ri;
  m_pi = pi;
  m_kalman = 0;
  SetStatusLost();
}

ArpaTarget::ArpaTarget() { 
    m_kalman = 0;
    SetStatusLost(); 
}

void ArpaTarget::CalculateSpeedandCourse() {
  int nr = 12;
  if (nr > nr_of_log_entries - 1) nr = nr_of_log_entries - 1;
  if (nr_of_log_entries <= 1 ){
      logbook[0].speed = 0.;
      logbook[0].course = 0.;
      return;
  }
  double lat1 = logbook[nr].pos.lat;
  double lat2 = logbook[0].pos.lat;
  double lon1 = logbook[nr].pos.lon;
  double lon2 = logbook[0].pos.lon;
  double dist = local_distance(lat1, lon1, lat2, lon2);  // dist in miles
  long delta_t = (logbook[0].time - logbook[nr].time).GetLo();  // get the lower 32 bits of the wxLongLong
  if (delta_t > 10) {
    double speed = (dist * 1000.) / (double)delta_t * 1852.;  // speed in m/s
    double course = local_bearing(lat1, lon1, lat2, lon2);
    logbook[0].speed = speed;
    logbook[0].course = course;
  } else {
    logbook[0].speed = logbook[1].speed;
    logbook[0].course = logbook[1].course;
  }
  // also do bearing and distance
  Polar pp;
  Position own_pos;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pp = Pos2Polar(logbook[0].pos, own_pos, m_ri->m_range_meters);
  bearing = (double)pp.angle * 360. / (double)LINES_PER_ROTATION;
  distance = (double)pp.r * (double)m_ri->m_range_meters / (double)RETURNS_PER_LINE / 1852.;
}

bool ArpaTarget::GetTarget(Polar* pol, Position* z) {
  // general target refresh
  bool contour_found = FindContourFromInside(pol);
  if (contour_found) {
  } else {
    int dist = OFF_LOCATION;
    if (status == AQUIRE0 || status == AQUIRE1) {
      dist = OFF_LOCATION * 2;
    }
    if (dist > pol->r - 5) dist = pol->r - 5; // don't search close to origin
    contour_found = FindNearestContour(pol, dist);
  }
  if (!contour_found) {
    LOG_INFO(wxT("BR24radar_pi: $$$ GetTarget No contour found r= %i"), pol->r);
    return false;
  }
  if (GetContour(pol, z) != 0) {
      LOG_INFO(wxT("BR24radar_pi: $$$ GetContour returned false"));
    return false;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ GetContour returned true"));
  return true;
}

void ArpaTarget::PassARPAtoOCPN(Polar* pol, OCPN_target_status status) {
    wxString s_TargID, s_Bear_Unit, s_Course_Unit;
    wxString s_speed, s_course, s_Dist_Unit, s_status;
    wxString s_bearing;
    wxString s_distance;
    wxString s_target_name;
    wxString nmea;
    char sentence[90];
    char checksum = 0;
    char* p;

    s_Bear_Unit = wxEmptyString;      // Bearing Units  R or empty
    s_Course_Unit = wxT("T");              // Course type R; Realtive T; true 
    s_Dist_Unit = wxT("N");                // Speed/Distance Unit K, N, S N= NM/h = Knots
    if (status == Q) s_status = wxT("Q");
    if (status == T) s_status = wxT("T");
    if (status == L) s_status = wxT("L");

    double dist = (double)pol->r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters / 1852.;
    double bearing = (double)pol->angle * 360. / (double)LINES_PER_ROTATION;
    double speed_kn = logbook[0].speed * 3600. / 1852.;
    if (bearing < 0) bearing += 360;
    //LOG_INFO(wxT("BR24radar_pi: $$$ send dist = %f, bearing = %f"), dist, bearing);
    s_TargID = wxString::Format(wxT("%2i"), target_id);
    s_speed = wxString::Format(wxT("%4.2f"), status == Q ? 0.0 : speed_kn);
    s_course = wxString::Format(wxT("%3.1f"), status == Q ? 0.0 : logbook[0].course);
    s_target_name = wxString::Format(wxT("MARPA%2i"), target_id);
    s_distance = wxString::Format(wxT("%f"), dist);
    s_bearing = wxString::Format(wxT("%f"), bearing);

    /* Code for TTM follows. Send speed and course using TTM*/
    LOG_INFO(wxT("BR24radar_pi: $$$ pushed speed = %f"), speed_kn);
    //                                    1  2  3  4  5  6  7  8 9 10 11 12 13 
    int   TTM = sprintf(sentence, "RATTM,%2s,%s,%s,%s,%s,%s,%s, , ,%s,%s,%s, ",
        (const char*)s_TargID.mb_str(),      // 1 target id
        (const char*)s_distance.mb_str(),    // 2 Targ distance
        (const char*)s_bearing.mb_str(),     // 3 Bearing fr own ship.
        (const char*)s_Bear_Unit.mb_str(),   // 4 Brearing unit ( T = true)
        (const char*)s_speed.mb_str(),       // 5 Target speed 
        (const char*)s_course.mb_str(),      // 6 Target Course. 
        (const char*)s_Course_Unit.mb_str(), // 7 Course ref T // 8 CPA Not used // 9 TCPA Not used
        (const char*)s_Dist_Unit.mb_str(),   // 10 S/D Unit N = knots/Nm 
        (const char*)s_target_name.mb_str(), // 11 Target name
        (const char*)s_status.mb_str());     // 12 Target Status L/Q/T // 13 Ref N/A

    for (p = sentence; *p; p++) {
        checksum ^= *p;
    }
    nmea.Printf(wxT("$%s*%02X\r\n"), sentence, (unsigned)checksum);
    //LOG_INFO(wxT("BR24radar_pi: $$$ pushed TTM= %s"), nmea);
    PushNMEABuffer(nmea);
}


void ArpaTarget::SetStatusLost() {
  status = LOST;
  contour_length = 0;
  contour_length = 0;
  nr_of_log_entries = 0;
  lost_count = 0;
  if (m_kalman) {
    m_kalman->~Kalman_Filter();  // delete the filter
    m_kalman = 0;
    Polar p;
    p.angle = 0;
    p.r = 0;
    PassARPAtoOCPN(&p,L);
    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman filter deleted"));
  }
}


void RadarArpa::DeleteAllTargets(){
    for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
        if (m_targets[i].status != LOST) {
            m_targets[i].SetStatusLost();
        }
    }
}

PLUGIN_END_NAMESPACE
