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
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
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

#include "RadarDraw.h"
#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

br24OptionsDialog::br24OptionsDialog(wxWindow *parent, PersistentSettings &settings, RadarType radar_type)
    : wxDialog(parent, wxID_ANY, _("BR24 Display Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
  wxString m_temp;

  m_parent = parent;
  m_settings = settings;

  int font_size_y, font_descent, font_lead;
  GetTextExtent(_T("0"), NULL, &font_size_y, &font_descent, &font_lead);
  wxSize small_button_size(-1, (int)(1.4 * (font_size_y + font_descent + font_lead)));

  int border_size = 4;
  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(topSizer);

  wxFlexGridSizer *DisplayOptionsBox = new wxFlexGridSizer(4, 5, 5);
  topSizer->Add(DisplayOptionsBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

  //  Range Units options

  wxString RangeModeStrings[] = {
      _("Nautical Miles"), _("Kilometers"),
  };

  m_RangeUnits =
      new wxRadioBox(this, wxID_ANY, _("Range Units"), wxDefaultPosition, wxDefaultSize, 2, RangeModeStrings, 1, wxRA_SPECIFY_COLS);

  m_RangeUnits->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnRangeUnitsClick), NULL, this);

  m_RangeUnits->SetSelection(m_settings.range_units);

  wxString GuardZoneStyleStrings[] = {
      _("Shading"), _("Outline"), _("Shading + Outline"),
  };
  m_GuardZoneStyle = new wxRadioBox(this, wxID_ANY, _("Guard Zone Styling"), wxDefaultPosition, wxDefaultSize,
                                    ARRAY_SIZE(GuardZoneStyleStrings), GuardZoneStyleStrings, 1, wxRA_SPECIFY_COLS);

  m_GuardZoneStyle->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnGuardZoneStyleClick), NULL,
                            this);
  m_GuardZoneStyle->SetSelection(m_settings.guard_zone_render_style);

  // Guard Zone Alarm

  wxStaticBox *guardZoneBox = new wxStaticBox(this, wxID_ANY, _("Guard Zone Sound"));
  wxStaticBoxSizer *guardZoneSizer = new wxStaticBoxSizer(guardZoneBox, wxVERTICAL);

  wxButton *select_sound = new wxButton(this, wxID_ANY, _("Select Alert Sound"), wxDefaultPosition, small_button_size, 0);
  select_sound->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnSelectSoundClick), NULL, this);
  guardZoneSizer->Add(select_sound, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, border_size);

  wxButton *test_sound = new wxButton(this, wxID_ANY, _("Test Alert Sound"), wxDefaultPosition, small_button_size, 0);
  test_sound->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnTestSoundClick), NULL, this);
  guardZoneSizer->Add(test_sound, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, border_size);

  wxStaticText *guardZoneTimeout =
      new wxStaticText(this, wxID_ANY, _("Repeat alarm after (sec)"), wxDefaultPosition, wxDefaultSize, 0);
  guardZoneSizer->Add(guardZoneTimeout, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, border_size);

  m_GuardZoneTimeout = new wxTextCtrl(this, wxID_ANY);
  guardZoneSizer->Add(m_GuardZoneTimeout, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, border_size);
  m_GuardZoneTimeout->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(br24OptionsDialog::OnGuardZoneTimeoutClick), NULL,
                              this);
  m_GuardZoneTimeout->SetValue(wxString::Format(wxT("%d"), m_settings.guard_zone_timeout));

  // Drawing Method

  wxStaticBox *drawingMethodBox = new wxStaticBox(this, wxID_ANY, _("GPU drawing method"));
  wxStaticBoxSizer *drawingMethodSizer = new wxStaticBoxSizer(drawingMethodBox, wxVERTICAL);

  wxArrayString DrawingMethods;
  RadarDraw::GetDrawingMethods(DrawingMethods);
  m_DrawingMethod = new wxComboBox(this, wxID_ANY, DrawingMethods[m_settings.drawing_method], wxDefaultPosition, wxDefaultSize,
                                   DrawingMethods, wxALIGN_CENTRE | wxST_NO_AUTORESIZE, wxDefaultValidator, _("Drawing Method"));
  drawingMethodSizer->Add(m_DrawingMethod, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  m_DrawingMethod->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnDrawingMethodClick), NULL,
                           this);

  // Menu options

  wxStaticBox *menuOptionsBox = new wxStaticBox(this, wxID_ANY, _("Control Menu Auto Hide"));
  wxStaticBoxSizer *menuOptionsSizer = new wxStaticBoxSizer(menuOptionsBox, wxVERTICAL);

  wxString MenuAutoHideStrings[] = {_("Never"), _("10 sec"), _("30 sec")};
  m_MenuAutoHide = new wxComboBox(this, wxID_ANY, MenuAutoHideStrings[m_settings.menu_auto_hide], wxDefaultPosition, wxDefaultSize,
                                  ARRAY_SIZE(MenuAutoHideStrings), MenuAutoHideStrings, wxALIGN_CENTRE | wxST_NO_AUTORESIZE,
                                  wxDefaultValidator, _("Auto hide after"));
  menuOptionsSizer->Add(m_MenuAutoHide, 0, wxALIGN_CENTER_VERTICAL | wxALL, border_size);
  m_MenuAutoHide->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(br24OptionsDialog::OnMenuAutoHideClick), NULL,
                          this);

  // Target trails colours

  wxStaticBox *trailBox = new wxStaticBox(this, wxID_ANY, _("Target trails"));
  wxStaticBoxSizer *trailSizer = new wxStaticBoxSizer(trailBox, wxVERTICAL);

  wxStaticText *trailStartText = new wxStaticText(this, wxID_ANY, _("Trail start color"));
  trailSizer->Add(trailStartText, 0, wxALL, border_size);
  m_TrailStartColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.trail_start_colour, wxDefaultPosition, wxSize(150, 30));
  m_TrailStartColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnTrailStartColourClick), NULL,
                              this);
  trailSizer->Add(m_TrailStartColour);

  wxStaticText *trailEndText = new wxStaticText(this, wxID_ANY, _("Trail end color"));
  trailSizer->Add(trailEndText, 0, wxALL, border_size);
  m_TrailEndColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.trail_end_colour, wxDefaultPosition, wxSize(150, 30));
  m_TrailEndColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnTrailEndColourClick), NULL,
                            this);
  trailSizer->Add(m_TrailEndColour);

  // Target colours

  wxStaticBox *colourBox = new wxStaticBox(this, wxID_ANY, _("Target colors"));
  wxStaticBoxSizer *colourSizer = new wxStaticBoxSizer(colourBox, wxVERTICAL);

  wxStaticText *weakText = new wxStaticText(this, wxID_ANY, _("Weak return color"));
  colourSizer->Add(weakText, 0, wxALL, border_size);
  m_WeakColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.weak_colour, wxDefaultPosition, wxSize(150, 30));
  m_WeakColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnWeakColourClick), NULL, this);
  colourSizer->Add(m_WeakColour);

  wxStaticText *intermediateText = new wxStaticText(this, wxID_ANY, _("Intermediate return color"));
  colourSizer->Add(intermediateText, 0, wxALL, border_size);
  m_IntermediateColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.intermediate_colour, wxDefaultPosition, wxSize(150, 30));
  m_IntermediateColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnIntermediateColourClick),
                                NULL, this);
  colourSizer->Add(m_IntermediateColour);

  wxStaticText *strongText = new wxStaticText(this, wxID_ANY, _("Strong return color"));
  colourSizer->Add(strongText, 0, wxALL, border_size);
  m_StrongColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.strong_colour, wxDefaultPosition, wxSize(150, 30));
  m_StrongColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnStrongColourClick), NULL, this);
  colourSizer->Add(m_StrongColour);

  wxStaticText *arpaText = new wxStaticText(this, wxID_ANY, _("ARPA edge color"));
  colourSizer->Add(arpaText, 0, wxALL, border_size);
  m_ArpaColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.arpa_colour, wxDefaultPosition, wxSize(150, 30));
  m_ArpaColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnArpaColourClick), NULL, this);
  colourSizer->Add(m_ArpaColour);

  // Other colours

  wxStaticBox *PPIColourBox = new wxStaticBox(this, wxID_ANY, _("Radar window colors"));
  wxStaticBoxSizer *PPIColourSizer = new wxStaticBoxSizer(PPIColourBox, wxVERTICAL);

  wxStaticText *backgroundText = new wxStaticText(this, wxID_ANY, _("Background color"));
  PPIColourSizer->Add(backgroundText, 0, wxALL, border_size);
  m_PPIBackgroundColour =
      new wxColourPickerCtrl(this, wxID_ANY, m_settings.ppi_background_colour, wxDefaultPosition, wxSize(150, 30));
  m_PPIBackgroundColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnPPIBackgroundColourClick),
                                 NULL, this);
  PPIColourSizer->Add(m_PPIBackgroundColour);

  wxStaticText *aisText = new wxStaticText(this, wxID_ANY, _("AIS text color"));
  PPIColourSizer->Add(aisText, 0, wxALL, border_size);
  m_AisTextColour = new wxColourPickerCtrl(this, wxID_ANY, m_settings.ais_text_colour, wxDefaultPosition, wxSize(150, 30));
  m_AisTextColour->Connect(wxEVT_COLOURPICKER_CHANGED, wxCommandEventHandler(br24OptionsDialog::OnAisTextColourClick), NULL, this);
  PPIColourSizer->Add(m_AisTextColour);

  DisplayOptionsBox->Add(m_RangeUnits, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(drawingMethodSizer, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(menuOptionsSizer, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(m_GuardZoneStyle, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(guardZoneSizer, 0, wxALL, border_size);
  DisplayOptionsBox->Add(trailSizer, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(colourSizer, 0, wxALL | wxEXPAND, border_size);
  DisplayOptionsBox->Add(PPIColourSizer, 0, wxALL | wxEXPAND, border_size);

  //  Options
  wxStaticBox *itemStaticBoxOptions = new wxStaticBox(this, wxID_ANY, _("Options"));
  wxStaticBoxSizer *itemStaticBoxSizerOptions = new wxStaticBoxSizer(itemStaticBoxOptions, wxVERTICAL);
  topSizer->Add(itemStaticBoxSizerOptions, 0, wxEXPAND | wxALL, border_size);

  m_ShowExtremeRange = new wxCheckBox(this, wxID_ANY, _("Show ring at extreme range"));
  itemStaticBoxSizerOptions->Add(m_ShowExtremeRange, 0, wxALL, border_size);
  m_ShowExtremeRange->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnShowExtremeRangeClick),
                              NULL, this);
  m_ShowExtremeRange->SetValue(m_settings.show_extreme_range);

  m_GuardZoneOnOverlay = new wxCheckBox(this, wxID_ANY, _("Show Guard Zone on overlay"));
  itemStaticBoxSizerOptions->Add(m_GuardZoneOnOverlay, 0, wxALL, border_size);
  m_GuardZoneOnOverlay->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnGuardZoneOnOverlayClick),
                                NULL, this);
  m_GuardZoneOnOverlay->SetValue(m_settings.guard_zone_on_overlay);

  m_TrailsOnOverlay = new wxCheckBox(this, wxID_ANY, _("Show Target trails on overlay"));
  itemStaticBoxSizerOptions->Add(m_TrailsOnOverlay, 0, wxALL, border_size);
  m_TrailsOnOverlay->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnTrailsOnOverlayClick), NULL,
                             this);
  m_TrailsOnOverlay->SetValue(m_settings.trails_on_overlay);

  m_EnableDualRadar = new wxCheckBox(this, wxID_ANY, _("Enable dual radar, 4G only"), wxDefaultPosition, wxDefaultSize,
                                     wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_EnableDualRadar, 0, wxALL, border_size);
  m_EnableDualRadar->SetValue(m_settings.enable_dual_radar);
  m_EnableDualRadar->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnEnableDualRadarClick), NULL,
                             this);
  if (radar_type == RT_4G) {
    m_EnableDualRadar->Enable();
  } else {
    m_EnableDualRadar->Disable();
  }

  m_IgnoreHeading = new wxCheckBox(this, wxID_ANY, _("Ignore radar heading"), wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_IgnoreHeading, 0, wxALL, border_size);
  m_IgnoreHeading->SetValue(m_settings.ignore_radar_heading);
  m_IgnoreHeading->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnIgnoreHeadingClick), NULL,
                           this);

  m_PassHeading = new wxCheckBox(this, wxID_ANY, _("Pass radar heading to OpenCPN"), wxDefaultPosition, wxDefaultSize,
                                 wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_PassHeading, 0, wxALL, border_size);
  m_PassHeading->SetValue(m_settings.pass_heading_to_opencpn);
  m_PassHeading->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnPassHeadingClick), NULL, this);

  m_COGHeading = new wxCheckBox(this, wxID_ANY, _("Enable COG as heading"), wxDefaultPosition, wxDefaultSize,
                                wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_COGHeading, 0, wxALL, border_size);
  m_COGHeading->SetValue(m_settings.enable_cog_heading);
  m_COGHeading->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnEnableCOGHeadingClick), NULL,
                        this);

  m_Emulator =
      new wxCheckBox(this, wxID_ANY, _("Emulator mode"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_Emulator, 0, wxALL, border_size);
  m_Emulator->SetValue(m_settings.emulator_on ? true : false);
  m_Emulator->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnEmulatorClick), NULL, this);

  m_ReverseZoom = new wxCheckBox(this, wxID_ANY, _("Reverse mouse wheel zoom direction"), wxDefaultPosition, wxDefaultSize,
                                 wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  itemStaticBoxSizerOptions->Add(m_ReverseZoom, 0, wxALL, border_size);
  m_ReverseZoom->SetValue(m_settings.reverse_zoom ? true : false);
  m_ReverseZoom->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24OptionsDialog::OnReverseZoomClick), NULL, this);

  // Accept/Reject button
  wxStdDialogButtonSizer *DialogButtonSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
  topSizer->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, border_size);

  DimeWindow(this);

  Fit();
  // SetMinSize(GetBestSize());
  SetLayoutAdaptationMode(wxDIALOG_ADAPTATION_MODE_ENABLED);
}

