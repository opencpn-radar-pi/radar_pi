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
#include "drawutil.h"
#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

#define LAST_PASS (2)              // adapt this when more passes added
#define MAX_DETECTION_SPEED (20.)  // in meters/second

class LocalPosition;
class Polar;

static bool SortTargetStatus(const std::unique_ptr<ArpaTarget>& one, const std::unique_ptr<ArpaTarget>& two) {
  return one->m_status > two->m_status;
}

ArpaTarget::ArpaTarget(radar_pi* pi, Arpa* arpa, int uid) : m_kalman(KalmanFilter()) {
  // makes new target with an existing id
  m_approaching_pix = 0;
  m_receding_pix = 0;
  m_total_pix = 0;
  m_radar_position.lat = 0.;
  m_radar_position.lon = 0.;
  m_ri = 0;
  m_arpa = arpa;
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
  m_position.sd_speed_kn = 0.;
  m_position.time = wxGetUTCTimeMillis();  // millis
  m_target_doppler = ANY;
  m_refreshed = NOT_FOUND;
  m_target_id = uid;
  m_polar_pos.r = 0;
  m_max_angle.angle = 0;
  m_max_angle.r = 0;
  m_min_angle.angle = 0;
  m_min_angle.r = 0;
  LOG_ARPA(wxT(" number of targets=%i, new target id=%i"), m_pi->m_arpa->GetTargetCount(), m_target_id);
}

ArpaTarget::~ArpaTarget() {}

// return M_XY((angle + m_spokes) % m_spokes, radius);

GeoPosition ArpaTarget::Polar2Pos(RadarInfo* ri, Polar pol, GeoPosition position) {
  // converts in a radar image angular data r ( 0 - max_spoke_len ) and angle (0 - max_spokes) to position (lat, lon)
 
  GeoPosition pos;
  // should be revised, use Mercator formula PositionBearingDistanceMercator()  TODO

  pos.lat = position.lat + ri->m_polar_lookup->GetPoint(pol.angle, pol.r).x / ri->m_pixels_per_meter / 60. / 1852.;

  pos.lon = position.lon + ri->m_polar_lookup->GetPoint(pol.angle, pol.r).y / ri->m_pixels_per_meter / 
    cos(deg2rad(position.lat)) / 60. / 1852.;

  //pos.pos.lat = position.lat + ((double)pol.r / ri->m_pixels_per_meter)  // Scale to fraction of distance from radar
  //                                     * cos(deg2rad(SCALE_SPOKES_TO_DEGREES(ri, pol.angle))) / 60. / 1852.;
  //pos.pos.lon = position.lon + ((double)pol.r / ri->m_pixels_per_meter)  // Scale to fraction of distance to radar
  //                                     * sin(deg2rad(SCALE_SPOKES_TO_DEGREES(ri, pol.angle))) / cos(deg2rad(position.lat)) / 60. /
  //                                     1852.;
  return pos;  // $$$ initialize other vars of pos
}

Polar ArpaTarget::Pos2Polar(RadarInfo* ri, GeoPosition pos, GeoPosition position) {
  // converts in a radar image a lat-lon position to angular data relative to position own_ship
  Polar pol;
  double dif_lat = pos.lat;
  dif_lat -= position.lat;
  double dif_lon = (pos.lon - position.lon) * cos(deg2rad(position.lat));
  pol.r = std::lround(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * ri->m_pixels_per_meter );
  LOG_ARPA(wxT("$$$Pos2Polar radar=%s, pix/m=%f, r=%i"), ri->m_name, ri->m_pixels_per_meter, pol.r);
  pol.angle = std::lround((atan2(dif_lon, dif_lat)) * (double)ri->m_spokes / (2. * PI));
  if (pol.angle < 0) pol.angle += ri->m_spokes;
  while (pol.angle >= (int) ri->m_spokes){
    pol.angle -= ri->m_spokes;
  }
  if (pol.r < 0) {
    LOG_INFO(wxT("$$$ error spokelength pol.r"), pol.r);
    pol.r = 0;
  }
  if (pol.r >= (int)ri->m_spoke_len_max) {
    LOG_INFO(wxT("$$$ error spokelength pol.r"), pol.r);
    //pol.r = ri->m_spoke_len_max - 1;
  }
  LOG_ARPA(wxT("$$$Pos2Polar pol.r=%i"), pol.r);
  return pol;
}

