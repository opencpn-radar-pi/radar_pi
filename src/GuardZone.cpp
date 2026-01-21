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
 *   Copyright (C) 2013-2025 by Douwe Fokkema             df@percussion.nl *
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

GuardZone::GuardZone(radar_pi* pi) {
  m_pi = pi;
  m_log_name = wxString::Format(wxT("Radar GuardZone"));
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

void GuardZone::ProcessSpoke(RadarInfo* ri, SpokeBearing angle, uint8_t* data, size_t len) {
  RadarInfo* rc = ri;
  if (!rc) return;
  size_t radar_index = rc->m_radar;
  size_t start_radius;
  {
    wxCriticalSection m_sort_tx_radars;  // protects the m_sorted_tx_radars array
    start_radius = ri->m_start_r;
  }
  AngleDegrees degAngle = SCALE_SPOKES_TO_DEGREES(ri, angle);
  
  LOG_INFO(wxT("$$$ current_radar= %s"), rc->m_name);
  size_t range_start = m_inner_range * rc->m_pixels_per_meter;  // Convert from meters to [0..spoke_len_max>
  size_t range_end = m_outer_range * rc->m_pixels_per_meter;    // Convert from meters to [0..spoke_len_max>
  size_t max = rc->m_spoke_len_max;
  LOG_INFO(wxT("$$$ range_start= %u, range_end=%u"), range_start, range_end);
  bool in_guard_zone = false;

  if (range_start < start_radius) {
    range_start = start_radius;
  }
  if (range_end > len) {
    range_end = len;
  }
  LOG_INFO(wxT("$$$1 range_start= %u, range_end=%u, len=%u, max=%u"), range_start, range_end, len, max);
  switch (m_type) {
    case GZ_ARC:
      LOG_INFO(wxT("$$$ degAngle= %i, m_start_bearing= %i, m_end_bearing= %i"), degAngle, m_start_bearing, m_end_bearing);
      if ((degAngle >= m_start_bearing && degAngle < m_end_bearing) ||
          (m_start_bearing >= m_end_bearing && (degAngle >= m_start_bearing || degAngle < m_end_bearing))) {
        if (range_start < len) {
          if (range_end > len) {
            range_end = len;
          }
          for (size_t r = range_start; r <= range_end; r++) {
            if (data[r] >= m_pi->m_settings.threshold_blue) {
              m_running_count[radar_index]++;
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
            m_running_count[radar_index]++;
          }
#ifdef TEST_GUARD_ZONE_LOCATION
          // Zap guard zone computation location to green so this is visible on screen
          else {
            data[r] = m_pi->m_settings.threshold_green;
          }
#endif
        }
        if (angle > m_last_angle[radar_index]) {
          in_guard_zone = true;
        }
      }
      break;

    default:
      in_guard_zone = false;
      break;
  }
  if (m_last_in_guard_zone[radar_index] && !in_guard_zone) {
    // last bearing that could add to m_running_count, so store as bogey_count;
    m_bogey_count[radar_index] = m_running_count[radar_index];
    m_running_count[radar_index] = 0;
    LOG_GUARD(wxT("%s angle=%d last_angle=%d guardzone=%d..%d (%d - %d) bogey_count=%d, radar_index=%d"), m_log_name.c_str(), angle,
              m_last_angle[radar_index],
              range_start, range_end, m_inner_range, m_outer_range, m_bogey_count[radar_index], radar_index);

    // When debugging with a static ship it is hard to find moving targets, so move
    // the guard zone instead. This slowly rotates the guard zone.
    if (m_pi->m_settings.guard_zone_debug_inc && m_type == GZ_ARC) {
      m_start_bearing += m_pi->m_settings.guard_zone_debug_inc;
      m_end_bearing += m_pi->m_settings.guard_zone_debug_inc;
      m_start_bearing %= DEGREES_PER_ROTATION;
      m_end_bearing %= DEGREES_PER_ROTATION;
    }
  }
  m_last_in_guard_zone[radar_index] = in_guard_zone;
  m_last_angle[radar_index] = angle;
}

  void GuardZone::ResetBogeys() {
    for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
      m_bogey_count[r] = -1;
      m_running_count[r] = 0;
      m_last_in_guard_zone[r] = false;
      m_last_angle[r] = 0;
    }
  }

// Search guard zone for ARPA targets
void GuardZone::SearchTargets() {
  ExtendedPosition own_pos;
  Doppler doppler = ANY;
  if (!m_arpa_on) {
    return;
  }
  RadarInfo* rc = 0;
  // Starting range in case of overlapping radars
  size_t start_r = 0;
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    {
      // use transmitting radars only
      wxCriticalSectionLocker lock(m_pi->m_sort_tx_radars);
      rc = m_pi->m_sorted_tx_radars[r];
      if (!rc) continue;
      start_r = rc->m_start_r;
    }
    if (!rc->GetRadarPosition(&own_pos.pos)          // No position // radar will also track targets in background (not shown)
        || m_pi->GetHeadingSource() == HEADING_NONE  // No heading
        || (m_pi->GetHeadingSource() == HEADING_FIX_HDM && m_pi->m_var_source == VARIATION_SOURCE_NONE)) {
      LOG_ARPA(wxT(" returned1"));
      return;
    }

    if (rc == 0) {
      return;
    }
    if (rc->m_state.GetValue() != RADAR_TRANSMIT) {  // Current radar is transmitting
      return;
    }
    if (rc->m_pixels_per_meter == 0.) {
      LOG_ARPA(wxT(" returned4"));
      return;
    }

    size_t range_start = m_inner_range * rc->m_pixels_per_meter;  // Convert from meters to 0..1024
    size_t range_end = m_outer_range * rc->m_pixels_per_meter;    // Convert from meters to 0..1024 (for Navico)
    
    if (range_start < start_r) {
      range_start = start_r;
      // Increase starting range to avoid 2 radars finding same target
      range_start = range_start + 20; 
    }
    if (range_end > rc->m_spoke_len_max) {
      range_end = rc->m_spoke_len_max - 20;
    }
    if (range_end <= range_start) continue;
    if (range_start < 10) range_start = 10;

    int hdt_spokes = SCALE_DEGREES_TO_SPOKES(rc, m_pi->GetHeadingTrue());
    hdt_spokes = MOD_SPOKES(rc, hdt_spokes);

    int start_bearing = SCALE_DEGREES_TO_SPOKES(rc, m_start_bearing) + hdt_spokes;
    int end_bearing = SCALE_DEGREES_TO_SPOKES(rc, m_end_bearing) + hdt_spokes;
    start_bearing = MOD_SPOKES(rc, start_bearing);
    end_bearing = MOD_SPOKES(rc, end_bearing);
    if (start_bearing > end_bearing) {
      end_bearing += rc->m_spokes;
    }
    if (m_type == GZ_CIRCLE) {
      start_bearing = rc->m_last_received_spoke - 1100;  // forward of the beam  // $$$ this is all empty! before mod
      end_bearing = rc->m_last_received_spoke;
    }

    if (start_bearing > end_bearing) {
      end_bearing += rc->m_spokes;
    }
    if (range_start < rc->m_spoke_len_max) {
      size_t outer_limit = rc->m_spoke_len_max;
      outer_limit = (size_t)outer_limit * 0.99;
      if (range_end > outer_limit) {
        range_end = outer_limit;
      }
      LOG_ARPA(wxT("target search start_bearing=%i, end_bearing=%i, range_start=%i, range_end=%i "), start_bearing, end_bearing,
               range_start, range_end);
      if (range_end < range_start) return;
      // loop with +2 increments as target must be larger than 2 pixels in width
      for (int angleIter = start_bearing; angleIter < end_bearing; angleIter += 2) {
        SpokeBearing angle = MOD_SPOKES(rc, angleIter);
        // LOG_ARPA(wxT("Found blob continue angle=%i, end_bearing=%i"), angle, end_bearing);
        wxLongLong time1 = rc->m_history[angle].time;
        // time2 must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
        wxLongLong time2 = rc->m_history[MOD_SPOKES(rc, angle + 3 * SCAN_MARGIN)].time;

        // check if this angle has been refreshed since last time
        // and if the beam has passed the target location with SCAN_MARGIN spokes
        /*if ((time1 > (m_arpa_update_time[angle] + SCAN_MARGIN2) &&
             time2 >= time1)) */

        int diff = rc->m_last_received_spoke - angle;
        if (diff > (int)rc->m_spokes / 2) diff -= (int)rc->m_spokes;
        if (diff < -(int)rc->m_spokes / 2) diff += rc->m_spokes;
        if (diff > 50)

        {  // the beam should have passed our "angle" AND a
           // point SCANMARGIN further set new refresh time
           // m_arpa_update_time[angle] = time1;
          for (int rrr = (int)range_start; rrr < (int)range_end; rrr++) {
            if (m_pi->m_arpa->MultiPix(rc, angle, rrr, doppler)) {
              // pixel found that does not belong to a known target
              Polar pol;
              pol.angle = angle;
              pol.r = rrr;
              LOG_ARPA(wxT("Found blob angle=%i, r=%i, doppler=%i, radar= %s"), angle, rrr, doppler, rc->m_name);
              int target_i = m_pi->m_arpa->AcquireNewARPATarget(rc, pol, 0, doppler);
              if (target_i == -1) break;
              // LOG_ARPA(wxT("Found blob  rrr=%i"), rrr);
            }
          }
        }
      }
    }
  }
  return;
}

PLUGIN_END_NAMESPACE