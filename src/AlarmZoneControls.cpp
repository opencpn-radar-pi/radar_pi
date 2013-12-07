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
#include "chcanv.h" //
#include "navutil.h"
#include "routeman.h"

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

bool    outer_set, inner_set;
 
//---------------------------------------------------------------------------------------
//          Alarm Controls Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(AlarmZoneDialog, wxDialog)

BEGIN_EVENT_TABLE(AlarmZoneDialog, wxDialog)

    EVT_CLOSE(AlarmZoneDialog::OnClose)
    EVT_BUTTON(ID_OK_Z, AlarmZoneDialog::OnIdOKClick)

END_EVENT_TABLE()

AlarmZoneDialog::AlarmZoneDialog()
{
    Init();
}

AlarmZoneDialog::~AlarmZoneDialog()
{
}

void AlarmZoneDialog::Init()
{    
}

bool AlarmZoneDialog::Create(wxWindow *parent, br24radar_pi *pPI, wxWindowID id,
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

void AlarmZoneDialog::CreateControls()
{
    int border_size = 4;

    wxBoxSizer  *AlarmZoneSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(AlarmZoneSizer);

    // Alarm Zone options 
    wxStaticBox         *BoxAlarmZone = new wxStaticBox(this, wxID_ANY, _("Alarm Zones"));
    wxStaticBoxSizer    *BoxAlarmZoneSizer = new wxStaticBoxSizer(BoxAlarmZone, wxVERTICAL);
    AlarmZoneSizer->Add(BoxAlarmZoneSizer, 0, wxEXPAND | wxALL, border_size);

    pZoneNumber = new wxTextCtrl(this, wxID_ANY);
    BoxAlarmZoneSizer->Add(pZoneNumber, 1, wxALIGN_LEFT | wxALL, 5);

    // Alarm Zone type selection
    wxString    ZoneTypeStrings[] =
    {
        _("Arc"),
        _("Circle")
    };

    pAlarmZoneType = new wxRadioBox (this, ID_ALARMZONES, _("Zone Type:"),
                                            wxDefaultPosition, wxDefaultSize,
                                            2, ZoneTypeStrings, 1, wxRA_SPECIFY_COLS );

    BoxAlarmZoneSizer->Add(pAlarmZoneType, 0, wxALL | wxEXPAND, 2);

    pAlarmZoneType->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED,
                                wxCommandEventHandler(AlarmZoneDialog::OnAlarmZoneModeClick),
                                NULL, this );

    //Inaner and Outer Ranges
    wxString m_temp;
    wxStaticText *pInner_Range_Text = new wxStaticText(this, wxID_ANY, _("Inner Range"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxAlarmZoneSizer->Add(pInner_Range_Text, 0, wxALIGN_LEFT | wxALL, 0);

    pInner_Range = new wxTextCtrl(this, wxID_ANY);
    BoxAlarmZoneSizer->Add(pInner_Range, 1, wxALIGN_LEFT | wxALL, 5);
    pInner_Range->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(AlarmZoneDialog::OnInner_Range_Value), NULL, this);

    wxStaticText *pOuter_Range_Text = new wxStaticText(this, wxID_ANY, _("Outer Range"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxAlarmZoneSizer->Add(pOuter_Range_Text, 0, wxALIGN_LEFT | wxALL, 0);

    pOuter_Range = new wxTextCtrl(this, wxID_ANY);
    BoxAlarmZoneSizer->Add(pOuter_Range, 1, wxALIGN_LEFT | wxALL, 5);
    pOuter_Range->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(AlarmZoneDialog::OnOuter_Range_Value), NULL, this);
   
    //1st and 2nd Arc Subtending Bearings
    wxStaticText *pStart_Bearing = new wxStaticText(this, wxID_ANY, _("Start Bearing"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxAlarmZoneSizer->Add(pStart_Bearing, 0, wxALIGN_LEFT | wxALL, 0);

    pStart_Bearing_Value = new wxTextCtrl(this, wxID_ANY);
    BoxAlarmZoneSizer->Add(pStart_Bearing_Value, 1, wxALIGN_LEFT | wxALL, 5);
    pStart_Bearing_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(AlarmZoneDialog::OnStart_Bearing_Value), NULL, this);
    

    wxStaticText *pEnd_Bearing = new wxStaticText(this, wxID_ANY, _("End Bearing"),wxDefaultPosition,
        wxDefaultSize, 0);
    BoxAlarmZoneSizer->Add(pEnd_Bearing, 0, wxALIGN_LEFT | wxALL, 0);

    pEnd_Bearing_Value = new wxTextCtrl(this, wxID_ANY);
    BoxAlarmZoneSizer->Add(pEnd_Bearing_Value, 1, wxALIGN_LEFT | wxALL, 5);
    pEnd_Bearing_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(AlarmZoneDialog::OnEnd_Bearing_Value), NULL, this);
   
    
    // The Close button 
    wxButton    *bClose = new wxButton(this, ID_OK_Z, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    AlarmZoneSizer->Add(bClose, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

}

//*********************************************************************************************************************
void AlarmZoneDialog::OnAlarmZoneDialogShow(int zone)
{
    wxString AlarmZoneText;
    AlarmZoneText.Printf(wxT("Zone: %i"), zone);
    pZoneNumber->SetValue(AlarmZoneText);

    if (zone == 1)
    {
        pAlarmZoneType->SetSelection(pPlugIn->Zone1.type);
        
        AlarmZoneText.Printf(wxT("%2.2f"), pPlugIn->Zone1.inner_range);
        pInner_Range->SetValue(AlarmZoneText);

        AlarmZoneText.Printf(wxT("%2.2f"), pPlugIn->Zone1.outer_range);
        pOuter_Range->SetValue(AlarmZoneText);

        if (pPlugIn->Zone1.type == 1) {
                pStart_Bearing_Value->Disable();
                pEnd_Bearing_Value->Disable();
            }
            else {
                pStart_Bearing_Value->Enable();
                pEnd_Bearing_Value->Enable();
            
                AlarmZoneText.Printf(wxT("%3.1f°  "), pPlugIn->Zone1.start_bearing);
                pStart_Bearing_Value->SetValue(AlarmZoneText);

                AlarmZoneText.Printf(wxT("%3.1f°  "), pPlugIn->Zone1.end_bearing);
                pEnd_Bearing_Value->SetValue(AlarmZoneText);
            }
    }

    if (zone == 2)
    {
        pAlarmZoneType->SetSelection(pPlugIn->Zone2.type);

        AlarmZoneText.Printf(wxT("%2.2f"),pPlugIn->Zone2.inner_range);
        pInner_Range->SetValue(AlarmZoneText);

        AlarmZoneText.Printf(wxT("%2.2f"),pPlugIn->Zone2.outer_range);
        pOuter_Range->SetValue(AlarmZoneText);

        if (pPlugIn->Zone2.type == 1) {
                pStart_Bearing_Value->Disable();
                pEnd_Bearing_Value->Disable();
            }
            else {
                pStart_Bearing_Value->Enable();
                pEnd_Bearing_Value->Enable();
            
                AlarmZoneText.Printf(wxT("%3.1f°  "), pPlugIn->Zone2.start_bearing);
                pStart_Bearing_Value->SetValue(AlarmZoneText);

                AlarmZoneText.Printf(wxT("%3.1f°  "), pPlugIn->Zone2.end_bearing);
                pEnd_Bearing_Value->SetValue(AlarmZoneText);
            }
    }
}

void AlarmZoneDialog::OnAlarmZoneModeClick(wxCommandEvent &event)
{
 if (pPlugIn->settings.alarm_zone == 1)
     pPlugIn->Zone1.type = pAlarmZoneType->GetSelection();

  if (pPlugIn->settings.alarm_zone == 2)
     pPlugIn->Zone2.type = pAlarmZoneType->GetSelection();
  
  if (pAlarmZoneType->GetSelection() == 1){
      pStart_Bearing_Value->Disable();
      pEnd_Bearing_Value->Disable();
    }
  else {
      pStart_Bearing_Value->Enable();
      pEnd_Bearing_Value->Enable();
  }

  outer_set = false;
  inner_set = false;

}

void AlarmZoneDialog::OnInner_Range_Value(wxCommandEvent &event)
{
    wxString temp = pInner_Range->GetValue();

    if (pPlugIn->settings.alarm_zone == 1)
         temp.ToDouble(&pPlugIn->Zone1.inner_range);

    if (pPlugIn->settings.alarm_zone == 2)
         temp.ToDouble(&pPlugIn->Zone2.inner_range);

}

void AlarmZoneDialog::OnOuter_Range_Value(wxCommandEvent &event)
{
    wxString temp = pOuter_Range->GetValue();

    if (pPlugIn->settings.alarm_zone == 1)
         temp.ToDouble(&pPlugIn->Zone1.outer_range);

    if (pPlugIn->settings.alarm_zone == 2)
         temp.ToDouble(&pPlugIn->Zone2.outer_range);
}

void AlarmZoneDialog::OnStart_Bearing_Value(wxCommandEvent &event)
{
    wxString temp = pStart_Bearing_Value->GetValue();

    if (pPlugIn->settings.alarm_zone == 1)
         temp.ToDouble(&pPlugIn->Zone1.start_bearing);

    if (pPlugIn->settings.alarm_zone == 2)
         temp.ToDouble(&pPlugIn->Zone2.start_bearing);
}

void AlarmZoneDialog::OnEnd_Bearing_Value(wxCommandEvent &event)
{
    wxString temp = pEnd_Bearing_Value->GetValue();

    if (pPlugIn->settings.alarm_zone == 1)
         temp.ToDouble(&pPlugIn->Zone1.end_bearing);

    if (pPlugIn->settings.alarm_zone == 2)
         temp.ToDouble(&pPlugIn->Zone2.end_bearing);
}

void AlarmZoneDialog::OnClose(wxCloseEvent &event)
{
    pPlugIn->OnAlarmZoneDialogClose();
    event.Skip();
}

void AlarmZoneDialog::OnIdOKClick(wxCommandEvent &event)
{
    pPlugIn->OnAlarmZoneDialogClose();
    event.Skip();
}

void AlarmZoneDialog::OnContextMenuAlarmCallback(double mark_rng, double mark_brg)
{ 

  if(!outer_set){      
    if (pPlugIn->settings.alarm_zone == 1){
         pPlugIn->Zone1.outer_range = mark_rng;
         pPlugIn->Zone1.start_bearing = mark_brg;
    }

    if (pPlugIn->settings.alarm_zone == 2){
         pPlugIn->Zone2.outer_range = mark_rng;
         pPlugIn->Zone2.start_bearing = mark_brg;
    }
    outer_set = true;
  }
  else if(!inner_set) {
      if (pPlugIn->settings.alarm_zone == 1){
          pPlugIn->Zone1.inner_range = mark_rng;
          if(pPlugIn->Zone1.outer_range < mark_rng){
              pPlugIn->Zone1.inner_range = pPlugIn->Zone1.outer_range;
              pPlugIn->Zone1.outer_range = mark_rng;
              }
          }
         pPlugIn->Zone1.end_bearing = mark_brg;

    if (pPlugIn->settings.alarm_zone == 2){
         pPlugIn->Zone2.inner_range = mark_rng;
          if(pPlugIn->Zone2.outer_range < mark_rng){
              pPlugIn->Zone2.inner_range = pPlugIn->Zone2.outer_range;
              pPlugIn->Zone2.outer_range = mark_rng;
              }
         pPlugIn->Zone2.end_bearing = mark_brg;
        }
    inner_set = true;
    }

  if(outer_set && inner_set) {
      outer_set = false;
      inner_set = false;  
  }

  OnAlarmZoneDialogShow(pPlugIn->settings.alarm_zone);
}

