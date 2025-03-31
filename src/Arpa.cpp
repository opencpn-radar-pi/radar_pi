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

#include "Arpa.h"

#include "GuardZone.h"
#include "RadarCanvas.h"
#include "RadarInfo.h"
#include "drawutil.h"
#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

#define LAST_PASS 2  // adapt this when more passes added

static bool SortTargetStatus(const std::unique_ptr<ArpaTarget>& one, const std::unique_ptr<ArpaTarget>& two) {
  return one->m_status > two->m_status;
}

ArpaTarget::ArpaTarget(radar_pi* pi, RadarInfo* ri, int uid) : m_kalman(KalmanFilter(ri->m_spokes)) {
  // makes new target with an existing id
  m_ri = ri;
  m_pi = pi;
  m_status = LOST;
  m_contour_length = 0;
  m_average_contour_length = 0;
  m_small_fast = false;
  m_previous_contour_length = 0;
  m_lost_count = 0;
  m_refresh_time = 0;
  m_automatic = false;
  m_position.speed_kn = 0.;
  m_course = 0.;
  m_stationary = 0;
  m_position.dlat_dt = 0.;
  m_position.dlon_dt = 0.;
  m_position.speed_kn = 0.;
  m_position.sd_speed_kn = 0.;
  m_doppler_target = ANY;
  m_refreshed = NOT_FOUND;
  m_target_id = uid;
  m_transferred_target = false;
  LOG_ARPA(wxT("%s: number of targets=%i, new target id=%i"), m_ri->m_name, m_ri->m_arpa->GetTargetCount(), m_target_id);
}

ArpaTarget::~ArpaTarget() {}

ExtendedPosition ArpaTarget::Polar2Pos(Polar pol, ExtendedPosition own_ship) {
  // The "own_ship" in the function call can be the position at an earlier time than the current position
  // converts in a radar image angular data r ( 0 - max_spoke_len ) and angle (0 - max_spokes) to position (lat, lon)
  // based on the own ship position own_ship
  ExtendedPosition pos;
  // should be revised, use Mercator formula PositionBearingDistanceMercator()  TODO
  pos.pos.lat = own_ship.pos.lat + ((double)pol.r / m_ri->m_pixels_per_meter)  // Scale to fraction of distance from radar
                                       * cos(deg2rad(SCALE_SPOKES_TO_DEGREES(pol.angle))) / 60. / 1852.;
  pos.pos.lon = own_ship.pos.lon + ((double)pol.r / m_ri->m_pixels_per_meter)  // Scale to fraction of distance to radar
                                       * sin(deg2rad(SCALE_SPOKES_TO_DEGREES(pol.angle))) / cos(deg2rad(own_ship.pos.lat)) / 60. /
                                       1852.;
  return pos;
}

Polar ArpaTarget::Pos2Polar(ExtendedPosition p, ExtendedPosition own_ship) {
  // converts in a radar image a lat-lon position to angular data relative to position own_ship
  Polar pol;
  double dif_lat = p.pos.lat;
  dif_lat -= own_ship.pos.lat;
  double dif_lon = (p.pos.lon - own_ship.pos.lon) * cos(deg2rad(own_ship.pos.lat));
  pol.r = (int)(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * m_ri->m_pixels_per_meter + 1);
  pol.angle = (int)((atan2(dif_lon, dif_lat)) * (double)m_ri->m_spokes / (2. * PI) + 1);  // + 1 to minimize rounding errors
  if (pol.angle < 0) pol.angle += m_ri->m_spokes;
  return pol;
}

bool Arpa::Pix(int ang, int rad, Doppler doppler) {
  if (rad <= 0 || rad >= (int)m_ri->m_spoke_len_max) {
    return false;
  }
  int angle1 = MOD_SPOKES(ang);
  if (angle1 < 0) return false;
  size_t angle = angle1;
  if (angle >= m_ri->m_spokes || angle < 0) {
    return false;
  }
  bool bit0 = (m_ri->m_history[angle].line[rad] & 0x80) != 0;  // above threshold bit
  bool bit1 = (m_ri->m_history[angle].line[rad] & 0x40) != 0;  // backup bit does not get cleared when target is refreshed
  bool bit2 = (m_ri->m_history[angle].line[rad] & 0x20) != 0;  // this is Doppler approaching bit
  bool bit3 = (m_ri->m_history[angle].line[rad] & 0x10) != 0;  // this is Doppler receding bit
  switch (doppler) {
    case ANY:
      return bit0;
    case NO_DOPPLER:
      return (bit0 && !bit2 && !bit3);
    case APPROACHING:
      return bit2;
    case RECEDING:
      return bit3;
    case ANY_DOPPLER:
      return (bit2 || bit3);
    case NOT_RECEDING:
      return (bit0 && !bit3);
    case NOT_APPROACHING:
      return (bit0 && !bit2);
    case ANY_PLUS:
      return (bit1);
  }
  LOG_INFO(wxT("Error no Doppler value"));
  return false;
}

bool ArpaTarget::Pix(int ang, int rad) {
  if (rad <= 0 || rad >= (int)m_ri->m_spoke_len_max) {
    return false;
  }
  int angle1 = MOD_SPOKES(ang);
  if (angle1 < 0) return false;
  size_t angle = angle1;
  if (angle >= m_ri->m_spokes || angle < 0) {
    return false;
  }
  bool bit0 = (m_ri->m_history[angle].line[rad] & 0x80) != 0;  // above threshold bit
  bool bit1 = (m_ri->m_history[angle].line[rad] & 0x40) != 0;  // backup bit does not get cleared when target is refreshed
  bool bit2 = (m_ri->m_history[angle].line[rad] & 0x20) != 0;  // this is Doppler approaching bit
  bool bit3 = (m_ri->m_history[angle].line[rad] & 0x10) != 0;  // this is Doppler receding bit

  switch (m_doppler_target) {
    case ANY:
      return bit0;
    case NO_DOPPLER:
      return (bit0 && !bit2 && !bit3);
    case APPROACHING:
      return bit2;
    case RECEDING:
      return bit3;
    case ANY_DOPPLER:
      return (bit2 || bit3);
    case NOT_RECEDING:
      return (bit0 && !bit3);
    case NOT_APPROACHING:
      return (bit0 && !bit2);
    case ANY_PLUS:
      return (bit1);
  }
  return (bit0);  // should not happen
}

