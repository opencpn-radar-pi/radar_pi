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
  Polar targ_pol;
  Position own_pos;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  targ_pol.Pos2Polar(target_pos, own_pos, m_ri->m_range_meters);
  int i_target = NextEmptyTarget();
  if (i_target == -1) {
    LOG_INFO(wxT("BR24radar_pi: RadarArpa:: max targets exceeded "));
    return;
  }
  m_targets[i_target].pol = targ_pol;  // set the Polar
  m_targets[i_target].status = aquire0;
  target_id_count++;
  if (target_id_count >= 100) target_id_count = 1;
  m_targets[i_target].target_id = target_id_count;

  return;
}

bool ArpaTarget::FindContourFromInside() {  // moves pol to contour of blob
  // true if success
  // false when failed
  int ang = pol.angle;
  int rad = pol.r;
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
    if (ang < pol.angle - MAX_CONTOUR_LENGTH / 2) {
      return false;
    }
    ang--;
    if (rad > 511) return false;
  }
  ang++;
  pol.angle = ang;
  return true;
}

int ArpaTarget::GetContour() {  // sets the measured_pos if succesfull
  // p must start on the contour of the blob
  // follows the contour in a clockwise direction

  wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);
  // the 4 possible translations to move from a point on the contour to the next
  Polar transl[4] = {0, 1, 1, 0, 0, -1, -1, 0};  // NB structure polar, not class Polar
  int count = 0;
  Polar start = pol;
  Polar current = pol;
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
  pol.angle = (max_angle.angle + min_angle.angle) / 2;
  if (max_r.r >= 511 || min_r.r >= 511) {
    return 10;  // return code 10 r too large
  }
  if (max_r.r < 2 || min_r.r < 2) {
    return 11;  // return code 11 r too small
  }
  if (logbook[0].time == m_ri->m_history[MOD_ROTATION2048(max_angle.angle)].time){
      // this target has the same time as previous, can't be OK, reject it
      LOG_INFO(wxT("BR24radar_pi: $$$ duplicate time found"));
      same_time = true;
      return 12;  // dupiplicate blob
  }
  same_time = false;
  pol.r = (max_r.r + min_r.r) / 2;
 
  PushLogbook();  // shift all entries down
  logbook[0].time = m_ri->m_history[MOD_ROTATION2048(max_angle.angle)].time;
  t_refresh = m_ri->m_history[MOD_ROTATION2048(max_angle.angle + SCAN_MARGIN)].time;
  Position p_own;
  p_own.lat = m_ri->m_history[MOD_ROTATION2048(pol.angle)].lat;  // get the position at receive time
  p_own.lon = m_ri->m_history[MOD_ROTATION2048(pol.angle)].lon;
  measured_pos = Polar2Pos(pol, p_own);  // using own ship location from the time of reception
  logbook[0].pos = measured_pos;
  LOG_INFO(wxT("BR24radar_pi: $$$ measured pos time = %i"), t_refresh.GetLo());
  logbook[0].pp = pol;
  nr_of_log_entries++;
  if (nr_of_log_entries > SIZE_OF_LOG) {
    nr_of_log_entries = SIZE_OF_LOG;
  }
  if (status >= aquire1) {
    CalculateSpeedandCourse();
  }
  return 0;  //  succes, blob found
}

int RadarArpa::NextEmptyTarget() {
  int index = 0;
  bool hit = false;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == lost) {
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
   double xx;
   double yy;
   /*glColor4ub(0, 250, 0, 250);
   if (radius < 480 && radius > 10){
       xx = polarLookup->x[angle][radius - 4] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[angle][radius - 4] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[angle][radius + 4] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[angle][radius + 4] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[angle - 5][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[angle - 5][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
       xx = polarLookup->x[angle + 5][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       yy = polarLookup->y[angle + 5][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
       glVertex2f(xx, yy);
   }*/

  glEnd();
}

void RadarArpa::DrawArpaTargets() {
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status != lost) {
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
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == lost) {
      continue;
    }
    m_targets[i].RefreshTarget();
  }
}

