/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *
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
#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

#undef TEST_GUARD_ZONE_LOCATION

void GuardZone::ProcessSpoke(SpokeBearing angle, UINT8* data, UINT8* hist, size_t len, int range) {
    if (!m_alarm_on) {
        ResetBogeys();
        return;
        
    }
//    m_pi->m_guard_bogey_confirmed = false;
  size_t range_start = m_inner_range * RETURNS_PER_LINE / range;  // Convert from meters to 0..511
  size_t range_end = m_outer_range * RETURNS_PER_LINE / range;    // Convert from meters to 0..511
  bool in_guard_zone = false;

  switch (m_type) {
    case GZ_ARC:
      if ((angle >= m_start_bearing && angle < m_end_bearing) ||
          (m_start_bearing >= m_end_bearing && (angle >= m_start_bearing || angle < m_end_bearing))) {
        if (range_start < RETURNS_PER_LINE) {
          if (range_end > RETURNS_PER_LINE) {
            range_end = RETURNS_PER_LINE;
          }

          for (size_t r = range_start; r <= range_end; r++) {
            if (!m_multi_sweep_filter || HISTORY_FILTER_ALLOW(hist[r])) {
              if (data[r] >= m_pi->m_settings.threshold_blue) {
                m_running_count++;
              }
#ifdef TEST_GUARD_ZONE_LOCATION
              // Zap guard zone computation location to green so this is visible on screen
              else {
                data[r] = m_pi->m_settings.threshold_green;
              }
#endif
            }
          }
        }
        in_guard_zone = true;
      }
      break;

    case GZ_CIRCLE:
      if (range_start < RETURNS_PER_LINE) {
        if (range_end > RETURNS_PER_LINE) {
          range_end = RETURNS_PER_LINE;
        }

        for (size_t r = range_start; r <= range_end; r++) {
          if (!m_multi_sweep_filter || HISTORY_FILTER_ALLOW(hist[r])) {
            if (data[r] >= m_pi->m_settings.threshold_blue) {
              m_running_count++;
            }
#ifdef TEST_GUARD_ZONE_LOCATION
            // Zap guard zone computation location to green so this is visible on screen
            else {
              data[r] = m_pi->m_settings.threshold_green;
            }
#endif
          }
        }
        if (angle > m_last_angle) {
          in_guard_zone = true;
        }
      }
      break;

    default:
      in_guard_zone = false;
      break;
  }

  if (m_last_in_guard_zone && !in_guard_zone) {
    // last bearing that could add to m_running_count, so store as bogey_count;
    m_bogey_count = m_running_count;
    m_running_count = 0;
    LOG_GUARD(wxT("%s angle=%d last_angle=%d range=%d guardzone=%d..%d (%d - %d) bogey_count=%d"), m_log_name.c_str(), angle,
              m_last_angle, range, range_start, range_end, m_inner_range, m_outer_range, m_bogey_count);

    // When debugging with a static ship it is hard to find moving targets, so move
    // the guard zone instead. This slowly rotates the guard zone.
    if (m_pi->m_settings.guard_zone_debug_inc && m_type == GZ_ARC) {
      m_start_bearing += LINES_PER_ROTATION - m_pi->m_settings.guard_zone_debug_inc;
      m_end_bearing += LINES_PER_ROTATION - m_pi->m_settings.guard_zone_debug_inc;
      m_start_bearing %= LINES_PER_ROTATION;
      m_end_bearing %= LINES_PER_ROTATION;
    }
  }

  m_last_in_guard_zone = in_guard_zone;
  m_last_angle = angle;
}