bool ArpaTarget::MultiPix(int ang, int rad) {  // checks if the blob has a contour of at least length pixels
  // pol must start on the contour of the blob
  // false if not
  // if false clears out pixels of the blob in hist
  wxCriticalSectionLocker lock(m_ri->m_exclusive);
  int length = m_ri->m_min_contour_length;
  Polar start;
  start.angle = ang;
  start.r = rad;
  if (!Pix(start.angle, start.r)) {
    return false;
  }
  Polar current = start;  // the 4 possible translations to move from a point on the contour to the next
  Polar max_angle;
  Polar min_angle;
  Polar max_r;
  Polar min_r;
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
  int aa;
  int rr;
  bool succes = false;
  int index = 0;
  max_r = current;
  max_angle = current;
  min_r = current;
  min_angle = current;  // check if p inside blob
  if (start.r >= (int)m_ri->m_spoke_len_max) {
    return false;  //  r too large
  }
  if (start.r < 3) {
    return false;  //  r too small
  }
  // first find the orientation of border point p
  for (int i = 0; i < 4; i++) {
    index = i;
    aa = current.angle + transl[index].angle;
    rr = current.r + transl[index].r;
    succes = !Pix(aa, rr);
    if (succes) break;
  }
  if (!succes) {
    // single pixel blob
    return false;
  }
  index += 1;  // determines starting direction
  if (index > 3) index -= 4;
  while (current.r != start.r || current.angle != start.angle || count == 0) {  // try all translations to find the next point
    // start with the "left most" translation relative to the
    // previous one
    index += 3;  // we will turn left all the time if possible
    for (int i = 0; i < 4; i++) {
      if (index > 3) index -= 4;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
      succes = Pix(aa, rr);
      if (succes) {  // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      return false;  // no next point found (this happens when the blob consists of one single pixel)
    }                // next point found
    current.angle = aa;
    current.r = rr;
    if (count >= length) {
      return true;
    }
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
  }  // contour length is less than m_min_contour_length
     // before returning false erase this blob so we do not have to check this one again
  if (min_angle.angle < 0) {
    min_angle.angle += m_ri->m_spokes;
    max_angle.angle += m_ri->m_spokes;
  }
  for (int a = min_angle.angle; a <= max_angle.angle; a++) {
    for (int r = min_r.r; r <= max_r.r; r++) {
      m_ri->m_history[MOD_SPOKES(a)].line[r] &= 63;
    }
  }
  return false;
}

bool ArpaTarget::FindContourFromInside(Polar* pol) {  // moves pol to contour of blob
  // true if success
  // false when failed
  int ang = pol->angle;
  int rad = pol->r;
  int limit = m_ri->m_spokes;

  if (rad >= (int)m_ri->m_spoke_len_max || rad < 3) {
    return false;
  }
  if (!(Pix(ang, rad))) {
    return false;
  }
  while (limit >= 0 && Pix(ang, rad)) {
    ang--;
    limit--;
  }
  ang++;
  pol->angle = ang;
  // check if the blob has the required min contour length
  if (MultiPix(ang, rad)) {
    return true;
  } else {
    return false;
  }
}

/**
 * Find a contour from the given start position on the edge of a blob.
 *
 * Follows the contour in a clockwise manner.
 *
 * Returns 0 if ok, or a small integer on error (but nothing is done with this)
 */
int ArpaTarget::GetContour(Polar* pol) {
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
  m_max_r = current;
  m_max_angle = current;
  m_min_r = current;
  m_min_angle = current;
  // check if p inside blob
  if (start.r >= (int)m_ri->m_spoke_len_max) {
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
      LOG_INFO(wxT("radar_pi::Arpa::GetContour no next point found count= %i"), count);
      return 7;  // return code 7, no next point found
    }
    // next point found
    current.angle = aa;
    current.r = rr;
    if (count < MAX_CONTOUR_LENGTH - 2) {
      m_contour[count] = current;
    }
    if (count == MAX_CONTOUR_LENGTH - 2) {
      m_contour[count] = start;  // shortcut to the beginning for drawing the contour
      current = start;           // this will cause the while to terminate
    }
    if (count < MAX_CONTOUR_LENGTH - 1) {
      count++;
    }
    if (current.angle > m_max_angle.angle) {
      m_max_angle = current;
    }
    if (current.angle < m_min_angle.angle) {
      m_min_angle = current;
    }
    if (current.r > m_max_r.r) {
      m_max_r = current;
    }
    if (current.r < m_min_r.r) {
      m_min_r = current;
    }
  }
  m_contour_length = count;
  //  CalculateCentroid(*target);    we better use the real centroid instead of the average, TODO
  if (m_min_angle.angle < 0) {
    m_min_angle.angle += m_ri->m_spokes;
    m_max_angle.angle += m_ri->m_spokes;  // attention, could be > m_spokes
  }
  pol->angle = (m_max_angle.angle + m_min_angle.angle) / 2;
  if (m_max_r.r >= (int)m_ri->m_spoke_len_max || m_min_r.r >= (int)m_ri->m_spoke_len_max) {
    return 10;  // return code 10 r too large
  }
  if (m_max_r.r < 2 || m_min_r.r < 2) {
    return 11;  // return code 11 r too small
  }
  if (pol->angle >= (int)m_ri->m_spokes) {
    pol->angle -= m_ri->m_spokes;
  }
  m_polar_pos.angle = pol->angle;
  pol->r = (m_max_r.r + m_min_r.r) / 2;
  m_polar_pos.r = pol->r;
  pol->time = m_ri->m_history[MOD_SPOKES(pol->angle)].time;
  m_radar_pos = m_ri->m_history[MOD_SPOKES(pol->angle)].pos;

  double poslat = m_radar_pos.lat;
  double poslon = m_radar_pos.lon;
  if (poslat > 90. || poslat < -90. || poslon > 180. || poslon < -180.) {
    // some additional logging, to be removed later
    LOG_INFO(wxT("**error wrong target pos, poslat = %f, poslon = %f"), poslat, poslon);
  }
  return 0;  //  success, blob found
}


