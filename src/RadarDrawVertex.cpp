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
 *   Copyright (C) 2012-2013 by Kees Verruijt         canboat@verruijt.net *
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

bool RadarDrawVertex::Init(int newColorOption) {
  if (newColorOption) {
    // Don't care in vertex mode
  }

  for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
    spokes[i].n = 0;
  }

  wxLogMessage(wxT("BR24radar_pi: CPU oriented OpenGL vertex draw loaded"));
  start_line = LINES_PER_ROTATION;
  end_line = 0;

  return true;
}

#define ADD_VERTEX_POINT(angle, radius, r, g, b, a) \
  {                                                 \
    p->x = polar_to_cart_x[angle][radius];          \
    p->y = polar_to_cart_y[angle][radius];          \
    p->red = r;                                     \
    p->green = g;                                   \
    p->blue = b;                                    \
    p->alpha = a;                                   \
    p++;                                            \
  }

void RadarDrawVertex::SetBlob(int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green, GLubyte blue,
                              GLubyte alpha) {
  int arc1 = MOD_ROTATION2048(angle_begin);
  int arc2 = MOD_ROTATION2048(angle_end);

  vertex_point* p = &spokes[arc1].points[spokes[arc1].n];

  // First triangle

  ADD_VERTEX_POINT(arc1, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);

  // Second triangle

  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r2, red, green, blue, alpha);

  spokes[arc1].n += 6;

  if (spokes[arc1].n >= VERTEX_MAX - VERTEX_PER_QUAD) {
    wxLogMessage(wxT("BR24radar_pi: vertices array limit overflow n=%d arc=%d"), spokes[arc1].n, arc1);
    spokes[arc1].n = VERTEX_MAX - VERTEX_PER_QUAD;  // Make room for last vertex
                                                    // of all, to make sure we
                                                    // draw outer border
  }
}

void RadarDrawVertex::ProcessRadarSpoke(SpokeBearing angle, UINT8* data, size_t len) {
  GLubyte red, green, blue;
  GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - m_pi->m_settings.overlay_transparency) / MAX_OVERLAY_TRANSPARENCY;
  BlobColor previous_color = BLOB_NONE;
  GLubyte strength = 0;

  spokes[angle].n = 0;  // Reset the spoke

  int r_begin = 0;
  int r_end = 0;

  for (size_t radius = 0; radius < len; radius++) {
    strength = data[radius];
    BlobColor actual_color = m_pi->m_color_map[strength];

    if (actual_color == previous_color) {
      // continue with same color, just register it
      r_end++;
    } else if (previous_color == BLOB_NONE && actual_color != BLOB_NONE) {
      // blob starts, no display, just register
      r_begin = radius;
      r_end = r_begin + 1;
      previous_color = actual_color;  // new color
    } else if (previous_color != BLOB_NONE && (previous_color != actual_color)) {
      red = m_pi->m_color_map_red[previous_color];
      green = m_pi->m_color_map_green[previous_color];
      blue = m_pi->m_color_map_blue[previous_color];

      SetBlob(angle, angle + 1, r_begin, r_end, red, green, blue, alpha);
      m_blobs++;

      previous_color = actual_color;
      if (actual_color != BLOB_NONE) {  // change of color, start new blob
        r_begin = radius;
        r_end = r_begin + 1;
      }
    }
  }

  if (previous_color != BLOB_NONE) {  // Draw final blob
    red = m_pi->m_color_map_red[previous_color];
    green = m_pi->m_color_map_green[previous_color];
    blue = m_pi->m_color_map_blue[previous_color];

    SetBlob(angle, angle + 1, r_begin, r_end, red, green, blue, alpha);
    m_blobs++;
  }
  m_spokes++;
}

void RadarDrawVertex::DrawRadarImage(wxPoint center, double scale) {
  size_t total_points = 0;

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);

  // if (overlay) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //}
  // else {
  // glClearColor(0, 0, 0, 1);
  // glClear(GL_COLOR_BUFFER_BIT);
  //}

  glPushMatrix();
  glTranslated(center.x, center.y, 0);
  glScaled(scale, scale, 1.);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
    int number_of_points = spokes[i].n;
    if (number_of_points > 0) {
      glVertexPointer(2, GL_FLOAT, sizeof(vertex_point), &spokes[i].points[0].x);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_point), &spokes[i].points[0].red);
      glDrawArrays(GL_TRIANGLES, 0, number_of_points);
    }
    total_points += number_of_points;
  }
  glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
  glDisableClientState(GL_COLOR_ARRAY);

  glPopMatrix();  // Undo translated/scaled
  glPopAttrib();  // Undo blend

  if (m_pi->m_settings.verbose >= 4) {
    wxLogMessage(wxT("BR24radar_pi: RadarDrawVertex::DrawRadarImage drawn %u points for %u spokes with %u blobs"), total_points,
                 m_spokes, m_blobs);
  }
  m_blobs = 0;
  m_spokes = 0;
}
