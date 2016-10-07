/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin, Marpa partition
 *           Target tracking
 * Authors:  Douwe Fokkema
 *           Kees Verruijt
 *           Håkan Svensson
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

#include "RadarMarpa.h"
//#include "RadarInfo.h"
#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

RadarMarpa::RadarMarpa(br24radar_pi* pi, RadarInfo* ri) {
    LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator call"));
  m_ri = ri;
  m_pi = pi;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++){
      m_targets[i].target_lost = true;
      m_targets[i].contour_length = 0;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator ready"));
}

 RadarMarpa::~RadarMarpa() {
 }

position RadarMarpa::Polar2Pos(polar alfa) {
  // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
  position pos;
  pos.lat = m_pi->m_ownship_lat +
            (double)alfa.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                cos(deg2rad(SCALE_RAW_TO_DEGREES2048(alfa.angle))) / 60. / 1852.;
  pos.lon = m_pi->m_ownship_lon +
            (double)alfa.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                sin(deg2rad(SCALE_RAW_TO_DEGREES2048(alfa.angle))) / cos(deg2rad(m_pi->m_ownship_lat)) / 60. / 1852.;
  return pos;
}

polar RadarMarpa::Pos2Polar(position p){
    // converts in a radar image a lat-lon position to angular data
    polar alfa;
    double dif_lat = p.lat;
     dif_lat  -= m_pi->m_ownship_lat;
    double dif_lon = (p.lon - m_pi->m_ownship_lon) * cos(deg2rad(m_pi->m_ownship_lat));
    alfa.r = (int) (sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * (double)RETURNS_PER_LINE / (double)m_ri->m_range_meters);
    alfa.angle = (int)((atan2(dif_lon, dif_lat)) * (double) LINES_PER_ROTATION / (2. * PI));
    return alfa;
}

int RadarMarpa::AquireNewTarget(position target_pos) {
  // aquires new target with position target_pos
  // returns target number if succesful
  // returns -1 when unsuccesful
  MarpaTarget targ;
  targ.time = wxGetUTCTimeMillis();
  targ.pol = Pos2Polar(target_pos);
  int i_target = NextEmptyTarget();
  if (i_target == -1) {
      LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: max targets exceeded "));
      return 0;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ AquireNewTarget i = %i"), i_target);
  m_targets[i_target] = targ;
  int ang = targ.pol.angle;
  int rad = targ.pol.r;
  GetToContour(&targ.pol);
  if (GetContour(&targ.pol, &m_targets[i_target])){
      m_targets[i_target].target_lost = false;
  }
  return 0;
}

bool RadarMarpa::GetToContour(polar *pol){  // moves pol to contour of blob
    // true if success
    //false when failed
    int ang = pol->angle;
    int rad = pol->r;
    if (rad > RETURNS_PER_LINE - 1) {
        LOG_INFO(wxT("BR24radar_pi: $$$ GetToContour outside image"));
        return false;
    }
    if (rad < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ GetToContour close to center"));
        return false;
    }
    if (!(PIX(ang, rad) )) {
        LOG_INFO(wxT("BR24radar_pi: $$$ target contour outside target ang= %i rad= %i"), ang, rad);
        return false;
    }
    while (PIX(ang, rad)){
        if (ang < pol->angle - MAX_TARGET_SIZE){
            LOG_INFO(wxT("BR24radar_pi: $$$ no target contour found"));
            return false;
            break;
        }
        ang--;
    }
    ang++;
    LOG_INFO(wxT("BR24radar_pi: $$$ target contour found angle = %i"), ang);
    pol->angle = ang;
    return true;
}

bool RadarMarpa::GetContour(polar* p, MarpaTarget* target) {
  // p must start on the contour of the blob
  // follows the contour in a clockwise direction
  // the 4 possible translations to move from a point on the contour to the next
  polar transl[4] = {0, 1, 1, 0, 0, -1, -1, 0};
  int count = 0;
  polar start = *p;
  polar current = *p;
  int aa;
  int rr;
  bool succes = false;
  int index = 0;
  int max_r = current.r;
  int max_a = current.angle;
  int min_r = current.r;
  int min_a = current.angle;
  // check if p inside blob
  if (start.r > RETURNS_PER_LINE - 2) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour r too large"));
    return false;
  }
  if (start.r < 4) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting in origin"));
    return false;
  }
  if (!PIX(start.angle, start.r)) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour starting point outside blob angle = %i, r= %i"), current.angle,
             current.r);
    return false;
  }
  // first find the orientation of border point p
  for (int i = 0; i < 4; i++) {
    index = i;
    aa = current.angle + transl[index].angle;
    rr = current.r + transl[index].r;
    succes = !PIX(aa, rr);
    if (succes) break;
  }
  if (!succes) {
    LOG_INFO(wxT("BR24radar_pi:  RadarMarpa::GetContour starting point not on contour angle = %i, r= %i"), current.angle,
             current.r);
    return false;
  }
  index += 1;  // determines starting direction
  if (index > 3) index -= 4;
  while (current.r != start.r || current.angle != start.angle || count == 0) {
    // try all translations to find the next point
    // start with the "left most" translation relative to the previous one
    // take the "first street to the left"
    index += 3;  // we will turn left all the time if possible

    for (int i = 0; i < 4; i++) {
      if (index > 3) index -= 4;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
      //  succes = PIX(aa, rr);   // this is the case where the whole blob is followed
      // but we neglect single pixel extensions of the blob

      if (rr > RETURNS_PER_LINE - 2) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting outside image"));
        return false;
      }
      if (rr < 1) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting in origin"));
        return false;
      }
      succes = PIX(aa, rr) && (PIX(aa, rr + 1) || PIX(aa, rr - 1)) && (PIX(aa + 1, rr) || PIX(aa - 1, rr));
      if (succes) {
        // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      LOG_INFO(wxT("BR24radar_pi::RadarMarpa::GetContour no next point found"));
      return false;
    }
    // next point found
    current.angle = aa;
    current.r = rr;
    target->contour[count].angle = aa;
    target->contour[count].r = rr;
    count++;
    if (aa > max_a) max_a = aa;
    if (aa < min_a) min_a = aa;
    if (rr > max_r) max_r = rr;
    if (rr < min_r) min_r = rr;
    if (count > MAX_CONTOUR_LENGTH) {
      // blob too large
      LOG_INFO(wxT("BR24radar_pi: RadarMarpa::GetContour  Blob too large"));
      return false;
    }
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour  Blob FOUND AND LOCKED"));
  target->contour_length = count;
  target->pol.angle = (max_a + min_a) / 2;
  target->pol.r = (max_r + min_r) / 2;
  return true;
}

int RadarMarpa::NextEmptyTarget() {
  int index = 0;
  bool hit = false;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].target_lost) {
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

PLUGIN_END_NAMESPACE