void br24OptionsDialog::OnRangeUnitsClick(wxCommandEvent &event) {
  m_settings.range_units = (RangeUnits)m_RangeUnits->GetSelection();
}

void br24OptionsDialog::OnGuardZoneStyleClick(wxCommandEvent &event) {
  m_settings.guard_zone_render_style = m_GuardZoneStyle->GetSelection();
}

void br24OptionsDialog::OnGuardZoneOnOverlayClick(wxCommandEvent &event) {
  m_settings.guard_zone_on_overlay = m_GuardZoneOnOverlay->GetValue();
}

void br24OptionsDialog::OnShowExtremeRangeClick(wxCommandEvent &event) {
  m_settings.show_extreme_range = m_ShowExtremeRange->GetValue();
}

void br24OptionsDialog::OnTrailsOnOverlayClick(wxCommandEvent &event) {
  m_settings.trails_on_overlay = m_TrailsOnOverlay->GetValue();
}

void br24OptionsDialog::OnTrailStartColourClick(wxCommandEvent &event) {
  m_settings.trail_start_colour = m_TrailStartColour->GetColour();
}

void br24OptionsDialog::OnTrailEndColourClick(wxCommandEvent &event) {
  m_settings.trail_end_colour = m_TrailEndColour->GetColour();
}

