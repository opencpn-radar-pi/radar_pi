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

#include "TrailBuffer.h"

#undef M_SETTINGS
#define M_SETTINGS m_ri->m_pi->m_settings

PLUGIN_BEGIN_NAMESPACE

// Allocated arrays are not two dimensional, so we make
// up a macro that makes it look that way. Note the 'stride'
// which is the length of the 2nd dimension, not the 1st.
// Striding the first dimension makes for better locality because
// we generally iterate over the range (process one spoke) so those
// values are now closer together in memory.
#define M_TRUE_TRAILS_STRIDE m_trail_size
#define M_TRUE_TRAILS(x, y) m_true_trails[x * M_TRUE_TRAILS_STRIDE + y]
#define M_RELATIVE_TRAILS_STRIDE m_max_spoke_len
#define M_RELATIVE_TRAILS(x, y) m_relative_trails[x * M_RELATIVE_TRAILS_STRIDE + y]

TrailBuffer::TrailBuffer(RadarInfo *ri, size_t spokes, size_t max_spoke_len) {
  m_ri = ri;
  m_spokes = spokes;
  m_max_spoke_len = max_spoke_len;
  m_trail_size = max_spoke_len * 2 + MARGIN * 2;
  m_true_trails = (TrailRevolutionsAge *)calloc(sizeof(TrailRevolutionsAge), m_trail_size * m_trail_size);
  m_relative_trails = (TrailRevolutionsAge *)calloc(sizeof(TrailRevolutionsAge), m_spokes * m_max_spoke_len);
  m_copy_true_trails = (TrailRevolutionsAge *)calloc(sizeof(TrailRevolutionsAge), m_trail_size * m_trail_size);
  m_copy_relative_trails = (TrailRevolutionsAge *)calloc(sizeof(TrailRevolutionsAge), m_spokes * m_max_spoke_len);

  if (!m_true_trails || !m_relative_trails || !m_copy_true_trails || !m_copy_relative_trails) {
    wxLogError(wxT("radar_pi: Out Of Memory, fatal!"));
    wxAbort();
  }
}

TrailBuffer::~TrailBuffer() {
  free(m_true_trails);
  free(m_relative_trails);
  free(m_copy_relative_trails);
  free(m_copy_true_trails);
}

void TrailBuffer::UpdateTrueTrails(SpokeBearing bearing, uint8_t *data, size_t len) {
  int motion = m_ri->m_trails_motion.GetValue();
  uint8_t weakest_normal_blob = m_ri->m_pi->m_settings.threshold_blue;
  size_t radius = 0;

  for (; radius < len - 1; radius++) {  //  len - 1 : no trails on range circle
    PointInt point = m_ri->m_polar_lookup->GetPointInt(bearing, radius);

    point.x += m_trail_size / 2 + m_offset.lat;
    point.y += m_trail_size / 2 + m_offset.lon;

    if (point.x >= 0 && point.x < (int)m_trail_size && point.y >= 0 && point.y < (int)m_trail_size) {
      UINT8 *trail = &M_TRUE_TRAILS(point.x, point.y);
      // when ship moves north, offset.lat > 0. Add to move trails image in opposite direction
      // when ship moves east, offset.lon > 0. Add to move trails image in opposite direction
      if (data[radius] >= weakest_normal_blob) {
        *trail = 1;
      } else {
        if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
          (*trail)++;
        }
        if (motion == TARGET_MOTION_TRUE) {
          data[radius] = m_ri->m_trail_colour[*trail];
        }
      }
    }
  }

  for (; radius < m_max_spoke_len; radius++) {
    PointInt point = m_ri->m_polar_lookup->GetPointInt(bearing, radius);

    point.x += m_trail_size / 2 + m_offset.lat;
    point.y += m_trail_size / 2 + m_offset.lon;

    if (point.x >= 0 && point.x < (int)m_trail_size && point.y >= 0 && point.y < (int)m_trail_size) {
      UINT8 *trail = &M_TRUE_TRAILS(point.x, m_trail_size + point.y);
      // when ship moves north, offset.lat > 0. Add to move trails image in opposite direction
      // when ship moves east, offset.lon > 0. Add to move trails image in opposite direction
      if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
        (*trail)++;
      }
    }
  }
}

