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

GuardZone::GuardZone(br24radar_pi* pi, RadarInfo* ri, int zone) {
  m_pi = pi;
  m_ri = ri;
  m_log_name = wxString::Format(wxT("BR24radar_pi: Radar %c GuardZone %d:"), m_ri->m_radar + 'A', zone + 1);
  m_type = GZ_CIRCLE;
  m_start_bearing = 0;
  m_end_bearing = 0;
  m_inner_range = 0;
  m_outer_range = 0;
  m_arpa_on = 0;
  m_alarm_on = 0;
  m_show_time = 0;
  CLEAR_STRUCT(arpa_update_time);
  ResetBogeys();
}

void GuardZone::ProcessSpoke(SpokeBearing angle, UINT8* data, UINT8* hist, size_t len, int range) {
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
        in_guard_zone = true;
      }
      break;

    case GZ_CIRCLE:
      if (range_start < RETURNS_PER_LINE) {
        if (range_end > RETURNS_PER_LINE) {
          range_end = RETURNS_PER_LINE;
        }

        for (size_t r = range_start; r <= range_end; r++) {
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

// Search guard zone for ARPA targets
void GuardZone::SearchTargets() {
  Position own_pos;

  if (!m_arpa_on) {
    return;
  }
  if (m_ri->m_arpa->GetTargetCount() >= MAX_NUMBER_OF_TARGETS - 2) {
    LOG_INFO(wxT("BR24radar_pi: No more scanning for ARPA targets, maximum number of targets reached"));
    return;
  }
  if (!m_pi->m_settings.show  // No radar shown
      || (m_pi->m_radar[0]->m_state.GetValue() != RADAR_TRANSMIT &&
          m_pi->m_radar[1]->m_state.GetValue() != RADAR_TRANSMIT)  // Radar not transmitting
      || !m_pi->GetRadarPosition(&own_pos.lat, &own_pos.lon)) {    // No position
    return;
  }
  if (m_ri->m_range_meters == 0) {
    return;
  }
  size_t range_start = m_inner_range * RETURNS_PER_LINE / m_ri->m_range_meters;  // Convert from meters to 0..511
  size_t range_end = m_outer_range * RETURNS_PER_LINE / m_ri->m_range_meters;    // Convert from meters to 0..511

  SpokeBearing hdt = SCALE_DEGREES_TO_RAW2048(m_pi->GetHeadingTrue());
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
      // next one must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
      wxLongLong time2 = m_ri->m_history[MOD_ROTATION2048(angle + 3 * SCAN_MARGIN)].time;

      // check if target has been refreshed since last time
      // and if the beam has passed the target location with SCAN_MARGIN spokes
      if ((time1 > (arpa_update_time[MOD_ROTATION2048(angle)] + SCAN_MARGIN2) &&
           time2 >= time1)) {  // the beam sould have passed our "angle" AND a point SCANMARGIN further
                               // set new refresh time
        arpa_update_time[MOD_ROTATION2048(angle)] = time1;
        for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {
          if (m_ri->m_arpa->GetTargetCount() >= MAX_NUMBER_OF_TARGETS - 1) {
            LOG_INFO(wxT("BR24radar_pi: No more scanning for ARPA targets in loop, maximum number of targets reached"));
            return;
          }
          if (m_ri->m_arpa->MultiPix(angle, rrr)) {
            bool next_r = false;
            if (next_r) continue;
            // pixel found that does not belong to a known target
            Polar pol;
            pol.angle = angle;
            pol.r = rrr;

            Position x = Polar2Pos(pol, own_pos, m_ri->m_range_meters);
            int target_i = m_ri->m_arpa->AcquireNewARPATarget(pol, 0);
            if (target_i == -1) break;
          }
        }
      }
    }
  }
  return;
}

PLUGIN_END_NAMESPACE