bool Arpa::Pix(RadarInfo* ri, int ang, int rad, Doppler doppler) {
  if (rad <= 0 || rad >= (int)ri->m_spoke_len_max) {
    return false;
  }
  int angle1 = MOD_SPOKES(ri, ang);
  if (angle1 < 0) return false;
  size_t angle = angle1;
  if (angle >= ri->m_spokes || angle < 0) {
    return false;
  }
  bool bit0 = (ri->m_history[angle].line[rad] & 0x80) != 0;  // above threshold bit
  bool bit1 = (ri->m_history[angle].line[rad] & 0x40) != 0;  // backup bit does not get cleared when target is refreshed
  bool bit2 = (ri->m_history[angle].line[rad] & 0x20) != 0;  // this is Doppler approaching bit
  bool bit3 = (ri->m_history[angle].line[rad] & 0x10) != 0;  // this is Doppler receding bit
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

bool ArpaTarget::Pix(RadarInfo* ri, int ang, int rad) {
  if (rad <= 0 || rad >= (int)ri->m_spoke_len_max) {
    return false;
  }
  int angle1 = MOD_SPOKES(ri, ang);
  if (angle1 < 0) return false;
  size_t angle = angle1;
  if (angle >= ri->m_spokes || angle < 0) {
    return false;
  }
  bool bit0 = (ri->m_history[angle].line[rad] & 0x80) != 0;  // above threshold bit
  bool bit1 = (ri->m_history[angle].line[rad] & 0x40) != 0;  // backup bit does not get cleared when target is refreshed
  bool bit2 = (ri->m_history[angle].line[rad] & 0x20) != 0;  // this is Doppler approaching bit
  bool bit3 = (ri->m_history[angle].line[rad] & 0x10) != 0;  // this is Doppler receding bit

  switch (m_target_doppler) {
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

bool Arpa::FindContourFromInside(RadarInfo* ri, Polar* pol, Doppler doppler) {  // moves pol to contour of blob
  // true if success
  // false when failed
  RadarInfo* radar = ri;
  Doppler doppl = doppler;

  int ang = pol->angle;
  int rad = pol->r;
  int limit = radar->m_spokes;

  if (rad >= (int)radar->m_spoke_len_max || rad < 3) {
    return false;
  }
  if (!(Pix(radar, ang, rad, doppl))) {
    LOG_ARPA(wxT("$$$q not inside ang=%i, rad=%i"), ang, rad);
    return false;
  }
  while (limit >= 0 && Pix(radar, ang, rad, doppler)) {
    ang--;
    limit--;
  }
  ang++;
  pol->angle = ang;
  // check if the blob has the required min contour length
  if (MultiPix(radar, ang, rad, doppl)) {
    return true;
  } else {
    LOG_ARPA(wxT("$$$ not contourlength"));
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


int ArpaTarget::GetContour(RadarInfo* ri, Polar* pol) {
 //wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);
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
  if (start.r >= (int)ri->m_spoke_len_max) {
    return 1;  // return code 1, r too large
  }
  if (start.r < 4) {
    return 2;  // return code 2, r too small
  }
  if (!Pix(ri, start.angle, start.r)) {
    return 3;  // return code 3, starting point outside blob
  }
  // first find the orientation of border point p
  for (int i = 0; i < 4; i++) {
    index = i;
    aa = current.angle + transl[index].angle;
    rr = current.r + transl[index].r;
    succes = !Pix(ri, aa, rr);
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
      succes = Pix(ri, aa, rr);
      if (succes) {
        // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      LOG_INFO(wxT("GetContour no next point found count= %i"), count);
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
  //  CalculateCentroid(*target);   // we better use the real centroid instead of the average, TODO
  if (m_min_angle.angle < 0) {
    m_min_angle.angle += ri->m_spokes;
    m_max_angle.angle += ri->m_spokes;  // attention, could be > m_spokes
  }
  if (m_min_angle.angle > m_max_angle.angle)
    wxLogMessage(wxT("$$$$$ m_min_angle.angle=%i, m_max_angle.angle=%i"), m_min_angle.angle, m_max_angle.angle);
    
  pol->angle = (m_max_angle.angle + m_min_angle.angle) / 2;
  if (m_max_r.r >= (int)ri->m_spoke_len_max || m_min_r.r >= (int)ri->m_spoke_len_max) {
    return 10;  // return code 10 r too large
  }
  if (m_max_r.r < 2 || m_min_r.r < 2) {
    return 11;  // return code 11 r too small
  }
  if (pol->angle >= (int)ri->m_spokes) {
    pol->angle -= ri->m_spokes;
  }
  m_polar_pos.angle = pol->angle;
  pol->r = (m_max_r.r + m_min_r.r) / 2;
  m_polar_pos.r = pol->r;
  pol->time = ri->m_history[MOD_SPOKES(ri, pol->angle)].time;
  GeoPosition radar_pos = ri->m_history[MOD_SPOKES(ri, pol->angle)].pos;

  double poslat = radar_pos.lat;
  double poslon = radar_pos.lon;
  if (poslat > 90. || poslat < -90. || poslon > 180. || poslon < -180.) {
    // some additional logging, to be removed later
    LOG_INFO(wxT("**error wrong target pos, poslat = %f, poslon = %f"), poslat, poslon);
  }
  return 0;  //  success, blob found
}


// Checks if the time is right to refresh the target, that is: the beam has just passed the target
// Sets m_ri at best radar for target
// Sets m_radar_position at the current position of the radar (m_ri)
// Sets m_refreshed = OUT_OF_SCOPE if target should not yet be refreshed
bool ArpaTarget::CheckRefreshTiming() {
  if (m_status == LOST) {
    m_refreshed = OUT_OF_SCOPE;
    return false;
  }
  m_ri = m_pi->FindBestRadarForTarget(
      m_position.pos);  // priliminary radar selection, choice of radar can change based on predicted position
  if (!m_ri) {
    m_refreshed = OUT_OF_SCOPE;
    return false;
  }
  m_ri->GetRadarPosition(&m_radar_position);
  if (!m_ri) {  // target may be out of range
    m_refreshed = OUT_OF_SCOPE;
    return false;
  }
  int rotation_period = m_ri->m_rotation_period.GetValue();
  if (rotation_period == 0) {
    rotation_period = 3000;  // default value, milli seconds
  }
  wxLongLong now = wxGetUTCTimeMillis();  // millis
  Polar pol = Pos2Polar(m_ri, m_position.pos, m_radar_position);
  int angle_dist = m_ri->m_last_received_spoke - pol.angle;
  if (angle_dist > (int) m_ri->m_spokes / 2) angle_dist -= (int) m_ri->m_spokes;
  if (angle_dist < - (int) m_ri->m_spokes / 2) angle_dist += m_ri->m_spokes;
  LOG_ARPA(wxT(" %s, id=%i, xangle=%i, r= %i, last_spoke=%i, angle_dist=%i "), m_ri->m_name, m_target_id, pol.angle, pol.r, m_ri->m_last_received_spoke, angle_dist);
  // 50 is a margin on the rotation period 
  if (((now > m_refresh_time + rotation_period * (m_lost_count + 1)) || m_status == 0) && abs(angle_dist) > 50) {
    return true;  // timing is OK to refresh target
  } else {
    // the next image of the target is not yet there
    m_refreshed = OUT_OF_SCOPE;
    return false;
  }
}

void ArpaTarget::Local2Ext(LocalPosition local_pos, ExtendedPosition* ext_pos) {
  ext_pos->pos.lat = local_pos.radar_pos.lat + local_pos.lat / 60. / 1852.;
  ext_pos->pos.lon = local_pos.radar_pos.lon + local_pos.lon / 60. / 1852. / cos(deg2rad(local_pos.radar_pos.lat));
  ext_pos->dlat_dt = local_pos.dlat_dt;  // m per second
  ext_pos->dlon_dt = local_pos.dlon_dt;  // m per second
  double speed_m_sec = sqrt(ext_pos->dlat_dt * ext_pos->dlat_dt + ext_pos->dlon_dt * ext_pos->dlon_dt);
  ext_pos->speed_kn = speed_m_sec * 3600. / 1852.;  // nautical miles per hour
  ext_pos->sd_speed_kn = local_pos.sd_speed_m_s * 3600. / 1852.;
};

void ArpaTarget::Ext2Local(ExtendedPosition ext_pos, LocalPosition* local_pos) {
  local_pos->lat = (ext_pos.pos.lat - local_pos->radar_pos.lat) * 60. * 1852.;
  LOG_INFO(wxT("$$$ ext_pos.pos.lat=%f, local_pos->radar_pos.lat=%f"), ext_pos.pos.lat, local_pos->radar_pos.lat);
  local_pos->lon = (ext_pos.pos.lon - local_pos->radar_pos.lon) * 60. * 1852. * cos(deg2rad(local_pos->radar_pos.lat));
  LOG_INFO(wxT("$$$ ext_pos.pos.lon=%f, local_pos->radar_pos.lon=%f"), ext_pos.pos.lon, local_pos->radar_pos.lon);
  local_pos->dlat_dt = ext_pos.dlat_dt;  // m per second
  local_pos->dlon_dt = ext_pos.dlon_dt;  // m per second
  local_pos->sd_speed_m_s = ext_pos.sd_speed_kn / 3600. * 1852.;
}

LocalPosition::LocalPosition(GeoPosition radar_position) {
  lat = 0.;
  lon = 0.;
  dlat_dt = 0.;
  dlon_dt = 0.;
  speed = 0.;
  sd_speed_m_s = 0.;
  radar_pos = radar_position;
  LOG_INFO(wxT("$$$ radar_position.lat=%f, radar_position.lon=%f"), radar_position.lat, radar_position.lon);
}

Polar::Polar() {
  r = 0;
  angle = 0;
  time = wxGetUTCTimeMillis();
};

void ArpaTarget::RefreshTarget(double speed, int pass) {
  wxCriticalSectionLocker lock(m_protect_target_data);
  ExtendedPosition previous_position = m_position;
  double delta_t;
  wxLongLong prev_refresh = m_refresh_time;
  wxLongLong now = wxGetUTCTimeMillis();  // millis
  if (m_refreshed == FOUND || m_refreshed == OUT_OF_SCOPE) {
    return;
  }
  if (!CheckRefreshTiming()) return;   // also sets m_radar_position
  LOG_ARPA(wxT("\n\n $$$ Start with next target, id=%i, status=%i, pass=%i, last_spoke=%i"), m_target_id, m_status, pass, 
    m_ri->m_last_received_spoke);

  // PREDICTION CYCLE
  LocalPosition predicted_local(m_radar_position);
  int rotation_period = m_ri->m_rotation_period.GetValue();
  if (rotation_period == 0) {
    rotation_period = 3000;  // default value, milli seconds
  }
  // Elapsed time since previous target seen
  delta_t = (double)(rotation_period * (m_lost_count + 1)) / 1000.;  // in seconds
  if (m_status == 0) {  // no previous position known
    delta_t = 0.;
  }
  LOG_ARPA(wxT("$$$ rotation_period= %i, m_lost_count=%i, delta_t=%f"), rotation_period, m_lost_count, delta_t);
  LOG_ARPA(wxT("%s: target begin prediction cycle m_target_id=%i, delta time=%f, m_refresh_time=%u, previous_position.time=%u, now=%u, m_state= %i"),
      m_ri->m_name, m_target_id, delta_t, m_refresh_time.GetLo(), previous_position.time.GetLo(), now.GetLo(), m_status);
  LOG_ARPA(wxT("$$$ target position.lat=%f, position.lon=%f"), m_position.pos.lat, m_position.pos.lon);

  if (m_position.pos.lat > 90. || m_position.pos.lat < -90.) {
    SetStatusLost();
    LOG_ARPA(wxT("%s: Lost, m_target_id=%i"), m_ri->m_name, m_target_id);
    m_refreshed = OUT_OF_SCOPE;
    LOG_ARPA(wxT("%s: 1 target out of scope id=%u"), m_ri->m_name, m_target_id);
    return;
  }

  // Convert to a local position in meters

  // Following positions are used:
  // m_position:      original target extended position
  // predicted_local: predicted position in local coordinates (meters)
  // predicted_pos:   predicted extended position of target
  // predicted_pol:   predicted polar position of target


  Ext2Local(m_position, &predicted_local);
  ExtendedPosition predicted_pos;
  m_kalman.Predict(&predicted_local, delta_t);  // predicted_local is now new predicted local position of the target
  Local2Ext(predicted_local, &predicted_pos);
  // Check if radar is still the best radar
  RadarInfo* ri = m_pi->FindBestRadarForTarget(predicted_pos.pos);
  if (!ri) {
    LOG_ARPA(wxT(" Change of radar, target out of range"));
    m_refreshed = OUT_OF_SCOPE;
    SetStatusLost();
    return;
  }
  if (m_ri != ri) {
    LOG_ARPA(wxT(" find best2ab, id=%i"), m_target_id);  // $$$
    m_ri = ri;
    LOG_ARPA(wxT(" Change of radar, m_target_id= %i, new radar= %s"), m_target_id, m_ri->m_name);
    // change of radar
  }
  Polar predicted_pol;
  predicted_pol = Pos2Polar(m_ri, predicted_pos.pos, m_radar_position);
  if (predicted_pol.r > m_ri->m_spoke_len_max) {
    m_refreshed = OUT_OF_SCOPE;
    LOG_ARPA(wxT("$$$ %s, Target out of range target_id=%i, r= %i"), m_ri->m_name, m_target_id, m_ri->m_spoke_len_max);
    SetStatusLost();
    return;
  }



 /* predicted_pol.angle = (int)(atan2(predicted_local.lon, predicted_local.lat) * m_ri->m_spokes / (2. * PI));
  if (predicted_pol.angle < 0) predicted_pol.angle += m_ri->m_spokes;
  predicted_pol.r = (int)(sqrt(predicted_local.lat * predicted_local.lat + predicted_local.lon * predicted_local.lon) * m_ri->m_pixels_per_meter);*/

  // zooming and target movement may  cause r to be out of bounds

 /* predicted_pol.angle = (int)(atan2(predicted_local.pos.lon, predicted_local.pos.lat) * m_ri->m_spokes / (2. * PI));
  if (predicted_pol.angle < 0) predicted_pol.angle += m_ri->m_spokes;
  predicted_pol.r = (int)(sqrt(predicted_local.pos.lat * predicted_local.pos.lat + predicted_local.pos.lon * predicted_local.pos.lon) * m_ri->m_pixels_per_meter);*/

  /*int delta_angle = m_ri->m_last_received_spoke - predicted_pol.angle;
  int spokes = m_ri->m_spokes;
  if (delta_angle < -spokes / 2) delta_angle += spokes;
  if (delta_angle > spokes / 2) delta_angle -= spokes;
  if (delta_angle < 50) {
    LOG_ARPA(wxT("$$$$ id=%i, angle=%i, spoke=%i, delta_angle=%i"), m_target_id, predicted_pol.angle, m_ri->m_last_received_spoke, delta_angle);
  }*/

  // zooming and target movement may  cause r to be out of bounds

  LOG_ARPA(wxT("%s: PREDICTION m_target_id=%i, pass=%i, status=%i, angle=%i, r= %i, contour=%i, speed=%f, sd_speed_kn=%f,"
               " lostcount=%i"),
           m_ri->m_name, m_target_id, pass, m_status, predicted_pol.angle, predicted_pol.r, m_contour_length,
           m_position.speed_kn, m_position.sd_speed_kn, m_lost_count);
  
  Polar position_pol = Pos2Polar(m_ri, m_position.pos, m_radar_position);  // this is still the original target position
  LOG_ARPA(wxT("$$$2 predicted_pol.r=%i"), predicted_pol.r);
  LOG_ARPA(wxT("%s: PREDICTION m_target_id=%i, pass=%i, status=%i, angle=%i, r= %i, contour=%i, speed=%f, sd_speed_kn=%f, "
               "Doppler=%i, lostcount=%i"),
           m_ri->m_name, m_target_id, pass, m_status, predicted_pol.angle, predicted_pol.r, m_contour_length, m_position.speed_kn,
           m_position.sd_speed_kn, m_target_doppler, m_lost_count);

  // zooming and target movement may  cause r to be out of bounds
 
  // MEASUREMENT CYCLE
  // search for the target at the expected polar position in predicted_pol

  int dist1 = (uint16_t)(speed * m_ri->m_rotation_period.GetValue() * m_ri->m_pixels_per_meter / 1000.);

  if (pass == LAST_PASS) {  // increase search radius for low status targets
    if (m_status <= 2) {
      dist1 *= 2;
    } else if (m_position.speed_kn > 15.) {
      dist1 *= 2;
    }
  }
  bool found = false;
  LOG_ARPA(wxT("%s: MEASUREMENT m_target_id=%i, pass=%i, status=%i, pred-angle=%i, pred-r= %i, contour=%i, average contour=%i, speed=%f, "
               "sd_speed_kn=%f, "
               "Doppler=%i, lostcount=%i"),
           m_ri->m_name, m_target_id, pass, m_status, predicted_pol.angle, predicted_pol.r, m_contour_length,
           m_average_contour_length, m_position.speed_kn, m_position.sd_speed_kn, m_target_doppler, m_lost_count);
  //LOG_ARPA(wxT("scantime=%u, refreshtime=%u"), now.GetLo(), m_refresh_time.GetLo());
  if (pass == LAST_PASS) {
    m_target_doppler = ANY;  // in the last pass accept enything within reach
  }
  Polar measured_pol;

  found = GetTarget(m_ri, predicted_pol, &measured_pol, dist1);  // main target search****************************************


  LOG_ARPA(wxT("%s: found= %i, id=%i, meas_angle=%i, meas_r= %i"), m_ri->m_name, found, m_target_id, measured_pol.angle,
           measured_pol.r);
  LOG_ARPA(wxT("%s: found= %i, id=%i, pos_angle=%i, pos_r= %i"), m_ri->m_name, found, m_target_id, position_pol.angle, position_pol.r);
  int max_a = m_max_angle.angle;
  int min_a = m_min_angle.angle;
  int max_r = m_max_r.r;
  int min_r = m_min_r.r;
  if ((max_a - min_a) < 0 || (max_a - min_a) > 200 || (max_r - min_r) < 0 || (max_r - min_r) > 200) LOG_ARPA(wxT("$$$$2"));
  LOG_ARPA(wxT("$$$0 m_max_angle=%i, m_min_angle=%i, m_max_r=%i, m_min_r=%i"), 
    m_max_angle.angle, m_min_angle.angle, m_max_r.r, m_min_r.r);

  m_radar_position = m_ri->m_history[MOD_SPOKES(m_ri, measured_pol.angle)].pos;
  // update radar position from estimated spoke position to found spoke position

  int dist_angle = (int)((predicted_pol.angle - measured_pol.angle) * ((double)predicted_pol.r / 326.));  // $$$
  int dist_radial = predicted_pol.r - measured_pol.r;                                                     // $$$
  int dist_total = (int)sqrt(double(dist_angle * dist_angle + dist_radial * dist_radial));  // $$$ delete these lines ??? only needed for diagnostics

  if (found) {
    LOG_ARPA(wxT("%s: id=%i, Found dist_angle=%i, dist_radial= %i, dist_total= %i, m_target_id=%i, predicted_pol.angle=%i, "
                 "measured_pol.angle=%i, doppler=%i"),
             m_ri->m_name, m_target_id, dist_angle, dist_radial, dist_total, m_target_id, predicted_pol.angle, measured_pol.angle,
             m_target_doppler);

    PixelCounter(m_ri);
    //StateTransition(m_ri ,&measured_pol);
    
    if (m_average_contour_length != 0 && pass <= LAST_PASS - 1 &&   //// $$$ don't accept big changes in contour
        (m_contour_length < m_average_contour_length / 2 || m_contour_length > m_average_contour_length * 2)) {
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

  if (found) {
    m_refresh_time = m_ri->m_history[MOD_SPOKES(m_ri, measured_pol.angle)].time;
    m_position.time = m_refresh_time;
    LOG_ARPA(wxT("set refresh time %u"), m_position.time.GetLo());
    
    ResetPixels(m_ri);
    LOG_ARPA(wxT(" target Found ResetPixels m_target_id=%i, angle=%i, r= %i, contour=%i, radar=%s, pass=%i, doppler=%i"),
             m_target_id, measured_pol.angle, measured_pol.r, m_contour_length, m_ri->m_name.c_str(), pass, m_target_doppler);
    int max = MAX_CONTOUR_LENGTH_USED;
    // if (m_contour_length >= max - 1) //LOG_ARPA(wxT(" large contour=%i"), m_contour_length);
    //  target too large? (land masses?) get rid of it
    if (m_contour_length >= max - 1) {
      // don't use this blob, could be radar interference
      // The pixels of the blob have been reset, so you won't find it again
      found = false;
    }
    LOG_ARPA(wxT("%s: target Found m_target_id=%i, angle=%i, r= %i, contour=%i, pass=%i"), m_ri->m_name, m_target_id,
             measured_pol.angle, measured_pol.r, m_contour_length, pass);
  }

  if (found) {
    m_lost_count = 0;
    LOG_ARPA(wxT("%s: p_own lat= %f, lon= %f, m_target_id=%i,"), m_ri->m_name, m_radar_position.lat, m_radar_position.lon, m_target_id);
    if (m_status == ACQUIRE0) {
      // as this is the first measurement, move target to measured position
      m_position.pos = Polar2Pos(m_ri, measured_pol, m_radar_position);  // using own ship location from the time of reception, only lat and lon
      m_position.dlat_dt = 0.;  // $$$ better calculate these
      m_position.dlon_dt = 0.;
      m_position.sd_speed_kn = 0.;
      LOG_ARPA(wxT("%s: calculated pos(%f,%f), m_target_id=%i,"), m_ri->m_name, m_position.pos.lat, m_position.pos.lon,
               m_target_id);
    }
    m_status++;
    m_ri = m_ri; // Save radar that was used for the contour

    // Kalman filter to  calculate the apostriori local position and speed based on measured position (pol)
    // For the Kalman filter a local coordinate system in meters is used with radar_position as origin;
    // Set local_pos to the measured location of the target
    
    if (m_status > 1) {
      m_kalman.Update_P(); // ***********************************************************************
      m_kalman.SetMeasurement(m_ri, &measured_pol, &predicted_local,
                              &predicted_pol);  // pol is measured position in polar coordinates, result in predicted_local
    } // ********************************************************************************************
    measured_pol.time = m_ri->m_history[MOD_SPOKES(m_ri, measured_pol.angle)].time;
    // predicted_local expected position in local coordinates
    m_position.time = m_ri->m_history[MOD_SPOKES(m_ri, measured_pol.angle)].time;
      // set the target time to the newly found time, this is the time the spoke was received
    double long_correction = cos(deg2rad(m_radar_position.lat));

    if (m_status != ACQUIRE1) {
      // if status == 1, then this was first measurement, keep position at measured position
      m_position.pos.lat = m_radar_position.lat + predicted_local.lat / 60. / 1852.;
      m_position.pos.lon = m_radar_position.lon + predicted_local.lon / 60. / 1852. / long_correction;
      m_position.dlat_dt = predicted_local.dlat_dt;  // meters / sec
      m_position.dlon_dt = predicted_local.dlon_dt;  // meters /sec   no correction for lattidude needed, already in m/sec
      m_position.sd_speed_kn = predicted_local.sd_speed_m_s * 3600. / 1852.;
    } else {   // status == 1
      (m_position.pos) = Polar2Pos(m_ri, measured_pol, m_radar_position);  // here m_position gets updated when a target is found
      m_position.dlat_dt = 0.;
      m_position.dlon_dt = 0.;
      m_position.speed_kn = 0;
      m_position.time = m_refresh_time;
    }
    Polar previous_position_pol = position_pol;
    position_pol = Pos2Polar(m_ri, m_position.pos, m_radar_position);  // Set polar to new position

#define FORCED_POSITION_STATUS 8
    // Here we bypass the Kalman filter to predict the speed of the target
    // Kalman filter is too slow to adjust to the speed of (fast) new targets
    // This method however only works for targets where the accuricy of the position is high,
    // that is small targets in relation to the size of the target.

    //if (m_status == 2) {  // determine if this is a small and fast target
    //  int dist_angle = measured_pol.angle - previous_position_pol.angle;
    //  int dist_r = measured_pol.r - previous_position_pol.r;
    //  int size_angle = MOD_SPOKES(m_ri, m_max_angle.angle - m_min_angle.angle);
    //  int size_r = m_max_r.r - m_min_r.r;
    //  if (size_r == 0) size_r = 1;
    //  if (size_angle == 0) size_angle = 1;
    //  double test = abs((double)dist_r / (double)size_r) + abs((double)dist_angle / (double)size_angle);
    //  LOG_ARPA(wxT("%s smallandfast, id=%i, status=%i, test=%f, dist_r=%i, size_r=%i, dist_angle=%i, size_angle=%i"), m_ri->m_name,
    //           m_target_id, m_status, test, dist_r, size_r, dist_angle, size_angle);
    //  if (test > 2.) {
    //    m_small_fast = true;
    //  }
    //  m_small_fast = true;  // $$$ all targets satisfy
    //}
    if (m_status >= 2 && m_status < FORCED_POSITION_STATUS && (m_status < 5 || m_position.speed_kn > 10.) /*&& m_small_fast*/) {
      GeoPosition prev_pos = previous_position.pos;
      GeoPosition new_pos = Polar2Pos(m_ri, measured_pol, m_radar_position);
      double delta_lat = new_pos.lat - prev_pos.lat;
      double delta_lon = new_pos.lon - prev_pos.lon;
      LOG_ARPA(wxT("Forced now= %u, measured= %u, previous= %u"), now.GetLo(), measured_pol.time.GetLo(), previous_position.time.GetLo());
      int delta_t = (measured_pol.time - previous_position.time).GetLo();
      if (delta_t > 1000) {  // delta_t < 1000; speed unreliable due to uncertainties in location
        double d_lat_dt = (delta_lat / (double)(delta_t)) * 60. * 1852. * 1000.;  // convert degrees/milli to meters/sec
        double d_lon_dt = (delta_lon / (double)(delta_t)) * cos(deg2rad(new_pos.lat)) * 60. * 1852. * 1000.;
        LOG_ARPA(wxT("%s, id=%i, FORCED m_status=%i, d_lat_dt=%f, d_lon_dt=%f, delta_lon_meter=%f, delta_lat_meter=%f, deltat=%u"), m_ri->m_name, m_target_id, m_status, d_lat_dt, d_lon_dt, delta_lon * 60 * 1852., delta_lat * 60 * 1852.,
          delta_t);

        // force new position and speed, dependent of overridefactor
        double factor = .8;
        factor = pow((factor), m_status - 1);
        m_position.pos.lat = m_position.pos.lat + factor * (new_pos.lat - m_position.pos.lat);
        m_position.pos.lon = m_position.pos.lon + factor * (new_pos.lon - m_position.pos.lon);
        m_position.dlat_dt = m_position.dlat_dt + factor * (d_lat_dt - m_position.dlat_dt);  // in meters/sec
        m_position.dlon_dt = m_position.dlon_dt + factor * (d_lon_dt - m_position.dlon_dt);  // in meters/sec
      }
    }
    
    if (m_status >= 1) {
      double s1 = m_position.dlat_dt;                                   // m per second
      double s2 = m_position.dlon_dt;                                   // m  per second
      double speed_m_sec = sqrt(s1 * s1 + s2 * s2);
      m_position.speed_kn = speed_m_sec * 3600. / 1852.;  // and convert to nautical miles per hour
      if (speed_m_sec > 200.) LOG_ARPA(wxT("$$$ far too fast"));
      if (speed_m_sec > MAX_DETECTION_SPEED * 1.5) {
        SetStatusLost();
        LOG_ARPA(wxT("$$$ too fast lost speed-m/sec=%f"), speed_m_sec);
        return;
      }
      double course = rad2deg(atan2(s2, s1));
      double turn = course - m_course;
      if (turn > 180.) {
        turn -= 360.;
      }
      if (turn < -180.) {
        turn += 360.;
      }
      turn = abs(turn);
      if (turn > 130. && speed_m_sec > 5. && m_status < 5 && m_course != 0.) {
      // a real target can not turn at this speed
        SetStatusLost();
        LOG_ARPA(wxT("$$$ Too fast turning, speed_m_sec=%f, turn=%f"), speed_m_sec, turn);
        return;
      }
      m_course = course;
      if (m_course < -0.001) m_course += 360.;
      m_previous_contour_length = m_contour_length;
      // send target data to OCPN and other radar
      

      if (m_status > FORCED_POSITION_STATUS) {
        position_pol = Pos2Polar(m_ri, m_position.pos, m_radar_position);
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
        if (m_target_id == 0) {
          m_target_id = m_pi->MakeNewTargetId();
        }
        PassAIVDMtoOCPN();  // status s not used
        position_pol = Pos2Polar(m_ri, m_position.pos, m_radar_position);
        PassTTMtoOCPN(&position_pol, T);  // T indicates status Active
      }
    }
    m_refreshed = FOUND;
    return;
  }

  else {  // target not found
    LOG_ARPA(wxT("%s: Not found m_target_id=%i, angle=%i, r= %i, pass=%i, lost_count=%i, status=%i"), 
      m_ri->m_name, m_target_id, position_pol.angle, position_pol.r, pass, m_lost_count, m_status);
    // not found in pass 0 or 1 (An other chance will follow)
    // try again later in next pass with a larger distance
    if (pass < LAST_PASS) {
      // LOG_ARPA(wxT(" NOT FOUND IN PASS=%i"), pass);
      // reset what we have done
      
      m_refresh_time = prev_refresh;
      m_position = previous_position;
    }
    if (m_small_fast && pass == LAST_PASS - 1 && m_status == 2) {  // status 2, as it was not found,status was not increased.
      // small and fast targets MUST be found in the third sweep, and on a small distance, that is in pass 1.
      LOG_ARPA(wxT(" %s smallandfast set lost id=%i"), m_ri->m_name, m_target_id);
      SetStatusLost();
      LOG_ARPA(wxT("%s: Lost, m_target_id=%i"), m_ri->m_name, m_target_id);
      m_refreshed = OUT_OF_SCOPE;
      m_ri = 0;
      return;
    }

    // delete low status targets immediately when not found
    if ((m_status <= 3 && pass == LAST_PASS) || m_status == 0) {
       /*LOG_ARPA(wxT("%s: low status deleted m_target_id=%i, angle=%i, r= %i, pass=%i, lost_count=%i"), m_ri->m_name,
         m_target_id, pol.angle, pol.r, pass, m_lost_count);*/
      if (m_status == 0 && m_contour_length < 500 && position_pol.r < 950 && m_ri == m_pi->m_radar[1] && m_min_r.r > 20) 
        LOG_ARPA(wxT("$$$ not found status == 0"));
      SetStatusLost();
      
      LOG_ARPA(wxT("%s: Lost, m_target_id=%i"), m_ri->m_name, m_target_id);
      m_refreshed = OUT_OF_SCOPE;
      return;
    }
    if (pass == LAST_PASS) {
      m_lost_count++;
    }

    // delete if not found too often
    if (m_lost_count > MAX_LOST_COUNT) {
      SetStatusLost();
      LOG_ARPA(wxT("%s: Lost, m_target_id=%i"), m_ri->m_name, m_target_id);
      m_refreshed = OUT_OF_SCOPE;
      return;
    }
    m_refreshed = NOT_FOUND;
  }
  return;
}  // end of target not found

  void ArpaTarget::PixelCounter(RadarInfo* ri) {
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
        byte = ri->m_history[MOD_SPOKES(ri, m_contour[i].angle)].line[radius];
        bit0 = (byte & 0x80) >> 7;  // above threshold bit
                                    // bit1 = (byte & 0x40) >> 6;  // backup bit does not get cleared when target is refreshed
        bit2 = (byte & 0x20) >> 5;  // this is Doppler approaching bit
        bit3 = (byte & 0x10) >> 4;  // this is Doppler receding bit
        m_total_pix += bit0;
        m_approaching_pix += bit2;
        m_receding_pix += bit3;
        radius++;
        if (radius >= 1023) continue;
      } while (ri->m_history[MOD_SPOKES(ri, m_contour[i].angle)].line[radius] >> 7);
    }
    LOG_ARPA(wxT("PixelCounter m_total_pix=%i, m_approaching_pix=%i, m_receiding_pix=%i"), m_total_pix, m_approaching_pix,
             m_receding_pix);
  }

  // Check doppler state of targets if Doppler is on
  void ArpaTarget::StateTransition(RadarInfo* ri, Polar * polar) {
    if (ri->m_doppler.GetValue() == 0 || m_target_doppler == ANY_PLUS) {
      LOG_ARPA(wxT("Check Doppler returned id=%i, doppler=%i"), m_target_id, m_target_doppler);
      return;
    }

    uint32_t check_to_doppler = (uint32_t)(0.85 * (double)m_total_pix);
    uint32_t check_to_any = (uint32_t)(0.80 * (double)m_total_pix);
    uint32_t check_not_approaching = (uint32_t)(0.80 * (double)(m_total_pix - m_approaching_pix));
    uint32_t check_not_receding = (uint32_t)(0.80 * (double)(m_total_pix - m_receding_pix));
    LOG_ARPA(wxT("Check Doppler id=%i, check_to_doppler=%i, check_to_any=%i, check_not_approaching=%i, check_not_receding=%i"),
             m_target_id, check_to_doppler, check_to_any, check_not_approaching, check_not_receding);
    switch (m_target_doppler) {
      case ANY_DOPPLER:
      case ANY:
        // convert to APPROACHING or RECEDING
        if (m_approaching_pix > m_receding_pix && m_approaching_pix > check_to_doppler) {
          m_target_doppler = APPROACHING;
          LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to APPROACHING"), m_target_id);
          return;
        } else if (m_receding_pix > m_approaching_pix && m_receding_pix > check_to_doppler) {
          m_target_doppler = RECEDING;
          LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to RECEDING"), m_target_id);
          return;
        } else if (m_target_doppler == ANY_DOPPLER) {
          m_target_doppler = ANY;
          LOG_ARPA(wxT("Doppler converted id=%i ANY_DOPPLER to ANY"), m_target_id);
          return;
        }
        return;

      case RECEDING:
        if (m_receding_pix < check_not_approaching /*check_to_any*/) {
          m_target_doppler = ANY;
          LOG_ARPA(wxT("Doppler converted id=%i RECEIDING to ANY"), m_target_id);
          return;
        }
        return;

      case APPROACHING:
        if (m_approaching_pix < check_not_receding /*check_to_any*/) {
          m_target_doppler = ANY;
          LOG_ARPA(wxT("Doppler converted id=%i APPROACHING to ANY"), m_target_id);
          return;
        }
        return;
      default:
        return;
    }
  }  

#define PIX(aa, rr)                                       \
  if (rr >= (int)ri->m_spoke_len_max - 1) continue;       \
  if (m_arpa->MultiPix(ri, aa, rr, m_target_doppler)) {   \
    pol->angle = aa;                                      \
    pol->r = rr;                                          \
    return true;                                          \
  }

  bool ArpaTarget::FindNearestContour(RadarInfo* ri, Polar * pol, int dist) {
    // make a search pattern along a square
    // returns the position of a point on contour of the nearest blob found in pol
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

  // General target refresh
  bool ArpaTarget::GetTarget(RadarInfo* ri, Polar predicted_pol, Polar * measured_pol, int dist1) {
    Polar pol = predicted_pol;
    *measured_pol = pol;   // in case not found measured pol = predicted-pol
    bool contour_found = false;
    int dist = dist1;
    int backup_contour_length = m_contour_length;
    LOG_ARPA(wxT(" GetTarget m_target_id=%i, m_status= %i, dist= %i, angle= %i, r= %i, lostcount=%i"), m_target_id, m_status, dist, pol.angle, pol.r, m_lost_count);
    if (dist > pol.r - 5) {
      dist = pol.r - 5;  // don't search close to origin
    }

    int a = pol.angle;
    int r = pol.r;
    if (Pix(ri, a, r)) {
      contour_found = m_arpa->FindContourFromInside(ri, &pol, ANY);
    } else {
      LOG_ARPA(wxT("$$$q not inside a=%i, r=%i"), a, r);
      contour_found = FindNearestContour(ri, &pol, dist);
    }
    if (!contour_found) {
      m_contour_length = backup_contour_length;
      LOG_ARPA(wxT("contour not found, pol.angle=%i, pol.r=%i"), pol.angle, pol.r);
      return false;
    }
    int cont = GetContour(ri, &pol);
    if (cont != 0) {
      // LOG_ARPA(wxT(" contour not found"));
      LOG_ARPA(wxT("%s: ARPA contour error %d at %d, %d"), ri->m_name, cont, a, r);
      m_contour_length = backup_contour_length;
      return false;
    }
    *measured_pol = pol;
    return true;
  }

  void ArpaTarget::PassAIVDMtoOCPN() {
    wxCriticalSectionLocker lock(m_protect_target_data);
    if (!m_pi->m_settings.AIVDMtoO) return;
    wxString s_TargID, s_Bear_Unit, s_Course_Unit;
    wxString s_speed, s_course, s_Dist_Unit, s_status;
    wxString s_target_name;
    wxString nmea;
    if (m_status == LOST) return;  // AIS has no "status lost" message
    s_Bear_Unit = wxEmptyString;   // Bearing Units  R or empty
    s_Course_Unit = wxT("T");      // Course type R; Realtive T; true
    s_Dist_Unit = wxT("N");        // Speed/Distance Unit K, N, S N= NM/h = Knots
    int mmsi = m_target_id % 1000000;
    double target_lat = m_position.pos.lat;
    double target_lon = m_position.pos.lon;
    wxString result = EncodeAIVDM(mmsi, m_position.speed_kn, m_position.pos.lon, m_position.pos.lat, m_course);
    PushNMEABuffer(result);
  }

  void ArpaTarget::PassTTMtoOCPN(Polar * pol, OCPN_target_status status) {
    wxCriticalSectionLocker lock(m_protect_target_data);
    if (!m_pi->m_settings.TTMtoO) return;
    LOG_ARPA(wxT("Sending TTM to O, target_id= %i"), m_target_id);
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

    if (m_target_doppler == ANY) {
      s_status = wxT("Q");  // yellow
    } else {
      s_status = wxT("T");
    }

    if (!m_ri) {
      LOG_ARPA(wxT("Error sending TTM to O, originating radar not known"));
      return;
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
#define TARGET_DISTANCE_FOR_BLANKING_SHADOW 4000.  // meters. For targets closer shadow will be suppressed

  void ArpaTarget::ResetPixels(RadarInfo* ri) {
    // resets the pixels of the current blob (plus DISTANCE_BETWEEN_TARGETS) so that blob will not be found again in the same sweep
    // We not only reset the blob but all pixels in a radial "square" covering the blob
    int max_a = m_max_angle.angle;
    int min_a = m_min_angle.angle;
    int max_r = m_max_r.r;
    int min_r = m_min_r.r;
    /*if (min_a <= 1550 && 1535 <= max_a  && min_r <= 960 && 940 <= max_r)
    LOG_ARPA(wxT("$$$3 m_max_angle=%i, m_min_angle=%i, m_max_r=%i, m_min_r=%i"), m_max_angle.angle, m_min_angle.angle, m_max_r.r, m_min_r.r);
    if (m_min_angle.angle > m_max_angle.angle)
      wxLogMessage(wxT("$$$$$ m_min_angle.angle=%i, m_max_angle.angle=%i"), m_min_angle.angle, m_max_angle.angle);*/
    for (int r = wxMax(m_min_r.r - DISTANCE_BETWEEN_TARGETS, 0);
         r <= wxMin(m_max_r.r + DISTANCE_BETWEEN_TARGETS, (int)ri->m_spoke_len_max - 1); r++) {
      for (int a = m_min_angle.angle - DISTANCE_BETWEEN_TARGETS; a <= m_max_angle.angle + DISTANCE_BETWEEN_TARGETS; a++) {
        ri->m_history[MOD_SPOKES(ri, a)].line[r] = ri->m_history[MOD_SPOKES(ri, a)].line[r] & 0x40;  // also clear both Doppler bits
      }
    }
    double distance_to_radar = m_polar_pos.r / ri->m_pixels_per_meter;
    // For larger targets clear the "shadow" of the target until 2 * r
    if (m_contour_length > 80 && distance_to_radar < TARGET_DISTANCE_FOR_BLANKING_SHADOW) {  // $$$ remove false to enable shadow
      LOG_ARPA(wxT("%s: Shadow cleared for target id=%i, m_min_angle.angle=%i, m_max_angle.angle=%i, m_max_r.r=%i "), 
        ri->m_name, m_target_id, m_min_angle.angle, m_max_angle.angle, m_max_r.r, distance_to_radar);
      int max = m_max_angle.angle;
      if (m_min_angle.angle - SHADOW_MARGIN > max + SHADOW_MARGIN) {
        max += ri->m_spokes;
      }
      for (int a = m_min_angle.angle - SHADOW_MARGIN; a <= max + SHADOW_MARGIN; a++) {
        for (uint16_t radius = m_max_r.r; radius < 2 * m_max_r.r && radius < ri->m_spoke_len_max; radius++) {
          ri->m_history[MOD_SPOKES(ri, a)].line[radius] = ri->m_history[MOD_SPOKES(ri, a)].line[radius] & 0x40;
        }
      }
    }
  }

  Arpa::Arpa(radar_pi * pi) {
    m_pi = pi;
    m_clear_contours = false;
    CLEAR_STRUCT(m_doppler_arpa_update_time);
  }

  Arpa::~Arpa() { m_targets.clear(); }

  bool Arpa::MultiPix(RadarInfo* ri, int ang, int rad, Doppler doppler) {
    // checks the blob has a contour of at least m_min_contour_length pixels
    // ang and rad must start on the contour of the blob
    // false if not
    // if false clears out pixels of the blob in hist
    //    wxCriticalSectionLocker lock(ArpaTarget::m_ri->m_exclusive);  // $$$ ??
    int length = ri->m_min_contour_length;
    Polar start;
    start.angle = ang;
    start.r = rad;

    if (!Pix(ri, start.angle, start.r, doppler)) {
      //LOG_ARPA(wxT("$$$ multipix not on contour ang=%i, rad=%i"), ang, rad);
      return false;
    }
    //LOG_ARPA(wxT("$$$ Multipix on contour ang=%i, rad=%i"), ang, rad);
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
    min_angle = current;
    if (start.r >= (int)ri->m_spoke_len_max) {
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
      succes = !Pix(ri, aa, rr, doppler);
      if (succes) break;
    }
    if (!succes) {
      LOG_ARPA(wxT("$$$"));
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
        succes = Pix(ri, aa, rr, doppler);
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
      min_angle.angle += ri->m_spokes;
      max_angle.angle += ri->m_spokes;
    }
    LOG_ARPA(wxT("$$$ multipix erase min_angle.angle=%i, max_angle.angle=%i, min_r.r=%i, max_r.r=%i, %s"), 
      min_angle.angle, max_angle.angle, min_r.r, max_r.r, ri->m_name);
    for (int a = min_angle.angle; a <= max_angle.angle; a++) {
      for (int r = min_r.r; r <= max_r.r; r++) {
        {
          ri->m_history[MOD_SPOKES(ri, a)].line[r] &= 63;  // 0x3F
        }
      }
    }
    return false;
  }


void Arpa::AcquireNewMARPATarget(RadarInfo* ri, ExtendedPosition target_pos) { 
  AcquireOrDeleteMarpaTarget(target_pos, ACQUIRE0); }

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
    LOG_ARPA(wxT(" Deleting (M)ARPA target at position (%f,%f) which is %f meters from (%f,%f)"),
             m_targets[del_target]->m_position.pos.lat, m_targets[del_target]->m_position.pos.lon, min_dist, pos.lat, pos.lon);
    m_targets.erase(m_targets.begin() + del_target);
  } else {
    LOG_ARPA(wxT(" Could not find (M)ARPA target to delete within %f meters from (%f,%f)"),  min_dist, pos.lat,
             pos.lon);
  }
}

void Arpa::AcquireOrDeleteMarpaTarget(ExtendedPosition target_pos, int status) {
  // acquires new target from mouse click position
  // no contour taken yet
  // target status 0
  // returns in X metric coordinates of click
  // constructs Kalman filter
  // make new target
  
  LOG_ARPA(wxT(" Adding (M)ARPA target at position %f / %f"),  target_pos.pos.lat, target_pos.pos.lon);
#ifdef __WXMSW__
  std::unique_ptr<ArpaTarget> target = std::make_unique<ArpaTarget>(m_pi, this, 0);
  #else
  std::unique_ptr<ArpaTarget> target = make_unique<ArpaTarget>(m_pi, this, 0);
 #endif
  target->m_position = target_pos;  // Expected position
  target->m_status = status;
  target->m_target_doppler = ANY;
  target->m_automatic = false;
  m_targets.push_back(std::move(target));
}

void Arpa::DrawContour(ArpaTarget* target) {
  wxCriticalSectionLocker lock(target->m_protect_target_data);
  if (target->m_lost_count > 0) {  // $$$ is dit goed?
    return;  // don't draw targets that were not seen last sweep
  }
  RadarInfo* radar = target->m_ri;
  if (radar == NULL) return;
 
  wxColor arpa = m_pi->m_settings.arpa_colour;
  glColor4ub(arpa.Red(), arpa.Green(), arpa.Blue(), arpa.Alpha());
  glLineWidth(3.0);

  glEnableClientState(GL_VERTEX_ARRAY);

  Point vertex_array[MAX_CONTOUR_LENGTH + 1];
  for (int i = 0; i < target->m_contour_length; i++) {
    int angle = target->m_contour[i].angle + (DEGREES_PER_ROTATION + OPENGL_ROTATION) * radar->m_spokes / DEGREES_PER_ROTATION;
    int radius = target->m_contour[i].r;
    if (radius <= 0 || radius >= (int)radar->m_spoke_len_max) {
      LOG_INFO(wxT("wrong values in DrawContour"));
      return;
    }
    vertex_array[i] = radar->m_polar_lookup->GetPoint(angle, radius);
    vertex_array[i].x = vertex_array[i].x / radar->m_pixels_per_meter;
    vertex_array[i].y = vertex_array[i].y / radar->m_pixels_per_meter;
  }
  glVertexPointer(2, GL_FLOAT, 0, vertex_array);
  glDrawArrays(GL_LINE_STRIP, 0, target->m_contour_length);
  glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
}

void Arpa::DrawArpaTargetsOverlay(double scale, double arpa_rotate) {
  wxPoint boat_center;
  RadarInfo* radar;
  if (m_pi->m_radar[0] != NULL && m_pi->m_radar[0]->m_state.GetValue() == RADAR_TRANSMIT) {
    radar = m_pi->m_radar[0];
  } else {
    if (m_pi->m_radar[1] != NULL && m_pi->m_radar[1]->m_state.GetValue() == RADAR_TRANSMIT) {
      radar = m_pi->m_radar[1];
    } else {
      return;
    }
  }   
  // Here we assume all radars have the same location and number of spokes
     // this is not the case for Raymarine, there number of spokes varies with range
     // TODO: make contours independent of radar, replace polar coordinates with cartesian coordinates

  if (!m_pi->m_settings.drawing_method) {  // Vertex drawing method
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status == LOST || (*target)->m_contour_length == 0) {
        continue;
      }
      double poslat = (*target)->m_radar_position.lat;
      double poslon = (*target)->m_radar_position.lon;
      // some additional logging, to be removed later
      if (poslat > 90. || poslat < -90. || poslon > 180. || poslon < -180.) {
        LOG_INFO(wxT("**error wrong target pos, id=%i, status=%i, poslat = %f, poslon = %f"), (*target)->m_target_id,
                 (*target)->m_status, poslat, poslon);
        continue;
      }

      GetCanvasPixLL(m_pi->m_vp, &boat_center, (*target)->m_radar_position.lat, (*target)->m_radar_position.lon);
      glPushMatrix();
      glTranslated(boat_center.x, boat_center.y, 0);
      glRotated(arpa_rotate, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);
      DrawContour(target->get());
      glPopMatrix();
    }
  } else {  // Shader drawing method
    GeoPosition radar_pos;
    radar->GetRadarPosition(&radar_pos);
    GetCanvasPixLL(m_pi->m_vp, &boat_center, radar_pos.lat, radar_pos.lon);
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

void Arpa::DrawArpaTargetsPanel(RadarInfo* ri, double scale, double arpa_rotate) {
  wxPoint boat_center;
  GeoPosition radar_pos, target_pos;
  double offset_lat = 0.;
  double offset_lon = 0.;

  if (!m_pi->m_settings.drawing_method) {  // this is Vertex drawing method
    for (auto target = m_targets.cbegin(); target != m_targets.cend(); target++) {
      if ((*target)->m_status == LOST) {
        continue;
      }
      if (ri != (*target)->m_ri) {  // target does not fit this radar, last refresh with other radar
        continue;
      }
      radar_pos = (*target)->m_radar_position;
      target_pos = (*target)->m_position.pos;
      offset_lat =
          (radar_pos.lat - target_pos.lat) * 60. * 1852. * ri->m_panel_zoom / 
        ri->m_range.GetValue();
      offset_lon = (radar_pos.lon - target_pos.lon) * 60. * 1852. * cos(deg2rad(target_pos.lat)) * 
        ri->m_panel_zoom / ri->m_range.GetValue();
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

 // remove targets with status LOST and set remaining NOT_FOUND
void Arpa::CleanUpLostTargets() {
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
  LOG_ARPA(wxT("\n\n\n\n: ********************************refresh loop start m_targets.size=%i "),
           m_targets.size());
  CleanUpLostTargets();
  sort(m_targets.begin(), m_targets.end(), SortTargetStatus);
  // main target refresh loop

  // pass 0 of target refresh  Only search for moving targets faster than 2 knots (? $$$)
  
  double speed = MAX_DETECTION_SPEED;
  
  LOG_ARPA(wxT("Search speed =%f"), speed);
  // First refresh the moving targets
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    if (((*target)->m_position.speed_kn >= 2.5 && (*target)->m_status >= STATUS_TO_OCPN )) {
      (*target)->RefreshTarget(speed / 4, 0);  // was 5
    }
  }

  // Pass 1 of target refresh
  // Secondly refresh all targets with a slightly larger margin
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    (*target)->RefreshTarget(speed / 3, 1);
  }

  // pass 2 of target refresh
  for (auto target = m_targets.begin(); target != m_targets.end(); ++target) {
    (*target)->RefreshTarget(speed, 2);
  }
  LOG_ARPA(wxT(" ********************************refresh loop end m_targets.size=%i \n\n\n"),
           m_targets.size());
}



void Arpa::CalculateCentroid(ArpaTarget* target) {
  // real calculation still to be done
}

void Arpa::DeleteAllTargets() { m_targets.clear(); }

bool Arpa::AcquireNewARPATarget(RadarInfo* ri, Polar pol, int status, Doppler doppler) {  // $$$ todo improve, without radar?
  // acquires new target at polar position pol
  // no contour taken yet
  // target status status, normally 0, if dummy target to delete a target -2
  // constructs Kalman filter
  ExtendedPosition own_pos;
  Doppler doppl = doppler;
  LOG_ARPA(wxT("Enter AcquireNewARPATarget, pol r= %i, angle=%i, status= %i"), pol.r, pol.angle, status);
  if (!ri->GetRadarPosition(&own_pos.pos)) {
    return false;
  }

  // make new target
  
#ifdef __WXMSW__
  std::unique_ptr<ArpaTarget> target = std::make_unique<ArpaTarget>(m_pi, m_pi->m_arpa, 0);
  #else
  std::unique_ptr<ArpaTarget> target = make_unique<ArpaTarget>(m_pi, m_pi->m_arpa, 0);
  #endif
  target->m_ri = ri;
  target->m_polar_pos = pol;
  target->m_target_doppler = doppl;
  target->m_position.pos = target->Polar2Pos(ri, pol, own_pos.pos);
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
  target->m_refreshed = NOT_FOUND;
  target->m_automatic = true;
  target->RefreshTarget(MAX_DETECTION_SPEED, 1);  // speed, pass
  m_targets.push_back(std::move(target));
  return true;
}

void Arpa::ClearContours() { m_clear_contours = true; }


bool Arpa::IsAtLeastOneRadarTransmitting() {
  for (size_t r = 0; r < RADARS; r++) {
    if (m_pi->m_radar[r] != NULL && m_pi->m_radar[r]->m_state.GetValue() == RADAR_TRANSMIT) {
      return true;
    }
  }
  return false;
}

void Arpa::SearchDopplerTargets(RadarInfo* ri) {
  ExtendedPosition own_pos;

  if (!m_pi->m_settings.show                       // No radar shown
      || m_pi->GetHeadingSource() == HEADING_NONE  // No heading
      || (m_pi->GetHeadingSource() == HEADING_FIX_HDM && m_pi->m_var_source == VARIATION_SOURCE_NONE)) {
    return;
  }

  if (!IsAtLeastOneRadarTransmitting()) {
    return;
  }

  size_t range_start = 20;  // Convert from meters to 0..511
  size_t range_end;
  int outer_limit = ri->m_spoke_len_max;
    
  outer_limit = (int)outer_limit * 0.93;
  range_end = outer_limit;  // Convert from meters to 0..511

  SpokeBearing start_bearing = 0;
  SpokeBearing end_bearing = ri->m_spokes;

  // loop with +2 increments as target must be larger than 2 pixels in width
  for (int angleIter = start_bearing; angleIter < end_bearing; angleIter += 2) {
    SpokeBearing angle = MOD_SPOKES(ri, angleIter);
    wxLongLong angle_time = ri->m_history[angle].time;
    // angle_time_plus_margin must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
    wxLongLong angle_time_plus_margin = ri->m_history[MOD_SPOKES(ri, angle + 3 * SCAN_MARGIN)].time;

    // check if target has been refreshed since last time
    // and if the beam has passed the target location with SCAN_MARGIN spokes
    if ((angle_time > (m_doppler_arpa_update_time[angle] + SCAN_MARGIN2) &&
         angle_time_plus_margin >= angle_time)) {  // the beam sould have passed our "angle" AND a
                                                   // point SCANMARGIN further set new refresh time
      m_doppler_arpa_update_time[angle] = angle_time;
      for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {
        if (m_pi->m_arpa->MultiPix(ri, angle, rrr, ANY_DOPPLER)) {
          // pixel found that does not belong to a known target
          Polar pol;
          pol.angle = angle;
          pol.r = rrr;
          if (!m_pi->m_arpa->AcquireNewARPATarget(ri, pol, 0, ANY_DOPPLER)) {
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