void ArpaTarget::RefreshTarget(int dist, int pass) {
  if (!m_ri || !m_ri->m_arpa) {
    return;
  }
  ExtendedPosition prev_X;
  // ExtendedPosition prev2_X;
  ExtendedPosition own_pos;
  Polar pol;
  double delta_t;
  LocalPosition x_local;
  wxLongLong prev_refresh = m_refresh_time;  
  // refresh may be called from guard directly, better check
  if (m_status == LOST || !m_ri->GetRadarPosition(&own_pos.pos)) {
    m_refreshed = OUT_OF_SCOPE;
    LOG_ARPA(wxT("%s: RefreshTarget out of scope id=%i pos(%f,%f) status=%i"), m_ri->m_name, m_target_id, m_position.pos.lat,
             m_position.pos.lon, m_status);
    return;
  }

  if (m_refreshed == FOUND || m_refreshed == OUT_OF_SCOPE) {
    LOG_ARPA(wxT("%s: RefreshTarget return id=%i pos(%f,%f) status=%i"), m_ri->m_name, m_target_id, m_position.pos.lat,
             m_position.pos.lon, m_status);
    return;
  }

  pol = Pos2Polar(m_position, own_pos);
  LOG_ARPA(wxT("%s: RefreshTarget m_target_id=%i, angle=%i, r= %i, contour=%i, pass=%i, doppler=%i"), m_ri->m_name,
           m_target_id, pol.angle, pol.r, m_contour_length, pass, m_doppler_target);
  int alfa0 = pol.angle;
  int r0 = pol.r;
  wxLongLong angle_time = m_ri->m_history[MOD_SPOKES(pol.angle + SCAN_MARGIN)].time;
  // angle_time is the time of a spoke SCAN_MARGIN spokes forward of the target, if that spoke is refreshed we assume that the target has been refreshed
 

  /*if (m_transferred_target) {
    OCPN_target_status s = Q;
    LOG_INFO(wxT(" target to TTM targetid=%i"), m_target_id);
    PassTTMtoOCPN(&pol, s);    
  }*/

  wxLongLong now = wxGetUTCTimeMillis();  // millis
  int rotation_period = m_ri->m_rotation_period.GetValue();
  if (rotation_period == 0) {
    rotation_period = 2400; // default value
  }
  if (angle_time < m_refresh_time + rotation_period - 100) {  // the 100 is a margin on the rotation period
    // the next image of the target is not yet there
    m_refreshed = OUT_OF_SCOPE;
    return;
  }

  // set new refresh time
  m_refresh_time = m_ri->m_history[MOD_SPOKES(pol.angle)].time;
  prev_X = m_position;  // save the previous target position
 
  // PREDICTION CYCLE

  LOG_ARPA(wxT("%s: Begin prediction cycle m_target_id=%i, status=%i, angle=%i, r= %i, contour=%i, pass=%i, lat=%f, lon=%f"),
           m_ri->m_name, m_target_id, m_status, pol.angle, pol.r, m_contour_length, pass, m_position.pos.lat, m_position.pos.lon);

  // estimated new target time
  if (m_refresh_time >= prev_X.time) {
    delta_t = ((double)((m_refresh_time - prev_X.time).GetLo())) / 1000.;  // in seconds
  } else {
    delta_t = 0;
  }
  if (m_status == 0) {
    delta_t = 0.;
  }

  LOG_ARPA(wxT("%s: target begin prediction cycle m_target_id=%i, delta time=%f, m_refresh_time=%u, prev_X.time=%u, now=%u"),
           m_ri->m_name, m_target_id, delta_t, m_refresh_time.GetLo(), prev_X.time.GetLo(), now.GetLo());

  if (m_position.pos.lat > 90. || m_position.pos.lat < -90.) {
    SetStatusLost();
    m_refreshed = OUT_OF_SCOPE;
    LOG_ARPA(wxT("%s: 1 target out of scope id=%u"), m_ri->m_name, m_target_id);
    return;
  }
  x_local.pos.lat = (m_position.pos.lat - own_pos.pos.lat) * 60. * 1852.;                                  // in meters
  x_local.pos.lon = (m_position.pos.lon - own_pos.pos.lon) * 60. * 1852. * cos(deg2rad(own_pos.pos.lat));  // in meters
  x_local.dlat_dt = m_position.dlat_dt;                                                                    // meters / sec
  x_local.dlon_dt = m_position.dlon_dt;                                                                    // meters / sec

  m_kalman.Predict(&x_local, delta_t);  // x_local is new estimated local position of the target
                                        // now set the polar to expected angular position from the expected local position

  pol.angle = (int)(atan2(x_local.pos.lon, x_local.pos.lat) * m_ri->m_spokes / (2. * PI));
  if (pol.angle < 0) pol.angle += m_ri->m_spokes;
  pol.r = (int)(sqrt(x_local.pos.lat * x_local.pos.lat + x_local.pos.lon * x_local.pos.lon) * m_ri->m_pixels_per_meter);
  // zooming and target movement may  cause r to be out of bounds
  LOG_ARPA(wxT("%s: PREDICTION m_target_id=%i, pass=%i, status=%i, angle=%i->%i, r= %i->%i, contour=%i, speed=%f, sd_speed_kn=%f, "
               "Doppler=%i, lostcount=%i"),
           m_ri->m_name, m_target_id, pass, m_status, alfa0, pol.angle, r0, pol.r, m_contour_length, m_position.speed_kn,
           m_position.sd_speed_kn, m_doppler_target, m_lost_count);
  if (pol.r >= (int)m_ri->m_spoke_len_max || pol.r <= 0) {
    m_refreshed = OUT_OF_SCOPE;
    // delete target if too far out
    LOG_ARPA(wxT("%s: R too big target deleted m_target_id=%i, angle=%i, r= %i, contour=%i, pass=%i"), m_ri->m_name, m_target_id,
             pol.angle, pol.r, m_contour_length, pass);
    SetStatusLost();
    return;
  }
  m_expected = pol;  // save expected polar position

  // MEASUREMENT CYCLE
  // now search for the target at the expected polar position in pol
  int dist1 = dist;

  if (pass == LAST_PASS) {   // this is doubtfull $$$
    if (m_status <= 2) {
      dist1 *= 2;
    }  
    else if (m_position.speed_kn > 15.) {
      dist1 *= 2;
    } /*else if (m_position.speed_kn > 30.) {
      dist1 *= 4;
    } */
  }

  Polar back = pol;

  // here we really search for the target
  bool found = false;
  LOG_ARPA(wxT("%s: MEASUREMENT m_target_id=%i, pass=%i, status=%i, angle=%i->%i, r= %i->%i, contour=%i, average contour=%i, speed=%f, sd_speed_kn=%f, "
               "Doppler=%i, lostcount=%i"),
           m_ri->m_name, m_target_id, pass, m_status, alfa0, pol.angle, r0, pol.r, m_contour_length,
           m_average_contour_length, m_position.speed_kn,
           m_position.sd_speed_kn, m_doppler_target, m_lost_count);
  LOG_ARPA(wxT("scantime=%u, refreshtime=%u"), m_ri->m_history[MOD_SPOKES(pol.angle + SCAN_MARGIN)].time.GetLo(),
           m_refresh_time.GetLo());
  if (pass == LAST_PASS) {
    m_doppler_target = ANY;  // in the last pass we are not critical
    }
  found = GetTarget(&pol, dist1);  // main target search


  int dist_angle = (int)((pol.angle - back.angle) * ((double)pol.r / 326.));
  int dist_radial = pol.r - back.r;
  int dist_total = (int)sqrt(double(dist_angle * dist_angle + dist_radial * dist_radial));
  if (found){
    LOG_ARPA(wxT("%s: id=%i, Found dist_angle=%i, dist_radial= %i, dist_total= %i, m_target_id=%i, pol.angle=%i, "
                 "back.angle=%i, doppler=%i"), m_ri->m_name, m_target_id,
             dist_angle, dist_radial, dist_total, m_target_id, pol.angle, back.angle, m_doppler_target);

 
    if (m_doppler_target != ANY) {
      Doppler backup = m_doppler_target;
      m_doppler_target = ANY;
      GetTarget(&pol, dist1);  // get the contour for the target ins ANY state
      PixelCounter();
      m_doppler_target = backup;
      GetTarget(&pol, dist1);  // restore target in original state
      StateTransition(&pol);   // adapt state if required
    } else {
      PixelCounter();
      StateTransition(&pol);
    }
    if (m_average_contour_length != 0 &&
        (m_contour_length < m_average_contour_length / 2 || m_contour_length > m_average_contour_length * 2)) {
      if (pass <= LAST_PASS - 1) {
        // Don't accept this hit
        // Search again in next pass
        LOG_ARPA(wxT(" %s, id=%i, reject weightedcontourlength=%i, m_contour_length=%i"), m_ri->m_name, m_target_id,
                 m_average_contour_length, m_contour_length);
        found = false;
      } else {
        LOG_ARPA(wxT(" %s, id=%i, accept weightedcontourlength=%i, m_contour_length=%i"), m_ri->m_name, m_target_id,
                 m_average_contour_length, m_contour_length);
      }
    }
  }
  if (found) {
    ResetPixels();
    LOG_ARPA(wxT(" target Found ResetPixels m_target_id=%i, angle=%i, r= %i, contour=%i, radar=%s, pass=%i, doppler=%i"),
             m_target_id, pol.angle, pol.r, m_contour_length, m_ri->m_name.c_str(), pass, m_doppler_target);
    int max = MAX_CONTOUR_LENGTH_USED;
    // if (m_contour_length >= max - 1) //LOG_ARPA(wxT(" large contour=%i"), m_contour_length);
    //  target too large? (land masses?) get rid of it
    if (m_contour_length >= max - 1) {
      // don't use this blob, could be radar interference
      // The pixels of the blob have been reset, so you won't find it again
      found = false;
    }
    LOG_ARPA(wxT("%s: target Found m_target_id=%i, angle=%i, r= %i, contour=%i, pass=%i"), m_ri->m_name, m_target_id, pol.angle,
             pol.r, m_contour_length, pass);
  }

  if (found) {
    m_lost_count = 0;
    ExtendedPosition p_own;
    p_own.pos = m_ri->m_history[MOD_SPOKES(pol.angle)].pos;
    LOG_ARPA(wxT("%s: p_own lat= %f, lon= %f, m_target_id=%i,"), m_ri->m_name, p_own.pos.lat, p_own.pos.lon, m_target_id);
    if (m_status == ACQUIRE0) {
      // as this is the first measurement, move target to measured position
      // ExtendedPosition p_own;
      // p_own.pos = m_ri->m_history[MOD_SPOKES(pol.angle)].pos;  // get the position at receive time
      m_position = Polar2Pos(pol, p_own);  // using own ship location from the time of reception, only lat and lon
      m_position.dlat_dt = 0.;
      m_position.dlon_dt = 0.;
      m_expected = pol;
      m_position.sd_speed_kn = 0.;
      LOG_ARPA(wxT("%s: calculated pos(%f,%f), m_target_id=%i,"), m_ri->m_name, m_position.pos.lat, m_position.pos.lon,
               m_target_id);
    }
    m_status++;

    // Kalman filter to  calculate the apostriori local position and speed based on found position (pol)
    if (m_status > 1) {
      m_kalman.Update_P();
      m_kalman.SetMeasurement(&pol, &x_local, &m_expected,
                              m_ri->m_pixels_per_meter);  // pol is measured position in polar coordinates
    }
    // x_local expected position in local coordinates

    m_position.time = pol.time;  // set the target time to the newly found time, this is the time the spoke was received

    if (m_status != ACQUIRE1) {
      // if status == 1, then this was first measurement, keep position at measured position
      m_position.pos.lat = own_pos.pos.lat + x_local.pos.lat / 60. / 1852.;
      m_position.pos.lon = own_pos.pos.lon + x_local.pos.lon / 60. / 1852. / cos(deg2rad(own_pos.pos.lat));
      m_position.dlat_dt = x_local.dlat_dt;  // meters / sec
      m_position.dlon_dt = x_local.dlon_dt;  // meters /sec
      m_position.sd_speed_kn = x_local.sd_speed_m_s * 3600. / 1852.;
    }

#define FORCED_POSITION_STATUS 8
    // Here we bypass the Kalman filter to predict the speed of the target
    // Kalman filter is too slow to adjust to the speed of (fast) new targets
    // This method however only works for targets where the accuricy of the position is high,
    // that is small targets in relation to the size of the target.

    if (m_status == 2) {  // determine if this is a small and fast target
      int dist_angle = pol.angle - alfa0;
      int dist_r = pol.r - r0;
      int size_angle = MOD_SPOKES (m_max_angle.angle - m_min_angle.angle);
      int size_r = m_max_r.r - m_min_r.r;
      if (size_r == 0) dist_r = 1;
      if (size_angle == 0) size_angle = 1;
      double test = abs((double)dist_r / (double)size_r) + abs((double)dist_angle / (double)size_angle);
      LOG_ARPA(wxT("%s smallandfast, id=%i, status=%i, test=%f, dist_r=%i, size_r=%i, dist_angle=%i, size_angle=%i"), m_ri->m_name,
               m_target_id, m_status, test, dist_r, size_r, dist_angle, size_angle);
      if (test > 2.) {
        m_small_fast = true;
      }
    }
    if (m_status >= 2 && m_status < FORCED_POSITION_STATUS && (m_status < 5 || m_position.speed_kn > 10.) && m_small_fast) {
      GeoPosition prev_pos = prev_X.pos;
      GeoPosition new_pos = Polar2Pos(pol, p_own).pos;
      double delta_lat = new_pos.lat - prev_pos.lat;
      double delta_lon = new_pos.lon - prev_pos.lon;
      int delta_t = (pol.time - prev_X.time).GetLo();
      if (delta_t > 1000) {  // delta_t < 1000; speed unreliable due to uncertainties in location
        double d_lat_dt = (delta_lat / (double)(delta_t)) * 60. * 1852. * 1000;
        double d_lon_dt = (delta_lon / (double)(delta_t)) * cos(deg2rad(new_pos.lat)) * 60. * 1852. * 1000;
        LOG_ARPA(wxT("%s, id=%i, FORCED m_status=%i, d_lat_dt=%f, d_lon_dt=%f, delta_lon_meter=%f, delta_lat_meter=%f, deltat=%u"), m_ri->m_name, m_target_id, m_status, d_lat_dt, d_lon_dt,
                 delta_lon * 60 * 1852., delta_lat * 60 * 1852., delta_t);
        // force new position and speed, dependent of overridefactor

        double factor = .8;
        factor = pow((factor), m_status - 1);
        m_position.pos.lat = m_position.pos.lat + factor * (new_pos.lat - m_position.pos.lat);
        m_position.pos.lon = m_position.pos.lon + factor * (new_pos.lon - m_position.pos.lon);
        m_position.dlat_dt = m_position.dlat_dt + factor * (d_lat_dt - m_position.dlat_dt);  // in meters/sec
        m_position.dlon_dt = m_position.dlon_dt + factor * (d_lon_dt - m_position.dlon_dt);  // in meters/sec
      }
    }

    // set refresh time to the time of the spoke where the target was found
    m_refresh_time = m_position.time;
    if (m_status >= 1) {
      double s1 = m_position.dlat_dt;                                   // m per second
      double s2 = m_position.dlon_dt;                                   // m  per second
      m_position.speed_kn = (sqrt(s1 * s1 + s2 * s2)) * 3600. / 1852.;  // and convert to nautical miles per hour
      m_course = rad2deg(atan2(s2, s1));

      LOG_ARPA(
          wxT("%s: FOUND %i CYCLE id=%i, status=%i, angle=%i->%i, r= %i->%i, contour=%i, speed=%f, sd_speed_kn=%f, doppler=%i"),
          m_ri->m_name, pass, m_target_id, m_status, alfa0, pol.angle, r0, pol.r, m_contour_length, m_position.speed_kn,
          m_position.sd_speed_kn, m_doppler_target);
      if (m_course < 0.) m_course += 360.;

      m_previous_contour_length = m_contour_length;
      // send target data to OCPN and other radar
      if (m_target_id == 0) {
        m_target_id = m_pi->MakeNewTargetId();
      }

      if (m_status > FORCED_POSITION_STATUS) {
        pol = Pos2Polar(m_position, own_pos);
      }
      if (m_status > 4) {  // 4? quite arbitrary, target should be stable
        TransferTargetToOtherRadar();
        SendTargetToNearbyRadar();
      }
#define WEIGHT_FACTOR 0.1
      if (m_contour_length != 0) {
        if (m_average_contour_length == 0 && m_contour_length != 0) {
          m_average_contour_length = m_contour_length;
        } else {
          m_average_contour_length =
              m_average_contour_length + (int)(WEIGHT_FACTOR * (m_contour_length - m_average_contour_length));
        }
      }

      if (m_status >= STATUS_TO_OCPN) {
        //  double dist2target = pol.r / m_ri->m_pixels_per_meter;
        PassAIVDMtoOCPN(&pol);  // status s not yet used

        //MakeAndTransmitTargetMessage();

        // MakeAndTransmitCoT();
      }
    }
    m_refreshed = FOUND;
    // A target that has been found is no longer considered a transferred target
    m_transferred_target = false;
    return;
  }

  else {  // target not found
    LOG_ARPA(wxT("%s: Not found m_target_id=%i, angle=%i, r= %i, pass=%i, lost_count=%i, status=%i"), m_ri->m_name, m_target_id,
             pol.angle, pol.r, pass, m_lost_count, m_status);
    // not found in pass 0 or 1 (An other chance will follow)
    // try again later in next pass with a larger distance
    if (pass < LAST_PASS) {
      // LOG_ARPA(wxT(" NOT FOUND IN PASS=%i"), pass);
      // reset what we have done
      pol.time = prev_X.time;
      m_refresh_time = prev_refresh;
      m_position = prev_X;
    }
    if (m_small_fast && pass == LAST_PASS - 1 && m_status == 2) {   // status 2, as it was not found,status was not increased.
    // small and fast targets MUST be found in the third sweep, and on a small distance, that is in pass 1.
      LOG_ARPA(wxT(" %s smallandfast set lost id=%i"), m_ri->m_name, m_target_id);
      SetStatusLost();
      m_refreshed = OUT_OF_SCOPE;
      return;
    }

    // delete low status targets immediately when not found
    if ((m_status <= 3 && pass == LAST_PASS) || m_status == 0) {
      LOG_ARPA(wxT("%s: low status deleted m_target_id=%i, angle=%i, r= %i, pass=%i, lost_count=%i"), m_ri->m_name, m_target_id,
               pol.angle, pol.r, pass, m_lost_count);
      SetStatusLost();
      m_refreshed = OUT_OF_SCOPE;
      return;
    }
    if (pass == LAST_PASS) {
      m_lost_count++;
    }

    // delete if not found too often
    if (m_lost_count > MAX_LOST_COUNT) {
      SetStatusLost();
      m_refreshed = OUT_OF_SCOPE;
      return;
    }
    m_refreshed = NOT_FOUND;
    // Send RATTM message also for not seen messages
    if (pass == LAST_PASS && m_status > STATUS_TO_OCPN) {
      pol = Pos2Polar(m_position, own_pos);
      if (m_status >= STATUS_TO_OCPN) {
        //   double dist2target = pol.r / m_ri->m_pixels_per_meter;
        LOG_ARPA(wxT(" pass not found as AIVDM targetid=%i"), m_target_id);
        if (m_transferred_target) {
        //  LOG_ARPA(wxT(" passTTM targetid=%i"), m_target_id);
        //  double s1 = m_position.dlat_dt;                                   // m per second
        //  double s2 = m_position.dlon_dt;                                   // m  per second
        //  m_course = rad2deg(atan2(s2, s1));
        //  PassTTMtoOCPN(&pol, s);
          
          PassAIVDMtoOCPN(&pol);
        }
        // MakeAndTransmitTargetMessage();
        // MakeAndTransmitCoT();
      }
    }
    m_transferred_target = false;
    return;
  }  // end of target not found
}

