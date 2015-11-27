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

#ifndef _RADARDRAWVERTEX_H_
#define _RADARDRAWVERTEX_H_

#include "br24radar_pi.h"

class RadarDrawVertex : public RadarDraw
{
public:
    RadarDrawVertex()
    {
      m_pi = 0;
      start_line = LINES_PER_ROTATION;
      end_line = 0;
    }

    bool Init(br24radar_pi * pi, int color_option);

    void DrawRadarImage(wxPoint center, double scale, double rotation, bool overlay);
    void ProcessRadarSpoke(SpokeBearing angle, UINT8 * data, size_t len);

    ~RadarDrawVertex()
    {
    }

private:
    void SetBlob(int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

    br24radar_pi  * m_pi;

    static const int VERTEX_PER_TRIANGLE = 3;
    static const int VERTEX_PER_QUAD = 2 * VERTEX_PER_TRIANGLE;
    static const int VERTEX_MAX = 100 * VERTEX_PER_QUAD; // Assume picture is no more complicated than this

    struct vertex_point {
        GLfloat x;
        GLfloat y;
        GLubyte red;
        GLubyte green;
        GLubyte blue;
        GLubyte alpha;
    };

    struct vertex_spoke {
        vertex_point    points[VERTEX_MAX];
        int             n;
    };

    vertex_spoke    spokes[LINES_PER_ROTATION];

    GLfloat         polar_to_cart_x[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
    GLfloat         polar_to_cart_y[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];

    int             start_line;
    int             end_line;
};

#endif /* _RADARDRAWVERTEX_H_ */

// vim: sw=4:ts=8:
