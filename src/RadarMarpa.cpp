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
  m_ri = ri;
  m_pi = pi;
  radar_lost_count = 0;
  number_of_targets = 0;
  for (int i = 0; i < MAX_NUMBER_OF_TARGETS; i++) {
    m_targets[i] = 0;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa creator ready"));
}

ArpaTarget::~ArpaTarget() {
    
    if (m_kalman){
        delete m_kalman;
        m_kalman = 0;
    }
}


RadarArpa::~RadarArpa() {
    int n = number_of_targets;
    number_of_targets = 0;
  for (int i = 0; i < n; i++) {
    if (m_targets[i]) {
      delete m_targets[i];
      m_targets[i] = 0;
    }
  }
}

Position Polar2Pos(Polar pol, Position own_ship, double range) {
  // The "own_ship" in the fumction call can be the position at an earlier time than the current position
  // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
  // based on the own ship position own_ship
  Position pos;
  pos.lat = own_ship.lat +
            (double)pol.r / (double)RETURNS_PER_LINE * range * cos(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / 60. / 1852.;
  pos.lon = own_ship.lon +
            (double)pol.r / (double)RETURNS_PER_LINE * range * sin(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) /
                cos(deg2rad(own_ship.lat)) / 60. / 1852.;
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
    if (rad <= 1 || rad >= RETURNS_PER_LINE - 1) {  //  avoid range ring
      return false;
    }
    return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
  }

  bool ArpaTarget::MultiPix(int ang, int rad) {
      // same as Pix, but only true if a blob of at least 3 pixels was found
      int test = 0;
      if (Pix(ang, rad)){
          test = Pix(ang + 1, rad) + Pix(ang - 1, rad) + Pix(ang, rad + 1) + Pix(ang, rad - 1);
      }
      if (test < 2) return false;
      else return true;
  }

  void RadarArpa::AquireNewTarget(Position target_pos, int status) {
    // aquires new target from mouse click position
    // no contour taken yet
    // target status aquire0
    // returns in X metric coordinates of click
    // constructs Kalman filter

    // make new target
    int i_target;
    if (number_of_targets < MAX_NUMBER_OF_TARGETS - 1 || (number_of_targets == MAX_NUMBER_OF_TARGETS - 1 && status == -2)) {
        if (m_targets[number_of_targets] == 0){
        m_targets[number_of_targets] = new ArpaTarget(m_pi, m_ri);
        }
      i_target = number_of_targets;
      number_of_targets++;
    } else {
      i_target = -1;
      LOG_INFO(wxT("BR24radar_pi: RadarArpa:: Error, max targets exceeded "));
      return;
    }
    m_targets[i_target]->X = target_pos;  // Expected position
    m_targets[i_target]->X.time = 0;
    m_targets[i_target]->X.dlat_dt = 0.;
    m_targets[i_target]->X.dlon_dt = 0.;
    m_targets[i_target]->status = status;

    if (!m_targets[i_target]->m_kalman) {
      m_targets[i_target]->m_kalman = new Kalman_Filter(m_ri->m_range_meters);
    }
    return;
  }

  bool ArpaTarget::FindContourFromInside(Polar * pol) {  // moves pol to contour of blob
    // true if success
    // false when failed
    int ang = pol->angle;
    int rad = pol->r;
    if (rad >= RETURNS_PER_LINE - 1 || rad < 3) {
      return false;
    }
    if (!(Pix(ang, rad))) {
      return false;
    }
    while (Pix(ang, rad)) {
      ang--;
    }
    ang++;
    pol->angle = ang;
    return true;
  }

  int ArpaTarget::GetContour(Polar * pol) {  // sets the measured_pos if succesfull
                                             // pol must start on the contour of the blob
                                             // follows the contour in a clockwise direction
                                             // returns metric position of the blob in Z
    wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);
    // the 4 possible translations to move from a point on the contour to the next
    Polar transl[4];  //   = { 0, 1,   1, 0,   0, -1,   -1, 0 };
    transl[0].angle = 0;
    transl[0].r = 1;

    transl[1].angle = 1;
    transl[1].r = 0;

    transl[2].angle = 0;
    transl[2].r = -1;

    transl[3].angle = -1;
    transl[3].r = 0;

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
    if (start.r >= RETURNS_PER_LINE - 1) {
      return 1;  // return code 1, r too large
    }
    if (start.r < 4) {
      return 2;  // return code 2, r too small
    }
    if (!Pix(start.angle, start.r)) {
      return 3;  // return code 3, starting point outside blob
    }
    // first find the orientation of border point p
    for (int i = 0; i < 4; i++) {
      index = i;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
      //  if (rr > 511) return 13;  // r too large
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
        succes = Pix(aa, rr);  
        if (succes) {
          // next point found

          break;
        }
        index += 1;
      }
      if (!succes) {
        LOG_INFO(wxT("BR24radar_pi::RadarArpa::GetContour no next point found count= %i"), count);
        return 7;  // return code 7, no next point found
      }
      // next point found
      current.angle = aa;
      current.r = rr;
      if (count < MAX_CONTOUR_LENGTH - 2) {
        contour[count] = current;
      }
      if (count == MAX_CONTOUR_LENGTH - 2) {
        contour[count] = start;  // shortcut to the beginning for drawing the contour
      }
      if (count < MAX_CONTOUR_LENGTH - 1) {
        count++;
      }
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
    }
    contour_length = count;
    //  CalculateCentroid(*target);    we better use the real centroid instead of the average, todo
    pol->angle = (max_angle.angle + min_angle.angle) / 2;
    if (max_r.r > RETURNS_PER_LINE - 1 || min_r.r > RETURNS_PER_LINE - 1) {
      return 10;  // return code 10 r too large
    }
    if (max_r.r < 2 || min_r.r < 2) {
      return 11;  // return code 11 r too small
    }
    pol->r = (max_r.r + min_r.r) / 2;
    pol->time = m_ri->m_history[MOD_ROTATION2048(pol->angle)].time;
    return 0;  //  succes, blob found
  }


  void RadarArpa::DrawContour(ArpaTarget* target) {
    // should be improved using vertex arrays
    PolarToCartesianLookupTable* polarLookup;
    polarLookup = GetPolarToCartesianLookupTable();
    glColor4ub(200, 200, 100, 250);
    glLineWidth(3.0);
    glBegin(GL_LINES);
    for (int i = 0; i < target->contour_length; i++) {
      double xx;
      double yy;
      int angle = MOD_ROTATION2048(target->contour[i].angle - 512);
      int radius = target->contour[i].r;
      if (radius <= 0 || radius >= RETURNS_PER_LINE) {
        return;
      }
      xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
      yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
      glVertex2f(xx, yy);
      int ii = i + 1;
      if (ii == target->contour_length) {
        ii = 0;  // start point again
      }
      if (radius <= 0 || radius >= RETURNS_PER_LINE) {
        return;
      }
      angle = MOD_ROTATION2048(target->contour[ii].angle - 512);
      radius = target->contour[ii].r;
      xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
      yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
      glVertex2f(xx, yy);
    }
    // draw expected pos for test
    // may crash for unknown reason, but usefull in debugging
    int angle = MOD_ROTATION2048(target->expected.angle - 512);
    int radius = target->expected.r;

    // following displays expected position with crosses that indicate the size of the search area
    // for debugging only

    /*double xx;
    double yy;
    int dist_a = (int)(326. / (double)radius * OFF_LOCATION / 2.);
    int dist_r = (int)((double)OFF_LOCATION / 2.);
    glColor4ub(0, 250, 0, 250);
    if (radius < 511 - dist_r && radius > dist_r) {
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
    }*/



    glEnd();
  }

  void RadarArpa::DrawArpaTargets() {
    for (int i = 0; i < number_of_targets; i++) {
      if (!m_targets[i]) continue;
    if (m_targets[i]->status != LOST) {
      DrawContour(m_targets[i]);
    }
  }
}