void ArpaTarget::PixelCounter() {
  LOG_ARPA(wxT("PixelCounter called, m_contour_length=%i"), m_contour_length);
  //  Counts total number of the various pixels in a blob
  m_total_pix = 0;
  m_approaching_pix = 0;
  m_receding_pix = 0;
  for (uint16_t i = 0; i < m_contour_length; i++) {
    bool bit0 = false;
   // bool bit1 = false;
    bool bit2 = false;
    bool bit3 = false;

    uint16_t radius = m_contour[i].r;
    uint8_t byte = 0;
    do {
      byte = m_ri->m_history[MOD_SPOKES(m_contour[i].angle)].line[radius];
      bit0 = (byte & 0x80) >> 7;  // above threshold bit
     // bit1 = (byte & 0x40) >> 6;  // backup bit does not get cleared when target is refreshed
      bit2 = (byte & 0x20) >> 5;  // this is Doppler approaching bit
      bit3 = (byte & 0x10) >> 4;  // this is Doppler receding bit
      m_total_pix += bit0;
      m_approaching_pix += bit2;
      m_receding_pix += bit3;
      radius++;
      if (radius >= 1023) continue;
    } while (m_ri->m_history[MOD_SPOKES(m_contour[i].angle)].line[radius] >> 7);
  }
  LOG_ARPA(wxT("PixelCounter m_total_pix=%i, m_approaching_pix=%i, m_receiding_pix=%i"), m_total_pix, m_approaching_pix,
           m_receding_pix);
}

// Check doppler state of targets if Doppler is on
void ArpaTarget::StateTransition(Polar* polar) {
  if (m_ri->m_doppler.GetValue() == 0 || m_doppler_target == ANY_PLUS) {
    LOG_ARPA(wxT("Check Doppler returned id=%i, doppler=%i"), m_target_id, m_doppler_target);
    return;
  }

  uint32_t check_to_doppler = (uint32_t)(0.85 * (double)m_total_pix);
  uint32_t check_to_any = (uint32_t)(0.80 * (double)m_total_pix);
  uint32_t check_not_approaching = (uint32_t)(0.80 * (double)(m_total_pix - m_approaching_pix));
  uint32_t check_not_receding = (uint32_t)(0.80 * (double)(m_total_pix - m_receding_pix));
  LOG_ARPA(wxT("Check Doppler id=%i, check_to_doppler=%i, check_to_any=%i, check_not_approaching=%i, check_not_receding=%i"),
           m_target_id, check_to_doppler, check_to_any, check_not_approaching, check_not_receding);
  switch (m_doppler_target) {
    case ANY_DOPPLER:
    case ANY:
      // convert to APPROACHING or RECEDING
      if (m_approaching_pix > m_receding_pix && m_approaching_pix > check_to_doppler) {
        m_doppler_target = APPROACHING;
        LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to APPROACHING"), m_target_id);
        return;
      } else if (m_receding_pix > m_approaching_pix && m_receding_pix > check_to_doppler) {
        m_doppler_target = RECEDING;
        LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to RECEDING"), m_target_id);
        return;
      } else if (m_doppler_target == ANY_DOPPLER) {
        m_doppler_target = ANY;
        LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to ANY"), m_target_id);
        return;
      }
      return;

    case RECEDING:
      if (m_receding_pix < check_not_approaching /*check_to_any*/) {
        m_doppler_target = ANY;
        LOG_ARPA(wxT("Doppler converted id=%i RECEIDING to ANY"), m_target_id);
        return;
      }
      return;

    case APPROACHING:
      if (m_approaching_pix < check_not_receding /*check_to_any*/) {
        m_doppler_target = ANY;
        LOG_ARPA(wxT("Doppler converted id=%i APPROACHING to ANY"), m_target_id);
        return;
      }
      return;
    default:
      return;
  }
}

