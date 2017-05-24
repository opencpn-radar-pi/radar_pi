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

#ifndef _DRAWUTIL_H_
#define _DRAWUTIL_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

extern void DrawArc(float cx, float cy, float r, float start_angle, float arc_angle, int num_segments);
extern void DrawOutlineArc(double r1, double r2, double a1, double a2, bool stippled);
extern void DrawFilledArc(double r1, double r2, double a1, double a2);
extern void CheckOpenGLError(const wxString& after);

struct PolarToCartesianLookupTable {
  GLfloat x[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  GLfloat y[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  int intx[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  int inty[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
};

extern PolarToCartesianLookupTable* GetPolarToCartesianLookupTable();

extern void DrawRoundRect(float x, float y, float width, float height, float radius = 0.0);

PLUGIN_END_NAMESPACE

#endif