void br24OptionsDialog::OnWeakColourClick(wxCommandEvent &event) { m_settings.weak_colour = m_WeakColour->GetColour(); }

void br24OptionsDialog::OnArpaColourClick(wxCommandEvent &event) { m_settings.arpa_colour = m_ArpaColour->GetColour(); }

void br24OptionsDialog::OnAisTextColourClick(wxCommandEvent &event) { m_settings.ais_text_colour = m_AisTextColour->GetColour(); }

void br24OptionsDialog::OnPPIBackgroundColourClick(wxCommandEvent &event) {
  m_settings.ppi_background_colour = m_PPIBackgroundColour->GetColour();
}

void br24OptionsDialog::OnIntermediateColourClick(wxCommandEvent &event) {
  m_settings.intermediate_colour = m_IntermediateColour->GetColour();
}

void br24OptionsDialog::OnStrongColourClick(wxCommandEvent &event) { m_settings.strong_colour = m_StrongColour->GetColour(); }

void br24OptionsDialog::OnSelectSoundClick(wxCommandEvent &event) {
  wxString *sharedData = GetpSharedDataLocation();
  wxString sound_dir;

  sound_dir.Append(*sharedData);
  sound_dir.Append(wxT("sounds"));

  wxFileDialog *openDialog = new wxFileDialog(NULL, _("Select Sound File"), sound_dir, wxT(""),
                                              _("WAV files (*.wav)|*.wav|All files (*.*)|*.*"), wxFD_OPEN);
  int response = openDialog->ShowModal();
  if (response == wxID_OK) {
    m_settings.alert_audio_file = openDialog->GetPath();
  }
}

