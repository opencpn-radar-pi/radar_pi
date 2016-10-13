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
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

RadarMarpa::RadarMarpa(br24radar_pi* pi, RadarInfo* ri) {
    LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator call"));
  m_ri = ri;
  m_pi = pi;
  for (int i = 0; i < NUMBER_OF_TARGETS; i++){
      m_targets[i].status = lost;
      m_targets[i].contour_length = 0;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator ready"));
}

 RadarMarpa::~RadarMarpa() {
 }

position RadarMarpa::Polar2Pos(polar alfa, position own_ship) {
  // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
    // based on the own ship position own_ship
  position pos;
  pos.lat = own_ship.lat +
            (double)alfa.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                cos(deg2rad(SCALE_RAW_TO_DEGREES2048(alfa.angle))) / 60. / 1852.;
  pos.lon = own_ship.lon +
            (double)alfa.r / (double)RETURNS_PER_LINE * (double)m_ri->m_range_meters *
                sin(deg2rad(SCALE_RAW_TO_DEGREES2048(alfa.angle))) / cos(deg2rad(m_pi->m_ownship_lat)) / 60. / 1852.;
  return pos;
}

polar RadarMarpa::Pos2Polar(position p, position own_ship){
    // converts in a radar image a lat-lon position to angular data
    polar alfa;
    double dif_lat = p.lat;
    dif_lat -= own_ship.lat;
    double dif_lon = (p.lon - own_ship.lon) * cos(deg2rad(own_ship.lat));
    alfa.r = (int) (sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * (double)RETURNS_PER_LINE / (double)m_ri->m_range_meters);
    alfa.angle = (int)((atan2(dif_lon, dif_lat)) * (double) LINES_PER_ROTATION / (2. * PI));
    return alfa;
}

bool RadarMarpa::Pix(int ang, int rad) {
  if (rad < 1 || rad >= RETURNS_PER_LINE - 1) {   // $$$ avoid range ring
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: Pix, r out of bound"));
    return false;
  }
  return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
}

void RadarMarpa::Aquire0NewTarget(position target_pos) {
  // aquires new target from mouse click
    // no contour taken yet
    // target status aquire0
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: enter Aquire0NewTarget"));
    polar targ_pol;
    position own_pos;
    own_pos.lat = m_pi->m_ownship_lat;
    own_pos.lon = m_pi->m_ownship_lon;
  targ_pol = Pos2Polar(target_pos, own_pos);
  int i_target = NextEmptyTarget();
  if (i_target == -1) {
      LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: max targets exceeded "));
      return;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ AquireNewTarget i = %i"), i_target);
  m_targets[i_target].pol = targ_pol;  // set the polar 
  m_targets[i_target].status = aquire0;
  return;
}

bool RadarMarpa::FindContourFromInside(polar *pol){  // moves pol to contour of blob
    // true if success
    //false when failed
    int ang = pol->angle;
    int rad = pol->r;
    if (rad > RETURNS_PER_LINE - 1) {
        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside outside image"));
        return false;
    }
    if (rad < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside close to center"));
        return false;
    }
    if (!(Pix(ang, rad) )) {
    //    LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside outside target ang= %i rad= %i"), ang, rad);
        return false;
    }
    while (Pix(ang, rad)){
        if (ang < pol->angle - MAX_CONTOUR_LENGTH / 2){
    //        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside no target contour found"));
            return false;
        }
        ang--;
    }
    ang++;
 //   LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside contour found angle = %i"), ang);
    pol->angle = ang;
    return true;
}

int RadarMarpa::GetContour(polar* p, MarpaTarget* target) {
  // p must start on the contour of the blob
  // follows the contour in a clockwise direction
  
    wxCriticalSectionLocker lock(m_ri->m_exclusive);
    // the 4 possible translations to move from a point on the contour to the next
  polar transl[4] = {0, 1, 1, 0, 0, -1, -1, 0};
  int count = 0;
  polar start = *p;
  polar current = *p;
  int aa;
  int rr;
  bool succes = false;
  int index = 0;
  target->max_r = current;
  target->max_angle = current;
  target->min_r = current;
  target->min_angle = current;
//  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour starting point  angle = %i, r= %i"), current.angle,
//      current.r);
  // check if p inside blob
  if (start.r > RETURNS_PER_LINE - 2) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour r too large"));
    return 1;  // return code 1, r too large
  }
  if (start.r < 4) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting in origin"));
    return 2; // return code 2, r too small
  }
  if (!Pix(start.angle, start.r)) {
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour starting point outside blob angle = %i, r= %i"), current.angle,
             current.r);
    return 3; // return code 3, starting point outside blob
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
    LOG_INFO(wxT("BR24radar_pi:  RadarMarpa::GetContour starting point not on contour angle = %i, r= %i"), current.angle,
             current.r);
    return 4; // return code 4, starting point not on contour
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
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting outside image"));
        return 5; // return code 5, getting outside image
      }
      if (rr < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting in origin"));
        return 6;  //  code 6, getting close to origin
      }
        succes = Pix(aa, rr);   // this is the case where the whole blob is followed
      // but we accept single pixel extensions of the blob

  //    succes = Pix(aa, rr) && (Pix(aa, rr + 1) || Pix(aa, rr - 1)) && (Pix(aa + 1, rr) || Pix(aa - 1, rr));
      // current point has a pixel and next point  is not a "single pixel path"

      if (succes) {
        // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      LOG_INFO(wxT("BR24radar_pi::RadarMarpa::GetContour no next point found"));
      return 7;  // return code 5, no next point found
    }
    // next point found
    current.angle = aa;
    current.r = rr;
    target->contour[count].angle = aa;
    target->contour[count].r = rr;
    count++;
    if (aa > target->max_angle.angle) {
        target->max_angle.angle = aa;
        target->max_angle.r = rr;
    }
    if (aa < target->min_angle.angle) {
        target->min_angle.angle = aa;
        target->min_angle.r = rr;
    }
    if (rr > target->max_r.r) {
        target->max_r.angle = aa;
        target->max_r.r = rr;
    }
    if (rr < target->min_r.r) {
        target->min_r.angle = aa;
        target->min_r.r = rr;
    }
    if (count >= MAX_CONTOUR_LENGTH) {
      // blob too large
      LOG_INFO(wxT("BR24radar_pi: RadarMarpa::GetContour  Blob too large count = %i"), count);
      return 8; // return code 8, Blob too large
    }
  }
  target->contour_length = count;