void ArpaTarget::TransferTargetToOtherRadar() {
  RadarInfo* other_radar = 0;
  LOG_ARPA(wxT("%s: TransferTargetToOtherRadar m_target_id=%i,"), m_ri->m_name, m_target_id);
  if (M_SETTINGS.radar_count != 2) {
    return;
  }
  if (!m_pi->m_radar[0] || !m_pi->m_radar[1] || !m_pi->m_radar[0]->m_arpa || !m_pi->m_radar[1]->m_arpa) {
    return;
  }
  if (m_pi->m_radar[0]->m_state.GetValue() != RADAR_TRANSMIT || m_pi->m_radar[1]->m_state.GetValue() != RADAR_TRANSMIT) {
    return;
  }
  LOG_ARPA(wxT("%s: this  radar pix/m=%f"), m_ri->m_name, m_ri->m_pixels_per_meter);
  RadarInfo* long_range = m_pi->GetLongRangeRadar();
  RadarInfo* short_range = m_pi->GetShortRangeRadar();

  if (m_ri == long_range) {
    other_radar = short_range;
    int border = (int)(m_ri->m_spoke_len_max * m_ri->m_pixels_per_meter / short_range->m_pixels_per_meter);
    // m_ri has largest range, other_radar smaller range. Don't transfer targets that are outside range of smaller radar
    if (m_expected.r > border) {
      // don't send small range targets to smaller radar
      return;
    }
  } else {
    other_radar = long_range;
    // this (m_ri) is the small range radar
    // we will only send larger range targets to other radar
  }
  DynamicTargetData data;
  data.target_id = m_target_id;
  data.P = m_kalman.P;
  data.position = m_position;
  LOG_ARPA(wxT("%s: lat= %f, lon= %f, m_target_id=%i,"), m_ri->m_name, m_position.pos.lat, m_position.pos.lon, m_target_id);
  data.status = m_status;
  other_radar->m_arpa->InsertOrUpdateTargetFromOtherRadar(&data, false);
}

void ArpaTarget::SendTargetToNearbyRadar() {
  LOG_ARPA(wxT("%s: Send target to nearby radar, m_target_id=%i,"), m_ri->m_name, m_target_id);
  RadarInfo* long_radar = m_pi->GetLongRangeRadar();
  if (m_ri != long_radar) {
    return;
  }
  DynamicTargetData data;
  data.target_id = m_target_id;
  data.P = m_kalman.P;
  data.position = m_position;
  LOG_ARPA(wxT("%s: lat= %f, lon= %f, m_target_id=%i,"), m_ri->m_name, m_position.pos.lat, m_position.pos.lon, m_target_id);
  data.status = m_status;
}

#define PIX(aa, rr)                                   \
  if (rr >= (int)m_ri->m_spoke_len_max - 1) continue; \
  if (MultiPix(aa, rr)) {                             \
    pol->angle = aa;                                  \
    pol->r = rr;                                      \
    return true;                                      \
  }

bool ArpaTarget::FindNearestContour(Polar* pol, int dist) {
  // make a search pattern along a square
  // returns the position of the nearest blob found in pol
  // dist is search radius (1 more or less) in radial pixels
  int a = pol->angle;
  int r = pol->r;
  int distance = dist;
  if (distance < 2) distance = 2;
  for (int j = 1; j <= distance; j++) {
    int dist_r = j;
    int dist_a = (int)(326. / (double)r * j);  // 326/r: conversion factor to make squares
                                               // if r == 326 circle would be 2 * PI * 326 = 2048
    if (dist_a == 0) dist_a = 1;
    for (int i = 0; i <= dist_a; i++) {  // "upper" side
      PIX(a - i, r + dist_r);            // search starting from the middle
      PIX(a + i, r + dist_r);
    }
    for (int i = 0; i < dist_r; i++) {  // "right hand" side
      PIX(a + dist_a, r + i);
      PIX(a + dist_a, r - i);
    }
    for (int i = 0; i <= dist_a; i++) {  // "lower" side
      PIX(a + i, r - dist_r);
      PIX(a - i, r - dist_r);
    }
    for (int i = 0; i < dist_r; i++) {  // "left hand" side
      PIX(a - dist_a, r + i);
      PIX(a - dist_a, r - i);
    }
  }
  return false;
}

bool ArpaTarget::GetTarget(Polar* pol, int dist1) {
  // general target refresh
  bool contour_found = false;
  int dist = dist1;
  int backup_angle = pol->angle;
  int backup_r = pol->r;
  int backup_contour_length = m_contour_length;
  // LOG_ARPA(wxT(" GetTarget m_target_id=%i, m_status= %i, dist= %i, angle= %i, r= %i, lostcount=%i"), m_target_id, m_status, dist,
  // pol->angle, pol->r, m_lost_count); if (m_status == ACQUIRE0 || m_status == ACQUIRE1) {
  //   dist *= 2;    //  TODO this is no good??
  // }
  if (dist > pol->r - 5) {
    dist = pol->r - 5;  // don't search close to origin
  }

  int a = pol->angle;
  int r = pol->r;

  if (Pix(a, r)) {
    contour_found = FindContourFromInside(pol);
  } else {
    contour_found = FindNearestContour(pol, dist);
  }
  if (!contour_found) {
    pol->angle = backup_angle;
    pol->r = backup_r;
    m_contour_length = backup_contour_length;
    return false;
  }
  int cont = GetContour(pol);
  if (cont != 0) {
    // LOG_ARPA(wxT(" contour not found"));
    LOG_ARPA(wxT("%s: ARPA contour error %d at %d, %d"), m_ri->m_name, cont, a, r);
    // reset pol in case of error
    pol->angle = backup_angle;
    pol->r = backup_r;
    m_contour_length = backup_contour_length;
    return false;
  }
  return true;
}

void ArpaTarget::PassAIVDMtoOCPN(Polar* pol) {
  if (!m_ri->m_AIVDMtoO.GetValue()) return;
  wxString s_TargID, s_Bear_Unit, s_Course_Unit;
  wxString s_speed, s_course, s_Dist_Unit, s_status;
  wxString s_bearing;
  wxString s_distance;
  wxString s_target_name;
  wxString nmea;

  if (m_status == LOST) return;  // AIS has no "status lost" message
  s_Bear_Unit = wxEmptyString;   // Bearing Units  R or empty
  s_Course_Unit = wxT("T");      // Course type R; Realtive T; true
  s_Dist_Unit = wxT("N");        // Speed/Distance Unit K, N, S N= NM/h = Knots

  // double dist = pol->r / m_ri->m_pixels_per_meter / 1852.;
  double bearing = pol->angle * 360. / m_ri->m_spokes;
  if (bearing < 0) bearing += 360;

  int mmsi = m_target_id % 1000000;
  GeoPosition radar_pos;
  m_ri->GetRadarPosition(&radar_pos);
  double target_lat, target_lon;

  target_lat = m_position.pos.lat;
  target_lon = m_position.pos.lon;
  wxString result = EncodeAIVDM(mmsi, m_position.speed_kn, target_lon, target_lat, m_course);
  PushNMEABuffer(result);
}

