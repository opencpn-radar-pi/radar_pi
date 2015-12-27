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
#include "RadarDraw.h"

PLUGIN_BEGIN_NAMESPACE

enum {
  // process ID's
  ID_OK,
  ID_RANGE_UNITS,
  ID_OVERLAYDISPLAYOPTION,
  ID_DISPLAYTYPE,
  ID_HEADINGSLIDER,
  ID_SELECT_SOUND,
  ID_TEST_SOUND,
  ID_PASS_HEADING,
  ID_DRAW_METHOD,
  ID_SELECT_AB,
  ID_EMULATOR
};

IMPLEMENT_CLASS(br24OptionsDialog, wxDialog)

BEGIN_EVENT_TABLE(br24OptionsDialog, wxDialog)

EVT_CLOSE(br24OptionsDialog::OnClose)
EVT_BUTTON(ID_OK, br24OptionsDialog::OnIdOKClick)
EVT_RADIOBUTTON(ID_OVERLAYDISPLAYOPTION, br24OptionsDialog::OnDisplayOptionClick)

END_EVENT_TABLE()

br24OptionsDialog::br24OptionsDialog() { Init(); }

br24OptionsDialog::~br24OptionsDialog() {}

void br24OptionsDialog::Init() {}

bool br24OptionsDialog::Create(wxWindow *parent, br24radar_pi *pi) {
  wxString m_temp;

  m_parent = parent;
  m_pi = pi;

  if (!wxDialog::Create(parent, wxID_ANY, _("BR24 Target Display Preferences"), wxDefaultPosition, wxDefaultSize,
                        wxDEFAULT_DIALOG_STYLE)) {
    return false;
  }

  int font_size_y, font_descent, font_lead;
  GetTextExtent(_T("0"), NULL, &font_size_y, &font_descent, &font_lead);
  wxSize small_button_size(-1, (int)(1.4 * (font_size_y + font_descent + font_lead)));

  int border_size = 4;
  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(topSizer);

  wxFlexGridSizer *DisplayOptionsBox = new wxFlexGridSizer(2, 5, 5);
  topSizer->Add(DisplayOptionsBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

  //  BR24 toolbox icon checkbox
  //    wxStaticBox* DisplayOptionsCheckBox = new wxStaticBox(this, wxID_ANY, _T(""));
  //    wxStaticBoxSizer* DisplayOptionsCheckBoxSizer = new wxStaticBoxSizer(DisplayOptionsCheckBox,
  //    wxVERTICAL);
  //    DisplayOptionsBox->Add(DisplayOptionsCheckBoxSizer, 0, wxEXPAND | wxALL, border_size);

  //  Range Units options

  wxString RangeModeStrings[] = {
      _("Nautical Miles"), _("Kilometers"),
  };

  pRangeUnits = new wxRadioBox(this, ID_RANGE_UNITS, _("Range Units"), wxDefaultPosition, wxDefaultSize, 2, RangeModeStrings, 1,
                               wxRA_SPECIFY_COLS);
  DisplayOptionsBox->Add(pRangeUnits, 0, wxALL | wxEXPAND, 2);

  pRangeUnits->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnRangeUnitsClick), NULL, this);

  pRangeUnits->SetSelection(m_pi->m_settings.range_units);

  /// Option m_settings
  wxString Overlay_Display_Options[] = {
      _("Monocolor-Red"), _("Multi-color"),
  };

  pOverlayDisplayOptions =
      new wxRadioBox(this, ID_OVERLAYDISPLAYOPTION, _("Overlay Display Options"), wxDefaultPosition, wxDefaultSize,
                     ARRAY_SIZE(Overlay_Display_Options), Overlay_Display_Options, 1, wxRA_SPECIFY_COLS);

  DisplayOptionsBox->Add(pOverlayDisplayOptions, 0, wxALL | wxEXPAND, 2);

  pOverlayDisplayOptions->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnDisplayOptionClick),
                                  NULL, this);

  pOverlayDisplayOptions->SetSelection(m_pi->m_settings.display_option);

  wxString GuardZoneStyleStrings[] = {
      _("Shading"), _("Outline"), _("Shading + Outline"),
  };
  pGuardZoneStyle = new wxRadioBox(this, ID_DISPLAYTYPE, _("Guard Zone Styling"), wxDefaultPosition, wxDefaultSize,
                                   ARRAY_SIZE(GuardZoneStyleStrings), GuardZoneStyleStrings, 1, wxRA_SPECIFY_COLS);

  DisplayOptionsBox->Add(pGuardZoneStyle, 0, wxALL | wxEXPAND, 2);
  pGuardZoneStyle->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnGuardZoneStyleClick), NULL,
                           this);
  pGuardZoneStyle->SetSelection(m_pi->m_settings.guard_zone_render_style);

  //  Calibration
  wxStaticBox *itemStaticBoxCalibration = new wxStaticBox(this, wxID_ANY, _("Calibration"));
  wxStaticBoxSizer *itemStaticBoxSizerCalibration = new wxStaticBoxSizer(itemStaticBoxCalibration, wxVERTICAL);
  DisplayOptionsBox->Add(itemStaticBoxSizerCalibration, 0, wxEXPAND | wxALL, border_size);

  // Heading correction
  wxStaticText *pStatic_Heading_Correction = new wxStaticText(this, wxID_ANY, _("Heading correction\n(-180 to +180)"));
  itemStaticBoxSizerCalibration->Add(pStatic_Heading_Correction, 1, wxALIGN_LEFT | wxALL, 2);

  pText_Heading_Correction_Value = new wxTextCtrl(this, wxID_ANY);
  itemStaticBoxSizerCalibration->Add(pText_Heading_Correction_Value, 1, wxALIGN_LEFT | wxALL, border_size);
  m_temp.Printf(wxT("%2.1f"), m_pi->m_settings.heading_correction);
  pText_Heading_Correction_Value->SetValue(m_temp);
  pText_Heading_Correction_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                          wxCommandEventHandler(br24OptionsDialog::OnHeading_Calibration_Value), NULL, this);

  // Guard Zone Alarm

  wxStaticBox *guardZoneBox = new wxStaticBox(this, wxID_ANY, _("Guard Zone Sound"));
  wxStaticBoxSizer *guardZoneSizer = new wxStaticBoxSizer(guardZoneBox, wxVERTICAL);
  DisplayOptionsBox->Add(guardZoneSizer, 0, wxEXPAND | wxALL, border_size);

  wxButton *pSelectSound = new wxButton(this, ID_SELECT_SOUND, _("Select Alert Sound"), wxDefaultPosition, small_button_size, 0);
  pSelectSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnSelectSoundClick), NULL, this);
  guardZoneSizer->Add(pSelectSound, 0, wxALL, border_size);

  wxButton *pTestSound = new wxButton(this, ID_TEST_SOUND, _("Test Alert Sound"), wxDefaultPosition, small_button_size, 0);
  pTestSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnTestSoundClick), NULL, this);
  guardZoneSizer->Add(pTestSound, 0, wxALL, border_size);

  // Drawing Method

  wxStaticBox *drawingMethodBox = new wxStaticBox(this, wxID_ANY, _("GPU drawing method"));
  wxStaticBoxSizer *drawingMethodSizer = new wxStaticBoxSizer(drawingMethodBox, wxVERTICAL);
  DisplayOptionsBox->Add(drawingMethodSizer, 0, wxEXPAND | wxALL, border_size);

  wxArrayString DrawingMethods;
  RadarDraw::GetDrawingMethods(DrawingMethods);
  cbDrawingMethod =
      new wxComboBox(this, ID_DRAW_METHOD, DrawingMethods[m_pi->m_settings.drawing_method], wxDefaultPosition, wxDefaultSize,
                     DrawingMethods, wxALIGN_CENTRE | wxST_NO_AUTORESIZE, wxDefaultValidator, _("Drawing Method"));
  drawingMethodSizer->Add(cbDrawingMethod, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  cbDrawingMethod->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnDrawingMethodClick), NULL,
                           this);

  //  Options
  wxStaticBox *itemStaticBoxOptions = new wxStaticBox(this, wxID_ANY, _("Options"));
  wxStaticBoxSizer *itemStaticBoxSizerOptions = new wxStaticBoxSizer(itemStaticBoxOptions, wxVERTICAL);
  topSizer->Add(itemStaticBoxSizerOptions, 0, wxEXPAND | wxALL, border_size);

  cbPassHeading = new wxCheckBox(this, ID_PASS_HEADING, _("Pass radar heading to OpenCPN"), wxDefaultPosition, wxDefaultSize,
                                 wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(cbPassHeading, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  cbPassHeading->SetValue(m_pi->m_settings.pass_heading_to_opencpn);
  cbPassHeading->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnPassHeadingClick), NULL, this);

  cbEnableDualRadar = new wxCheckBox(this, ID_SELECT_AB, _("Enable dual radar, 4G only"), wxDefaultPosition, wxDefaultSize,
                                     wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(cbEnableDualRadar, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  cbEnableDualRadar->SetValue(m_pi->m_settings.enable_dual_radar);
  cbEnableDualRadar->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnEnableDualRadarClick), NULL,
                             this);

  cbEmulator =
      new wxCheckBox(this, ID_EMULATOR, _("Emulator mode"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(cbEmulator, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  cbEmulator->SetValue(m_pi->m_settings.emulator_on ? true : false);
  cbEmulator->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnEmulatorClick), NULL, this);

  // Accept/Reject button
  wxStdDialogButtonSizer *DialogButtonSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
  topSizer->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, border_size);

  DimeWindow(this);

  Fit();
  SetMinSize(GetBestSize());

  return true;
}