void TrailBuffer::UpdateRelativeTrails(SpokeBearing angle, uint8_t *data, size_t len) {
  int motion = m_ri->m_trails_motion.GetValue();
  UINT8 *trail = &M_RELATIVE_TRAILS(angle, 0);
  uint8_t weakest_normal_blob = m_ri->m_pi->m_settings.threshold_blue;
  size_t radius = 0;

  for (; radius < len - 1; radius++, trail++) {  // len - 1 : no trails on range circle
    if (data[radius] >= weakest_normal_blob) {
      *trail = 1;
    } else {
      if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
        (*trail)++;
      }
      if (motion == TARGET_MOTION_RELATIVE) {
        data[radius] = m_ri->m_trail_colour[*trail];
      }
    }
  }
  for (; radius < m_max_spoke_len; radius++, trail++)  // And clear out empty bit of spoke when spoke_len < max_spoke_len
  {
    *trail = 0;
  }
}

void TrailBuffer::ZoomTrails(float zoom_factor) {
  uint8_t *flip;
  // zoom_factor > 1 -> zoom in, enlarge image

  memset(m_copy_relative_trails, 0, m_spokes * m_max_spoke_len);

  // zoom relative trails

  for (size_t i = 0; i < m_spokes; i++) {
    for (size_t j = 0; j < m_max_spoke_len; j++) {
      size_t index_j = (size_t)((float)j * zoom_factor);
      if (index_j >= m_max_spoke_len) break;
      if (M_RELATIVE_TRAILS(i, j) != 0) {
        m_copy_relative_trails[i * M_RELATIVE_TRAILS_STRIDE + index_j] = M_RELATIVE_TRAILS(i, j);
      }
    }
  }
  // Now exchange the two
  flip = m_relative_trails;
  m_relative_trails = m_copy_relative_trails;
  m_copy_relative_trails = flip;

  memset(m_copy_true_trails, 0, m_trail_size * m_trail_size);

  // zoom true trails

  for (size_t i = wxMax(m_trail_size / 2 + m_offset.lat - m_max_spoke_len, 0);
       i < wxMin(m_trail_size / 2 + m_offset.lat + m_max_spoke_len, m_trail_size); i++) {
    int index_i = (int((float)(i - m_trail_size / 2 + m_offset.lat) * zoom_factor)) + m_trail_size / 2 - m_offset.lat * zoom_factor;
    if (index_i >= m_trail_size - 1) break;  // allow adding an additional pixel later
    if (index_i < 0) continue;
    for (size_t j = wxMax(m_trail_size / 2 + m_offset.lon - m_max_spoke_len, 0);
         j < wxMin(m_trail_size / 2 + m_offset.lon + m_max_spoke_len, m_trail_size); j++) {
      int index_j =
          (int((float)(j - m_trail_size / 2 + m_offset.lon) * zoom_factor)) + m_trail_size / 2 - m_offset.lon * zoom_factor;
      if (index_j >= m_trail_size - 1) break;
      if (index_j < 0) continue;
      uint8_t pixel = M_TRUE_TRAILS(i, j);
      if (pixel != 0) {  // many to one mapping, prevent overwriting trails with 0
        m_copy_true_trails[index_i * M_TRUE_TRAILS_STRIDE + index_j] = pixel;
        if (zoom_factor > 1.2) {
          // add an extra pixel in the y direction
          m_copy_true_trails[index_i * M_TRUE_TRAILS_STRIDE + index_j + 1] = pixel;
          if (zoom_factor > 1.6) {
            // also add pixels in the x direction
            m_copy_true_trails[(index_i + 1) * M_TRUE_TRAILS_STRIDE + index_j] = pixel;
            m_copy_true_trails[(index_i + 1) * M_TRUE_TRAILS_STRIDE + index_j + 1] = pixel;
          }
        }
      }
    }
  }
  flip = m_true_trails;
  m_true_trails = m_copy_true_trails;
  m_copy_true_trails = flip;

  m_offset.lon *= zoom_factor;
  m_offset.lat *= zoom_factor;
}

