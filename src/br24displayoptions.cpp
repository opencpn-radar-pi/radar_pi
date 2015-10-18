/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
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

#include "br24radar_pi.h"

IMPLEMENT_CLASS(BR24DisplayOptionsDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24DisplayOptionsDialog, wxDialog)

EVT_CLOSE(BR24DisplayOptionsDialog::OnClose)
EVT_BUTTON(ID_OK, BR24DisplayOptionsDialog::OnIdOKClick)
EVT_RADIOBUTTON(ID_DISPLAYTYPE, BR24DisplayOptionsDialog::OnDisplayModeClick)
EVT_RADIOBUTTON(ID_OVERLAYDISPLAYOPTION, BR24DisplayOptionsDialog::OnDisplayOptionClick)

END_EVENT_TABLE()

BR24DisplayOptionsDialog::BR24DisplayOptionsDialog()
{
    Init();
}

BR24DisplayOptionsDialog::~BR24DisplayOptionsDialog()
{
}

void BR24DisplayOptionsDialog::Init()
{
}

bool BR24DisplayOptionsDialog::Create(wxWindow *parent, br24radar_pi *ppi)
{
    wxString m_temp;

    pParent = parent;
    pPlugIn = ppi;

    if (!wxDialog::Create(parent, wxID_ANY, _("BR24 Target Display Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)) {
        return false;
    }

    int font_size_y, font_descent, font_lead;
    GetTextExtent( _T("0"), NULL, &font_size_y, &font_descent, &font_lead );
    wxSize small_button_size( -1, (int) ( 1.4 * ( font_size_y + font_descent + font_lead ) ) );

    int border_size = 4;
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    wxFlexGridSizer * DisplayOptionsBox = new wxFlexGridSizer(2, 5, 5);
    topSizer->Add(DisplayOptionsBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

    //  BR24 toolbox icon checkbox
    //    wxStaticBox* DisplayOptionsCheckBox = new wxStaticBox(this, wxID_ANY, _T(""));
    //    wxStaticBoxSizer* DisplayOptionsCheckBoxSizer = new wxStaticBoxSizer(DisplayOptionsCheckBox, wxVERTICAL);
    //    DisplayOptionsBox->Add(DisplayOptionsCheckBoxSizer, 0, wxEXPAND | wxALL, border_size);

    //  Range Units options

    wxString RangeModeStrings[] = {
        _("Nautical Miles"),
        _("Kilometers"),
    };

    pRangeUnits = new wxRadioBox(this, ID_RANGE_UNITS, _("Range Units"),
                                 wxDefaultPosition, wxDefaultSize,
                                 2, RangeModeStrings, 1, wxRA_SPECIFY_COLS);
    DisplayOptionsBox->Add(pRangeUnits, 0, wxALL | wxEXPAND, 2);

    pRangeUnits->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                         wxCommandEventHandler(BR24DisplayOptionsDialog::OnRangeUnitsClick), NULL, this);

    pRangeUnits->SetSelection(pPlugIn->settings.range_units);

    /// Option settings
    wxString Overlay_Display_Options[] = {
        _("Monocolor-Red"),
        _("Multi-color"),
        _("Multi-color 2"),
    };

    pOverlayDisplayOptions = new wxRadioBox(this, ID_OVERLAYDISPLAYOPTION, _("Overlay Display Options"),
                                            wxDefaultPosition, wxDefaultSize,
                                            3, Overlay_Display_Options, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pOverlayDisplayOptions, 0, wxALL | wxEXPAND, 2);

    pOverlayDisplayOptions->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                                    wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayOptionClick), NULL, this);

    pOverlayDisplayOptions->SetSelection(pPlugIn->settings.display_option);

    /*
    pDisplayMode = new wxRadioBox(this, ID_DISPLAYTYPE, _("Radar Display"),
                                  wxDefaultPosition, wxDefaultSize,
                                  ARRAY_SIZE(DisplayModeStrings), DisplayModeStrings, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pDisplayMode, 0, wxALL | wxEXPAND, 2);

    pDisplayMode->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                          wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayModeClick), NULL, this);

    pDisplayMode->SetSelection(pPlugIn->settings.display_mode[0]);
    */
    wxString GuardZoneStyleStrings[] = {
        _("Shading"),
        _("Outline"),
        _("Shading + Outline"),
    };
    pGuardZoneStyle = new wxRadioBox(this, ID_DISPLAYTYPE, _("Guard Zone Styling"),
                                     wxDefaultPosition, wxDefaultSize,
                                     3, GuardZoneStyleStrings, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pGuardZoneStyle, 0, wxALL | wxEXPAND, 2);
    pGuardZoneStyle->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                             wxCommandEventHandler(BR24DisplayOptionsDialog::OnGuardZoneStyleClick), NULL, this);
    pGuardZoneStyle->SetSelection(pPlugIn->settings.guard_zone_render_style);


    //  Calibration
    wxStaticBox* itemStaticBoxCalibration = new wxStaticBox(this, wxID_ANY, _("Calibration"));
    wxStaticBoxSizer* itemStaticBoxSizerCalibration = new wxStaticBoxSizer(itemStaticBoxCalibration, wxVERTICAL);
    DisplayOptionsBox->Add(itemStaticBoxSizerCalibration, 0, wxEXPAND | wxALL, border_size);

    // Heading correction
    wxStaticText *pStatic_Heading_Correction = new wxStaticText(this, wxID_ANY, _("Heading correction\n(-180 to +180)"));
    itemStaticBoxSizerCalibration->Add(pStatic_Heading_Correction, 1, wxALIGN_LEFT | wxALL, 2);

    pText_Heading_Correction_Value = new wxTextCtrl(this, wxID_ANY);
    itemStaticBoxSizerCalibration->Add(pText_Heading_Correction_Value, 1, wxALIGN_LEFT | wxALL, border_size);
    m_temp.Printf(wxT("%2.1f"), pPlugIn->settings.heading_correction);
    pText_Heading_Correction_Value->SetValue(m_temp);
    pText_Heading_Correction_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                            wxCommandEventHandler(BR24DisplayOptionsDialog::OnHeading_Calibration_Value), NULL, this);

    // Guard Zone Alarm

    wxStaticBox* guardZoneBox = new wxStaticBox(this, wxID_ANY, _("Guard Zone Sound"));
    wxStaticBoxSizer* guardZoneSizer = new wxStaticBoxSizer(guardZoneBox, wxVERTICAL);
    DisplayOptionsBox->Add(guardZoneSizer, 0, wxEXPAND | wxALL, border_size);

    wxButton *pSelectSound = new wxButton(this, ID_SELECT_SOUND, _("Select Alert Sound"), wxDefaultPosition, small_button_size, 0);
    pSelectSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                          wxCommandEventHandler(BR24DisplayOptionsDialog::OnSelectSoundClick), NULL, this);
    guardZoneSizer->Add(pSelectSound, 0, wxALL, border_size);

    wxButton *pTestSound = new wxButton(this, ID_TEST_SOUND, _("Test Alert Sound"), wxDefaultPosition, small_button_size, 0);
    pTestSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                        wxCommandEventHandler(BR24DisplayOptionsDialog::OnTestSoundClick), NULL, this);
    guardZoneSizer->Add(pTestSound, 0, wxALL, border_size);

    //  Options
    wxStaticBox* itemStaticBoxOptions = new wxStaticBox(this, wxID_ANY, _("Options"));
    wxStaticBoxSizer* itemStaticBoxSizerOptions = new wxStaticBoxSizer(itemStaticBoxOptions, wxVERTICAL);
    topSizer->Add(itemStaticBoxSizerOptions, 0, wxEXPAND | wxALL, border_size);

    cbPassHeading = new wxCheckBox(this, ID_PASS_HEADING, _("Pass radar heading to OpenCPN"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    itemStaticBoxSizerOptions->Add(cbPassHeading, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
    cbPassHeading->SetValue(pPlugIn->settings.passHeadingToOCPN ? true : false);
    cbPassHeading->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED,
                             wxCommandEventHandler(BR24DisplayOptionsDialog::OnPassHeadingClick), NULL, this);

    cbUseShader = new wxCheckBox(this, ID_USE_SHADER, _("Use GPU shader for rendering"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    itemStaticBoxSizerOptions->Add(cbUseShader, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
    cbUseShader->SetValue(pPlugIn->settings.useShader);
    cbUseShader->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED,
                             wxCommandEventHandler(BR24DisplayOptionsDialog::OnUseShaderClick), NULL, this);

    cbEnableDualRadar = new wxCheckBox(this, ID_SELECT_AB, _("Enable dual radar, 4G only"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    itemStaticBoxSizerOptions->Add(cbEnableDualRadar, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
    cbEnableDualRadar->SetValue(pPlugIn->settings.enable_dual_radar ? true : false);
    cbEnableDualRadar->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(BR24DisplayOptionsDialog::OnEnableDualRadarClick), NULL, this);

    cbEmulator = new wxCheckBox(this, ID_EMULATOR, _("Emulator mode"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    itemStaticBoxSizerOptions->Add(cbEmulator, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
    cbEmulator->SetValue(pPlugIn->settings.emulator_on ? true : false);
    cbEmulator->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(BR24DisplayOptionsDialog::OnEmulatorClick), NULL, this);


    // Accept/Reject button
    wxStdDialogButtonSizer* DialogButtonSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    topSizer->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, border_size);

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}

void BR24DisplayOptionsDialog::OnRangeUnitsClick(wxCommandEvent &event)
{
    pPlugIn->settings.range_units = pRangeUnits->GetSelection();
}

void BR24DisplayOptionsDialog::OnDisplayOptionClick(wxCommandEvent &event)
{
    pPlugIn->settings.display_option = pOverlayDisplayOptions->GetSelection();
}

void BR24DisplayOptionsDialog::OnDisplayModeClick(wxCommandEvent &event)
{
    pPlugIn->SetDisplayMode((DisplayModeType) pDisplayMode->GetSelection());
}

void BR24DisplayOptionsDialog::OnGuardZoneStyleClick(wxCommandEvent &event)
{
    pPlugIn->settings.guard_zone_render_style = pGuardZoneStyle->GetSelection();
}

void BR24DisplayOptionsDialog::OnSelectSoundClick(wxCommandEvent &event)
{
    wxString *sharedData = GetpSharedDataLocation();
    wxString sound_dir;

    sound_dir.Append( *sharedData );
    sound_dir.Append( wxT("sounds") );

    wxFileDialog *openDialog = new wxFileDialog( NULL, _("Select Sound File"), sound_dir, wxT(""),
                                                _("WAV files (*.wav)|*.wav|All files (*.*)|*.*"), wxFD_OPEN );
    int response = openDialog->ShowModal();
    if (response == wxID_OK ) {
        pPlugIn->settings.alert_audio_file = openDialog->GetPath();
    }
}

void BR24DisplayOptionsDialog::OnEnableDualRadarClick(wxCommandEvent &event)
{
    pPlugIn->settings.enable_dual_radar = cbEnableDualRadar->GetValue();
}

void BR24DisplayOptionsDialog::OnTestSoundClick(wxCommandEvent &event)
{
    if (!pPlugIn->settings.alert_audio_file.IsEmpty()) {
        PlugInPlaySound(pPlugIn->settings.alert_audio_file);
    }
}

void BR24DisplayOptionsDialog::OnHeading_Calibration_Value(wxCommandEvent &event)
{
    wxString temp = pText_Heading_Correction_Value->GetValue();
    temp.ToDouble(&pPlugIn->settings.heading_correction);
}

void BR24DisplayOptionsDialog::OnPassHeadingClick(wxCommandEvent &event)
{
    pPlugIn->settings.passHeadingToOCPN = cbPassHeading->GetValue();
}

void BR24DisplayOptionsDialog::OnUseShaderClick(wxCommandEvent &event)
{
    pPlugIn->settings.useShader = cbUseShader->GetValue();
}

void BR24DisplayOptionsDialog::OnEmulatorClick(wxCommandEvent &event)
{
    pPlugIn->settings.emulator_on = cbEmulator->GetValue();
}


void BR24DisplayOptionsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->SaveConfig();
    this->Hide();
}

void BR24DisplayOptionsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->SaveConfig();
    this->Hide();
}
