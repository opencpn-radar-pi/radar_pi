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

#include "RadarDraw.h"
#include "RadarDrawVertex.h"
#include "RadarDrawShader.h"

// Factory to generate a particular draw implementation
RadarDraw * RadarDraw::make_Draw( br24radar_pi * pi, int useShader )
{
    if (pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("Creating new %s draw method"), useShader ? "Shader" : "Vertex");
    }
    if (useShader) {
        return new RadarDrawShader(pi);
    }
    else {
        return new RadarDrawVertex(pi);
    }
}

RadarDraw::~RadarDraw()
{
}

// vim: sw=4:ts=8:
