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
//#include <wx/glcanvas.h>
#include "wx/datetime.h"
//#include <wx/fileconf.h>
//#include <fstream>
//#include "chart1.h"
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
    ID_TEXTCTRL1 = 10000,
    ID_PLUS_TEN,
    ID_PLUS,
    ID_VALUE,
    ID_MINUS,
    ID_MINUS_TEN,
    ID_AUTO,

    ID_RANGE,
    ID_GAIN,
    ID_SEA,
    ID_RAIN,
    ID_TRANSPARENCY,
    ID_REJECTION,
    
    ID_ZONE1,
    ID_ZONE2,

};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(BR24ControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24ControlsDialog, wxDialog)

    EVT_CLOSE(BR24ControlsDialog::OnClose)
//    EVT_BUTTON(ID_OK,    BR24ControlsDialog::OnIdOKClick)
    EVT_BUTTON(ID_PLUS_TEN,  BR24ControlsDialog::OnPlusTenClick)
    EVT_BUTTON(ID_PLUS,  BR24ControlsDialog::OnPlusClick)
    EVT_BUTTON(ID_VALUE, BR24ControlsDialog::OnValueClick)
    EVT_BUTTON(ID_MINUS, BR24ControlsDialog::OnMinusClick)
    EVT_BUTTON(ID_MINUS_TEN, BR24ControlsDialog::OnMinusTenClick)
    EVT_BUTTON(ID_AUTO,  BR24ControlsDialog::OnAutoClick)

    EVT_BUTTON(ID_RANGE, BR24ControlsDialog::OnRadarControlButtonClick)
    EVT_BUTTON(ID_GAIN, BR24ControlsDialog::OnRadarControlButtonClick)
    EVT_BUTTON(ID_SEA, BR24ControlsDialog::OnRadarControlButtonClick)
    EVT_BUTTON(ID_RAIN, BR24ControlsDialog::OnRadarControlButtonClick)
    EVT_BUTTON(ID_TRANSPARENCY, BR24ControlsDialog::OnRadarControlButtonClick)
    EVT_BUTTON(ID_REJECTION, BR24ControlsDialog::OnRadarControlButtonClick)

    EVT_BUTTON(ID_ZONE1, BR24ControlsDialog::OnZone1ButtonClick)
    EVT_BUTTON(ID_ZONE2, BR24ControlsDialog::OnZone2ButtonClick)

    EVT_MOVE(BR24ControlsDialog::OnMove)
    EVT_SIZE(BR24ControlsDialog::OnSize)

END_EVENT_TABLE()

//Ranges are metric for BR24 - the hex codes are little endian = 10 X range value

static const wxString g_range_names[2][18] = {
    {
        wxT("50 m"),
        wxT("75 m"),
        wxT("100 m"),
        wxT("250 m"),
        wxT("500 m"),
        wxT("750 m"),
        wxT("1 km"),
        wxT("1.5 km"),
        wxT("2 km"),
        wxT("3 km"),
        wxT("4 km"),
        wxT("6 km"),
        wxT("8 km"),
        wxT("12 km"),
        wxT("16 km"),
        wxT("24 km"),
        wxT("36 km"),
        wxT("48 km")
    },
    {
        wxT("1/20 NM"),
        wxT("1/10 NM"),
        wxT("1/8 NM"),
        wxT("1/4 NM"),
        wxT("1/2 NM"),
        wxT("3/4 NM"),
        wxT("1 NM"),
        wxT("2 NM"),
        wxT("3 NM"),
        wxT("4 NM"),
        wxT("6 NM"),
        wxT("8 NM"),
        wxT("12 NM"),
        wxT("16 NM"),
        wxT("24 NM"),
        wxT("36 NM"),
        wxT("36 NM")  // pad to same length as metric
    }

};

static const int g_range_distances[2][18] = {
    {
        50,
        75,
        100,
        250,
        500,
        750,
        1000,
        1500,
        2000,
        3000,
        4000,
        6000,
        8000,
        12000,
        16000,
        24000,
        36000,
        48000
    },
    {
        1852/20,
        1852/10,
        1852/8,
        1852/4,
        1852/2,
        1852*3/4,
        1852*1,
        1852*2,
        1852*3,
        1852*4,
        1852*6,
        1852*8,
        1852*12,
        1852*16,
        1852*24,
        1852*36
    }
};

static const int METRIC_RANGE_COUNT = 18;
static const int MILE_RANGE_COUNT = 16;

static const int g_range_maxValue[2] = { MILE_RANGE_COUNT, METRIC_RANGE_COUNT };

static const wxString g_rejection_names[4] = { wxT("Off"), wxT("Low"), wxT("Medium"), wxT("High") };

void RadarControlButton::SetValue(int newValue)
{
    if (newValue < minValue) {
        value = minValue;
    } else if (newValue > maxValue) {
        value = maxValue;
    } else {
        value = newValue;
    }
    
    wxString label;

    if (names) {
        label.Printf(wxT("%s\n%s"), firstLine, names[value]);
    } else {
        label.Printf(wxT("%s\n%d"), firstLine, value);
    }
    
    this->SetLabel(label);
    
    isAuto = false;
    pPlugIn->SetControlValue(controlType, value);
}

void RadarControlButton::SetAuto()
{
    wxString label;
    
    label.Printf(wxT("%s\nAUTO"), firstLine);
    this->SetLabel(label);
    
    isAuto = true;
    
    technicalValue = -1;
}

void RadarRangeControlButton::SetValue(int newValue)
{
    int units = pPlugIn->settings.range_units;
    
    wxLogMessage(wxT("Range units = %d\n"), units);
    
    maxValue = g_range_maxValue[units] - 1;

    if (newValue >= minValue && newValue <= maxValue) {
        value = newValue;
    }
    else if (value < minValue) {
        value = 0;
    } else if (value > maxValue) {
        value = maxValue;
    }
    wxLogMessage(wxT("Range index = %d\n"), value);
    
    int meters = g_range_distances[units][value];
    wxLogMessage(wxT("Range meters = %d\n"), meters);
    pPlugIn->SetRangeMeters(meters);
    pPlugIn->settings.auto_range_mode = false;

    wxString label;
    wxString rangeText = g_range_names[units][value];
    
    label.Printf(wxT("%s\n%s"), firstLine, rangeText);
    
    this->SetLabel(label);
    
    isAuto = false;
}

void RadarRangeControlButton::SetAuto()
{
    int units = pPlugIn->settings.range_units;
    
    wxLogMessage(wxT("Range units = %d\n"), units);
    
    maxValue = g_range_maxValue[units] - 1;
    int newValue = pPlugIn->settings.range_index;
    if (newValue >= minValue && newValue <= maxValue) {
        value = newValue;
    }
    else if (value < minValue) {
        value = 0;
    } else if (value > maxValue) {
        value = maxValue;
    }
    
    wxString label;
    wxString rangeText = g_range_names[units][value];
    
    label.Printf(wxT("%s\nAUTO(%s)"), firstLine, rangeText);
    
    this->SetLabel(label);
    
    isAuto = true;
    pPlugIn->settings.auto_range_mode = true;
}

BR24ControlsDialog::BR24ControlsDialog()
{
//      printf("BR24BUIDialog ctor\n");
    Init();
}

BR24ControlsDialog::~BR24ControlsDialog()
{
}


void BR24ControlsDialog::Init()
{
    // Initialize all members that need initialization
}

bool BR24ControlsDialog::Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id,
                                const wxString& caption,
                                const wxPoint& pos, const wxSize& size, long style)
{

    pParent = parent;
    pPlugIn = ppi;

    g_font = *OCPNGetFont(_("Dialog"), 14);
    wxTextCtrl *t = new wxTextCtrl(parent, id, wxT("Transparency")); 
    g_buttonSize = wxSize(t->GetBestSize().GetWidth() + 10, 50);
    if (ppi->settings.verbose) {
        wxLogMessage(wxT("Dynamic button width = %d"), g_buttonSize.GetWidth());
    }

#ifdef wxMSW
    long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#else
    long wstyle =                 wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#endif
    
    // Determine desired button width

    wxSize size_min = wxSize(g_buttonSize.GetWidth(), 4 * g_buttonSize.GetHeight());
    if (!wxDialog::Create(parent, id, caption, pos, size_min, wstyle)) {
        return false;
    }

    CreateControls();
    DimeWindow(this);
    Fit();
    size_min = GetBestSize();
    SetMinSize(size_min);
    SetSize(size_min);
    return true;
}