//  CalculateCentroid(*target);   $$$
  target->pol.angle = (target->max_angle.angle + target->min_angle.angle) / 2;
  target->pol.r = (target->max_angle.r + target->min_angle.r) / 2;
  target->PushLogbook();  // shift all entries
  target->logbook[0].time = m_ri->m_history[MOD_ROTATION2048(target->max_angle.angle)].time;
  position p_own;
  p_own.lat = m_ri->m_history[MOD_ROTATION2048(target->pol.angle)].lat;  // get the position from receive time
  p_own.lon = m_ri->m_history[MOD_ROTATION2048(target->pol.angle)].lon;
  target->logbook[0].pos = Polar2Pos(target->pol, p_own);

  return 0;   //  succes, blob found
}  

int RadarMarpa::NextEmptyTarget() {
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
  LOG_INFO(wxT("BR24radar_pi: $$$ empty target i = %i"), index);
  return index;
}

void RadarMarpa::DrawContour(MarpaTarget target){
    PolarToCartesianLookupTable *polarLookup;
    polarLookup = GetPolarToCartesianLookupTable();
    glColor4ub(40, 40, 100, 250);
    glLineWidth(3.0);
    glBegin(GL_LINES);
 //   LOG_INFO(wxT("BR24radar_pi: $$$ target DrawContour2 contour_length= %i"), target.contour_length);

    for (int i = 0; i < target.contour_length; i++){
        double xx;
        double yy;
   //     LOG_INFO(wxT("BR24radar_pi: $$$ target DrawContour3 i=%i"), i);
        int angle = MOD_ROTATION2048(target.contour[i].angle - 512);
        int radius = target.contour[i].r;
        if (radius < 0){
    //        LOG_INFO(wxT("BR24radar_pi:RadarMarpa::DrawContour r < 0"));
            return;
        }
        xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
        yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
        glVertex2f(xx, yy);
 //       LOG_INFO(wxT("BR24radar_pi: $$$ target DrawContour4 i=%i"), i);
        int ii = i + 1;
        if (ii == target.contour_length) {
      //      LOG_INFO(wxT("BR24radar_pi:R $$$draw start point ii = %i length =%i"), ii, target.contour_length);
            ii = 0;  // start point again
        }
        if (radius < 0){
    //        LOG_INFO(wxT("BR24radar_pi:RadarMarpa::DrawContour r < 0"));
            return;
        }
        angle = MOD_ROTATION2048(target.contour[ii].angle - 512);
        radius = target.contour[ii].r;
    //    LOG_INFO(wxT("BR24radar_pi: $$$ target DrawContour4 i=%i ii = %i"), i, ii);
        xx = polarLookup->x[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
        yy = polarLookup->y[angle][radius] * m_ri->m_range_meters / RETURNS_PER_LINE;
        glVertex2f(xx, yy);
    }
    glEnd();
}

void RadarMarpa::DrawMarpaTargets() {
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status != lost) {
   //     LOG_INFO(wxT("BR24radar_pi: RadarMarpa::DrawMarpaTargets $$$target to draw i = %i status = %i"), i, m_targets[i].status);
      DrawContour(m_targets[i]);
   //   LOG_INFO(wxT("BR24radar_pi: RadarMarpa::DrawMarpaTargets $$$target drawn i = %i status = %i"), i, m_targets[i].status);
    }
  }
}

