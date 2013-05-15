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
    ID_OK,
    ID_TRANSLIDER,
    ID_RANGEMODE,
    ID_RANGE,
    ID_CLUTTER,
    ID_GAIN,
    ID_REJECTION
};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(BR24ControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24ControlsDialog, wxDialog)

    EVT_CLOSE(BR24ControlsDialog::OnClose)
    EVT_BUTTON(ID_OK, BR24ControlsDialog::OnIdOKClick)
    EVT_MOVE(BR24ControlsDialog::OnMove)
    EVT_SIZE(BR24ControlsDialog::OnSize)
    EVT_RADIOBUTTON(ID_RANGEMODE, BR24ControlsDialog::OnRangeModeClick)
    EVT_RADIOBUTTON(ID_CLUTTER, BR24ControlsDialog::OnFilterProcessClick)
    EVT_RADIOBUTTON(ID_REJECTION, BR24ControlsDialog::OnRejectionModeClick)

END_EVENT_TABLE()


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
}


bool BR24ControlsDialog::Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id,
                                const wxString& caption,
                                const wxPoint& pos, const wxSize& size, long style)
{

    pParent = parent;
    pPlugIn = ppi;

    long wstyle = wxDEFAULT_FRAME_STYLE;
//      if ( ( global_color_scheme != GLOBAL_COLOR_SCHEME_DAY ) && ( global_color_scheme != GLOBAL_COLOR_SCHEME_RGB ) )
//            wstyle |= ( wxNO_BORDER );

    wxSize size_min = size;
//      size_min.IncTo ( wxSize ( 500,600 ) );
    if (!wxDialog::Create(parent, id, caption, pos, size_min, wstyle)) {
        return false;
    }

    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}

void BR24ControlsDialog::CreateControls()
{
    int border_size = 4;


// A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

// A second box sizer to give more space around the controls
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

//  Operation Mode options
    wxStaticBox* BoxOperation = new wxStaticBox(this, wxID_ANY, _("Operational Control"));
    wxStaticBoxSizer* BoxSizerOperation = new wxStaticBoxSizer(BoxOperation, wxVERTICAL);
    boxSizer->Add(BoxSizerOperation, 0, wxEXPAND | wxALL, border_size);

    wxString RangeModeStrings[] = {
        _("Manual"),
        _("Automatic"),
    };

    pRangeMode = new wxRadioBox(this, ID_RANGEMODE, _("Range Mode"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, RangeModeStrings, 1, wxRA_SPECIFY_COLS);

    BoxSizerOperation->Add(pRangeMode, 0, wxALL | wxEXPAND, 2);

    pRangeMode->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                            wxCommandEventHandler(BR24ControlsDialog::OnRangeModeClick), NULL, this);
    if (pPlugIn->settings.auto_range_mode) {
        pRangeMode->SetSelection(1);
    } else {
        pRangeMode->SetSelection(0);
        pPlugIn->SetRange(0);
    }

    //Transparency slider
    wxStaticBox* transliderbox = new wxStaticBox(this, wxID_ANY, _("Transparency"));
    wxStaticBoxSizer* transliderboxsizer = new wxStaticBoxSizer(transliderbox, wxVERTICAL);
    BoxSizerOperation->Add(transliderboxsizer, 0, wxALL | wxEXPAND, 2);

    pTranSlider = new wxSlider(this, ID_TRANSLIDER, 90 , 10, 100, wxDefaultPosition,  wxDefaultSize,
                               wxSL_HORIZONTAL,  wxDefaultValidator, _("slider"));

    transliderboxsizer->Add(pTranSlider, 0, wxALL | wxEXPAND, 2);

    pTranSlider->Connect(wxEVT_SCROLL_CHANGED,
                         wxCommandEventHandler(BR24ControlsDialog::OnTransSlider), NULL, this);

    pTranSlider->SetValue(pPlugIn->settings.overlay_transparency * 100);
    pPlugIn->UpdateDisplayParameters();

//  Image Conditioning Options
    wxStaticBox* BoxConditioning = new wxStaticBox(this, wxID_ANY, _("Signal Conditioning"));
    wxStaticBoxSizer* BoxConditioningSizer = new wxStaticBoxSizer(BoxConditioning, wxVERTICAL);
    boxSizer->Add(BoxConditioningSizer, 0, wxEXPAND | wxALL, border_size);

// Rejection settings
    wxString RejectionStrings[] = {
        _("Off"),
        _("Low"),
        _("Medium"),
        _("High"),
    };

    pRejectionMode = new wxRadioBox(this, ID_REJECTION, _("Rejection"),
                                    wxDefaultPosition, wxDefaultSize,
                                    4, RejectionStrings, 1, wxRA_SPECIFY_COLS);

    BoxConditioningSizer->Add(pRejectionMode, 0, wxALL | wxEXPAND, 2);

    pRejectionMode->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                            wxCommandEventHandler(BR24ControlsDialog::OnRejectionModeClick), NULL, this);

    pRejectionMode->SetSelection(pPlugIn->settings.rejection);