void TrailBuffer::UpdateTrailPosition() {
  GeoPosition radar;
  GeoPositionPixels shift;

  // When position changes the trail image is not moved, only the pointer to the center
  // of the image (offset) is changed.
  // So we move the image around within the m_trails.true_trails buffer (by moving the pointer).
  // But when there is no room anymore (margin used) the whole trails image is shifted
  // and the offset is reset
  if (m_offset.lon >= MARGIN || m_offset.lon <= -MARGIN) {
    LOG_INFO(wxT("radar_pi: offset lon too large %d"), m_offset.lon);
    m_offset.lon = 0;
  }
  if (m_offset.lat >= MARGIN || m_offset.lat <= -MARGIN) {
    LOG_INFO(wxT("radar_pi: offset lat too large %d"), m_offset.lat);
    m_offset.lat = 0;
  }

  // zooming of trails required? First check conditions
  if (m_ri->m_old_range == 0 || m_ri->m_range_meters == 0) {
    ClearTrails();
    if (m_ri->m_range_meters == 0) {
      return;
    }
    m_ri->m_old_range = m_ri->m_range_meters;
  } else if (m_ri->m_old_range != m_ri->m_range_meters) {
    // zoom trails
    float zoom_factor = (float)m_ri->m_old_range / (float)m_ri->m_range_meters;
    m_ri->m_old_range = m_ri->m_range_meters;

    // center the image before zooming
    // otherwise the offset might get too large
    ShiftImageLatToCenter();
    ShiftImageLonToCenter();
    ZoomTrails(zoom_factor);
  }

  if (!m_ri->GetRadarPosition(&radar) || m_ri->m_pi->GetHeadingSource() == HEADING_NONE) {
    return;
  }

  // Did the ship move? No, return.
  if (m_pos.lat == radar.lat && m_pos.lon == radar.lon) {
    return;
  }

  // Check the movement of the ship
  double dif_lat = radar.lat - m_pos.lat;  // going north is positive
  double dif_lon = radar.lon - m_pos.lon;  // moving east is positive
  m_pos = radar;

  // get (floating point) shift of the ship in radar pixels
  double fshift_lat = dif_lat * 60. * 1852. / (double)m_ri->m_range_meters * (double)(m_ri->m_spoke_len);
  double fshift_lon = dif_lon * 60. * 1852. / (double)m_ri->m_range_meters * (double)(m_ri->m_spoke_len);
  fshift_lon *= cos(deg2rad(radar.lat));  // at higher latitudes a degree of longitude is fewer meters

  // Get the integer pixel shift, first add previous rounding error
  shift.lat = (int)(fshift_lat + m_dif.lat);
  shift.lon = (int)(fshift_lon + m_dif.lon);

#ifdef TODO
  // Check for changes in the direction of movement, part of the image buffer has to be erased
  if (shift.lat > 0 && m_dir.lat <= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction (this area might not be empty)
    memset(&m_trails.true_trails[TRAILS_SIZE - MARGIN + m_offset.lat][0], 0, TRAILS_SIZE * (MARGIN - m_offset.lat));
    m_dir.lat = 1;
  }

  if (shift.lat < 0 && m_dir.lat >= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    memset(&m_trails.true_trails[0][0], 0, TRAILS_SIZE * (MARGIN + m_offset.lat));
    m_dir.lat = -1;
  }

  if (shift.lon > 0 && m_dir.lon <= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    for (size_t i = 0; i < TRAILS_SIZE; i++) {
      memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN + m_offset.lon], 0, MARGIN - m_offset.lon);
    }
    m_dir.lon = 1;
  }

  if (shift.lon < 0 && m_dir_lon >= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    for (size_t i = 0; i < TRAILS_SIZE; i++) {
      memset(&m_trails.true_trails[i][0], 0, MARGIN + m_offset.lon);
    }
    m_dir.lon = -1;
  }
