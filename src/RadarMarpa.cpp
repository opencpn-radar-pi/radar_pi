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

    // test of Metric2Polar(MetricPoint q, Position own_ship, int range)
    //Polar2Metric(Polar pol, Position own_ship, int range){

    MetricPoint q, z;
    Polar p;
    Position own;
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

Position ArpaTarget::Polar2Pos(Polar pol, Position own_ship) {
  // The "own_ship" in the fumction call can be the position at an earlier time than the current position
  // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
  // based on the own ship position own_ship
  Position pos;
  pos.lat = own_ship.lat +
            (double)pol.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                cos(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / 60. / 1852.;
  pos.lon = own_ship.lon +
            (double)pol.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                sin(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / cos(deg2rad(own_ship.lat)) / 60. / 1852.;
  return pos;
}

void Polar::Pos2Polar(Position p, Position own_ship, int range) {
  // converts in a radar image a lat-lon position to angular data
  double dif_lat = p.lat;
  dif_lat -= own_ship.lat;
  double dif_lon = (p.lon - own_ship.lon) * cos(deg2rad(own_ship.lat));
  r = (int)(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * (double)RETURNS_PER_LINE / (double)range);
  angle = (int)((atan2(dif_lon, dif_lat)) * (double)LINES_PER_ROTATION / (2. * PI));
}

bool ArpaTarget::Pix(int ang, int rad) {
  if (rad < 1 || rad >= RETURNS_PER_LINE - 1) {  // $$$ avoid range ring
    return false;
  }
  return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
}

void RadarArpa::Aquire0NewTarget(Position target_pos) {
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
  m_targets[i_target].X = Pos2Metric(target_pos);  // Expected position
  m_targets[i_target].X.time = wxGetUTCTimeMillis();
  m_targets[i_target].X.dlat_dt = 0.;
  m_targets[i_target].X.dlon_dt = 0.;
  m_targets[i_target].status = 0;
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

int ArpaTarget::GetContour(Polar* pol, MetricPoint* z) {  // sets the measured_pos if succesfull
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
  
  LOG_INFO(wxT("BR24radar_pi: $$$ blob found  target_time= %i"),  target_time.GetLo());
  
  Position p_own;
  p_own.lat = m_ri->m_history[MOD_ROTATION2048(pol->angle)].lat;  // get the position at receive time
  p_own.lon = m_ri->m_history[MOD_ROTATION2048(pol->angle)].lon;
  // Z is the measured metric position of the target
  MetricPoint z;
  *z = Polar2Metric(*pol, p_own, m_ri->m_range_meters);  // using own ship location from the time of reception
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
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == lost) {
      continue;
    }
 //   LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets  entered, found i = %i"), i);
    m_targets[i].RefreshTarget();
  }
}


//logbook[0].pp = pol;
//nr_of_log_entries++;
//if (nr_of_log_entries > SIZE_OF_LOG) {
//    nr_of_log_entries = SIZE_OF_LOG;

void ArpaTarget::RefreshTarget() {
  //   LOG_INFO(wxT("BR24radar_pi: $$$ refresh target entered"));

  MetricPoint prev_X;
  MetricPoint prev2_X;
  Polar pol;
  Position own_pos;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pol = Metric2Polar(X, own_pos, m_ri->m_range_meters);

  wxLongLong time_hist = m_ri->m_history[MOD_ROTATION2048(pol.angle + SCAN_MARGIN)].time;
  if (t_refresh >= time_hist) {
    // check if target has been refreshed since last time
    // + SCAN_MARGIN because target may have mooved
    return;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ ***t_refresh %i, time_hist= %i"), t_refresh.GetLo(), time_hist.GetLo());
  // set new refresh time
  t_refresh = m_ri->m_history[MOD_ROTATION2048(pol.angle + SCAN_MARGIN)].time;

  wxLongLong t_target = m_ri->m_history[MOD_ROTATION2048(pol.angle)].time;  // estimated new target time
  prev2_X = prev_X;
  prev_X = X;  // save the previous target position

  // get estimate
  X.time = t_target;
  int delta_t = (prev_X.time - X.time).GetLo();

  // set speed

  m_kalman->Predict(&X, delta_t);  // X is new estimated position vector

  // now set the polar to expected position
  pol = Metric2Polar(X, own_pos, m_ri->m_range_meters);
  // zooming and target movemedt may  cause r to be out of bounds
  if (pol.r >= RETURNS_PER_LINE || pol.r <= 0) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets r too large or negative r = %i"), pol.r);
    SetStatusLost();
    LOG_INFO(wxT("BR24radar_pi: $$$ target lost"));
    return;
  }
  expected = pol;  // $$$ for test only

  // get measurement
  MetricPoint z;
  if (GetTarget(&pol, &z)) {
    // target refreshed
    LOG_INFO(wxT("BR24radar_pi: $$$ ***Gettarget true estimated time %i, target time %i"), X.time.GetLo(), z.time.GetLo());
    // check if target has a new later time than previous target
    if (z.time <= prev_X.time) {
      // this is very wrong, found old target again, reset what we have done
      X = prev_X;
      prev_X = prev2_X;
      LOG_INFO(wxT("BR24radar_pi: $$$ Gettarget same time found prev target time %i, target time %i"), prev_X.time.GetLo(),
               z.time.GetLo());
      return;
    }
    lost_count = 0;
    if (status == 0) {
      X = z;  // as we have no history we move target to measured position
      LOG_INFO(wxT("BR24radar_pi: $$$ true case =aquire1 "));
    }
    status++;
    LOG_INFO(wxT("BR24radar_pi: $$$ new status = %i"), status);

    m_kalman->SetMeasurement(&z, &X);  // X is new estimated position, improved with measured position

  } else {
      if (same_time){
          LOG_INFO(wxT("BR24radar_pi: $$$ Gettarget same time"));
          xpos = prev_xpos;  // reset the target
          return;
      }
    switch (status) {
      case aquire0:
        SetStatusLost();
        break;
      case aquire1:
        SetStatusLost();
        //     LOG_INFO(wxT("BR24radar_pi: $$$ case aquire1 lost"));
        break;
      case aquire2:
        lost_count++;
        if (lost_count < MAX_LOST_COUNT) {
          break;  // give it another chance
        } else {
          SetStatusLost();
          //          LOG_INFO(wxT("BR24radar_pi: $$$ case aquire2 lost"));
          break;
        }
      case aquire3:
      case active:
        lost_count++;
        if (lost_count < MAX_LOST_COUNT) {
          break;  // try again next sweep
        } else {
          SetStatusLost();
          break;
        }
      default:
        break;
    }
    
    xpos.time = m_ri->m_history[MOD_ROTATION2048(pol.angle)].time;
    logbook[0].time = m_ri->m_history[MOD_ROTATION2048(pol.angle)].time;
  }
  if (status > aquire2) {
    Position xx;
    xx.lat = xpos.lat / 1852. / 60.;
    xx.lon = xpos.lon / 1852. / 60.;
    xx.lon /= cos(deg2rad(xx.lat));
    Position own_pos;
    own_pos.lat = m_pi->m_ownship_lat;
    own_pos.lon = m_pi->m_ownship_lon;
    pol.Pos2Polar(xx, own_pos, m_ri->m_range_meters);
    if (pol.r >= RETURNS_PER_LINE || pol.r <= 0) {
      LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets r too large or negative r = %i"), pol.r);
      SetStatusLost();
      LOG_INFO(wxT("BR24radar_pi: $$$ target lost"));
      return;
    }
    logbook[0].pos = xx;
    

    CalculateSpeedandCourse();
    double speed_now;
    /*double s1 = xpos.lat - prev_xpos.lat;
    double s2 = xpos.lon - prev_xpos.lon;*/
    double s1 = xpos.d_lat;
    double s2 = xpos.d_lon;
    double tt = (double)((xpos.time - prev_xpos.time).GetLo()) / 1000.;
    speed_now = (sqrt(s1 * s1 + s2 * s2)) / tt;
    LOG_INFO(wxT("BR24radar_pi: $$$ ** xpos.time. %i, prev_xpos.time %i"), xpos.time.GetLo(), prev_xpos.time.GetLo());
    LOG_INFO(wxT("BR24radar_pi: $$$ *********Speed = %f, time %f, s1= %f, s2= %f, Heading = %f"), speed_now * 3600./1852., tt, s1, s2,
             logbook[0].course);
    // send target data to OCPN
    if (status >= aquire3){
        OCPN_target_status s;
        if (status == aquire3) s = Q;
        if (status == active) s = T;
        if (status == LOST) s = L;
            PassARPAtoOCPN(s);
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

void ArpaTarget::UpdatePolar() {
  // find polar of a target based on expected position and own position
  // calculate expected new position of target first
  Position target_pos = logbook[0].pos;
  double speed = logbook[0].speed;
  double course = logbook[0].course;
  wxLongLong delta_t = wxGetUTCTimeMillis() - logbook[0].time;
  double dist = delta_t.GetLo() * speed / 1000.;
  target_pos.lat = target_pos.lat + cos(deg2rad(course)) * dist / 1852. / 60;
  target_pos.lon = target_pos.lon + sin(deg2rad(course)) * dist / 1852. / 60 / cos(deg2rad(target_pos.lat));
  Position own_pos;
  // and find the new polar
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pol.Pos2Polar(target_pos, own_pos, m_ri->m_range_meters);
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
  pp.Pos2Polar(logbook[0].pos, own_pos, m_ri->m_range_meters);
  bearing = (double)pp.angle * 360. / (double)LINES_PER_ROTATION;
  distance = (double)pp.r * (double)m_ri->m_range_meters / (double)RETURNS_PER_LINE / 1852.;
}

bool ArpaTarget::GetTarget(Polar* pol, MetricPoint* z) {
  // general target refresh

  bool contour_found = FindContourFromInside();
  
  if (contour_found) {
  } else {
    int dist = OFF_LOCATION;
    if (status == aquire0 || status == aquire1) {
      dist = OFF_LOCATION * 2;
    }
    if (dist > pol->r - 5) dist = pol->r - 5; // don't search close to origin
    contour_found = FindNearestContour(Polar* pol, dist);
  }
  if (!contour_found) {
    LOG_INFO(wxT("BR24radar_pi: $$$ GetTarget No contour found r= %i"), pol.r);
    return false;
  }
  if (GetContour() != 0) {
      LOG_INFO(wxT("BR24radar_pi: $$$ GetContour returned false"));
    return false;
  }
  return true;
}

void ArpaTarget::PassARPAtoOCPN(OCPN_target_status status) {
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

    double dist = (double)pol.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters / 1852.;
    double bearing = (double)pol.angle * 360. / (double)LINES_PER_ROTATION;
    double speed_kn = logbook[0].speed * 3600. / 1852.;
    if (bearing < 0) bearing += 360;
    //LOG_INFO(wxT("BR24radar_pi: $$$ send dist = %f, bearing = %f"), dist, bearing);
    s_TargID = wxString::Format(wxT("%2i"), target_id);
    s_speed = wxString::Format(wxT("%4.2f"), speed_kn);
    s_course = wxString::Format(wxT("%3.1f"), logbook[0].course);
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
        (const char*)s_status.mb_str());    // 12 Target Status L/Q/T // 13 Ref N/A

    for (p = sentence; *p; p++) {
        checksum ^= *p;
    }
    nmea.Printf(wxT("$%s*%02X\r\n"), sentence, (unsigned)checksum);
    //LOG_INFO(wxT("BR24radar_pi: $$$ pushed TTM= %s"), nmea);
    PushNMEABuffer(nmea);
}

void RadarArpa::PassARPATargetsToOCPN() {
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == active) {
      m_targets[i].PassARPAtoOCPN(Q);
    }
  }
}

MetricPoint Pos2Metric(Position p){
  MetricPoint q;
  q.lat = p.lat * 60.* 1852.;
  q.lon = p.lon * 60.* 1852.;
  q.lon *= cos(deg2rad(p.lat));
  q.dlat_dt = nan("");
  q.dlon_dt = nan("");
  q.time = nanl("");
  return q;
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
    PassARPAtoOCPN(L);
    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman filter deleted"));
  }
}