//  Cluster Options
    wxString FilterProcessStrings[] = {
        _("Auto Gain"),
        _("Manual Gain"),
        _("Rain Clutter - Manual"),
        _("Sea Clutter - Auto"),
        _("Sea Clutter - Manual"),
    };

    pFilterProcess = new wxRadioBox(this, ID_CLUTTER, _("Tuning"),
                                    wxDefaultPosition, wxDefaultSize,
                                    5, FilterProcessStrings, 1, wxRA_SPECIFY_COLS);

    BoxConditioningSizer->Add(pFilterProcess, 0, wxALL | wxEXPAND, 2);
    pFilterProcess->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                            wxCommandEventHandler(BR24ControlsDialog::OnFilterProcessClick), NULL, this);
    // pPlugIn->SetFilterProcess(pFilterProcess->GetSelection(), sel_gain);

//  Gain slider

    wxStaticBox* BoxGain = new wxStaticBox(this, wxID_ANY, _("0-100%"));
    wxStaticBoxSizer* sliderGainsizer = new wxStaticBoxSizer(BoxGain, wxVERTICAL);
    BoxConditioningSizer->Add(sliderGainsizer, 0, wxALL | wxEXPAND, 2);

    pGainSlider = new wxSlider(this, ID_GAIN, 90 , 10, 100, wxDefaultPosition,  wxDefaultSize,
                               wxSL_HORIZONTAL,  wxDefaultValidator, _("slider"));

    sliderGainsizer->Add(pGainSlider, 0, wxALL | wxEXPAND, 2);

    pGainSlider->Connect(wxEVT_SCROLL_CHANGED,
                         wxCommandEventHandler(BR24ControlsDialog::OnGainSlider), NULL, this);

// A horizontal box sizer to contain OK
    wxBoxSizer* AckBox = new wxBoxSizer(wxHORIZONTAL);
    boxSizer->Add(AckBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

// The OK button
    wxButton* bOK = new wxButton(this, ID_OK, _("&Close"),
                                 wxDefaultPosition, wxDefaultSize, 0);
    AckBox->Add(bOK, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
}

void BR24ControlsDialog::OnRangeModeClick(wxCommandEvent &event)
{
    pPlugIn->SetRangeMode(pRangeMode->GetSelection());
}

void BR24ControlsDialog::OnTransSlider(wxCommandEvent &event)
{
    pPlugIn->settings.overlay_transparency = ((double)pTranSlider->GetValue()) / 100.;
    pPlugIn->UpdateDisplayParameters();
}

void BR24ControlsDialog::OnFilterProcessClick(wxCommandEvent &event)
{
    int sel_gain = 0;

    pPlugIn->settings.filter_process = pFilterProcess->GetSelection();
    switch (pPlugIn->settings.filter_process) {
        case 1: {
                sel_gain = pPlugIn->settings.gain;
                break;
            }
        case 2: {
                sel_gain = pPlugIn->settings.sea_clutter_gain;
                break;
            }
        case 4: {
                sel_gain = pPlugIn->settings.rain_clutter_gain;
                break;
            }
    }
    pGainSlider->SetValue(sel_gain);
    pPlugIn->SetFilterProcess(pPlugIn->settings.filter_process, sel_gain);
}

void BR24ControlsDialog::OnRejectionModeClick(wxCommandEvent &event)
{
    pPlugIn->SetRejectionMode(pRejectionMode->GetSelection());
}

void BR24ControlsDialog::OnGainSlider(wxCommandEvent &event)
{
    int sel_gain = pGainSlider->GetValue();
    pPlugIn->SetFilterProcess(pPlugIn->settings.filter_process, sel_gain);

    switch (pPlugIn->settings.filter_process) {
        case 1: {
                pPlugIn->settings.gain = sel_gain;
                break;
            }
        case 2: {
                pPlugIn->settings.sea_clutter_gain = sel_gain;
                break;
            }
        case 4: {
                sel_gain = sel_gain * 8 / 10;
                pPlugIn->settings.rain_clutter_gain = sel_gain;
                break;
            }
    }
}
void BR24ControlsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
}


