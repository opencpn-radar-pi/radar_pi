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
    ID_OK_Z,
    ID_ALARMZONES
};

wxString GuardZoneNames[3] = { wxT("Off"), wxT("Arc"), wxT("Circle") };

bool    outer_set;

//---------------------------------------------------------------------------------------
//          Guard Controls Implementation
//---------------------------------------------------------------------------------------

IMPLEMENT_CLASS(GuardZoneDialog, wxDialog)

BEGIN_EVENT_TABLE(GuardZoneDialog, wxDialog)

    EVT_CLOSE(GuardZoneDialog::OnClose)
    EVT_BUTTON(ID_OK_Z, GuardZoneDialog::OnIdOKClick)

END_EVENT_TABLE()

GuardZoneDialog::GuardZoneDialog()
{
    Init();
}

GuardZoneDialog::~GuardZoneDialog()
{
}

void GuardZoneDialog::Init()
{
}

bool GuardZoneDialog::Create(wxWindow *parent, br24radar_pi *pPI, wxWindowID id,
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

    wxSize  size_min = wxSize(200, 200);

    if(!wxDialog::Create(parent, id, m_caption, pos, size_min, wstyle)) return false;

    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}


void GuardZoneDialog::CreateControls()
{
    int border_size = 4;

    wxBoxSizer  *GuardZoneSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(GuardZoneSizer);

    // Guard Zone options
    wxStaticBox         *BoxGuardZone = new wxStaticBox(this, wxID_ANY, _("Guard Zones"));
    wxStaticBoxSizer    *BoxGuardZoneSizer = new wxStaticBoxSizer(BoxGuardZone, wxVERTICAL);
    GuardZoneSizer->Add(BoxGuardZoneSizer, 0, wxEXPAND | wxALL, border_size);

    pZoneNumber = new wxTextCtrl(this, wxID_ANY);
    BoxGuardZoneSizer->Add(pZoneNumber, 1, wxALIGN_LEFT | wxALL, 5);

    pGuardZoneType = new wxRadioBox (this, ID_ALARMZONES, _("Zone Type:"),
                                            wxDefaultPosition, wxDefaultSize,
                                            3, GuardZoneNames, 1, wxRA_SPECIFY_COLS );

    BoxGuardZoneSizer->Add(pGuardZoneType, 0, wxALL | wxEXPAND, 2);

    pGuardZoneType->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED,
                                wxCommandEventHandler(GuardZoneDialog::OnGuardZoneModeClick),
                                NULL, this );

    //Inaner and Outer Ranges
    wxString m_temp;
    wxStaticText *pInner_Range_Text = new wxStaticText(this, wxID_ANY, _("Inner Range"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxGuardZoneSizer->Add(pInner_Range_Text, 0, wxALIGN_LEFT | wxALL, 0);

    pInner_Range = new wxTextCtrl(this, wxID_ANY);
    BoxGuardZoneSizer->Add(pInner_Range, 1, wxALIGN_LEFT | wxALL, 5);
    pInner_Range->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(GuardZoneDialog::OnInner_Range_Value), NULL, this);

    wxStaticText *pOuter_Range_Text = new wxStaticText(this, wxID_ANY, _("Outer Range"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxGuardZoneSizer->Add(pOuter_Range_Text, 0, wxALIGN_LEFT | wxALL, 0);

    pOuter_Range = new wxTextCtrl(this, wxID_ANY);
    BoxGuardZoneSizer->Add(pOuter_Range, 1, wxALIGN_LEFT | wxALL, 5);
    pOuter_Range->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(GuardZoneDialog::OnOuter_Range_Value), NULL, this);

    //1st and 2nd Arc Subtending Bearings
    wxStaticText *pStart_Bearing = new wxStaticText(this, wxID_ANY, _("Start Bearing"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxGuardZoneSizer->Add(pStart_Bearing, 0, wxALIGN_LEFT | wxALL, 0);

    pStart_Bearing_Value = new wxTextCtrl(this, wxID_ANY);
    BoxGuardZoneSizer->Add(pStart_Bearing_Value, 1, wxALIGN_LEFT | wxALL, 5);
    pStart_Bearing_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(GuardZoneDialog::OnStart_Bearing_Value), NULL, this);


    wxStaticText *pEnd_Bearing = new wxStaticText(this, wxID_ANY, _("End Bearing"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxGuardZoneSizer->Add(pEnd_Bearing, 0, wxALIGN_LEFT | wxALL, 0);

    pEnd_Bearing_Value = new wxTextCtrl(this, wxID_ANY);
    BoxGuardZoneSizer->Add(pEnd_Bearing_Value, 1, wxALIGN_LEFT | wxALL, 5);
    pEnd_Bearing_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(GuardZoneDialog::OnEnd_Bearing_Value), NULL, this);


    // The Close button
    wxButton    *bClose = new wxButton(this, ID_OK_Z, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    GuardZoneSizer->Add(bClose, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

}

//*********************************************************************************************************************

void GuardZoneDialog::SetVisibility()
{
    GuardZoneType zoneType = (GuardZoneType) pGuardZoneType->GetSelection();

    pPlugIn->guardZones[pPlugIn->settings.guard_zone].type = zoneType;

    if (zoneType == GZ_OFF) {
        pStart_Bearing_Value->Disable();
        pEnd_Bearing_Value->Disable();
        pInner_Range->Disable();
        pOuter_Range->Disable();
    } else if (pGuardZoneType->GetSelection() == GZ_CIRCLE) {
        pStart_Bearing_Value->Disable();
        pEnd_Bearing_Value->Disable();
        pInner_Range->Enable();
        pOuter_Range->Enable();
    }
    else {
        pStart_Bearing_Value->Enable();
        pEnd_Bearing_Value->Enable();
        pInner_Range->Enable();
        pOuter_Range->Enable();
    }
}


void GuardZoneDialog::OnGuardZoneDialogShow(int zone)
{
    wxString GuardZoneText;
    GuardZoneText.Printf(_T("%s: %i"),_("Zone"), zone + 1);
    pZoneNumber->SetValue(GuardZoneText);

    pGuardZoneType->SetSelection(pPlugIn->guardZones[zone].type);

    GuardZoneText.Printf(wxT("%2.2f"), pPlugIn->guardZones[zone].inner_range);
    pInner_Range->SetValue(GuardZoneText);

    GuardZoneText.Printf(wxT("%2.2f"), pPlugIn->guardZones[zone].outer_range);
    pOuter_Range->SetValue(GuardZoneText);

    GuardZoneText.Printf(wxT("%3.1f"), pPlugIn->guardZones[zone].start_bearing);
    pStart_Bearing_Value->SetValue(GuardZoneText);
    GuardZoneText.Printf(wxT("%3.1f"), pPlugIn->guardZones[zone].end_bearing);
    pEnd_Bearing_Value->SetValue(GuardZoneText);

    SetVisibility();
}

void GuardZoneDialog::OnGuardZoneModeClick(wxCommandEvent &event)
{
    SetVisibility();
    outer_set = false;
}

void GuardZoneDialog::OnInner_Range_Value(wxCommandEvent &event)
{
    wxString temp = pInner_Range->GetValue();

    temp.ToDouble(&pPlugIn->guardZones[pPlugIn->settings.guard_zone].inner_range);
}

void GuardZoneDialog::OnOuter_Range_Value(wxCommandEvent &event)
{
    wxString temp = pOuter_Range->GetValue();

    temp.ToDouble(&pPlugIn->guardZones[pPlugIn->settings.guard_zone].outer_range);
}

void GuardZoneDialog::OnStart_Bearing_Value(wxCommandEvent &event)
{
    wxString temp = pStart_Bearing_Value->GetValue();

    temp.ToDouble(&pPlugIn->guardZones[pPlugIn->settings.guard_zone].start_bearing);
}

void GuardZoneDialog::OnEnd_Bearing_Value(wxCommandEvent &event)
{
    wxString temp = pEnd_Bearing_Value->GetValue();

    temp.ToDouble(&pPlugIn->guardZones[pPlugIn->settings.guard_zone].end_bearing);
}

void GuardZoneDialog::OnClose(wxCloseEvent &event)
{
    pPlugIn->OnGuardZoneDialogClose();
    event.Skip();
}

void GuardZoneDialog::OnIdOKClick(wxCommandEvent &event)
{
    pPlugIn->OnGuardZoneDialogClose();
    event.Skip();
}

void GuardZoneDialog::OnContextMenuGuardCallback(double mark_rng, double mark_brg)
{
    if(!outer_set) {
        pPlugIn->guardZones[pPlugIn->settings.guard_zone].outer_range = mark_rng;
        pPlugIn->guardZones[pPlugIn->settings.guard_zone].start_bearing = mark_brg;

        outer_set = true;
    }
    else {
        pPlugIn->guardZones[pPlugIn->settings.guard_zone].inner_range = mark_rng;
        if (pPlugIn->guardZones[pPlugIn->settings.guard_zone].outer_range < mark_rng) {
            pPlugIn->guardZones[pPlugIn->settings.guard_zone].inner_range = pPlugIn->guardZones[pPlugIn->settings.guard_zone].outer_range;
            pPlugIn->guardZones[pPlugIn->settings.guard_zone].outer_range = mark_rng;
        }
        pPlugIn->guardZones[pPlugIn->settings.guard_zone].end_bearing = mark_brg;
        outer_set = false;
    }

    OnGuardZoneDialogShow(pPlugIn->settings.guard_zone);
}
