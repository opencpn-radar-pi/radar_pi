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
//#include "chart1.h"

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

IMPLEMENT_CLASS(GuardZoneBogey, wxDialog)

BEGIN_EVENT_TABLE(GuardZoneBogey, wxDialog)

EVT_CLOSE(GuardZoneBogey::OnClose)
EVT_BUTTON(ID_CONFIRM, GuardZoneBogey::OnIdConfirmClick)
EVT_BUTTON(ID_CLOSE, GuardZoneBogey::OnIdCloseClick)

END_EVENT_TABLE()

GuardZoneBogey::GuardZoneBogey()
{
    Init();
}

GuardZoneBogey::~GuardZoneBogey()
{
}

void GuardZoneBogey::Init()
{
}

bool GuardZoneBogey::Create(wxWindow *parent, br24radar_pi *pPI, wxWindowID id,
                            const wxString  &m_caption, const wxPoint   &pos,
                            const wxSize    &size, long style)
{
    pParent = parent;
    pPlugIn = pPI;

#ifdef wxMSW
    long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#else
    long wstyle =                 wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#endif

    wxSize  size_min = size;

    if(!wxDialog::Create(parent, id, m_caption, pos, size_min, wstyle)) return false;

    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}


void GuardZoneBogey::CreateControls()
{
    const int border = 5;

    wxBoxSizer  *GuardZoneBogeySizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(GuardZoneBogeySizer);

    pBogeyCountText = new wxStaticText(this, wxID_ANY, _("Zone 1: unknown\nZone 2: unknown\nTimeout\n"), wxDefaultPosition, wxDefaultSize, 0);
    GuardZoneBogeySizer->Add(pBogeyCountText, 0, wxALIGN_LEFT | wxALL, border);


    wxButton    *bConfirm = new wxButton(this, ID_CONFIRM, _("&Confirm"), wxDefaultPosition, wxDefaultSize, 0);
    GuardZoneBogeySizer->Add(bConfirm, 0, wxALIGN_CENTER_VERTICAL | wxALL, border);

    wxButton    *bClose = new wxButton(this, ID_CLOSE, _("C&onfirm + Close"), wxDefaultPosition, wxDefaultSize, 0);
    GuardZoneBogeySizer->Add(bClose, 0, wxALIGN_CENTER_VERTICAL | wxALL, border);
}

//*********************************************************************************************************************

void GuardZoneBogey::SetBogeyCount(int *bogey_count, int next_alarm)
{
    wxString text;
	static wxString previous_text;
    wxString t;
	
	if (pPlugIn->br_radar_state[0] == RADAR_ON){
		t.Printf(_("Radar A:\n"));
		text << t;
		for (int z = 0; z < GUARD_ZONES; z++) {
			text << _("Zone");
			t.Printf(wxT(" %d: %d\n"), z + 1, bogey_count[z]);
	//		wxLogMessage(wxT("BR24radar_pi: XXset bogeycount z=%d, bogey_count[z]= %d"),z, bogey_count[z]);
			text += t;
		}
		t.Printf(_("\n"));
		text += t;
	}
	if (pPlugIn->br_radar_state[1] == RADAR_ON){
		t.Printf(_("Radar B:\n"));
		text << t;
		for (int z = 0; z < GUARD_ZONES; z++) {
			text << _("Zone");
			t.Printf(wxT(" %d: %d\n"), z + 1, bogey_count[z + 2]);
			text += t;
		}
	}  

    if (next_alarm >= 0) {
        t.Printf(_("Next alarm in %d s"), next_alarm);
        text += t;
  }
/*	else{
		t.Printf(_("\n"));
		text += t;
	}*/
	if (previous_text != text){
		pBogeyCountText->SetLabel(text);
	}
	previous_text = text;
	Fit();

}

void GuardZoneBogey::OnClose(wxCloseEvent &event)
{
    pPlugIn->OnGuardZoneBogeyClose();
    event.Skip();
}

void GuardZoneBogey::OnIdConfirmClick(wxCommandEvent &event)
{
    pPlugIn->OnGuardZoneBogeyConfirm();
    event.Skip();
}

void GuardZoneBogey::OnIdCloseClick(wxCommandEvent &event)
{
    pPlugIn->OnGuardZoneBogeyClose();
    event.Skip();
}