void br24OptionsDialog::OnGuardZoneTimeoutClick(wxCommandEvent &event) {
  wxString temp = m_GuardZoneTimeout->GetValue();

  m_settings.guard_zone_timeout = strtol(temp.c_str(), 0, 0);
}

void br24OptionsDialog::OnEnableCOGHeadingClick(wxCommandEvent &event) { m_settings.enable_cog_heading = m_COGHeading->GetValue(); }

void br24OptionsDialog::OnEnableDualRadarClick(wxCommandEvent &event) {
  m_settings.enable_dual_radar = m_EnableDualRadar->GetValue();
}

void br24OptionsDialog::OnTestSoundClick(wxCommandEvent &event) {
  if (!m_settings.alert_audio_file.IsEmpty()) {
    PlugInPlaySound(m_settings.alert_audio_file);
  }
}

void br24OptionsDialog::OnIgnoreHeadingClick(wxCommandEvent &event) {
  m_settings.ignore_radar_heading = m_IgnoreHeading->GetValue();
}

void br24OptionsDialog::OnPassHeadingClick(wxCommandEvent &event) {
  m_settings.pass_heading_to_opencpn = m_PassHeading->GetValue();
}

void br24OptionsDialog::OnMenuAutoHideClick(wxCommandEvent &event) { m_settings.menu_auto_hide = m_MenuAutoHide->GetSelection(); }

void br24OptionsDialog::OnDrawingMethodClick(wxCommandEvent &event) { m_settings.drawing_method = m_DrawingMethod->GetSelection(); }

void br24OptionsDialog::OnEmulatorClick(wxCommandEvent &event) { m_settings.emulator_on = m_Emulator->GetValue(); }
void br24OptionsDialog::OnReverseZoomClick(wxCommandEvent &event) { m_settings.reverse_zoom = m_ReverseZoom->GetValue(); }

PLUGIN_END_NAMESPACE