void ArpaTarget::PassTTMtoOCPN(Polar* pol, OCPN_target_status status) {
  // if (!m_ri->m_TTMtoO.GetValue()) return;  // also remove from conf file
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
  // switch (status) {
  // case Q:
  //   s_status = wxT("Q");  // yellow
  //   break;
  // case T:
  //   s_status = wxT("T");  // green
  //   break;
  // case L:
  //   LOG_ARPA(wxT(" id=%i, status == lost"), m_target_id);
  //   s_status = wxT("L");  // ?
  //   break;
  // }

  if (m_doppler_target == ANY) {
    s_status = wxT("Q");  // yellow
  } else {
    s_status = wxT("T");
  }

  double dist = pol->r / m_ri->m_pixels_per_meter / 1852.;
  double bearing = pol->angle * 360. / m_ri->m_spokes;

  if (bearing < 0) bearing += 360;
  s_TargID = wxString::Format(wxT("%2i"), m_target_id);
  s_speed = wxString::Format(wxT("%4.2f"), m_position.speed_kn);
  s_course = wxString::Format(wxT("%3.1f"), m_course);
  if (m_automatic) {
    s_target_name = wxString::Format(wxT("ARPA%2i"), m_target_id);
  } else {
    s_target_name = wxString::Format(wxT("MARPA%2i"), m_target_id);
  }
  s_distance = wxString::Format(wxT("%f"), dist);
  s_bearing = wxString::Format(wxT("%f"), bearing);

  /* Code for TTM follows. Send speed and course using TTM*/
  snprintf(sentence, sizeof(sentence), "RATTM,%2s,%s,%s,%s,%s,%s,%s, , ,%s,%s,%s, ",
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
  LOG_ARPA(wxT("%s: send TTM, target=%i string=%s"), m_ri->m_name, m_target_id, nmea);
  PushNMEABuffer(nmea);
}

#define COPYTOMESSAGE(xxx, bitsize)                   \
  for (int i = 0; i < bitsize; i++) {                 \
    bitmessage[index - i - 1] = xxx[bitsize - i - 1]; \
  }                                                   \
  index -= bitsize;

wxString ArpaTarget::EncodeAIVDM(int mmsi, double speed, double lon, double lat, double course) {
  // For encoding !AIVDM type 1 messages following the spec in https://gpsd.gitlab.io/gpsd/AIVDM.html
  // Sender is ecnoded as AI. There is no official identification for radar targets.

  bitset<168> bitmessage;
  int index = 168;
  bitset<6> type(1);  // 6
  COPYTOMESSAGE(type, 6);
  bitset<2> repeat(0);  // 8
  COPYTOMESSAGE(repeat, 2);
  bitset<30> mmsix(mmsi);  // 38
  COPYTOMESSAGE(mmsix, 30);
  bitset<4> navstatus(0);  // under way using engine    // 42
  COPYTOMESSAGE(navstatus, 4);
  bitset<8> rot(0);  // not turning                  // 50
  COPYTOMESSAGE(rot, 8);
  bitset<10> speedx(round(speed * 10));  // 60
  COPYTOMESSAGE(speedx, 10);
  bitset<1> accuracy(0);  // 61
  COPYTOMESSAGE(accuracy, 1);
  bitset<28> lonx(round(lon * 600000));  // 89
  COPYTOMESSAGE(lonx, 28);
  bitset<27> latx(round(lat * 600000));  // 116
  COPYTOMESSAGE(latx, 27);
  bitset<12> coursex(round(course * 10));  // COG       // 128
  COPYTOMESSAGE(coursex, 12);
  bitset<9> true_heading(511);  // 137
  COPYTOMESSAGE(true_heading, 9);
  bitset<6> timestamp(60);  // 60 means not available   // 143
  COPYTOMESSAGE(timestamp, 6);
  bitset<2> maneuvre(0);  // 145
  COPYTOMESSAGE(maneuvre, 2);
  bitset<3> spare;  // 148
  COPYTOMESSAGE(spare, 3);
  bitset<1> flags(0);  // 149
  COPYTOMESSAGE(flags, 1);
  bitset<19> rstatus(0);  // 168
  COPYTOMESSAGE(rstatus, 19);
  wxString AIVDM = "AIVDM,1,1,,A,";
  bitset<6> char_data;
  uint8_t character;
  for (int i = 168; i > 0; i -= 6) {
    for (int j = 0; j < 6; j++) {
      char_data[j] = bitmessage[i - 6 + j];
    }
    character = (uint8_t)char_data.to_ulong();
    if (character > 39) character += 8;
    character += 48;
    AIVDM += character;
  }
  AIVDM += ",0";
  // calculate checksum
  char checks = 0;
  for (size_t i = 0; i < AIVDM.length(); i++) {
    checks ^= (char)AIVDM[i];
  }
  AIVDM.Printf(wxT("!%s*%02X\r\n"), AIVDM, (unsigned)checks);
  LOG_ARPA(wxT("%s: AIS length=%i, string=%s"), m_ri->m_name, AIVDM.length(), AIVDM);
  return AIVDM;
}

void ArpaTarget::SetStatusLost() {
  m_contour_length = 0;
  m_previous_contour_length = 0;
  m_lost_count = 0;
  m_kalman.ResetFilter();
  m_status = LOST;
  m_automatic = false;
  m_refresh_time = 0;
  m_position.speed_kn = 0.;
  m_course = 0.;
  m_stationary = 0;
  m_position.dlat_dt = 0.;
  m_position.dlon_dt = 0.;
}

#define SHADOW_MARGIN 5
#define TARGET_DISTANCE_FOR_BLANKING_SHADOW 6000.  // meters

void ArpaTarget::ResetPixels() {
  // resets the pixels of the current blob (plus DISTANCE_BETWEEN_TARGETS) so that blob will not be found again in the same sweep
  // We not only reset the blob but all pixels in a radial "square" covering the blob

  for (int r = wxMax(m_min_r.r - DISTANCE_BETWEEN_TARGETS, 0);
       r <= wxMin(m_max_r.r + DISTANCE_BETWEEN_TARGETS, (int)m_ri->m_spoke_len_max - 1); r++) {
    for (int a = m_min_angle.angle - DISTANCE_BETWEEN_TARGETS; a <= m_max_angle.angle + DISTANCE_BETWEEN_TARGETS; a++) {
      m_ri->m_history[MOD_SPOKES(a)].line[r] = m_ri->m_history[MOD_SPOKES(a)].line[r] & 0x40;  // also clear both Doppler bits
    }
  }
 
  double distance_to_radar = m_polar_pos.r / m_ri->m_pixels_per_meter;
  // For larger targets clear the "shadow" of the target until 2 * r
  if (m_contour_length > 20 && distance_to_radar < TARGET_DISTANCE_FOR_BLANKING_SHADOW) {
    LOG_ARPA(wxT("%s: Shadow cleared for target id=%i, m_min_angle.angle=%i, m_max_angle.angle=%i, m_max_r.r=%i "), m_ri->m_name,
             m_target_id, m_min_angle.angle, m_max_angle.angle, m_max_r.r, distance_to_radar);
    int max = m_max_angle.angle;
    if (m_min_angle.angle - SHADOW_MARGIN > max + SHADOW_MARGIN) {
      max += m_ri->m_spokes;
    }
    for (int a = m_min_angle.angle - SHADOW_MARGIN; a <= max + SHADOW_MARGIN;
         a++) {
      for (uint16_t radius = m_max_r.r; radius < 4 * m_max_r.r && radius < m_ri->m_spoke_len_max; radius++) {
        m_ri->m_history[MOD_SPOKES(a)].line[radius] = m_ri->m_history[MOD_SPOKES(a)].line[radius] & 0x40;
      }
    }
  }
}

Arpa::Arpa(radar_pi* pi, RadarInfo* ri) {
  m_ri = ri;
  m_pi = pi;
  m_clear_contours = false;
  CLEAR_STRUCT(m_doppler_arpa_update_time);
}

Arpa::~Arpa() { m_targets.clear(); }

bool Arpa::MultiPix(int ang, int rad, Doppler doppler) {
  // checks the blob has a contour of at least length pixels
  // pol must start on the contour of the blob
  // false if not
  // if false clears out pixels of the blob in hist
  //    wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);
  int length = m_ri->m_min_contour_length;
  Polar start;
  start.angle = ang;
  start.r = rad;

  if (!Pix(start.angle, start.r, doppler)) {
    return false;
  }
  Polar current = start;  // the 4 possible translations to move from a point on the contour to the next
  Polar max_angle;
  Polar min_angle;
  Polar max_r;
  Polar min_r;
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
  int aa;
  int rr;
  bool succes = false;
  int index = 0;
  max_r = current;
  max_angle = current;
  min_r = current;
  min_angle = current;  // check if p inside blob
  if (start.r >= (int)m_ri->m_spoke_len_max) {
    return false;  //  r too large
  }
  if (start.r < 3) {
    return false;  //  r too small
  }
  // first find the orientation of border point p
  for (int i = 0; i < 4; i++) {
    index = i;
    aa = current.angle + transl[index].angle;
    rr = current.r + transl[index].r;
    succes = !Pix(aa, rr, doppler);
    if (succes) break;
  }
  if (!succes) {
    return false;
  }
  index += 1;  // determines starting direction
  if (index > 3) index -= 4;
  while (current.r != start.r || current.angle != start.angle ||
         count == 0) {  // try all translations to find the next point  // start with the "left most" translation relative to the
    // previous one
    index += 3;  // we will turn left all the time if possible
    for (int i = 0; i < 4; i++) {
      if (index > 3) index -= 4;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
      succes = Pix(aa, rr, doppler);
      if (succes) {  // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      return false;  // no next point found
    }                // next point found
    current.angle = aa;
    current.r = rr;
    if (count >= length) {
      return true;
    }
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
  }  // contour length is less than m_min_contour_length
  // before returning false erase this blob so we do not have to check this one again
  if (min_angle.angle < 0) {
    min_angle.angle += m_ri->m_spokes;
    max_angle.angle += m_ri->m_spokes;
  }
  for (int a = min_angle.angle; a <= max_angle.angle; a++) {
    for (int r = min_r.r; r <= max_r.r; r++) {
      m_ri->m_history[MOD_SPOKES(a)].line[r] &= 63;  // 0x3F
    }
  }
  return false;
}

void Arpa::AcquireNewMARPATarget(ExtendedPosition target_pos) { AcquireOrDeleteMarpaTarget(target_pos, ACQUIRE0); }

void Arpa::DeleteTarget(const GeoPosition& pos) {
  // delete the target that is closest to the position
  double min_dist = 1000;
  int del_target = -1;
  for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
    if ((*target)->m_status != LOST) {
      double dif_lat = pos.lat - (*target)->m_position.pos.lat;
      double dif_lon = (pos.lon - (*target)->m_position.pos.lon) * cos(deg2rad(pos.lat));
      double dist2 = dif_lat * dif_lat + dif_lon * dif_lon;
      if (dist2 < min_dist) {
        min_dist = dist2;
        del_target = target - m_targets.cbegin();
      }
    }
  }

  if (del_target > -1) {
    LOG_ARPA(wxT("%s: Deleting (M)ARPA target at position (%f,%f) which is %f meters from (%f,%f)"), m_ri->m_name,
             m_targets[del_target]->m_position.pos.lat, m_targets[del_target]->m_position.pos.lon, min_dist, pos.lat, pos.lon);
    m_targets.erase(m_targets.begin() + del_target);
  } else {
    LOG_ARPA(wxT("%s: Could not find (M)ARPA target to delete within %f meters from (%f,%f)"), m_ri->m_name, min_dist, pos.lat,
             pos.lon);
  }
}

void Arpa::AcquireOrDeleteMarpaTarget(ExtendedPosition target_pos, int status) {
  // acquires new target from mouse click position
  // no contour taken yet
  // target status acquire0
  // returns in X metric coordinates of click
  // constructs Kalman filter
  // make new target

  LOG_ARPA(wxT("%s: Adding (M)ARPA target at position %f / %f"), m_ri->m_name, target_pos.pos.lat, target_pos.pos.lon);
#ifdef __WXMSW__
  std::unique_ptr<ArpaTarget> target = std::make_unique<ArpaTarget>(m_pi, m_ri, 0);
  #else
  std::unique_ptr<ArpaTarget> target = make_unique<ArpaTarget>(m_pi, m_ri, 0);
 #endif
  target->m_position = target_pos;  // Expected position
  target->m_status = status;
  target->m_doppler_target = ANY;
  target->m_automatic = false;

  m_targets.push_back(std::move(target));
}

void Arpa::DrawContour(const ArpaTarget* target) {
  if (target->m_lost_count > 0) {
    return;  // don't draw targets that were not seen last sweep
  }
  wxColor arpa = m_pi->m_settings.arpa_colour;
  glColor4ub(arpa.Red(), arpa.Green(), arpa.Blue(), arpa.Alpha());
  glLineWidth(3.0);

  glEnableClientState(GL_VERTEX_ARRAY);

  Point vertex_array[MAX_CONTOUR_LENGTH + 1];
  for (int i = 0; i < target->m_contour_length; i++) {
    int angle = target->m_contour[i].angle + (DEGREES_PER_ROTATION + OPENGL_ROTATION) * m_ri->m_spokes / DEGREES_PER_ROTATION;
    int radius = target->m_contour[i].r;
    if (radius <= 0 || radius >= (int)m_ri->m_spoke_len_max) {
      LOG_INFO(wxT("wrong values in DrawContour"));
      return;
    }
    vertex_array[i] = m_ri->m_polar_lookup->GetPoint(angle, radius);
    vertex_array[i].x = vertex_array[i].x / m_ri->m_pixels_per_meter;
    vertex_array[i].y = vertex_array[i].y / m_ri->m_pixels_per_meter;
  }

  glVertexPointer(2, GL_FLOAT, 0, vertex_array);
  glDrawArrays(GL_LINE_STRIP, 0, target->m_contour_length);

  glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
}

void Arpa::DrawArpaTargetsOverlay(double scale, double arpa_rotate) {
  wxPoint boat_center;
  GeoPosition radar_pos;
  if (!m_pi->m_settings.drawing_method && m_ri->GetRadarPosition(&radar_pos)) {
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status == LOST || (*target)->m_contour_length == 0) {
        continue;
      }
      double poslat = (*target)->m_radar_pos.lat;
      double poslon = (*target)->m_radar_pos.lon;
      // some additional logging, to be removed later
      if (poslat > 90. || poslat < -90. || poslon > 180. || poslon < -180.) {
        LOG_INFO(wxT("**error wrong target pos, id=%i, status=%i, poslat = %f, poslon = %f"), (*target)->m_target_id,
                 (*target)->m_status, poslat, poslon);
        continue;
      }

      GetCanvasPixLL(m_ri->m_pi->m_vp, &boat_center, (*target)->m_radar_pos.lat, (*target)->m_radar_pos.lon);
      glPushMatrix();
      glTranslated(boat_center.x, boat_center.y, 0);
      glRotated(arpa_rotate, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);
      DrawContour(target->get());
      glPopMatrix();
    }
  } else {
    m_ri->GetRadarPosition(&radar_pos);
    GetCanvasPixLL(m_ri->m_pi->m_vp, &boat_center, radar_pos.lat, radar_pos.lon);
    glPushMatrix();
    glTranslated(boat_center.x, boat_center.y, 0);
    glRotated(arpa_rotate, 0.0, 0.0, 1.0);
    glScaled(scale, scale, 1.);
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status != LOST) {
        DrawContour(target->get());
      }
    }
    glPopMatrix();
  }
}

