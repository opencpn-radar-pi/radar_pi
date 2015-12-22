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
#include "RadarCanvas.h"

enum {  // process ID's
  ID_CONFIRM,
  ID_CLOSE
};

BEGIN_EVENT_TABLE(RadarPanel, wxPanel)
EVT_LEFT_UP(RadarPanel::OnMouseClick)
END_EVENT_TABLE()

RadarPanel::RadarPanel(br24radar_pi* pi, RadarInfo* ri, wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _("RADAR")) {
  m_parent = parent;
  m_pi = pi;
  m_ri = ri;
}

bool RadarPanel::Create() {
  m_aui_mgr = GetFrameAuiManager();
  m_aui_name = wxString::Format(wxT("BR24radar_pi-%d"), m_ri->radar);
  wxAuiPaneInfo p = wxAuiPaneInfo()
                        .Name(m_aui_name)
                        .Caption(m_ri->name)
                        .CaptionVisible(true)  // Show caption even when docked
                        .Dockable(true)        // Dockable everywhere
                        .MinSize(wxSize(256, 256))
                        .BestSize(wxSize(512, 512))
                        .FloatingSize(wxSize(512, 512))
                        .Float()
                        .CloseButton(true)
                        .Gripper(false);

  m_ri->radar_canvas =
      new RadarCanvas(m_pi, m_ri, this, wxSize(256, 256));  // m_pi->m_dialogLocation[DL_RADARWINDOW + radar].size);
  if (!m_ri->radar_canvas) {
    wxLogMessage(wxT("BR24radar_pi %s: Unable to create RadarCanvas"), m_ri->name);
    return false;
  }

  wxBoxSizer* Sizer = new wxBoxSizer(wxHORIZONTAL);
  Sizer->Add(m_ri->radar_canvas, 0, wxEXPAND | wxALL, 0);
  SetSizer(Sizer);

  DimeWindow(this);
  Fit();
  Layout();
  SetMinSize(GetBestSize());
  Refresh();

  m_aui_mgr->AddPane(this, p);
  m_aui_mgr->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(RadarPanel::close), NULL, this);

  m_aui_mgr->Update();
  wxLogMessage(wxT("BR24radar_pi: Added panel %s to AUI control manager"), m_aui_name.c_str());

  return true;
}

RadarPanel::~RadarPanel() {}

void RadarPanel::SetCaption(wxString name) { m_aui_mgr->GetPane(this).Caption(name); }

void RadarPanel::resized(wxSizeEvent& evt) {
  wxSize s = GetClientSize();
  wxSize n;
  n.x = MIN(s.x, s.y);
  n.y = s.x;

  if (n.x != s.x || n.y != s.y) {
    // SetClientSize(n);
    // m_parent->Layout();
  }
  // Fit();
}

void RadarPanel::close(wxAuiManagerEvent& event) {
  // m_visible = false;
  event.Skip();
}

void RadarPanel::ShowFrame(bool visible) {
  m_aui_mgr->GetPane(this).Show(visible);
  // m_visible = visible;
  m_aui_mgr->Update();
}

void RadarPanel::OnMouseClick(wxMouseEvent& event) {
  m_pi->ShowRadarControl(m_ri->radar, true);
  event.Skip();
}

#include "pi_trail.h"