void BR24ControlsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
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
    pPlugIn->SetBR24ControlsDialogSizeX(p.x);
    pPlugIn->SetBR24ControlsDialogSizeY(p.y);

    event.Skip();
}
//---------------------------------------------------------------------------------------
//          Manual Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(BR24ManualDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24ManualDialog, wxDialog)

    EVT_CLOSE(BR24ManualDialog::OnClose)
    EVT_BUTTON(ID_OK, BR24ManualDialog::OnIdOKClick)
    EVT_MOVE(BR24ManualDialog::OnMove)
    EVT_SIZE(BR24ManualDialog::OnSize)

END_EVENT_TABLE()


BR24ManualDialog::BR24ManualDialog()
{
    Init();
}

BR24ManualDialog::~BR24ManualDialog()
{
}


void BR24ManualDialog::Init()
{
}


bool BR24ManualDialog::Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id,
                              const wxString& m_caption,
                              const wxPoint& pos, const wxSize& size, long style)
{

    pParent = parent;
    pPlugIn = ppi;


    //    As a display optimization....
    //    if current color scheme is other than DAY,
    //    Then create the dialog ..WITHOUT.. borders and title bar.
    //    This way, any window decorations set by external themes, etc
    //    will not detract from night-vision

    long wstyle = wxDEFAULT_FRAME_STYLE;
//      if ( ( global_color_scheme != GLOBAL_COLOR_SCHEME_DAY ) && ( global_color_scheme != GLOBAL_COLOR_SCHEME_RGB ) )
//            wstyle |= ( wxNO_BORDER );

    wxSize size_min = size;
//      size_min.IncTo ( wxSize ( 500,600 ) );
    if (!wxDialog::Create(parent, id, m_caption, pos, size_min, wstyle)) {
        return false;
    }


    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}

void BR24ManualDialog::CreateControls()
{
    int border_size = 4;

// A top-level sizer
    wxBoxSizer* ManualSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(ManualSizer);

//  Operating options
    wxStaticBox* BoxRange = new wxStaticBox(this, wxID_ANY, _("Manual Control"));
    wxStaticBoxSizer* BoxRangeSizer = new wxStaticBoxSizer(BoxRange, wxVERTICAL);
    ManualSizer->Add(BoxRangeSizer, 0, wxEXPAND | wxALL, border_size);

    wxString RangeStrings[] = {
        _("50 meters"),
        _("75"),
        _("100"),
        _("250"),
        _("500"),
        _("750"),
        _("1000"),
        _("1500"),
        _("2000"),
        _("3000"),
        _("4000"),
        _("6000"),
        _("8000"),
        _("12000"),
        _("16000"),
        _("24000")
    };

    pRangeSelect = new wxRadioBox(this, ID_RANGE, _("Range:"),
                                  wxDefaultPosition, wxDefaultSize,
                                  16, RangeStrings, 1, wxRA_SPECIFY_COLS);

    BoxRangeSizer->Add(pRangeSelect, 0, wxALL | wxEXPAND, 2);

    pRangeSelect->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                          wxCommandEventHandler(BR24ManualDialog::OnRangeClick), NULL, this);

    pRangeSelect->SetSelection(0);

// The Close button
    wxButton* bClose = new wxButton(this, ID_OK, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    ManualSizer->Add(bClose, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
}

void BR24ManualDialog::OnRangeClick(wxCommandEvent &event)
{
    pPlugIn->settings.range_index = pRangeSelect->GetSelection();
    pPlugIn->SetRange(pPlugIn->settings.range_index);
    wxString message(_("Range: %d"), pPlugIn->settings.range_index);
    wxMessageDialog dlg(GetOCPNCanvasWindow(),  message, _T("BR24 Radar"));
}


void BR24ManualDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->OnBR24ManualDialogClose();
    event.Skip();
}


void BR24ManualDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->OnBR24ManualDialogClose();
    event.Skip();
}

void BR24ManualDialog::OnMove(wxMoveEvent& event)
{
    //    Record the dialog position
    wxPoint p =  GetPosition();
    pPlugIn->SetBR24ManualDialogX(p.x);
    pPlugIn->SetBR24ManualDialogY(p.y);

    event.Skip();
}

void BR24ManualDialog::OnSize(wxSizeEvent& event)
{
    //    Record the dialog size
    wxSize p = event.GetSize();
    pPlugIn->SetBR24ManualDialogSizeX(p.x);
    pPlugIn->SetBR24ManualDialogSizeY(p.y);

    event.Skip();
}