void RadarMarpa::RefreshMarpaTargets() {
  for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
    if (m_targets[i].status == aquire0) {
      //  new target, no contour yet
      Aquire1NewTarget(&m_targets[i]);
    }
    if (m_targets[i].status != lost) {
      polar p;
      if (m_targets[i].logbook[0].time == m_ri->m_history[MOD_ROTATION2048(m_targets[i].max_angle.angle + OFF_LOCATION)].time) {
        // + OFF_LOCATION because target may have mooved
        continue;
      }
      p = m_targets[i].pol;
      bool contour = FindContourFromInside(&p);

      if (!contour) {
        contour = FindNearestContour(&p, 50);
      }
      if (!contour) {
        LOG_INFO(wxT("BR24radar_pi: $$$ AA refresh target contour NOT found i = %i"), i);
        // $$$ status = lost
        continue;
      }
      int cont = GetContour(&p, &m_targets[i]);
      if (cont != 0) {
        if (cont == 8) {  // blob too large
          LOG_INFO(wxT("BR24radar_pi: $$$ refresh target i = %i blob too large, LOST"), i);
        }
        LOG_INFO(wxT("BR24radar_pi: $$$ refresh target i = %i ERROR"), i);
        m_targets[i].status = lost;
        return;
      }
      // target i refreshed
    }
  }
  return;
}

void RadarMarpa::Aquire1NewTarget(MarpaTarget* target) {
  // input a target with status aquire0
  // will add contour and position
  int ang = target->pol.angle;
  int rad = target->pol.r;
  bool contour_found = FindContourFromInside(&target->pol);
  polar targ_pol = target->pol;
  if (contour_found) {
    //   LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindContourFromInside OK"));
  } else {
    contour_found = FindNearestContour(&targ_pol, 60);
    if (contour_found) {
      LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindNearestContour OK"));
    }
  }
  if (!contour_found) {
    LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget No contour found"));
    target->status = lost;
    return;
  }
 // LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget Voor contour start  target.status= %i, length=%i"), m_targets[0].status,
 //          m_targets[0].contour_length);
  if (GetContour(&targ_pol, target) != 0) {
    
      LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget GetContour FAILED m_targets[i_target].status= %i, length=%i"), target->status,
        target->contour_length);
    target->status = lost;
    return;
  } 
  target->status = aquire1;
  LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget STATUS 1"));
  

  LOG_INFO(wxT("BR24radar_pi: $$$ lat2 = %f"), target->logbook[0].pos.lat);   
  LOG_INFO(wxT("BR24radar_pi: $$$ contour = %i"), target->contour_length);
  
  LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget time =  poslat= %f poslon= %f"), 
      target->logbook[0].pos.lat, target->logbook[0].pos.lon);
  return;
}

#define PIX(aa, rr)                                                                     \
  if (Pix(aa, rr)) {                                                                    \
    p->angle = aa;                                                                      \
    p->r = rr;                                                                          \
    LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour PIX FOUND a= %i r= %i"), aa, rr); \
    return true;                                                                        \
  }

// $$$ to do: no single pixel targets
bool RadarMarpa::FindNearestContour(polar* p, int dist) {
  // make a search pattern along an approximate octagon
  int a = p->angle;
  int r = p->r;
//  LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour Called dist= %i  "), dist);
  LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour Called dist= %i a= %i, r= %i "), dist, a, r);
  for (int i = 1; i < dist; i++) {
    int octa = (int)(i * .414 + .6);  // length of half side of octagon
 //   LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour Octa = %i, i = %i"), octa, i);
    for (int j = -octa; j <= octa; j++) {  // top side
                                           //    LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour j = %i"), j);
      PIX(a + j, r + i);                   // use the symmetry to check equivalent points
      PIX(a + i, r + j);                   // 90 degrees right rotation
      PIX(a + j, r - i);                   // mirror horizontal axis
      PIX(a - i, r + j);                   // mirror vertical axis
    }
    for (int j = 0; j <= octa; j++) {  // 45 degrees side
                                       //    LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour second block j = %i"), j);
      PIX(a + octa + j, r + i - j);
      PIX(a - octa - j, r + i - j);
      PIX(a + octa + j, r - i + j);  // mirror horizontal axis
      PIX(a - octa - j, r - i + j);
    }
  }
  LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour not found dist= %i"), dist);
  return false;
}

//bool RadarMarpa::FindContour(polar* p){
//    return false;
//}

void RadarMarpa::CalculateCentroid(MarpaTarget *target){
   /* target->pol.angle = (target->max_angle.angle + target->min_angle.angle) / 2;
    target->pol.r = (target->max_angle.r + target->min_angle.r) / 2;*/
    // real calculation still to be done
}

void MarpaTarget::PushLogbook(){
    int num = sizeof(LogEntry) * (SIZE_OF_LOG - 1);
    memmove(&logbook[1], logbook, num);
}

PLUGIN_END_NAMESPACE