void br24OptionsDialog::OnRangeUnitsClick(wxCommandEvent &event) { m_pi->m_settings.range_units = pRangeUnits->GetSelection(); }

void br24OptionsDialog::OnDisplayOptionClick(wxCommandEvent &event) {
  m_pi->m_settings.display_option = pOverlayDisplayOptions->GetSelection();
  m_pi->ComputeColorMap();
}

void br24OptionsDialog::OnGuardZoneStyleClick(wxCommandEvent &event) {
  m_pi->m_settings.guard_zone_render_style = pGuardZoneStyle->GetSelection();
}

void br24OptionsDialog::OnSelectSoundClick(wxCommandEvent &event) {
  wxString *sharedData = GetpSharedDataLocation();
  wxString sound_dir;

  sound_dir.Append(*sharedData);
  sound_dir.Append(wxT("sounds"));

  wxFileDialog *openDialog = new wxFileDialog(NULL, _("Select Sound File"), sound_dir, wxT(""),
                                              _("WAV files (*.wav)|*.wav|All files (*.*)|*.*"), wxFD_OPEN);
  int response = openDialog->ShowModal();
  if (response == wxID_OK) {
    m_pi->m_settings.alert_audio_file = openDialog->GetPath();
  }
}

void br24OptionsDialog::OnEnableDualRadarClick(wxCommandEvent &event) {
  m_pi->m_settings.enable_dual_radar = cbEnableDualRadar->GetValue();
}

