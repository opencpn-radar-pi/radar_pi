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

#ifndef _RADARDRAWVERTEX_H_
#define _RADARDRAWVERTEX_H_

#include "RadarDraw.h"
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

#define BUFFER_SIZE (2000000)

class RadarDrawVertex : public RadarDraw {
 public:
  RadarDrawVertex(RadarInfo* ri) {
    wxCriticalSectionLocker lock(m_exclusive);

    m_ri = ri;

    for (size_t i = 0; i < ARRAY_SIZE(m_vertices); i++) {
      m_vertices[i].count = 0;
      m_vertices[i].allocated = 0;
      m_vertices[i].timeout = 0;
      m_vertices[i].points = 0;
    }
    m_count = 0;
    m_oom = false;

    m_polarLookup = GetPolarToCartesianLookupTable();
  }

  bool Init();
  void DrawRadarImage();
  void ProcessRadarSpoke(int transparency, SpokeBearing angle, UINT8* data, size_t len);

  ~RadarDrawVertex() {
    wxCriticalSectionLocker lock(m_exclusive);

    for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
      if (m_vertices[i].points) {
        free(m_vertices[i].points);
      }
    }
  }

 private:
  RadarInfo* m_ri;

  static const int VERTEX_PER_TRIANGLE = 3;
  static const int VERTEX_PER_QUAD = 2 * VERTEX_PER_TRIANGLE;
  static const int MAX_BLOBS_PER_LINE = RETURNS_PER_LINE;

  struct VertexPoint {
    GLfloat x;
    GLfloat y;
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
  };

  struct VertexLine {
    VertexPoint* points;
    time_t timeout;
    size_t count;
    size_t allocated;
  };

  PolarToCartesianLookupTable* m_polarLookup;

  wxCriticalSection m_exclusive;  // protects the following
  VertexLine m_vertices[LINES_PER_ROTATION];
  unsigned int m_count;
  bool m_oom;

  void SetBlob(VertexLine* line, int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green, GLubyte blue,
               GLubyte alpha);
};

PLUGIN_END_NAMESPACE

#endif /* _RADARDRAWVERTEX_H_ */
