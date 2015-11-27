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

#include "drawutil.h"

static void draw_blob_gl( double ca, double sa, double radius, double arc_width, double blob_heigth )
{
    const double blob_start = 0.0;
    const double blob_end = blob_heigth;

    double xm1 = (radius + blob_start) * ca;
    double ym1 = (radius + blob_start) * sa;
    double xm2 = (radius + blob_end) * ca;
    double ym2 = (radius + blob_end) * sa;

    double arc_width_start2 = (radius + blob_start) * arc_width;
    double arc_width_end2 = (radius + blob_end) * arc_width;

    double xa = xm1 + arc_width_start2 * sa;
    double ya = ym1 - arc_width_start2 * ca;

    double xb = xm2 + arc_width_end2 * sa;
    double yb = ym2 - arc_width_end2 * ca;

    double xc = xm1 - arc_width_start2 * sa;
    double yc = ym1 + arc_width_start2 * ca;

    double xd = xm2 - arc_width_end2 * sa;
    double yd = ym2 + arc_width_end2 * ca;

    glBegin(GL_TRIANGLES);
    glVertex2d(xa, ya);
    glVertex2d(xb, yb);
    glVertex2d(xc, yc);

    glVertex2d(xb, yb);
    glVertex2d(xc, yc);
    glVertex2d(xd, yd);
    glEnd();
}

void DrawArc( float cx, float cy, float r, float start_angle, float arc_angle, int num_segments )
{
    float theta = arc_angle / float(num_segments - 1); // - 1 comes from the fact that the arc is open

    float tangential_factor = tanf(theta);
    float radial_factor = cosf(theta);

    float x = r * cosf(start_angle);
    float y = r * sinf(start_angle);

    glBegin(GL_LINE_STRIP);
    for (int ii = 0; ii < num_segments; ii++) {
        glVertex2f(x + cx, y + cy);

        float tx = -y;
        float ty = x;

        x += tx * tangential_factor;
        y += ty * tangential_factor;

        x *= radial_factor;
        y *= radial_factor;
    }
    glEnd();
}

void DrawOutlineArc( double r1, double r2, double a1, double a2, bool stippled )
{
    if (a1 > a2) {
        a2 += 360.0;
    }
    int  segments = (a2 - a1) * 4;
    bool circle = (a1 == 0.0 && a2 == 359.0);

    if (!circle) {
        a1 -= 0.5;
        a2 += 0.5;
    }
    a1 = deg2rad(a1);
    a2 = deg2rad(a2);

    if (stippled) {
        glEnable (GL_LINE_STIPPLE);
        glLineStipple (1, 0x0F0F);
        glLineWidth(2.0);
    } else {
        glLineWidth(3.0);
    }

    DrawArc(0.0, 0.0, r1, a1, a2 - a1, segments);
    DrawArc(0.0, 0.0, r2, a1, a2 - a1, segments);

    if (!circle) {
        glBegin(GL_LINES);
        glVertex2f(r1 * cosf(a1), r1 * sinf(a1));
        glVertex2f(r2 * cosf(a1), r2 * sinf(a1));
        glVertex2f(r1 * cosf(a2), r1 * sinf(a2));
        glVertex2f(r2 * cosf(a2), r2 * sinf(a2));
        glEnd();
    }
}

void DrawFilledArc( double r1, double r2, double a1, double a2 )
{
    if (a1 > a2) {
        a2 += 360.0;
    }

    for (double n = a1; n <= a2; ++n ) {
        double nr = deg2rad(n);
        draw_blob_gl(cos(nr), sin(nr), r2, deg2rad(0.5), r1 - r2);
    }
}