void RadarArpa::RefreshArpaTargets() {
  // remove targets with status LOST and put them at the end
  for (int i = 0; i < number_of_targets; i++) {
    if (m_targets[i]) {
      if (m_targets[i]->status == LOST) {
        // and swap the deleted target with the last one
          ArpaTarget* swap = m_targets[i];
          // we keep the lost target for later use, destruction and construction is expensive
        m_targets[i] = m_targets[number_of_targets - 1];
        m_targets[number_of_targets - 1] = swap;
        number_of_targets--;
      }
    }
  }
  int target_to_delete = -1;
  // find a target with status FOR_DELETION if it is there
  for (int i = 0; i < number_of_targets; i++) {
      if (!m_targets[i]) continue;
    if (m_targets[i]->status == FOR_DELETION) {
      target_to_delete = i;
    }
  }
  if (target_to_delete != -1) {
    // delete the target that is closest to the target with status FOR_DELETION
    Position x = m_targets[target_to_delete]->X;
    double min_dist = 1000;
    int del_target = -1;
    for (int i = 0; i < number_of_targets; i++) {
        if (!m_targets[i]) continue;
      if (i == target_to_delete || m_targets[i]->status == LOST) continue;
      double dif_lat = x.lat - m_targets[i]->X.lat;
      double dif_lon = (x.lon - m_targets[i]->X.lon) * cos(deg2rad(x.lat));
      double dist2 = dif_lat * dif_lat + dif_lon * dif_lon;
      if (dist2 < min_dist) {
        min_dist = dist2;
        del_target = i;
      }
    }
    // del_target is the index of the target closest to target with index target_to_delete
    if (del_target != -1) {
      m_targets[del_target]->SetStatusLost();
    }
    m_targets[target_to_delete]->SetStatusLost();
  }

  // main target refresh loop
  for (int i = 0; i < number_of_targets; i++) {
      if (!m_targets[i]) continue;
    if (m_targets[i]->status == LOST) {
      continue;
    }
    m_targets[i]->RefreshTarget();
  }
  if (m_pi->m_settings.guard_zone_on_overlay) {
    m_ri->m_guard_zone[0]->SearchTargets();
  }
  if (m_pi->m_settings.guard_zone_on_overlay) {
      m_ri->m_guard_zone[1]->SearchTargets();
  }
  // check for duplicates
  bool dup = false;
  for (int i = 0; i < number_of_targets; i++) {
    if (!m_targets[i]) continue;
    if (m_targets[i]->status == LOST) {
      continue;
    }
    for (int j = i + 1; j < number_of_targets; j++) {
      if (!m_targets[j]) continue;
      if (m_targets[j]->status == LOST) {
        continue;
      }
      if (!m_targets[i]) { 
          break; }
      if (!m_targets[j]) { 
          break; }
      if (m_targets[i]->status == LOST) {
        break;
      }
      if (m_targets[j]->pol_z.angle == m_targets[i]->pol_z.angle && m_targets[j]->pol_z.r == m_targets[i]->pol_z.r) {
        // duplicate found
        dup = true;  // if at least one duplicate found
        // a mature target should not get a new duplicate
        // but two mature targets may go together for a few sweeps
        int dup_to_delete = j;  // target with the lowest status
        if (m_targets[j]->status > m_targets[i]->status) {
          dup_to_delete = i;
        }
        // delete the stationary target
        if (m_targets[j]->stationary && !m_targets[i]->stationary) {
            dup_to_delete = j;
        }
        if (m_targets[i]->stationary && !m_targets[j]->stationary){
            dup_to_delete = i;
        }
        if (m_targets[dup_to_delete]->status < 5) {
          // it's new, kill it immediately and get out
          m_targets[dup_to_delete]->SetStatusLost();
          continue;
        }
        if (m_targets[dup_to_delete]->duplicate_count == 0) {
            m_targets[dup_to_delete]->duplicate_count = m_targets[dup_to_delete]->status;
        }
        else if (m_targets[dup_to_delete]->duplicate_count + MAX_DUP <= m_targets[dup_to_delete]->status) {
            m_targets[dup_to_delete]->SetStatusLost();
        }
      }
    }
  }
  if (!dup) {
    for (int i = 0; i < number_of_targets; i++) {
        if (!m_targets[i]) continue;
      m_targets[i]->duplicate_count = 0;  // reset all duplicate counters
    }
  }
}

void ArpaTarget::RefreshTarget() {
  Position prev_X;
  Position prev2_X;
  Position own_pos;
  Polar pol;

  if (status == LOST) return;  // refresh may be called from guard directly, better check
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  pol = Pos2Polar(X, own_pos, m_ri->m_range_meters);

  wxLongLong time1 = m_ri->m_history[MOD_ROTATION2048(pol.angle)].time;
  wxLongLong time2 = m_ri->m_history[MOD_ROTATION2048(pol.angle + SCAN_MARGIN)].time;
  // check if target has been refreshed since last time
  // and if the beam has passed the target location with SCAN_MARGIN spokes
  // if (time1 > (t_refresh + 1000) && time2 >= time1) {  // the beam sould have passed our "angle" AND a point SCANMARGIN further
  // always refresh when status == 0
  if ((time1 > (t_refresh + SCAN_MARGIN2) && time2 >= time1) ||
      status == 0) {  // the beam sould have passed our "angle" AND a point SCANMARGIN further
    // set new refresh time
    t_refresh = time1;
    prev2_X = prev_X;
    prev_X = X;  // save the previous target position

    // PREDICTION CYCLE
    X.time = time1;                                                       // estimated new target time
    double delta_t = ((double)((X.time - prev_X.time).GetLo())) / 1000.;  // in seconds
    if (status == 0) {
      delta_t = 0.;
    } 

    LocalPosition x_local;
    x_local.lat = (X.lat - own_pos.lat) * 60. * 1852.;                              // in meters
    x_local.lon = (X.lon - own_pos.lon) * 60. * 1852. * cos(deg2rad(own_pos.lat));  // in meters
    x_local.dlat_dt = X.dlat_dt;                                                    // meters / sec
    x_local.dlon_dt = X.dlon_dt;                                                    // meters / sec
    m_kalman->Predict(&x_local, delta_t);  // x_local is new estimated local position of the target
    // now set the polar to expected angular position from the expected local position
    pol.angle = (int)(atan2(x_local.lon, x_local.lat) * LINES_PER_ROTATION / (2. * PI));
    pol.r = (int)(sqrt(x_local.lat * x_local.lat + x_local.lon * x_local.lon) * (double)RETURNS_PER_LINE /
                  (double)m_ri->m_range_meters);

    // zooming and target movement may  cause r to be out of bounds
    if (pol.r >= RETURNS_PER_LINE || pol.r <= 0) {
      SetStatusLost();
      return;
    }
    expected = pol;  // save expected polar position
    // now search for the target at the expected polar position in pol
    if (GetTarget(&pol)) {
      pol_z = pol;
      if (contour_length < MIN_CONTOUR_LENGTH && (status == AQUIRE0 || status == AQUIRE1)) {
        // target too small during aquisition
        SetStatusLost();
        return;
      }
      // target refreshed, measured position in pol
      // check if target has a new later time than previous target
      if (pol.time <= prev_X.time) {
        // found old target again, reset what we have done
        LOG_INFO(wxT("BR24radar_pi: Error Gettarget same time found"));
        X = prev_X;
        prev_X = prev2_X;
        return;
      }
      lost_count = 0;
      if (status == AQUIRE0) {
        // as this is the first measurement, move target to measured position
        Position p_own;
        p_own.lat = m_ri->m_history[MOD_ROTATION2048(pol.angle)].lat;  // get the position at receive time
        p_own.lon = m_ri->m_history[MOD_ROTATION2048(pol.angle)].lon;
        X = Polar2Pos(pol, p_own, m_ri->m_range_meters);  // using own ship location from the time of reception
        X.dlat_dt = 0.;
        X.dlon_dt = 0.;
        delta_t = 2.5;  // not relevant as speed is 0
      }
      status++;
      // target get an id when status  == STATUS_TO_OCPN
      if (status == STATUS_TO_OCPN) {
        target_id_count++;
        if (target_id_count >= 10000) target_id_count = 1;
        target_id = target_id_count;
      }

      m_kalman->SetMeasurement(&pol, &x_local, &expected, m_ri->m_range_meters);  // pol is measured position in polar coordinates
      // x_local expected position in local coordinates
      // expected  is expected position in polar coordinates
      X.time = pol.time;  // set the target time to the newly found time
    } else {
      // target not found
      if (status == AQUIRE0 || status == AQUIRE1) {
        SetStatusLost();
        return;
      } else {
        lost_count++;
        if (lost_count >= MAX_LOST_COUNT) {
          SetStatusLost();
          return;
        }
      }
      // target was not found but gets another chance
    }
    X.lat = own_pos.lat + x_local.lat / 60. / 1852.;
    X.lon = own_pos.lon + x_local.lon / 60. / 1852. / cos(deg2rad(own_pos.lat));
    X.dlat_dt = x_local.dlat_dt;  // meters / sec
    X.dlon_dt = x_local.dlon_dt;  // meters /sec
    X.sd_speed_kn = x_local.sd_speed_m_s * 3600. / 1852.;
    // set refresh time to the time of the spoke where the target was found
    t_refresh = X.time;
    if (status >= 2) {
        double s1 = X.dlat_dt;                                 // m per second
        double s2 = X.dlon_dt;                                 // m  per second
        speed_kn = (sqrt(s1 * s1 + s2 * s2)) * 3600. / 1852.;  // and convert to nautical miles per hour
        course = rad2deg(atan2(s2, s1));
        if (course < 0) course += 360.;
    //    LOG_INFO(wxT("BR24radar_pi: $$$ speed_kn= %f, X.sd_speed_kn= %f, target_id %i stationary= %i"),speed_kn,X.sd_speed_kn,target_id, stationary);
        if (speed_kn < (double)TARGET_SPEED_DIV_SDEV * X.sd_speed_kn) {
            speed_kn = 0.;
            course = 0.;
            stationary++;
            if (stationary > 2) stationary = 2;
        }
        else {
            stationary--;
            if (stationary < 0) stationary = 0;
        }
      // send target data to OCPN
      pol = Pos2Polar(X, own_pos, m_ri->m_range_meters);
      if (status >= STATUS_TO_OCPN) {
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
  if (MultiPix(aa, rr)) {      \
    pol->angle = aa;      \
    pol->r = rr;          \
    return true;          \
  }

bool ArpaTarget::FindNearestContour(Polar* pol, int dist) {
  // make a search pattern along a square
  // returns the position of the nearest blob found in pol
  int a = pol->angle;
  int r = pol->r;
  if (dist < 2) dist = 2;
  for (int j = 0; j <= dist; j++) {
    int dist_r = j;
    int dist_a = (int)(326. / (double)r * j );   // 326/r: conversion factor to make squares
    for (int i = 0; i <  dist_a; i++) {  // "upper" side
        PIX(a - i, r + dist_r);
        PIX(a + i, r + dist_r);
    }
    for (int i = 0; i < dist_r; i++) {  // "right hand" side
        PIX(a + dist_a, r + i);
        PIX(a + dist_a, r - i);
    }
    for (int i = 0; i <dist_a; i++) {  // "lower" side
        PIX( a + i, r - dist_r);
        PIX( a - i, r - dist_r);
    }
    for (int i = 0; i < dist_r; i++) {  // "left hand" side
        PIX(a - dist_a, r + i);
        PIX(a - dist_a, r - i);
    }
  }
  return false;
}

void RadarArpa::CalculateCentroid(ArpaTarget* target) {
  // real calculation still to be done
}

ArpaTarget::ArpaTarget(br24radar_pi* pi, RadarInfo* ri) {
  ArpaTarget::m_ri = ri;
  m_pi = pi;
  m_kalman = 0;
  status = LOST;
  contour_length = 0;
  lost_count = 0;
  duplicate_count = 0;
  target_id = 0;
  t_refresh = 0;
  arpa = false;
  speed_kn = 0.;
  course = 0.;
  stationary = 0;   
}

ArpaTarget::ArpaTarget() {
  m_kalman = 0;
  status = LOST;
  contour_length = 0;
  lost_count = 0;
  duplicate_count = 0;
  target_id = 0;
  t_refresh = 0;
  arpa = false;
  speed_kn = 0.;
  course = 0.;
  stationary = 0;
}

bool ArpaTarget::GetTarget(Polar* pol) {
  // general target refresh
    bool contour_found = false;
    int dist = OFF_LOCATION;
    if (status == AQUIRE0 || status == AQUIRE1) {
        dist = OFF_LOCATION * 2;
    }
    if (dist > pol->r - 5) dist = pol->r - 5;  // don't search close to origin

    int a = pol->angle;
    int r = pol->r;
    if (Pix(a, r)){
        contour_found = FindContourFromInside(pol);
    }
    else{
        contour_found = FindNearestContour(pol, dist);
    }
  if (!contour_found) {
    return false;
  }
  int cont = GetContour(pol);
  if (cont != 0){
      LOG_INFO(wxT("BR24radar_pi: GEt Contour ERROR id=%i return code %i"), target_id, cont);
      return false;
  }
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

  s_Bear_Unit = wxEmptyString;  // Bearing Units  R or empty
  s_Course_Unit = wxT("T");     // Course type R; Realtive T; true
  s_Dist_Unit = wxT("N");       // Speed/Distance Unit K, N, S N= NM/h = Knots
  if (status == Q) s_status = wxT("Q");
  if (status == T) s_status = wxT("T");
  if (status == L) { 
      s_status = wxT("L"); 
  }

  double dist = (double)pol->r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters / 1852.;
  double bearing = (double)pol->angle * 360. / (double)LINES_PER_ROTATION;

  if (bearing < 0) bearing += 360;
  s_TargID = wxString::Format(wxT("%4i"), target_id);
  s_speed = wxString::Format(wxT("%4.2f"), status == Q ? 0.0 : speed_kn);
  s_course = wxString::Format(wxT("%3.1f"), status == Q ? 0.0 : course);
  if (arpa == true){
      s_target_name = wxString::Format(wxT("ARPA%4i"), target_id);
  }
  else{
      s_target_name = wxString::Format(wxT("MARPA%4i"), target_id);
  }
  s_distance = wxString::Format(wxT("%f"), dist);
  s_bearing = wxString::Format(wxT("%f"), bearing);

  /* Code for TTM follows. Send speed and course using TTM*/
  int TTM = sprintf(sentence, "RATTM,%2s,%s,%s,%s,%s,%s,%s, , ,%s,%s,%s, ",
                    (const char*)s_TargID.mb_str(),       // 1 target id
                    (const char*)s_distance.mb_str(),     // 2 Targ distance
                    (const char*)s_bearing.mb_str(),      // 3 Bearing fr own ship.
                    (const char*)s_Bear_Unit.mb_str(),    // 4 Brearing unit ( T = true)
                    (const char*)s_speed.mb_str(),        // 5 Target speed
                    (const char*)s_course.mb_str(),       // 6 Target Course.
                    (const char*)s_Course_Unit.mb_str(),  // 7 Course ref T // 8 CPA Not used // 9 TCPA Not used
                    (const char*)s_Dist_Unit.mb_str(),    // 10 S/D Unit N = knots/Nm
                    (const char*)s_target_name.mb_str(),  // 11 Target name
                    (const char*)s_status.mb_str());      // 12 Target Status L/Q/T // 13 Ref N/A

  for (p = sentence; *p; p++) {
    checksum ^= *p;
  }
  nmea.Printf(wxT("$%s*%02X\r\n"), sentence, (unsigned)checksum);
//  LOG_INFO(wxT("BR24radar_pi: RadarArpa:: string send %s"), nmea);
  PushNMEABuffer(nmea);
}

void ArpaTarget::SetStatusLost() {
  contour_length = 0;
  lost_count = 0;
  duplicate_count = 0;
  if (m_kalman) {
      // reset kalman filter, don't delete it, too  expensive
      m_kalman->ResetFilter();
  }
  if (status >= STATUS_TO_OCPN) {
    Polar p;
    p.angle = 0;
    p.r = 0;
    PassARPAtoOCPN(&p, L);
  }
  duplicate_count = 0;
  status = LOST;
  target_id = 0;
  arpa = false;
  t_refresh = 0;
  speed_kn = 0.;
  course = 0.;
  stationary = 0;
}

void RadarArpa::DeleteAllTargets() {
  for (int i = 0; i < number_of_targets; i++) {
      if (!m_targets[i]) continue;
      m_targets[i]->SetStatusLost();
    
  }
}

void RadarArpa::AquireNewTarget(Polar pol, int status, int* target_i) {
  // aquires new target from mouse click position
  // no contour taken yet
  // target status status, normally 0, if dummy target to delete a target -2
  // returns in X metric coordinates of click
  // constructs Kalman filter
  Position own_pos;
  Position target_pos;
  own_pos.lat = m_pi->m_ownship_lat;
  own_pos.lon = m_pi->m_ownship_lon;
  target_pos = Polar2Pos(pol, own_pos, m_ri->m_range_meters);
  // make new target
  int i_target;
  if (number_of_targets < MAX_NUMBER_OF_TARGETS - 1 || (number_of_targets == MAX_NUMBER_OF_TARGETS - 1 && status == -2)) {
      if (m_targets[number_of_targets] == 0){
          m_targets[number_of_targets] = new ArpaTarget(m_pi, m_ri);
      }
    i_target = number_of_targets;
    number_of_targets++;
  } else {
    i_target = -1;
    LOG_INFO(wxT("BR24radar_pi: RadarArpa:: Error, max targets exceeded %i"), number_of_targets);
    *target_i = i_target;
    return;
  }
  m_targets[i_target]->X = target_pos;  // Expected position
  m_targets[i_target]->X.time = 0;
  m_targets[i_target]->X.dlat_dt = 0.;
  m_targets[i_target]->X.dlon_dt = 0.;
  m_targets[i_target]->status = status;
  if (!m_targets[i_target]->m_kalman) {
    m_targets[i_target]->m_kalman = new Kalman_Filter(m_ri->m_range_meters);
  }
  *target_i = i_target;
  return;
}

PLUGIN_END_NAMESPACE