#endif

  // save the rounding fraction and appy it next time
  m_dif.lat = fshift_lat + m_dif.lat - (double)shift.lat;
  m_dif.lon = fshift_lon + m_dif.lon - (double)shift.lon;

  if (shift.lat >= MARGIN || shift.lat <= -MARGIN || shift.lon >= MARGIN || shift.lon <= -MARGIN) {  // huge shift, reset trails
    ClearTrails();
    if (!m_ri->GetRadarPosition(&m_pos)) {
      m_pos.lat = 0.;
      m_pos.lon = 0.;
    }
    LOG_INFO(wxT("radar_pi: %s Large movement trails reset"), m_ri->m_name.c_str());
    return;
  }

  // offset lon too large: shift image
  if (abs(m_offset.lon + shift.lon) >= MARGIN) {
    ShiftImageLonToCenter();
  }

  // offset lat too large: shift image in lat direction
  if (abs(m_offset.lat + shift.lat) >= MARGIN) {
    ShiftImageLatToCenter();
  }
  // apply the shifts to the offset
  m_offset.lat += shift.lat;
  m_offset.lon += shift.lon;
}

// shifts the true trails image in lon direction to center
void TrailBuffer::ShiftImageLonToCenter() {
  int keep;
  int shift;

  if (m_offset.lon >= MARGIN || m_offset.lon <= -MARGIN) {  // abs no good
    LOG_INFO(wxT("radar_pi: offset lon too large %i"), m_offset.lon);
    ClearTrails();
    return;
  }

  if (m_offset.lon > 0) {
    shift = m_offset.lon;
    keep = m_trail_size - shift;
    for (size_t i = 0; i < m_trail_size; i++) {
      memmove(&M_TRUE_TRAILS(i, 0), &M_TRUE_TRAILS(i, shift), keep);
      memset(&M_TRUE_TRAILS(i, keep), 0, shift);
    }
  }
  if (m_offset.lon < 0) {
    int shift = -m_offset.lon;
    int keep = m_trail_size - shift;

    for (size_t i = 0; i < m_trail_size; i++) {
      memmove(&M_TRUE_TRAILS(i, 0), &M_TRUE_TRAILS(i, shift), keep);
      memset(&M_TRUE_TRAILS(i, keep), 0, shift);
    }
  }
  m_offset.lon = 0;
}

// shifts the true trails image in lat direction to center
void TrailBuffer::ShiftImageLatToCenter() {
  size_t shift;

  if (m_offset.lat >= MARGIN || m_offset.lat <= -MARGIN) {  // abs not ok
    LOG_INFO(wxT("radar_pi: offset lat too large %i"), m_offset.lat);
    ClearTrails();
    return;
  }

  if (m_offset.lat > 0) {
    shift = m_offset.lat * m_trail_size;
    memmove(m_true_trails + shift, m_true_trails, m_trail_size * m_trail_size - shift);
    memset(m_true_trails + m_trail_size * m_trail_size - shift, 0, shift);
  }
  if (m_offset.lat < 0) {
    shift = -m_offset.lat * m_trail_size;
    memmove(m_true_trails, m_true_trails + shift, m_trail_size * m_trail_size - shift);
    memset(m_true_trails, 0, shift);
  }
  m_offset.lat = 0;
}

void TrailBuffer::ClearTrails() {
  LOG_VERBOSE(wxT("radar_pi: ClearTrails"));
  if (m_true_trails) {
    memset(m_true_trails, 0, m_trail_size * m_trail_size);
    m_offset.lat = 0;
    m_offset.lon = 0;
  }
  if (m_relative_trails) {
    memset(m_relative_trails, 0, m_spokes * m_max_spoke_len);
  }
}

PLUGIN_END_NAMESPACE
