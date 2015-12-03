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

#include "RadarPanel.h"

RadarPanel::RadarPanel( br24radar_pi * pi, RadarInfo * ri, wxWindow * parent ) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _("RADAR"))
{
    m_parent = parent;
    m_pi = pi;
    m_ri = ri;

    m_aui_mgr = GetFrameAuiManager();
}

void RadarPanel::Create( )
{
    wxAuiPaneInfo p = wxAuiPaneInfo()
                        .Name(wxT("BR24radar_pi"))
                        .Caption(_("RADAR"))
                        .CaptionVisible(true)  // Show caption even when docked
                        .Dockable(true)        // Dockable everywhere
                        .MinSize(wxSize(256,256))
                        .BestSize(wxSize(512,512))
                        .FloatingSize(wxSize(512,512))
                        .FloatingPosition(100, 100)
                        .Show(true)
                        .CloseButton(true)
                        .Gripper(false);
    m_aui_mgr->AddPane(this, p);
    m_aui_mgr->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(RadarPanel::close), NULL, this);
    m_aui_mgr->Update();
}

RadarPanel::~RadarPanel()
{
}

void RadarPanel::moved( wxMoveEvent& evt )
{
    m_pi->m_dialogLocation[DL_RADARWINDOW + m_ri->radar].pos = GetPosition();
}

void RadarPanel::resized( wxSizeEvent& evt )
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

void RadarPanel::close( wxAuiManagerEvent& event )
{
    // m_visible = false;
    event.Skip();
}

void RadarPanel::ShowFrame( bool visible )
{
    m_aui_mgr->GetPane(this).Show(visible);
    // m_visible = visible;
    m_aui_mgr->Update();
}

// vim: sw=4:ts=8:
