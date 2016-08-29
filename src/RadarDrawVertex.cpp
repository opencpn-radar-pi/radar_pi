/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
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

#include "RadarDrawVertex.h"

PLUGIN_BEGIN_NAMESPACE

bool RadarDrawVertex::Init() { return true; }

#define ADD_VERTEX_POINT(angle, radius, r, g, b, a)          \
  {                                                          \
    line->points[count].x = m_polarLookup->x[angle][radius]; \
    line->points[count].y = m_polarLookup->y[angle][radius]; \
    line->points[count].red = r;                             \
    line->points[count].green = g;                           \
    line->points[count].blue = b;                            \
    line->points[count].alpha = a;                           \
    count++;                                                 \
  }

void RadarDrawVertex::SetBlob(VertexLine* line, int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green,
                              GLubyte blue, GLubyte alpha) {
  if (r2 == 0) {
    return;
  }
  int arc1 = MOD_ROTATION2048(angle_begin);
  int arc2 = MOD_ROTATION2048(angle_end);
  size_t count = line->count;

  if (line->count + VERTEX_PER_QUAD > line->allocated) {
    const size_t extra = 8 * VERTEX_PER_QUAD;
    line->points = (VertexPoint*)realloc(line->points, (line->allocated + extra) * sizeof(VertexPoint));
    line->allocated += extra;
    m_count += extra;
  }

  if (!line->points) {
    if (!m_oom) {
      wxLogError(wxT("BR24radar_pi: Out of memory"));
      m_oom = true;
    }
    return;
  }

  // First triangle
  ADD_VERTEX_POINT(arc1, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);

  // Second triangle

  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r2, red, green, blue, alpha);

  line->count = count;
}

void RadarDrawVertex::ProcessRadarSpoke(int transparency, SpokeBearing angle, UINT8* data, size_t len) {
  wxColour colour;
  GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - transparency) / MAX_OVERLAY_TRANSPARENCY;
  BlobColour previous_colour = BLOB_NONE;
  GLubyte strength = 0;
  time_t now = time(0);

  wxCriticalSectionLocker lock(m_exclusive);

  int r_begin = 0;
  int r_end = 0;

  if (angle < 0 || angle >= LINES_PER_ROTATION) {
    return;
  }

  VertexLine* line = &m_vertices[angle];

  if (!line->points) {
    static size_t INITIAL_ALLOCATION = 600;  // Empirically found to be enough for a complicated picture
    line->allocated = INITIAL_ALLOCATION * VERTEX_PER_QUAD;
    m_count += INITIAL_ALLOCATION * VERTEX_PER_QUAD;
    line->points = (VertexPoint*)malloc(line->allocated * sizeof(VertexPoint));
    if (!line->points) {
      if (!m_oom) {
        wxLogError(wxT("BR24radar_pi: Out of memory"));
        m_oom = true;
      }
      line->allocated = 0;
      line->count = 0;
      return;
    }
  }
  line->count = 0;
  line->timeout = now + m_ri->m_pi->m_settings.max_age;

  for (size_t radius = 0; radius < len; radius++) {
    strength = data[radius];
    BlobColour actual_colour = m_ri->m_colour_map[strength];

    if (actual_colour == previous_colour) {
      // continue with same color, just register it
      r_end++;
    } else if (previous_colour == BLOB_NONE && actual_colour != BLOB_NONE) {
      // blob starts, no display, just register
      r_begin = radius;
      r_end = r_begin + 1;
      previous_colour = actual_colour;  // new color
    } else if (previous_colour != BLOB_NONE && (previous_colour != actual_colour)) {
      colour = m_ri->m_colour_map_rgb[previous_colour];

      SetBlob(line, angle, angle + 1, r_begin, r_end, colour.Red(), colour.Green(), colour.Blue(), alpha);

      previous_colour = actual_colour;
      if (actual_colour != BLOB_NONE) {  // change of color, start new blob
        r_begin = radius;
        r_end = r_begin + 1;
      }
    }
  }

  if (previous_colour != BLOB_NONE) {  // Draw final blob
    colour = m_ri->m_colour_map_rgb[previous_colour];

    SetBlob(line, angle, angle + 1, r_begin, r_end, colour.Red(), colour.Green(), colour.Blue(), alpha);
  }
}

void RadarDrawVertex::DrawRadarImage() {
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  time_t now = time(0);
  {
    wxCriticalSectionLocker lock(m_exclusive);

    for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
      VertexLine* line = &m_vertices[i];
      if (!line->count || TIMED_OUT(now, line->timeout)) {
        continue;
      }

      glVertexPointer(2, GL_FLOAT, sizeof(VertexPoint), &line->points[0].x);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexPoint), &line->points[0].red);
      glDrawArrays(GL_TRIANGLES, 0, line->count);
    }
  }
  glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
  glDisableClientState(GL_COLOR_ARRAY);
}

PLUGIN_END_NAMESPACE