Position MetricPoint::Metric2Pos() {
    Position p;
    p.lon = lon / cos(deg2rad(lat)) / 1852. / 60;
    p.lat = lat / 1852. / 60;
    return p;
}

Polar Metric2Polar(MetricPoint q, Position own_ship, int range){
    // converts in a radar image a metric lat-lon position to angular data
    double dif_lat = -own_ship.lat * 60. * 1852.;
    dif_lat += q.lat;
    double dif_lon = q.lon - own_ship.lon * 60. * 1852. * cos(deg2rad(own_ship.lat));
    Polar p;
    p.r = (int)(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * (double)RETURNS_PER_LINE / (double)range);
    p.angle = (int)((atan2(dif_lon, dif_lat)) * (double)LINES_PER_ROTATION / (2. * PI));
    p.time = q.time;
    return p;
}

MetricPoint Polar2Metric(Polar pol, Position own_ship, int range){
        MetricPoint q;
        q.lat = own_ship.lat * 60. * 1852. +
        (double)pol.r / (double)RETURNS_PER_LINE * (double)range *
        cos(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) ;
        q.lon = own_ship.lon * 60. * 1852. / cos(deg2rad(own_ship.lat)) +
        (double)pol.r / (double)RETURNS_PER_LINE * (double)range *
        sin(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle)));
        q.time = pol.time;
        q.dlat_dt = nan("");
        q.dlon_dt = nan("");
    return q;
}

PLUGIN_END_NAMESPACE
