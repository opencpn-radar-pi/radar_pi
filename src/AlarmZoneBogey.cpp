/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif                          //precompiled headers

#include <wx/socket.h>
#include "wx/apptrait.h"
#include <wx/glcanvas.h>
#include "wx/sckaddr.h"
#include "wx/datetime.h"
#include <wx/fileconf.h>
#include "chart1.h"

#include <math.h>
#include <fstream>

using namespace std;

#ifdef __WXGTK__
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

#ifdef __WXMSW__
#include "GL/glu.h"
#endif

#include "br24radar_pi.h"

enum {                                      // process ID's
    ID_CONFIRM,
    ID_CLOSE
};

IMPLEMENT_CLASS(AlarmZoneBogey, wxDialog)

BEGIN_EVENT_TABLE(AlarmZoneBogey, wxDialog)

    EVT_CLOSE(AlarmZoneBogey::OnClose)
    EVT_BUTTON(ID_CONFIRM, AlarmZoneBogey::OnIdConfirmClick)
    EVT_BUTTON(ID_CLOSE, AlarmZoneBogey::OnIdCloseClick)

END_EVENT_TABLE()

AlarmZoneBogey::AlarmZoneBogey()
{
    Init();
}

AlarmZoneBogey::~AlarmZoneBogey()
{
}

void AlarmZoneBogey::Init()
{
}

bool AlarmZoneBogey::Create(wxWindow *parent, br24radar_pi *pPI, wxWindowID id,
                                const wxString  &m_caption, const wxPoint   &pos,
                                const wxSize    &size, long style)
{
    pParent = parent;
    pPlugIn = pPI;

    long    wstyle = wxDEFAULT_FRAME_STYLE;

    wxSize  size_min = size;

    if(!wxDialog::Create(parent, id, m_caption, pos, size_min, wstyle)) return false;

    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}


void AlarmZoneBogey::CreateControls()
{
    const int border = 5;

    wxBoxSizer  *AlarmZoneBogeySizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(AlarmZoneBogeySizer);

    pBogeyCountText = new wxStaticText(this, wxID_ANY, _("Bogey Count"), wxDefaultPosition, wxDefaultSize, 0);
    AlarmZoneBogeySizer->Add(pBogeyCountText, 0, wxALIGN_LEFT | wxALL, border);

    wxButton    *bConfirm = new wxButton(this, ID_CONFIRM, _("&Confirm"), wxDefaultPosition, wxDefaultSize, 0);
    AlarmZoneBogeySizer->Add(bConfirm, 0, wxALIGN_CENTER_VERTICAL | wxALL, border);

    wxButton    *bClose = new wxButton(this, ID_CLOSE, _("C&onfirm + Close"), wxDefaultPosition, wxDefaultSize, 0);
    AlarmZoneBogeySizer->Add(bClose, 0, wxALIGN_CENTER_VERTICAL | wxALL, border);
}

//*********************************************************************************************************************

void AlarmZoneBogey::SetBogeyCount(int *bogey_count, int next_alarm)
{
    wxString text;
    wxString t;
    
    text = wxT("");
    for (size_t z = 0; z < GUARD_ZONES; z++) {
        t.Printf(wxT("Zone %d: %d "), z + 1, bogey_count[z]);
        text += t;
    }

    if (next_alarm >= 0) {
        t.Printf(wxT("Next alarm in %d seconds"), next_alarm);
        text += t;
    }
    pBogeyCountText->SetLabel(text);
}

void AlarmZoneBogey::OnClose(wxCloseEvent &event)
{
    pPlugIn->OnAlarmZoneBogeyClose();
    event.Skip();
}

void AlarmZoneBogey::OnIdConfirmClick(wxCommandEvent &event)
{
    pPlugIn->OnAlarmZoneBogeyConfirm();
    event.Skip();
}

void AlarmZoneBogey::OnIdCloseClick(wxCommandEvent &event)
{
    pPlugIn->OnAlarmZoneBogeyClose();
    event.Skip();
}