// Search  guard zone for targets
void GuardZone::SearchTargets() {
  if (!m_arpa_on) return;
  
  if (m_type == GZ_OFF) {
    return;
  }
  if (m_ri->m_range_meters == 0) {

    return;
  }
  size_t range_start = m_inner_range * RETURNS_PER_LINE / m_ri->m_range_meters;  // Convert from meters to 0..511
  size_t range_end = m_outer_range * RETURNS_PER_LINE / m_ri->m_range_meters;    // Convert from meters to 0..511

  SpokeBearing hdt = SCALE_DEGREES_TO_RAW2048(m_pi->m_hdt);
  SpokeBearing start_bearing = m_start_bearing + hdt;
  SpokeBearing end_bearing = m_end_bearing + hdt;
  start_bearing = MOD_ROTATION2048(start_bearing);
  end_bearing = MOD_ROTATION2048(end_bearing);
  if (start_bearing > end_bearing) {
    end_bearing += LINES_PER_ROTATION;
  }
  if (m_type == GZ_CIRCLE) {
    start_bearing = 0;
    end_bearing = LINES_PER_ROTATION;
  }

  if (range_start < RETURNS_PER_LINE) {
    if (range_end > RETURNS_PER_LINE) {
      range_end = RETURNS_PER_LINE;
    }
    if (range_end < range_start) return;

    for (int angle = start_bearing; angle < end_bearing; angle += 2) {
      // check if this angle has been updated by the beam since last time
      // and if possible targets have been refreshed

      wxLongLong time1 = m_ri->m_history[MOD_ROTATION2048(angle)].time;
      wxLongLong time2 = m_ri->m_history[MOD_ROTATION2048(angle + 2 * SCAN_MARGIN)].time;

      // check if target has been refreshed since last time
      // and if the beam has passed the target location with SCAN_MARGIN spokes
      if ((time1 > (arpa_update_time[MOD_ROTATION2048(angle)] + SCAN_MARGIN2) &&
           time2 >= time1)) {  // the beam sould have passed our "angle" AND a point SCANMARGIN further
        // set new refresh time
          arpa_update_time[MOD_ROTATION2048(angle)] = time1;

        /*LOG_INFO(wxT("BR24radar_pi: $$$ past timecheck angle=%i, m_start_bearing % i, m_end_bearing %i, start bearing %i,
        end_bearing %i"), angle, m_start_bearing,
            m_end_bearing, start_bearing, end_bearing);
        LOG_INFO(wxT("BR24radar_pi: $$$ ARPA refresh time %u"), time1.GetLo());*/

        for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {  // $$$ type size_t

          if (Pix(angle, rrr)) {
            bool next_r = false;
            // check all targets if this pixel is within the area of the target
            for (int i = 0; i < m_ri->m_marpa->number_of_targets; i++) {
              if (!m_ri->m_marpa->m_targets[i]) continue;
              ArpaTarget* t = m_ri->m_marpa->m_targets[i];
              if (t->status == LOST) {
                continue;
              }
              int min_ang = t->min_angle.angle - 1;
              int max_ang = t->max_angle.angle + 1;
              unsigned int tim1 = arpa_update_time[MOD_ROTATION2048(angle)].GetLo();
              /*  LOG_INFO(wxT("BR24radar_pi: $$$ i=%i, angle=%i, r=%i time= %u"), i, angle, rrr, tim1);
                LOG_INFO(wxT("BR24radar_pi: $$$ t->min_r.r= %i, t->max_r.r= %i, t->min_angle.angle= %i, t->max_angle.angle=%i"),
                t->min_r.r, t->max_r.r, t->min_angle.angle, t->max_angle.angle);*/
              if (t->min_r.r <= rrr && t->max_r.r >= rrr &&
                  ((min_ang <= angle && max_ang >= angle) ||
                   ((min_ang <= angle + LINES_PER_ROTATION) && (max_ang >= angle + LINES_PER_ROTATION)) ||
                   ((min_ang <= angle - LINES_PER_ROTATION && max_ang >= angle - LINES_PER_ROTATION)))) {
                // r and angle area in the area of a blob with a margin to allow for movement
                /*if (time1 > t->t_refresh){
                    LOG_INFO(wxT("BR24radar_pi: $$$  XXXX wrong timeing time1 %u, t->t_refresh %u status %i"), time1.GetLo(),
                t->t_refresh.GetLo(),t->status) ;
                }*/
                //  LOG_INFO(wxT("BR24radar_pi: $$$ break i= %i, r= %i, t->min_r.r= %i, t->max_r.r= %i, min_ang= %i, max_ang= %i"),
                //  i,rrr,t->min_r.r,
                //      t->max_r.r, min_ang, max_ang);
                rrr = t->max_r.r + 1;  // skip rest of this blob
                next_r = true;
                break;  // get out of target loop
              }
            }  // end loop over targets
            if (next_r) continue;
            // pixel found that does not belong to a known target

            //       LOG_INFO(wxT("BR24radar_pi: $$$ unknown pixel found angle %i, r %i"), angle, rrr);
            Position own_pos;
            Polar pol;
            pol.angle = angle;
            pol.r = rrr;
            own_pos.lat = m_pi->m_ownship_lat;
            own_pos.lon = m_pi->m_ownship_lon;
            Position x;
            x = Polar2Pos(pol, own_pos, m_ri->m_range_meters);
            int target_i;
            m_ri->m_marpa->AquireNewTarget(pol, 0, &target_i);
            if (target_i == -1) break;                            // $$$ how to handle max targets exceeded
            m_ri->m_marpa->m_targets[target_i]->RefreshTarget();  // make first contour and max min values
          }                                                       //  if (Pix(angle, r))

        }  //  while loop r
      }    // timing
    }      // next angle
  }        // r > RETURNS_PER_LINE
  return;
}

bool GuardZone::Pix(int ang, int rad) {
  if (rad < 1 || rad >= RETURNS_PER_LINE - 1) {  //  avoid range ring
    return false;
  }
  return ((m_ri->m_history[MOD_ROTATION2048(ang)].line[rad] & 1) != 0);
}

PLUGIN_END_NAMESPACE