void Arpa::DrawArpaTargetsPanel(double scale, double arpa_rotate) {
  wxPoint boat_center;
  GeoPosition radar_pos, target_pos;
  double offset_lat = 0.;
  double offset_lon = 0.;

  if (!m_pi->m_settings.drawing_method && m_ri->GetRadarPosition(&radar_pos)) {
    m_ri->GetRadarPosition(&radar_pos);
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status == LOST) {
        continue;
      }
      target_pos = (*target)->m_radar_pos;
      offset_lat = (radar_pos.lat - target_pos.lat) * 60. * 1852. * m_ri->m_panel_zoom / m_ri->m_range.GetValue();
      offset_lon = (radar_pos.lon - target_pos.lon) * 60. * 1852. * cos(deg2rad(target_pos.lat)) * m_ri->m_panel_zoom /
                   m_ri->m_range.GetValue();
      glPushMatrix();
      glRotated(arpa_rotate, 0.0, 0.0, 1.0);
      glTranslated(-offset_lon, offset_lat, 0);
      glScaled(scale, scale, 1.);
      DrawContour(target->get());
      glPopMatrix();
    }
  }

  else {
    glPushMatrix();
    glTranslated(0., 0., 0.);
    glRotated(arpa_rotate, 0.0, 0.0, 1.0);
    glScaled(scale, scale, 1.);
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status != LOST) {
        DrawContour(target->get());
      }
    }
    glPopMatrix();
  }
}

void Arpa::CleanUpLostTargets() {
  // remove targets with status LOST
  for (auto target = m_targets.begin(); target != m_targets.end();) {
    if ((*target)->m_status == LOST) {
      target = m_targets.erase(target);
    } else {
      (*target)->m_refreshed = NOT_FOUND;
      ++target;
    }
  }
}

void Arpa::RefreshAllArpaTargets() {
  LOG_ARPA(wxT("\n\n\n\n%s: ********************************refresh loop start m_targets.size=%i, last spoke=%u "), m_ri->m_name,
           m_targets.size(), m_ri->m_last_received_spoke);
  if (m_ri->m_pixels_per_meter == 0) return;
  CleanUpLostTargets();
  sort(m_targets.begin(), m_targets.end(), SortTargetStatus);

  // main target refresh loop

  // pass 0 of target refresh  Only search for moving targets faster than 2 knots as long as autolearnng is initializing
  // When autolearn is ready, apply for all targets
  #define MAX_DETECTION_SPEED 40. // in knots
  int speed = MAX_DETECTION_SPEED / 2; // m/sec
  uint16_t search_radius = (uint16_t)(speed * m_ri->m_rotation_period.GetValue() * m_ri->m_pixels_per_meter / 1000.);
  LOG_ARPA(wxT("%s, Search radius =%i, pix/m=%f"), m_ri->m_name, search_radius, m_ri->m_pixels_per_meter);
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    if (((*target)->m_position.speed_kn >= 2.5 && (*target)->m_status >= STATUS_TO_OCPN )) {
      (*target)->RefreshTarget(search_radius / 4, 0);  // was 5
    }
  }

  // pass 1 of target refresh
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    (*target)->RefreshTarget(search_radius / 3, 1);
  }

  // pass 2 of target refresh
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    (*target)->RefreshTarget(search_radius, 2);
  }
  LOG_ARPA(wxT("%s: ********************************refresh loop end m_targets.size=%i, last spoke=%u \n\n\n"), m_ri->m_name,
           m_targets.size(), m_ri->m_last_received_spoke);
}