void ArpaTarget::RefreshTarget() {

  wxLongLong time_hist = m_ri->m_history[MOD_ROTATION2048(max_angle.angle + SCAN_MARGIN)].time;
  int time_hist32 = time_hist.GetLo();
 // LOG_INFO(wxT("BR24radar_pi: $$$ ***t_refresh %i, time_hist= %i"), t_refresh.GetLo(), time_hist32);
  if (t_refresh >= time_hist) {
    // check if target has been refreshed since last time
    // + SCAN_MARGIN because target may have mooved
    return;
  }
  // set new refresh time
  t_refresh = m_ri->m_history[MOD_ROTATION2048(max_angle.angle + SCAN_MARGIN)].time;
  if (status > aquire1) {
    prev_xpos = xpos;

    // get estimate

    if (m_kalman && status >= aquire3) {
      



      xpos = m_kalman->Predict(xpos);  // xpos is new estimated position
      xpos.time = logbook[0].time;
      
      // now set the polar to expected position
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




    } else {
      LOG_INFO(wxT("BR24radar_pi: $$$ Update polar called"));
      UpdatePolar();  // update expected polar of target based on speed and course from the log
                      // this is executed during target aquisition
    }
    // zooming may  cause r to be out of bounds
    if (pol.r >= RETURNS_PER_LINE || pol.r <= 0) {
      LOG_INFO(wxT("BR24radar_pi: $$$ RefreshArpaTargets r too large or negative r = %i"), pol.r);
      SetStatusLost();
      LOG_INFO(wxT("BR24radar_pi: $$$ target lost"));
      return;
    }
  }
  expected = pol;  // $$$ for test only

  // get measurement

  if (GetTarget()) {
    // target refreshed
      LOG_INFO(wxT("BR24radar_pi: $$$ Gettarget returned true"));
    lost_count = 0;
    switch (status) {
      case aquire0:
        status = aquire1;
        //      LOG_INFO(wxT("BR24radar_pi: $$$ true case =aquire1 "));
        break;
      case aquire1:
        status = aquire2;
        if (!m_kalman) {
          xpos = Pos2Metric(measured_pos);
          xpos.time = logbook[0].time;
          xpos.d_lat = 2.5 * logbook[0].speed * cos(deg2rad(logbook[0].course));
          xpos.d_lon = 2.5 * logbook[0].speed * sin(deg2rad(logbook[0].course));
          prev_xpos = xpos;
          m_kalman = new Kalman_Filter();
        }
        break;
      case aquire2:
        status = aquire3;
        //         LOG_INFO(wxT("BR24radar_pi: $$$ true case =aquire3 "));
        break;
      case aquire3:
        status = active;
        //         LOG_INFO(wxT("BR24radar_pi: $$$ true case =active "));
        break;
      case active:
        //            LOG_INFO(wxT("BR24radar_pi: $$$ true case was active "));
        break;
      default:
        break;
    }

    

    if (m_kalman && status > aquire2) {
      MetricPoint z = Pos2Metric(measured_pos);
      z.time = logbook[0].time;
      xpos.time = logbook[0].time;
      /*z.d_lat = 2.5 * logbook[0].speed * cos(deg2rad(logbook[0].course));
      z.d_lon = 2.5 * logbook[0].speed * sin(deg2rad(logbook[0].course));*/
      z.d_lat = z.lat - xpos.lat;
      z.d_lon = z.lon - xpos.lon;



      xpos = m_kalman->SetMeasurement(z, xpos);  // X is new estimated position, improved with measured position


      /*xpos.d_lat = xpos.lat - prev_xpos.lat;
      xpos.d_lon = xpos.lon - prev_xpos.lon;*/
    }
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
        if (lost_count < 2) {
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
    
    xpos.time = m_ri->m_history[MOD_ROTATION2048(max_angle.angle)].time;
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
    logbook[0].time = m_ri->m_history[MOD_ROTATION2048(max_angle.angle)].time;

    CalculateSpeedandCourse();
    double speed_now;
    double s1 = xpos.lat - prev_xpos.lat;
    double s2 = xpos.lon - prev_xpos.lon;
    double tt = (double)((xpos.time - prev_xpos.time).GetLo()) / 1000.;
    speed_now = (sqrt(s1 * s1 + s2 * s2)) / tt;
    LOG_INFO(wxT("BR24radar_pi: $$$ ** xpos.time. %i, prev_xpos.time %i"), xpos.time.GetLo(), prev_xpos.time.GetLo());
    LOG_INFO(wxT("BR24radar_pi: $$$ *********Speed = %f, time %f, s1= %f, s2= %f, Heading = %f"), speed_now, tt, s1, s2,
             logbook[0].course);
    // send target data to OCPN
    if (status >= aquire3){
        OCPN_target_status s;
        if (status == aquire3) s = Q;
        if (status == active) s = T;
        if (status == lost) s = L;
            PassARPAtoOCPN(s);
    }
  }
  return;
}

#define PIX(aa, rr)       \
  if (rr > 510) continue; \
  if (Pix(aa, rr)) {      \
    pol.angle = aa;       \
    pol.r = rr;           \
    return true;          \
  }

bool ArpaTarget::FindNearestContour(int dist) {
  // $$$ to do: no single pixel targets
  // make a search pattern along an approximate octagon
  int a = pol.angle;
  int r = pol.r;
  for (int i = 1; i < dist; i++) {
    int octa = (int)(i * .414 + .6);       // length of half side of octagon
    for (int j = -octa; j <= octa; j++) {  // top side
      PIX(a + j, r + i);                   // use the symmetry to check equivalent points
      PIX(a + i, r + j);                   // 90 degrees right rotation
      PIX(a + j, r - i);                   // mirror horizontal axis
      PIX(a - i, r + j);                   // mirror vertical axis
    }
    for (int j = 0; j <= octa; j++) {  // 45 degrees side
      PIX(a + octa + j, r + i - j);
      PIX(a - octa - j, r + i - j);
      PIX(a + octa + j, r - i + j);  // mirror horizontal axis
      PIX(a - octa - j, r - i + j);
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

bool ArpaTarget::GetTarget() {
  // general target refresh

  bool contour_found = FindContourFromInside();
  
  if (contour_found) {
  } else {
    int dist = OFF_LOCATION;
    if (status == aquire0 || status == aquire1) {
      dist = OFF_LOCATION * 2;
    }
    if (dist > pol.r - 5) dist = pol.r - 5; // don't search close to origin
    contour_found = FindNearestContour(dist);
   
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

void ArpaTarget::PassARPAtoOCPN( OCPN_target_status status ) {
    wxString s_TargID, s_Bear_Unit, s_Course_Unit;
    wxString s_speed, s_course, s_Dist_Unit, s_status;
    wxString s_bearing;
    wxString s_distance;
    wxString s_target_name;
    wxString nmea;
    char sentence [90];
    char checksum = 0;
    char* p;

    s_Bear_Unit = wxEmptyString;      // Bearing Units  R or empty
    s_Course_Unit = "T";              // Course type R; Realtive T; true 
    s_Dist_Unit = "N";                // Speed/Distance Unit K, N, S N= NM/h = Knots
    if ( status == Q ) s_status = "Q";
    if ( status == T ) s_status = "T";
    if ( status == L ) s_status = "L";

    double dist = (double)pol.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters / 1852.;
    double bearing = (double)pol.angle * 360. / (double)LINES_PER_ROTATION;
    double speed_kn = logbook [0].speed * 3600. / 1852.;
    if ( bearing < 0 ) bearing += 360;
    //LOG_INFO(wxT("BR24radar_pi: $$$ send dist = %f, bearing = %f"), dist, bearing);
    s_TargID = wxString::Format( wxT( "%2i" ), target_id );
    s_speed = wxString::Format( wxT( "%4.2f" ), speed_kn );
    s_course = wxString::Format( wxT( "%3.1f" ), logbook [0].course );
    s_target_name = wxString::Format( wxT( "MARPA%2i" ), target_id );
    s_distance = wxString::Format( wxT( "%f" ), dist );
    s_bearing = wxString::Format( wxT( "%f" ), bearing );

    /* Code for TTM follows. Send speed and course using TTM*/
    LOG_INFO( wxT( "BR24radar_pi: $$$ pushed speed = %f" ), speed_kn );
    //                                    1  2  3  4  5  6  7  8 9 10 11 12 13 
    int   TTM = sprintf( sentence, "RATTM,%2s,%s,%s,%s,%s,%s,%s, , ,%s,%s,%s, ",
        (const char*)s_TargID.mb_str(),      // 1 target id
        (const char*)s_distance.mb_str(),    // 2 Targ distance
        (const char*)s_bearing.mb_str(),     // 3 Bearing fr own ship.
        (const char*)s_Bear_Unit.mb_str(),   // 4 Brearing unit ( T = true)
        (const char*)s_speed.mb_str(),       // 5 Target speed 
        (const char*)s_course.mb_str(),      // 6 Target Course. 
        (const char*)s_Course_Unit.mb_str(), // 7 Course ref T // 8 CPA Not used // 9 TCPA Not used
        (const char*)s_Dist_Unit.mb_str(),   // 10 S/D Unit N = knots/Nm 
        (const char*)s_target_name.mb_str(), // 11 Target name
        (const char*)s_status.mb_str() );    // 12 Target Status L/Q/T // 13 Ref N/A

    for ( p = sentence; *p; p++ ) {
        checksum ^= *p;
    }
    nmea.Printf( wxT( "$%s*%02X\r\n" ), sentence, (unsigned)checksum );
    //LOG_INFO(wxT("BR24radar_pi: $$$ pushed TTM= %s"), nmea);
    PushNMEABuffer( nmea );
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
  return q;
}

void ArpaTarget::SetStatusLost() {
  status = lost;
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

PLUGIN_END_NAMESPACE
