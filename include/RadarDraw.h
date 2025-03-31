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

#ifndef _RADAR_DRAW_H_
#define _RADAR_DRAW_H_

#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class RadarDraw {
public:
    static RadarDraw* make_Draw(radar_pi* pi, RadarInfo* ri, int draw_method);

    virtual bool Init(size_t spokes, size_t max_spoke_len) = 0;
    virtual void DrawRadarOverlayImage(double radar_scale, double panel_rotate)
        = 0;
    virtual void DrawRadarPanelImage(double panel_scale, double panel_rotate)
        = 0;
    virtual void ProcessRadarSpoke(int transparency, SpokeBearing angle,
        uint8_t* data, size_t len, GeoPosition spoke_pos)
        = 0;

    virtual ~RadarDraw() = 0;

    static void GetDrawingMethods(wxArrayString& methods);
};

PLUGIN_END_NAMESPACE

#endif
