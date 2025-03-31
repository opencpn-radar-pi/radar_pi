/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
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
#include "GuardZone.h"

#include "Arpa.h"
#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

#undef TEST_GUARD_ZONE_LOCATION

GuardZone::GuardZone(radar_pi* pi, RadarInfo* ri, int zone) {
  m_pi = pi;
  m_ri = ri;
  m_log_name = wxString::Format(wxT("Radar %c GuardZone %d:"), (char)(m_ri->m_radar + 'A'), zone + 1);
  m_type = GZ_CIRCLE;
  m_start_bearing = 0;
  m_end_bearing = 0;
  m_inner_range = 0;
  m_outer_range = 0;
  m_arpa_on = 0;
  m_alarm_on = 0;
  m_show_time = 0;
  CLEAR_STRUCT(m_arpa_update_time);
  ResetBogeys();
}

void GuardZone::ProcessSpoke(SpokeBearing angle, uint8_t* data, uint8_t* hist, size_t len) {
  size_t range_start = m_inner_range * m_ri->m_pixels_per_meter;  // Convert from meters to [0..spoke_len_max>
  size_t range_end = m_outer_range * m_ri->m_pixels_per_meter;    // Convert from meters to [0..spoke_len_max>
  bool in_guard_zone = false;
  AngleDegrees degAngle = SCALE_SPOKES_TO_DEGREES(angle);

  switch (m_type) {
    case GZ_ARC:
      if ((degAngle >= m_start_bearing && degAngle < m_end_bearing) ||
          (m_start_bearing >= m_end_bearing && (degAngle >= m_start_bearing || degAngle < m_end_bearing))) {
        if (range_start < len) {
          if (range_end > len) {
            range_end = len;
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
      if (range_start < len) {
        if (range_end > len) {
          range_end = len;
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
    LOG_GUARD(wxT("%s angle=%d last_angle=%d guardzone=%d..%d (%d - %d) bogey_count=%d"), m_log_name.c_str(), angle, m_last_angle,
              range_start, range_end, m_inner_range, m_outer_range, m_bogey_count);

    // When debugging with a static ship it is hard to find moving targets, so move
    // the guard zone instead. This slowly rotates the guard zone.
    if (m_pi->m_settings.guard_zone_debug_inc && m_type == GZ_ARC) {
      m_start_bearing += m_pi->m_settings.guard_zone_debug_inc;
      m_end_bearing += m_pi->m_settings.guard_zone_debug_inc;
      m_start_bearing %= DEGREES_PER_ROTATION;
      m_end_bearing %= DEGREES_PER_ROTATION;
    }
  }

  m_last_in_guard_zone = in_guard_zone;
  m_last_angle = angle;
}

// Search guard zone for ARPA targets
void GuardZone::SearchTargets() {
  ExtendedPosition own_pos;
  Doppler doppler = ANY;
  if (!m_arpa_on) {
    return;
  }
  if (!m_pi->m_settings.show                       // No radar shown
      || !m_ri->GetRadarPosition(&own_pos.pos)     // No position
      || m_pi->GetHeadingSource() == HEADING_NONE  // No heading
      || (m_pi->GetHeadingSource() == HEADING_FIX_HDM && m_pi->m_var_source == VARIATION_SOURCE_NONE)) {
    LOG_ARPA(wxT(" returned1"));
    return;
  }

  if (m_ri == 0) {
    return;
  }
  if (m_ri->m_state.GetValue() != RADAR_TRANSMIT) {  // Current radar is transmitting
    return;
  }
  if (m_ri->m_pixels_per_meter == 0.) {
    LOG_ARPA(wxT(" returned4"));
    return;
  }
  size_t range_start = m_inner_range * m_ri->m_pixels_per_meter;  // Convert from meters to 0..511
  size_t range_end = m_outer_range * m_ri->m_pixels_per_meter;    // Convert from meters to 0..511
  if (range_start < 1) range_start = 1;
  if (range_start >= range_end) return;
  int hdt = SCALE_DEGREES_TO_SPOKES(m_pi->GetHeadingTrue());
  while (hdt >= (int)m_ri->m_spokes) {
    hdt -= m_ri->m_spokes;
  }
  while (hdt < 0) {
    hdt += m_ri->m_spokes;
  }
  int start_bearing = SCALE_DEGREES_TO_SPOKES(m_start_bearing) + hdt;
  int end_bearing = SCALE_DEGREES_TO_SPOKES(m_end_bearing) + hdt;
  start_bearing = MOD_SPOKES(start_bearing);
  end_bearing = MOD_SPOKES(end_bearing);
  if (start_bearing > end_bearing) {
    end_bearing += m_ri->m_spokes;
  }
  if (m_type == GZ_CIRCLE) {
    start_bearing = m_ri->m_last_received_spoke + 600;  // forward of the beam
    end_bearing = m_ri->m_last_received_spoke - 500;
  }
  if (start_bearing > end_bearing) {
    end_bearing += m_ri->m_spokes;
  }
  if (range_start < m_ri->m_spoke_len_max) {
    size_t outer_limit = m_ri->m_spoke_len_max;
    outer_limit = (size_t)outer_limit * 0.93;
    if (range_end > outer_limit) {
      range_end = outer_limit;
    }
    if (range_end < range_start) return;

    // loop with +2 increments as target must be larger than 2 pixels in width
    for (int angleIter = start_bearing; angleIter < end_bearing; angleIter += 2) {
      SpokeBearing angle = MOD_SPOKES(angleIter);
      wxLongLong time1 = m_ri->m_history[angle].time;
      // time2 must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
      wxLongLong time2 = m_ri->m_history[MOD_SPOKES(angle + 3 * SCAN_MARGIN)].time;

      // check if target has been refreshed since last time
      // and if the beam has passed the target location with SCAN_MARGIN spokes
      if ((time1 > (m_arpa_update_time[angle] + SCAN_MARGIN2) &&
           time2 >= time1)) {  // the beam sould have passed our "angle" AND a
                               // point SCANMARGIN further set new refresh time
        m_arpa_update_time[angle] = time1;
        for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {
          if (m_ri->m_arpa->MultiPix(angle, rrr, doppler)) {
            // pixel found that does not belong to a known target
            Polar pol;
            pol.angle = angle;
            pol.r = rrr;
            LOG_ARPA(wxT("Found blob angle=%i, r=%i, doppler=%i"), angle, rrr, doppler);
            int target_i = m_ri->m_arpa->AcquireNewARPATarget(pol, 0, doppler);
            if (target_i == -1) break;
          }
        }
      }
    }
  }
  return;
}

PLUGIN_END_NAMESPACE