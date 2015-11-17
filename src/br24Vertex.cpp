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

#include "br24radar_pi.h"

bool br24Vertex::Init( br24radar_pi * ppi, int newColorOption )
{
    pPlugin = ppi;

    if (newColorOption) {
        // Don't care in vertex mode
    }

    // initialise polar_to_cart_y[arc + 1][radius] arrays
    for (int arc = 0; arc < LINES_PER_ROTATION + 1; arc++) {
        GLfloat sine = sinf((GLfloat) arc * PI * 2 / LINES_PER_ROTATION);
        GLfloat cosine = cosf((GLfloat) arc * PI * 2 / LINES_PER_ROTATION);
        for (int radius = 0; radius < RETURNS_PER_LINE + 1; radius++) {
            polar_to_cart_y[arc][radius] = (GLfloat) radius * sine;
            polar_to_cart_x[arc][radius] = (GLfloat) radius * cosine;
        }
    }

    wxLogMessage(wxT("BR24radar_pi: CPU oriented OpenGL vertex draw loaded"));
    start_line = LINES_PER_ROTATION;
    end_line = 0;

    return true;
}

void br24Vertex::ClearSpoke(int angle)
{
    spokes[angle].n = 0;
}

#define ADD_VERTEX_POINT(angle, radius, r, g, b, a)       \
    { \
      p->x = polar_to_cart_x[angle][radius]; \
      p->y = polar_to_cart_y[angle][radius]; \
      p->red = r; \
      p->green = g; \
      p->blue = b; \
      p->alpha = a; \
      p++; \
    }

void br24Vertex::SetBlob(int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    int arc1 = MOD_ROTATION2048(angle_begin);
    int arc2 = MOD_ROTATION2048(angle_end);

    vertex_point * p = &spokes[arc1].points[spokes[arc1].n];

    wxLogMessage(wxT("BR24radar_pi: > vertices array n=%d arc=%d"), spokes[arc1].n, arc1);

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
        spokes[arc1].n = VERTEX_MAX - VERTEX_PER_QUAD; // Make room for last vertex of all, to make sure we draw outer border
    }

    wxLogMessage(wxT("BR24radar_pi: < vertices array n=%d arc=%d"), spokes[arc1].n, arc1);
}

void br24Vertex::DrawRadarImage( wxPoint center, double scale, bool overlay )
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);

    if (overlay) {
//        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glPushMatrix();
    glTranslated(center.x, center.y, 0);

    glPushMatrix();
    glScaled(scale, scale, 1.);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < LINES_PER_ROTATION; i++) {
        int number_of_points = spokes[i].n;
        if (number_of_points > 0)
        {
            glVertexPointer(2, GL_FLOAT, sizeof(vertex_point), &spokes[i].points[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_point), &spokes[i].points[0].red);
            //glVertexPointer(2, GL_FLOAT, 12, spokes[i].points);
            //glColorPointer(4, GL_UNSIGNED_BYTE, 12, &spokes[i].points[0].red);
            glDrawArrays(GL_TRIANGLES, 0, number_of_points);
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix(); // Undo scaled
    glPopMatrix(); // Undo rotated

    glPopAttrib(); // Undo blend
}