/**
 * Inject the target in this radar.
 *
 * Called from the main thread and from the InterRadar receive thread.
 */
void Arpa::InsertOrUpdateTargetFromOtherRadar(const DynamicTargetData* data, bool remote) {
  wxCriticalSectionLocker lock(m_ri->m_exclusive);

  // This method works on the other radar than TransferTargetToOtherRadar
  // find target
  bool found = false;
  int uid = data->target_id;
  LOG_ARPA(wxT("%s: InsertOrUpdateTarget id=%i"), m_ri->m_name, uid);
  ArpaTarget* updated_target = 0;
  for (auto target = m_targets.begin(); target != m_targets.end(); target++) {
    //LOG_ARPA(wxT("%s: InsertOrUpdateTarget id=%i, found=%i"), m_ri->m_name, uid, (*target)->m_target_id);
    if ((*target)->m_target_id == uid) {  // target found!
      updated_target = (*target).get();
      found = true;
      LOG_ARPA(wxT("%s: InsertOrUpdateTarget found target id=%d pos=%d"), m_ri->m_name, uid, target - m_targets.begin());
      break;
    }
  }
  if (!found) {
    // make new target with existing uid
    LOG_ARPA(wxT("%s: InsertOrUpdateTarget new target id=%d, pos=%ld"), m_ri->m_name, uid, m_targets.size());
#ifdef __WXMSW__
    std::unique_ptr<ArpaTarget> new_target = std::make_unique<ArpaTarget>(m_pi, m_ri, uid);
    #else
    std::unique_ptr<ArpaTarget> new_target = make_unique<ArpaTarget>(m_pi, m_ri, uid);
    #endif
    updated_target = new_target.get();
    m_targets.push_back(std::move(new_target));
    ExtendedPosition own_pos;
    if (remote) {
      m_ri->GetRadarPosition(&own_pos);
      Polar pol = updated_target->Pos2Polar(data->position, own_pos);
      LOG_ARPA(wxT("%s: InsertOrUpdateTarget new target id=%d polar=%i"), m_ri->m_name, uid, pol.angle);
      // set estimated time of last refresh as if it was a local target
      updated_target->m_refresh_time = m_ri->m_history[MOD_SPOKES(pol.angle)].time;
    }
  }
  //LOG_ARPA(wxT("%s: InsertOrUpdateTarget processing id=%i"), m_ri->m_name, uid);
  updated_target->m_kalman.P = data->P;
  updated_target->m_position = data->position;
  updated_target->m_status = data->status;
  LOG_ARPA(wxT("%s: transferred id=%i, lat= %f, lon= %f, status=%i,"), m_ri->m_name, updated_target->m_target_id,
           updated_target->m_position.pos.lat, updated_target->m_position.pos.lon, updated_target->m_status);
  updated_target->m_doppler_target = ANY;
  updated_target->m_lost_count = 0;
  updated_target->m_automatic = true;
  double s1 = updated_target->m_position.dlat_dt;  // m per second
  double s2 = updated_target->m_position.dlon_dt;                                   // m  per second
  updated_target->m_course = rad2deg(atan2(s2, s1));
  if (remote) {   // inserted or updated target originated from another radar
    updated_target->m_transferred_target = true;
    //LOG_ARPA(wxT(" m_transferred_target = true targetid=%i"), updated_target->m_target_id);
  }
  return;
}

void Arpa::CalculateCentroid(ArpaTarget* target) {
  // real calculation still to be done
}

void Arpa::DeleteAllTargets() { m_targets.clear(); }

bool Arpa::AcquireNewARPATarget(Polar pol, int status, Doppler doppler) {
  // acquires new target at polar position pol
  // no contour taken yet
  // target status status, normally 0, if dummy target to delete a target -2
  // constructs Kalman filter
  ExtendedPosition own_pos;
  ExtendedPosition target_pos;
  Doppler doppl = doppler;
  if (!m_ri->GetRadarPosition(&own_pos.pos)) {
    return false;
  }

  // make new target
#ifdef __WXMSW__
  std::unique_ptr<ArpaTarget> target = std::make_unique<ArpaTarget>(m_pi, m_ri, 0);
  #else
  std::unique_ptr<ArpaTarget> target = make_unique<ArpaTarget>(m_pi, m_ri, 0);
  #endif
  target_pos = target->Polar2Pos(pol, own_pos);
  target->m_doppler_target = doppl;
  target->m_position = target_pos;  // Expected position
  target->m_position.time = wxGetUTCTimeMillis();
  target->m_position.dlat_dt = 0.;
  target->m_position.dlon_dt = 0.;
  target->m_position.speed_kn = 0.;
  target->m_position.sd_speed_kn = 0.;
  target->m_status = status;
  target->m_max_angle.angle = 0;
  target->m_min_angle.angle = 0;
  target->m_max_r.r = 0;
  target->m_min_r.r = 0;
  target->m_doppler_target = doppl;
  target->m_refreshed = NOT_FOUND;
  target->m_automatic = true;
  target->RefreshTarget(TARGET_SEARCH_RADIUS1, 1);

  m_targets.push_back(std::move(target));
  return true;
}

void Arpa::ClearContours() { m_clear_contours = true; }

void Arpa::ProcessIncomingMessages() {
  DynamicTargetData* target;
  if (m_clear_contours) {
    m_clear_contours = false;
    for (auto target = m_targets.begin(); target != m_targets.end(); target++) {
      (*target)->m_contour_length = 0;
      (*target)->m_previous_contour_length = 0;
    }
  }

  while ((target = GetIncomingRemoteTarget()) != NULL) {
    InsertOrUpdateTargetFromOtherRadar(target, true);
    delete target;
  }
}

bool Arpa::IsAtLeastOneRadarTransmitting() {
  for (size_t r = 0; r < RADARS; r++) {
    if (m_pi->m_radar[r] != NULL && m_pi->m_radar[r]->m_state.GetValue() == RADAR_TRANSMIT) {
      return true;
    }
  }
  return false;
}

void Arpa::SearchDopplerTargets() {
  ExtendedPosition own_pos;

  if (!m_pi->m_settings.show                       // No radar shown
      || !m_ri->GetRadarPosition(&own_pos.pos)     // No position
      || m_pi->GetHeadingSource() == HEADING_NONE  // No heading
      || (m_pi->GetHeadingSource() == HEADING_FIX_HDM && m_pi->m_var_source == VARIATION_SOURCE_NONE)) {
    return;
  }

  if (m_ri->m_pixels_per_meter == 0. || !IsAtLeastOneRadarTransmitting()) {
    return;
  }

  size_t range_start = 20;  // Convert from meters to 0..511
  size_t range_end;
  int outer_limit = m_ri->m_spoke_len_max;
  outer_limit = (int)outer_limit * 0.93;
  range_end = outer_limit;  // Convert from meters to 0..511

  SpokeBearing start_bearing = 0;
  SpokeBearing end_bearing = m_ri->m_spokes;

  // loop with +2 increments as target must be larger than 2 pixels in width
  for (int angleIter = start_bearing; angleIter < end_bearing; angleIter += 2) {
    SpokeBearing angle = MOD_SPOKES(angleIter);
    wxLongLong angle_time = m_ri->m_history[angle].time;
    // angle_time_plus_margin must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
    wxLongLong angle_time_plus_margin = m_ri->m_history[MOD_SPOKES(angle + 3 * SCAN_MARGIN)].time;

    // check if target has been refreshed since last time
    // and if the beam has passed the target location with SCAN_MARGIN spokes
    if ((angle_time > (m_doppler_arpa_update_time[angle] + SCAN_MARGIN2) &&
         angle_time_plus_margin >= angle_time)) {  // the beam sould have passed our "angle" AND a
                                                   // point SCANMARGIN further set new refresh time
      m_doppler_arpa_update_time[angle] = angle_time;
      for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {
        if (m_ri->m_arpa->MultiPix(angle, rrr, ANY_DOPPLER)) {
          // pixel found that does not belong to a known target
          Polar pol;
          pol.angle = angle;
          pol.r = rrr;
          if (!m_ri->m_arpa->AcquireNewARPATarget(pol, 0, ANY_DOPPLER)) {
            break;
          }
        }
      }
    }
  }

  return;
}

DynamicTargetData* Arpa::GetIncomingRemoteTarget() {
  wxCriticalSectionLocker lock(m_remote_target_lock);
  DynamicTargetData* next;
  if (m_remote_target_queue.empty()) {
    next = NULL;
  } else {
    next = m_remote_target_queue.front();
    m_remote_target_queue.pop_front();
  }
  return next;
}

/**
 * Safe to call from any thread
 */
void Arpa::StoreRemoteTarget(DynamicTargetData* target) {
  wxCriticalSectionLocker lock(m_remote_target_lock);
  m_remote_target_queue.push_back(target);
}

PLUGIN_END_NAMESPACE
