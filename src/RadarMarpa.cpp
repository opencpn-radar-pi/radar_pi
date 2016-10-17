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
#include "RadarInfo.h"
#include "br24radar_pi.h"
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

RadarMarpa::RadarMarpa(br24radar_pi* pi, RadarInfo* ri) {
    LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator call"));
    
    m_ri = ri;
 m_pi = pi;
  m_targets = new MarpaTarget[NUMBER_OF_TARGETS];  
  for (int i = 0; i < NUMBER_OF_TARGETS; i++){
      m_targets[i].set(pi, ri);
      m_targets[i].contour_length = 0;
      m_targets[i].status = lost;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ radarmarpa creator ready"));
}

void MarpaTarget::set(br24radar_pi* pi, RadarInfo* ri){
    m_ri = ri;
    m_pi = pi;

}


 RadarMarpa::~RadarMarpa() {
 }

 

Position MarpaTarget::Polar2Pos(Polar pol, Position own_ship) {
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

Polar RadarMarpa::Pos2Polar(Position p, Position own_ship){
    // converts in a radar image a lat-lon position to angular data
    Polar alfa;
    double dif_lat = p.lat;
    dif_lat -= own_ship.lat;
    double dif_lon = (p.lon - own_ship.lon) * cos(deg2rad(own_ship.lat));
    alfa.r = (int)(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * (double)RETURNS_PER_LINE / (double)m_ri->m_range_meters);
    alfa.angle = (int)((atan2(dif_lon, dif_lat)) * (double)LINES_PER_ROTATION / (2. * PI));
    return alfa;
}

bool MarpaTarget::Pix(int ang, int rad) {
    if (rad < 1 || rad >= RETURNS_PER_LINE - 1) {   // $$$ avoid range ring
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: Pix, r out of bound"));
        return false;
    }
    return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
}

void RadarMarpa::Aquire0NewTarget(Position target_pos) {
  // aquires new target from mouse click
    // no contour taken yet
    // target status aquire0
    LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: enter Aquire0NewTarget"));
    Polar targ_pol;
    Position own_pos;
    own_pos.lat = m_pi->m_ownship_lat;
    own_pos.lon = m_pi->m_ownship_lon;
    targ_pol = Pos2Polar(target_pos, own_pos);
    int i_target = NextEmptyTarget();
  if (i_target == -1) {
      LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa:: max targets exceeded "));
      return;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ AquireNewTarget i = %i"), i_target);
  m_targets[i_target].pol = targ_pol;  // set the Polar 
  m_targets[i_target].status = aquire0;
  return;
}

bool MarpaTarget::FindContourFromInside(){  // moves pol to contour of blob
    // true if success
    //false when failed
    int ang = pol.angle;
    int rad = pol.r;
    if (rad > RETURNS_PER_LINE - 1) {
        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside outside image"));
        return false;
    }
    if (rad < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside close to center"));
        return false;
    }
    if (!(Pix(ang, rad))) {
    //    LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside outside target ang= %i rad= %i"), ang, rad);
        return false;
    }
    while (Pix(ang, rad)){
        if (ang < pol.angle - MAX_CONTOUR_LENGTH / 2){
    //        LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside no target contour found"));
            return false;
        }
        ang--;
    }
    ang++;
 //   LOG_INFO(wxT("BR24radar_pi: $$$ FindContourFromInside contour found angle = %i"), ang);
    pol.angle = ang;
    return true;
}

int MarpaTarget::GetContour() {
  // p must start on the contour of the blob
  // follows the contour in a clockwise direction
  
    wxCriticalSectionLocker lock(MarpaTarget::m_ri->m_exclusive);
    // the 4 possible translations to move from a point on the contour to the next
  Polar transl[4] = {0, 1, 1, 0, 0, -1, -1, 0};   // NB structure polar, not class Polar
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
  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour starting point  angle = %i, r= %i"), current.angle,
      current.r);
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
   /*   LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour start while loop "));
      LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour current.angle = %i, current.r= %i count=%i"), current.angle, current.r, count);*/
    // try all translations to find the next point
    // start with the "left most" translation relative to the previous one
    index += 3;  // we will turn left all the time if possible
    for (int i = 0; i < 4; i++) {
      if (index > 3) index -= 4;
      aa = current.angle + transl[index].angle;
      rr = current.r + transl[index].r;
    //  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour loop aa = %i, rr = %i count=%i"), aa, rr, count);
      if (rr > RETURNS_PER_LINE - 2) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting outside image"));
        return 5; // return code 5, getting outside image
      }
      if (rr < 3) {
        LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour getting in origin"));
        return 6;  //  code 6, getting close to origin
      }
      succes = Pix(aa, rr);    // this is the case where the whole blob is followed
      // but we accept single pixel extensions of the blob
      if (succes) {
        // next point found
        break;
      }
      index += 1;
    }
    if (!succes) {
      LOG_INFO(wxT("BR24radar_pi::RadarMarpa::GetContour no next point found"));
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
      LOG_INFO(wxT("BR24radar_pi: RadarMarpa::GetContour  Blob too large count = %i"), count);
      return 8; // return code 8, Blob too large
    }
  }
  contour_length = count;
//  CalculateCentroid(*target);   $$$
  pol.angle = (max_angle.angle + min_angle.angle) / 2;
  pol.r = (max_angle.r + min_angle.r) / 2;
  PushLogbook();  // shift all entries
  logbook[0].time = MarpaTarget::m_ri->m_history[MOD_ROTATION2048(max_angle.angle)].time;
  Position p_own;
  p_own.lat = MarpaTarget::m_ri->m_history[MOD_ROTATION2048(pol.angle)].lat;  // get the position from receive time
  p_own.lon = MarpaTarget::m_ri->m_history[MOD_ROTATION2048(pol.angle)].lon;
  logbook[0].pos = Polar2Pos(pol, p_own);
  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::GetContour BLOB CONTOUR FOUND"));
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
        LOG_INFO(wxT("BR24radar_pi: $$$ aquir1 called"));
        m_targets[i].Aquire1NewTarget();
    }
    if (m_targets[i].status != lost) {
      if (m_targets[i].logbook[0].time == m_ri->m_history[MOD_ROTATION2048(m_targets[i].max_angle.angle + OFF_LOCATION)].time) {
            // check if target has been refreshed since last time
        // + OFF_LOCATION because target may have mooved
          LOG_INFO(wxT("BR24radar_pi: $$$ RefreshMarpaTargets same pos no refresh next target i=%i "),i);
        continue;
      }
      bool contour = m_targets[i].FindContourFromInside();  //   $$$$ update position first!!

      if (!contour) {
          contour = m_targets[i].FindNearestContour(50);
      }
      if (!contour) {
        LOG_INFO(wxT("BR24radar_pi: $$$ AA refresh target contour NOT found i = %i"), i);
        // $$$ status = lost
        continue;
      }
      int cont = m_targets[i].GetContour();
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

void MarpaTarget::Aquire1NewTarget() {
  // input a target with status aquire0
  // will add contour and position
  int ang = pol.angle;
  int rad = pol.r;
  bool contour_found = FindContourFromInside();
  LOG_INFO(wxT("BR24radar_pi: $$$ RadarMarpa::Aquire1NewTarget CONTOUR FROM INSIDE FOUND"));
  Polar targ_pol = pol;
  if (contour_found) {
       LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindContourFromInside OK"));
  } else {
      LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindContourFromInside NOT OK"));
      contour_found = FindNearestContour(60);
    if (contour_found) {
      LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindNearestContour OK"));
    }
    else{
        LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget FindNearestContour NOT OK"));
    }
  }
  if (!contour_found) {
    LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget No contour found"));
    status = lost;
    return;
  }
  LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget Voor contour start  target.status= %i, length=%i"), status,
           contour_length);
  if (GetContour() != 0) {
    
      LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget GetContour FAILED m_targets[i_target].status= %i, length=%i"), status,
        contour_length);
    status = lost;
    return;
  } 
  status = aquire1;
  LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget STATUS 1"));
  LOG_INFO(wxT("BR24radar_pi: $$$ lat2 = %f"), logbook[0].pos.lat);   
  LOG_INFO(wxT("BR24radar_pi: $$$ contour = %i"), contour_length);
  LOG_INFO(wxT("BR24radar_pi: $$$ Aquire1NewTarget time =  poslat= %f poslon= %f"), 
      logbook[0].pos.lat, logbook[0].pos.lon);
  return;
}

#define PIX(aa, rr)                                                                     \
  if (Pix(aa, rr)) {                                                                    \
    pol.angle = aa;                                                                      \
    pol.r = rr;                                                                          \
    LOG_INFO(wxT("BR24radar_pi: $$$FindNearestContour PIX FOUND a= %i r= %i"), aa, rr); \
    return true;                                                                        \
    }

bool MarpaTarget::FindNearestContour(int dist) {
    // $$$ to do: no single pixel targets
  // make a search pattern along an approximate octagon
    
  int a = pol.angle;
  int r = pol.r;
  
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

//bool RadarMarpa::FindContour(Polar* p){
//    return false;
//}

void RadarMarpa::CalculateCentroid(MarpaTarget *target){
   /* target->pol.angle = (target->max_angle.angle + target->min_angle.angle) / 2;
    target->pol.r = (target->max_angle.r + target->min_angle.r) / 2;*/
    // real calculation still to be done
}

void MarpaTarget::PushLogbook(){
    LOG_INFO(wxT("BR24radar_pi: $$$PushLogbook"));
    int num = sizeof(LogEntry) * (SIZE_OF_LOG - 1);
    memmove(&logbook[1], logbook, num);
}

MarpaTarget::MarpaTarget(br24radar_pi* pi, RadarInfo* ri){
    MarpaTarget::m_ri = ri;
    m_pi = pi;
    status = lost;
    contour_length = 0;
   /* logbook = new LogEntry[SIZE_OF_LOG];
    pol = new Polar();*/
   // contour = new Polar[MAX_CONTOUR_LENGTH + 1];

}

MarpaTarget::MarpaTarget(){
    status = lost;
    contour_length = 0;
   // LogEntry *logbook = new LogEntry[SIZE_OF_LOG];
}
PLUGIN_END_NAMESPACE