void br24OptionsDialog::OnTestSoundClick(wxCommandEvent &event) {
  if (!m_pi->m_settings.alert_audio_file.IsEmpty()) {
    PlugInPlaySound(m_pi->m_settings.alert_audio_file);
  }
}

void br24OptionsDialog::OnHeading_Calibration_Value(wxCommandEvent &event) {
  wxString temp = pText_Heading_Correction_Value->GetValue();
  temp.ToDouble(&m_pi->m_settings.heading_correction);
}

void br24OptionsDialog::OnPassHeadingClick(wxCommandEvent &event) {
  m_pi->m_settings.pass_heading_to_opencpn = cbPassHeading->GetValue();
}

void br24OptionsDialog::OnDrawingMethodClick(wxCommandEvent &event) {
  m_pi->m_settings.drawing_method = cbDrawingMethod->GetSelection();
  wxLogMessage(wxT("BR24radar_pi: new drawing method %d selected"), m_pi->m_settings.drawing_method);
}

void br24OptionsDialog::OnEmulatorClick(wxCommandEvent &event) { m_pi->m_settings.emulator_on = cbEmulator->GetValue(); }

void br24OptionsDialog::OnClose(wxCloseEvent &event) {
  m_pi->SaveConfig();
  this->Hide();
}

void br24OptionsDialog::OnIdOKClick(wxCommandEvent &event) {
  m_pi->SaveConfig();
  this->Hide();
}

PLUGIN_END_NAMESPACE