void BR24ControlsDialog::CreateControls()
{
    static int BORDER = 0;

// A top-level sizer
    topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);
    
    //**************** EDIT BOX ******************//
    // A box sizer to contain RANGE button
    editBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(editBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
    
    // The +10 button
    bPlusTen = new wxButton(this, ID_PLUS_TEN, _("+10"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bPlusTen, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bPlusTen->SetFont(g_font);

    // The + button
    bPlus = new wxButton(this, ID_PLUS, _("+1"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bPlus, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bPlus->SetFont(g_font);

    // The VALUE button
    bValue = new wxButton(this, ID_VALUE, _("Value"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bValue->SetFont(g_font);

    // The - button
    bMinus = new wxButton(this, ID_MINUS, _("-1"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMinus->SetFont(g_font);
    
    // The -10 button
    bMinusTen = new wxButton(this, ID_MINUS_TEN, _("-10"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bMinusTen, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMinusTen->SetFont(g_font);
    
    // The Auto button
    bAuto = new wxButton(this, ID_AUTO, _("Auto"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bAuto, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bAuto->SetFont(g_font);

    topSizer->Hide(editBox);

    //**************** CONTROL BOX ******************//
    // These are the controls that the users sees when the dialog is started

    // A box sizer to contain RANGE, GAIN etc button
    controlBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(controlBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
    
    // The RANGE button
    bRange = new RadarRangeControlButton(this, ID_RANGE, _("Range"), pPlugIn);
    controlBox->Add(bRange, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    
    // The GAIN button
    bGain = new RadarControlButton(this, ID_GAIN, _("Gain"), pPlugIn, CT_GAIN, true, pPlugIn->settings.gain);
    controlBox->Add(bGain, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    
    // The SEA button
    bSea = new RadarControlButton(this, ID_SEA, _("Sea Clutter"), pPlugIn, CT_SEA, true, pPlugIn->settings.sea_clutter_gain);
    controlBox->Add(bSea, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    
    // The RAIN button
    bRain = new RadarControlButton(this, ID_RAIN, _("Rain Clutter"), pPlugIn, CT_RAIN, false, pPlugIn->settings.rain_clutter_gain);
    controlBox->Add(bRain, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    
    // The TRANSPARENCY button
    bTransparency = new RadarControlButton(this, ID_TRANSPARENCY, _("Transparency"), pPlugIn, CT_TRANSPARENCY, false, pPlugIn->settings.overlay_transparency);
    controlBox->Add(bTransparency, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bTransparency->minValue = MIN_OVERLAY_TRANSPARENCY;
    bTransparency->maxValue = MAX_OVERLAY_TRANSPARENCY;

    // The REJECTION button
    bRejection = new RadarControlButton(this, ID_REJECTION, _("Interf. Rej"), pPlugIn, CT_REJECTION, false, pPlugIn->settings.rejection);
    controlBox->Add(bRejection, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bRejection->minValue = 0;
    bRejection->maxValue = 3;
    bRejection->names = g_rejection_names;
    bRejection->SetValue(pPlugIn->settings.rejection); // redraw after adding names

    // The GUARD ZONE 1 button
    bGuard1 = new wxButton(this, ID_ZONE1, _("Guard Zone 1"), wxDefaultPosition, g_buttonSize, 0);
    controlBox->Add(bGuard1, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bGuard1->SetFont(g_font);

    // The GUARD ZONE 2 button
    bGuard2 = new wxButton(this, ID_ZONE2, _("Guard Zone 2"), wxDefaultPosition, g_buttonSize, 0);
    controlBox->Add(bGuard2, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bGuard2->SetFont(g_font);

    UpdateGuardZoneState();
    
    pPlugIn->UpdateDisplayParameters();
}

void BR24ControlsDialog::UpdateGuardZoneState()
{
    wxString label;

    label.Printf(wxT("Guard Zone 1\n%s"), GuardZoneNames[pPlugIn->guardZones[0].type]);
    bGuard1->SetLabel(label);

    label.Printf(wxT("Guard Zone 2\n%s"), GuardZoneNames[pPlugIn->guardZones[1].type]);
    bGuard2->SetLabel(label);
}

void BR24ControlsDialog::SetActualRange(long range)
{
    const int * ranges;
    int         n;
    
    if (pPlugIn->settings.range_units < 1) {                    /* NMi or Mi */
        n = (int) MILE_RANGE_COUNT;
        ranges = g_range_distances[0];
    }
    else {
        n = (int) METRIC_RANGE_COUNT;
        ranges = g_range_distances[1];
    }
    
    for (; n > 0; n--) {
        if (ranges[n] < range) {
            break;
        }
    }
    bRange->SetValue(n);
}

void BR24ControlsDialog::OnZone1ButtonClick(wxCommandEvent &event)
{
    pPlugIn->Select_Alarm_Zones(0);
}

void BR24ControlsDialog::OnZone2ButtonClick(wxCommandEvent &event)
{
    pPlugIn->Select_Alarm_Zones(1);
}

void BR24ControlsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
}


void BR24ControlsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
}

void BR24ControlsDialog::OnPlusTenClick(wxCommandEvent& event)
{
    editControl->SetValue(editControl->value + 10);
    wxString label = editControl->GetLabel();
    
    bValue->SetLabel(label);
}

void BR24ControlsDialog::OnPlusClick(wxCommandEvent& event)
{
    editControl->SetValue(editControl->value + 1);
    wxString label = editControl->GetLabel();
    
    bValue->SetLabel(label);
}

void BR24ControlsDialog::OnValueClick(wxCommandEvent &event)
{
    topSizer->Hide(editBox);
    topSizer->Show(controlBox);
    topSizer->Layout();
}

void BR24ControlsDialog::OnAutoClick(wxCommandEvent &event)
{
    editControl->SetAuto();

    topSizer->Hide(editBox);
    topSizer->Show(controlBox);
    topSizer->Layout();
}

void BR24ControlsDialog::OnMinusClick(wxCommandEvent& event)
{
    editControl->SetValue(editControl->value - 1);
        
    wxString label = editControl->GetLabel();
    bValue->SetLabel(label);
}

void BR24ControlsDialog::OnMinusTenClick(wxCommandEvent& event)
{
    editControl->SetValue(editControl->value - 10);
    
    wxString label = editControl->GetLabel();
    bValue->SetLabel(label);
}

void BR24ControlsDialog::EnterEditMode(RadarControlButton * button)
{
    editControl = button;
    bValue->SetLabel(button->GetLabel());
    topSizer->Hide(controlBox);
    topSizer->Show(editBox);
    if (editControl->hasAuto) {
        bAuto->Show();
    }
    else {
        bAuto->Hide();
    }
    if (editControl->maxValue > 20) {
        bPlusTen->Show();
        bMinusTen->Show();
    }
    else {
        bPlusTen->Hide();
        bMinusTen->Hide();
    }
    editBox->Layout();
    topSizer->Layout();
}


void BR24ControlsDialog::OnRadarControlButtonClick(wxCommandEvent& event)
{
    EnterEditMode((RadarControlButton *) event.GetEventObject());
}



void BR24ControlsDialog::OnMove(wxMoveEvent& event)
{
    //    Record the dialog position
    wxPoint p =  GetPosition();
    pPlugIn->SetBR24ControlsDialogX(p.x);
    pPlugIn->SetBR24ControlsDialogY(p.y);

    event.Skip();
}

void BR24ControlsDialog::OnSize(wxSizeEvent& event)
{
    //    Record the dialog size
    wxSize p = event.GetSize();
    pPlugIn->SetBR24ControlsDialogSizeX(p.GetWidth());
    pPlugIn->SetBR24ControlsDialogSizeY(p.GetHeight());

    event.Skip();
}
