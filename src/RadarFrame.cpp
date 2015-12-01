/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Authors:  David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2015 by Kees Verruijt         canboat@verruijt.net *
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

#include "RadarFrame.h"

BEGIN_EVENT_TABLE(RadarFrame, wxFrame)
    EVT_CLOSE(RadarFrame::close)
    EVT_MOVE(RadarFrame::moved)
    EVT_SIZE(RadarFrame::resized)
END_EVENT_TABLE()

RadarFrame::RadarFrame( br24radar_pi * pi, RadarInfo * ri, wxWindow * parent ) :
    wxFrame(parent, wxID_ANY, _("RADAR"))
{
    m_parent = parent;
    m_pi = pi;
    m_ri = ri;
}

RadarFrame::~RadarFrame()
{
}

void RadarFrame::moved( wxMoveEvent& evt )
{
    m_pi->m_dialogLocation[DL_RADARWINDOW + m_ri->radar].pos = GetPosition();
}

void RadarFrame::resized( wxSizeEvent& evt )
{
    wxSize s = GetClientSize();
    wxSize n;
    n.x = MIN(s.x, s.y);
    n.y = s.x;

    if (n.x != s.x || n.y != s.y) {
        //SetClientSize(n);
        //m_parent->Layout();
    }
    m_pi->m_dialogLocation[DL_RADARWINDOW + m_ri->radar].size = GetSize();
    //Fit();
}

void RadarFrame::close( wxCloseEvent& evt )
{
    this->Hide();
    evt.Skip();
}

// vim: sw=4:ts=8:
