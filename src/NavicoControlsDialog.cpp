/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
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

#include "NavicoControlsDialog.h"
#include "RadarMarpa.h"
#include "RadarPanel.h"

PLUGIN_BEGIN_NAMESPACE

enum {  // process ID's
  ID_TEXTCTRL1 = 10000,
  ID_BACK,
  ID_PLUS_TEN,
  ID_PLUS,
  ID_VALUE,
  ID_MINUS,
  ID_MINUS_TEN,
  ID_AUTO,
  ID_TRAILS_MOTION,

  ID_TRANSPARENCY,
  ID_INTERFERENCE_REJECTION,
  ID_TARGET_BOOST,
  ID_TARGET_EXPANSION,
  ID_NOISE_REJECTION,
  ID_TARGET_SEPARATION,
  ID_REFRESHRATE,
  ID_SCAN_SPEED,
  ID_INSTALLATION,
  ID_PREFERENCES,

  ID_BEARING_ALIGNMENT,
  ID_ANTENNA_HEIGHT,
  ID_ANTENNA_FORWARD,
  ID_ANTENNA_STARBOARD,
  ID_LOCAL_INTERFERENCE_REJECTION,
  ID_SIDE_LOBE_SUPPRESSION,
  ID_MAIN_BANG_SIZE,

  ID_RANGE,
  ID_GAIN,
  ID_SEA,
  ID_RAIN,

  ID_CLEAR_CURSOR,
  ID_ACQUIRE_TARGET,
  ID_DELETE_TARGET,
  ID_DELETE_ALL_TARGETS,

  ID_TARGETS,
  ID_TARGET_TRAILS,
  ID_CLEAR_TRAILS,
  ID_ORIENTATION,

  ID_TRANSMIT,
  ID_STANDBY,
  ID_TIMED_IDLE,
  ID_TIMED_RUN,

  ID_SHOW_RADAR,
  ID_RADAR_OVERLAY,
  ID_ADJUST,
  ID_ADVANCED,
  ID_VIEW,
  ID_BEARING,
  ID_ZONE1,
  ID_ZONE2,
  ID_POWER,

  ID_CONFIRM_BOGEY,

  ID_MESSAGE,
  ID_BPOS,
  ID_HEADING,
  ID_RADAR,
  ID_DATA,

  ID_BEARING_SET,  // next one should be BEARING_LINES higher
  ID_NEXT = ID_BEARING_SET + BEARING_LINES,

};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(NavicoControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(NavicoControlsDialog, wxDialog)

EVT_CLOSE(NavicoControlsDialog::OnClose)
EVT_BUTTON(ID_BACK, NavicoControlsDialog::OnBackClick)
EVT_BUTTON(ID_PLUS_TEN, NavicoControlsDialog::OnPlusTenClick)
EVT_BUTTON(ID_PLUS, NavicoControlsDialog::OnPlusClick)
EVT_BUTTON(ID_MINUS, NavicoControlsDialog::OnMinusClick)
EVT_BUTTON(ID_MINUS_TEN, NavicoControlsDialog::OnMinusTenClick)
EVT_BUTTON(ID_AUTO, NavicoControlsDialog::OnAutoClick)
EVT_BUTTON(ID_TRAILS_MOTION, NavicoControlsDialog::OnTrailsMotionClick)

EVT_BUTTON(ID_TRANSPARENCY, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INTERFERENCE_REJECTION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_BOOST, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_EXPANSION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_NOISE_REJECTION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_SEPARATION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_REFRESHRATE, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SCAN_SPEED, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INSTALLATION, NavicoControlsDialog::OnInstallationButtonClick)
EVT_BUTTON(ID_PREFERENCES, NavicoControlsDialog::OnPreferencesButtonClick)

EVT_BUTTON(ID_BEARING_ALIGNMENT, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_HEIGHT, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_FORWARD, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_STARBOARD, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_LOCAL_INTERFERENCE_REJECTION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SIDE_LOBE_SUPPRESSION, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_MAIN_BANG_SIZE, NavicoControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_POWER, NavicoControlsDialog::OnPowerButtonClick)
EVT_BUTTON(ID_SHOW_RADAR, NavicoControlsDialog::OnRadarShowButtonClick)
EVT_BUTTON(ID_RADAR_OVERLAY, NavicoControlsDialog::OnRadarOverlayButtonClick)
EVT_BUTTON(ID_RANGE, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_GAIN, NavicoControlsDialog::OnRadarGainButtonClick)
EVT_BUTTON(ID_SEA, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_RAIN, NavicoControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_TARGETS, NavicoControlsDialog::OnTargetsButtonClick)
EVT_BUTTON(ID_TARGET_TRAILS, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_CLEAR_TRAILS, NavicoControlsDialog::OnClearTrailsButtonClick)
EVT_BUTTON(ID_ORIENTATION, NavicoControlsDialog::OnOrientationButtonClick)

EVT_BUTTON(ID_ADJUST, NavicoControlsDialog::OnAdjustButtonClick)
EVT_BUTTON(ID_ADVANCED, NavicoControlsDialog::OnAdvancedButtonClick)
EVT_BUTTON(ID_VIEW, NavicoControlsDialog::OnViewButtonClick)

EVT_BUTTON(ID_BEARING, NavicoControlsDialog::OnBearingButtonClick)
EVT_BUTTON(ID_ZONE1, NavicoControlsDialog::OnZone1ButtonClick)
EVT_BUTTON(ID_ZONE2, NavicoControlsDialog::OnZone2ButtonClick)

EVT_BUTTON(ID_MESSAGE, NavicoControlsDialog::OnMessageButtonClick)

EVT_BUTTON(ID_BEARING_SET, NavicoControlsDialog::OnBearingSetButtonClick)
EVT_BUTTON(ID_CLEAR_CURSOR, NavicoControlsDialog::OnClearCursorButtonClick)
EVT_BUTTON(ID_ACQUIRE_TARGET, NavicoControlsDialog::OnAcquireTargetButtonClick)
EVT_BUTTON(ID_DELETE_TARGET, NavicoControlsDialog::OnDeleteTargetButtonClick)
EVT_BUTTON(ID_DELETE_ALL_TARGETS, NavicoControlsDialog::OnDeleteAllTargetsButtonClick)

EVT_BUTTON(ID_TRANSMIT, NavicoControlsDialog::OnTransmitButtonClick)
EVT_BUTTON(ID_STANDBY, NavicoControlsDialog::OnStandbyButtonClick)
EVT_BUTTON(ID_TIMED_IDLE, NavicoControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TIMED_RUN, NavicoControlsDialog::OnRadarControlButtonClick)

EVT_MOVE(NavicoControlsDialog::OnMove)
EVT_CLOSE(NavicoControlsDialog::OnClose)

END_EVENT_TABLE()

static wxSize g_buttonSize;

wxString interference_rejection_names[4];
wxString target_separation_names[4];
wxString noise_rejection_names[3];
wxString target_boost_names[3];
wxString target_expansion_names[2];
wxString scan_speed_names[2];
wxString timed_idle_times[8];
wxString timed_run_times[3];
wxString target_trail_names[TRAIL_ARRAY_SIZE];
wxString sea_clutter_names[2];

bool NavicoControlsDialog::Create(wxWindow* parent, radar_pi* ppi, RadarInfo* ri, wxWindowID id, const wxString& caption,
                                  const wxPoint& pos) {
  if (!ControlsDialog::Create(parent, ppi, ri, id, caption, pos)) {
    return false;
  }

  CreateControls();

  return true;
}

void NavicoControlsDialog::CreateControls() {
  static int BORDER = 0;
  wxString backButtonStr;
  backButtonStr << wxT("<<\n") << _("Back");

  // A top-level sizer
  m_top_sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(m_top_sizer);

  /*
   * Here be dragons...
   * Since I haven't been able to create buttons that adapt up, and at the same
   * time calculate the biggest button, and I want all buttons to be the same width I use a trick.
   * I generate a multiline StaticText containing all the (long) button labels and find out what the
   * width of that is, and then generate the buttons using that width.
   * I know, this is a hack, but this way it works relatively nicely even with translations.
   */

  wxBoxSizer* testBox = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(testBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  wxString label;
  label << _("Standby / Transmit") << wxT("\n");
  label << _("Noise rejection") << wxT("\n");
  label << _("Target expansion") << wxT("\n");
  label << _("Interference rejection") << wxT("\n");
  label << _("Target separation") << wxT("\n");
  label << _("Scan speed") << wxT("\n");
  label << _("Target boost") << wxT("\n");
  label << _("Installation") << wxT("\n");
  label << _("Bearing alignment") << wxT("\n");
  label << _("Antenna height") << wxT("\n");
  label << _("Antenna forward of GPS") << wxT("\n");
  label << _("Antenna starboard of GPS") << wxT("\n");
  label << _("Local interference rej.") << wxT("\n");
  label << _("Side lobe suppression") << wxT("\n");
  label << _("Main bang size") << wxT("\n");
  label << _("Guard zones") << wxT("\n");
  label << _("Zone type") << wxT("\n");
  label << _("Guard zones") << wxT("\n");
  label << _("Inner range") << wxT("\n");
  label << _("Outer range") << wxT("\n");
  label << _("Start bearing") << wxT("\n");
  label << _("End bearing") << wxT("\n");
  label << _("Range") << wxT("\n");
  label << _("Gain") << wxT("\n");
  label << _("Sea clutter") << wxT("\n");
  label << _("Rain clutter") << wxT("\n");
  label << _("Clear cursor") << wxT("\n");
  label << _("Place EBL/VRM") << wxT("\n");
  label << _("Target trails") << wxT("\n");
  label << _("Off/Relative/True trails") << wxT("\n");
  label << _("Clear trails") << wxT("\n");
  label << _("Orientation") << wxT("\n");
  label << _("Refresh rate") << wxT("\n");
  label << _("Transparency") << wxT("\n");
  label << _("Overlay") << wxT("\n");
  label << _("Adjust") << wxT("\n");
  label << _("Advanced") << wxT("\n");
  label << _("View") << wxT("\n");
  label << _("EBL/VRM") << wxT("\n");
  label << _("Timed Transmit") << wxT("\n");
  label << _("Info") << wxT("\n");

  wxStaticText* testMessage =
      new wxStaticText(this, ID_BPOS, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  testMessage->SetFont(m_pi->m_font);
  testBox->Add(testMessage, 0, wxALL, 2);

  wxStaticText* testButtonText =
      new wxStaticText(this, ID_BPOS, wxT("Button"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  testButtonText->SetFont(m_pi->m_font);
  testBox->Add(testButtonText, 0, wxALL, 2);

  wxStaticText* testButton2Text =
      new wxStaticText(this, ID_BPOS, wxT("Button\ndata"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  testButton2Text->SetFont(m_pi->m_font);
  testBox->Add(testButton2Text, 0, wxALL, 2);

  m_top_sizer->Fit(this);
  m_top_sizer->Layout();
  int width = m_top_sizer->GetSize().GetWidth() + 20;

  wxSize bestSize = GetBestSize();
  if (width < bestSize.GetWidth()) {
    width = bestSize.GetWidth();
  }
  if (width < 100) {
    width = 100;
  }
  if (width > 500) {
    width = 500;
  }

#define BUTTON_BORDER 4
#ifdef __WXMAC__
#define BUTTON_HEIGTH_FUDGE 1 + BUTTON_BORDER
#else
#define BUTTON_HEIGTH_FUDGE 1 + 2 * BUTTON_BORDER
#endif

  g_buttonSize = wxSize(width, testButton2Text->GetBestSize().y * BUTTON_HEIGTH_FUDGE);
  LOG_DIALOG(wxT("%s Dynamic button width = %d height = %d"), m_log_name.c_str(), g_buttonSize.x, g_buttonSize.y);

  m_top_sizer->Hide(testBox);
  m_top_sizer->Remove(testBox);
  // delete testBox; -- this core dumps!
  // Determined desired button width

  //**************** EDIT BOX ******************//
  // A box sizer to contain RANGE button
  m_edit_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_edit_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // A box sizer to contain RANGE button
  m_edit_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_edit_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The <<Back button
  RadarButton* back_button = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_edit_sizer->Add(back_button, 0, wxALL, BORDER);

  // The +10 button
  m_plus_ten_button = new RadarButton(this, ID_PLUS_TEN, g_buttonSize, _("+10"));
  m_edit_sizer->Add(m_plus_ten_button, 0, wxALL, BORDER);

  // The + button
  m_plus_button = new RadarButton(this, ID_PLUS, g_buttonSize, _("+"));
  m_edit_sizer->Add(m_plus_button, 0, wxALL, BORDER);

  // The VALUE text
  wxSize valueSize = wxSize(g_buttonSize.x, g_buttonSize.y);
  m_value_text = new wxStaticText(this, ID_VALUE, _("Value"), wxDefaultPosition, valueSize, wxALIGN_CENTRE_HORIZONTAL);
  m_edit_sizer->Add(m_value_text, 0, wxALL, BORDER);
  m_value_text->SetFont(m_pi->m_fat_font);
  m_value_text->SetBackgroundColour(*wxLIGHT_GREY);

  // The COMMENT text
  m_comment_text = new DynamicStaticText(this, ID_VALUE, wxT(""), wxDefaultPosition, g_buttonSize, wxALIGN_CENTRE_HORIZONTAL);
  m_edit_sizer->Add(m_comment_text, 0, wxALL, BORDER);
  m_comment_text->SetBackgroundColour(*wxLIGHT_GREY);

  // The - button
  m_minus_button = new RadarButton(this, ID_MINUS, g_buttonSize, _("-"));
  m_edit_sizer->Add(m_minus_button, 0, wxALL, BORDER);

  // The -10 button
  m_minus_ten_button = new RadarButton(this, ID_MINUS_TEN, g_buttonSize, _("-10"));
  m_edit_sizer->Add(m_minus_ten_button, 0, wxALL, BORDER);

  // The Auto button
  m_auto_button = new RadarButton(this, ID_AUTO, g_buttonSize, _("Auto"));
  m_edit_sizer->Add(m_auto_button, 0, wxALL, BORDER);

  m_top_sizer->Hide(m_edit_sizer);

  //**************** ADVANCED BOX ******************//
  // These are the controls that the users sees when the Advanced button is selected

  m_advanced_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_advanced_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bAdvancedBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_advanced_sizer->Add(bAdvancedBack, 0, wxALL, BORDER);

  // The NOISE REJECTION button
  noise_rejection_names[0] = _("Off");
  noise_rejection_names[1] = _("Low");
  noise_rejection_names[2] = _("High");

  m_noise_rejection_button =
      new RadarControlButton(this, ID_NOISE_REJECTION, g_buttonSize, _("Noise rejection"), CT_NOISE_REJECTION, false, 0);
  m_advanced_sizer->Add(m_noise_rejection_button, 0, wxALL, BORDER);
  m_noise_rejection_button->minValue = 0;
  m_noise_rejection_button->maxValue = ARRAY_SIZE(noise_rejection_names) - 1;
  m_noise_rejection_button->names = noise_rejection_names;
  m_noise_rejection_button->SetLocalValue(m_ri->m_noise_rejection.GetButton());  // redraw after adding names

  m_advanced_4G_sizer = new wxBoxSizer(wxVERTICAL);
  m_advanced_sizer->Add(m_advanced_4G_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  // The TARGET EXPANSION button
  target_expansion_names[0] = _("Off");
  target_expansion_names[1] = _("On");
  m_target_expansion_button =
      new RadarControlButton(this, ID_TARGET_EXPANSION, g_buttonSize, _("Target expansion"), CT_TARGET_EXPANSION, false, 0);
  m_advanced_sizer->Add(m_target_expansion_button, 0, wxALL, BORDER);
  m_target_expansion_button->minValue = 0;
  m_target_expansion_button->maxValue = ARRAY_SIZE(target_expansion_names) - 1;
  m_target_expansion_button->names = target_expansion_names;
  m_target_expansion_button->SetLocalValue(m_ri->m_target_expansion.GetButton());  // redraw after adding names

  // The REJECTION button

  interference_rejection_names[0] = _("Off");
  interference_rejection_names[1] = _("Low");
  interference_rejection_names[2] = _("Medium");
  interference_rejection_names[3] = _("High");

  m_interference_rejection_button = new RadarControlButton(this, ID_INTERFERENCE_REJECTION, g_buttonSize,
                                                           _("Interference rejection"), CT_INTERFERENCE_REJECTION, false, 0);
  m_advanced_sizer->Add(m_interference_rejection_button, 0, wxALL, BORDER);
  m_interference_rejection_button->minValue = 0;
  m_interference_rejection_button->maxValue = ARRAY_SIZE(interference_rejection_names) - 1;
  m_interference_rejection_button->names = interference_rejection_names;
  m_interference_rejection_button->SetLocalValue(m_ri->m_interference_rejection.GetButton());  // redraw after adding names

  // The TARGET SEPARATION button

  target_separation_names[0] = _("Off");
  target_separation_names[1] = _("Low");
  target_separation_names[2] = _("Medium");
  target_separation_names[3] = _("High");

  m_target_separation_button =
      new RadarControlButton(this, ID_TARGET_SEPARATION, g_buttonSize, _("Target separation"), CT_TARGET_SEPARATION, false, 0);
  m_advanced_4G_sizer->Add(m_target_separation_button, 0, wxALL, BORDER);
  m_target_separation_button->minValue = 0;
  m_target_separation_button->maxValue = ARRAY_SIZE(target_separation_names) - 1;
  m_target_separation_button->names = target_separation_names;
  m_target_separation_button->SetLocalValue(m_ri->m_target_separation.GetButton());  // redraw after adding names

  // The SCAN SPEED button
  scan_speed_names[0] = _("Normal");
  scan_speed_names[1] = _("Fast");
  m_scan_speed_button = new RadarControlButton(this, ID_SCAN_SPEED, g_buttonSize, _("Scan speed"), CT_SCAN_SPEED, false, 0);
  m_advanced_sizer->Add(m_scan_speed_button, 0, wxALL, BORDER);
  m_scan_speed_button->minValue = 0;
  m_scan_speed_button->maxValue = ARRAY_SIZE(scan_speed_names) - 1;
  m_scan_speed_button->names = scan_speed_names;
  m_scan_speed_button->SetLocalValue(m_ri->m_scan_speed.GetButton());  // redraw after adding names

  // The TARGET BOOST button
  target_boost_names[0] = _("Off");
  target_boost_names[1] = _("Low");
  target_boost_names[2] = _("High");
  m_target_boost_button = new RadarControlButton(this, ID_TARGET_BOOST, g_buttonSize, _("Target boost"), CT_TARGET_BOOST, false, 0);
  m_advanced_sizer->Add(m_target_boost_button, 0, wxALL, BORDER);
  m_target_boost_button->minValue = 0;
  m_target_boost_button->maxValue = ARRAY_SIZE(target_boost_names) - 1;
  m_target_boost_button->names = target_boost_names;
  m_target_boost_button->SetLocalValue(m_ri->m_target_boost.GetButton());  // redraw after adding names

  // The INSTALLATION button
  RadarButton* bInstallation = new RadarButton(this, ID_INSTALLATION, g_buttonSize, _("Installation"));
  m_advanced_sizer->Add(bInstallation, 0, wxALL, BORDER);

  // The PREFERENCES button
  RadarButton* bPreferences = new RadarButton(this, ID_PREFERENCES, g_buttonSize, _("Preferences"));
  m_advanced_sizer->Add(bPreferences, 0, wxALL, BORDER);

  m_top_sizer->Hide(m_advanced_sizer);

  //**************** Installation BOX ******************//
  // These are the controls that the users sees when the Installation button is selected

  m_installation_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_installation_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bInstallationBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_installation_sizer->Add(bInstallationBack, 0, wxALL, BORDER);

  // The BEARING ALIGNMENT button
  m_bearing_alignment_button =
      new RadarControlButton(this, ID_BEARING_ALIGNMENT, g_buttonSize, _("Bearing alignment"), CT_BEARING_ALIGNMENT, false,
                             m_ri->m_bearing_alignment.GetButton(), _("degrees"), _("relative to bow"));
  m_installation_sizer->Add(m_bearing_alignment_button, 0, wxALL, BORDER);
  m_bearing_alignment_button->minValue = -179;
  m_bearing_alignment_button->maxValue = 180;

  // The ANTENNA HEIGHT button
  m_antenna_height_button = new RadarControlButton(this, ID_ANTENNA_HEIGHT, g_buttonSize, _("Antenna height"), CT_ANTENNA_HEIGHT,
                                                   false, m_ri->m_antenna_height.GetButton(), _("m"), _("above sealevel"));
  m_installation_sizer->Add(m_antenna_height_button, 0, wxALL, BORDER);
  m_antenna_height_button->minValue = 0;
  m_antenna_height_button->maxValue = 30;

  // The ANTENNA FORWARD button
  m_antenna_forward_button =
      new RadarControlButton(this, ID_ANTENNA_FORWARD, g_buttonSize, _("Antenna forward"), CT_ANTENNA_FORWARD, false,
                             m_pi->m_settings.antenna_forward, _("m"), _("relative to GPS") + wxT("\n") + _("negative = behind"));
  m_installation_sizer->Add(m_antenna_forward_button, 0, wxALL, BORDER);
  m_antenna_forward_button->minValue = -200;
  m_antenna_forward_button->maxValue = 200;

  // The ANTENNA STARBOARD button
  m_antenna_starboard_button =
      new RadarControlButton(this, ID_ANTENNA_STARBOARD, g_buttonSize, _("Antenna starboard"), CT_ANTENNA_STARBOARD, false,
                             m_pi->m_settings.antenna_starboard, _("m"), _("relative to GPS") + wxT("\n") + _("negative = port"));
  m_installation_sizer->Add(m_antenna_starboard_button, 0, wxALL, BORDER);
  m_antenna_starboard_button->minValue = -50;
  m_antenna_starboard_button->maxValue = 50;

  // The LOCAL INTERFERENCE REJECTION button
  m_local_interference_rejection_button = new RadarControlButton(
      this, ID_LOCAL_INTERFERENCE_REJECTION, g_buttonSize, _("Local interference rej."), CT_LOCAL_INTERFERENCE_REJECTION, false, 0);
  m_installation_sizer->Add(m_local_interference_rejection_button, 0, wxALL, BORDER);
  m_local_interference_rejection_button->minValue = 0;
  m_local_interference_rejection_button->maxValue =
      ARRAY_SIZE(target_separation_names) - 1;  // off, low, medium, high, same as target separation
  m_local_interference_rejection_button->names = target_separation_names;
  m_local_interference_rejection_button->SetLocalValue(m_ri->m_local_interference_rejection.GetButton());

  // The SIDE LOBE SUPPRESSION button
  m_side_lobe_suppression_button = new RadarControlButton(this, ID_SIDE_LOBE_SUPPRESSION, g_buttonSize, _("Side lobe suppression"),
                                                          CT_SIDE_LOBE_SUPPRESSION, true, 0, wxT("%"));
  m_installation_sizer->Add(m_side_lobe_suppression_button, 0, wxALL, BORDER);
  m_side_lobe_suppression_button->minValue = 0;
  m_side_lobe_suppression_button->maxValue = 100;
  m_side_lobe_suppression_button->SetLocalValue(m_ri->m_side_lobe_suppression.GetButton());  // redraw after adding names

  // The MAIN BANG SIZE button
  m_main_bang_size_button = new RadarControlButton(this, ID_MAIN_BANG_SIZE, g_buttonSize, _("Main bang size"), CT_MAIN_BANG_SIZE,
                                                   false, m_pi->m_settings.main_bang_size);
  m_installation_sizer->Add(m_main_bang_size_button, 0, wxALL, BORDER);
  m_main_bang_size_button->minValue = 0;
  m_main_bang_size_button->maxValue = 10;

  m_top_sizer->Hide(m_installation_sizer);

  //***************** GUARD ZONE EDIT BOX *************//

  m_guard_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_guard_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The <<Back button
  RadarButton* guard_back_button = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_guard_sizer->Add(guard_back_button, 0, wxALL, BORDER);

  m_guard_zone_text = new wxStaticText(this, wxID_ANY, _("Guard zones"));
  m_guard_sizer->Add(m_guard_zone_text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

  wxStaticText* type_Text = new wxStaticText(this, wxID_ANY, _("Zone type"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(type_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_guard_zone_type = new wxRadioBox(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, ARRAY_SIZE(guard_zone_names),
                                     guard_zone_names, 1, wxRA_SPECIFY_COLS);
  m_guard_sizer->Add(m_guard_zone_type, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_guard_zone_type->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(NavicoControlsDialog::OnGuardZoneModeClick),
                             NULL, this);

  // Inner and Outer Ranges
  wxStaticText* inner_range_Text = new wxStaticText(this, wxID_ANY, _("Inner range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(inner_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_inner_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_inner_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_inner_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(NavicoControlsDialog::OnInner_Range_Value), NULL, this);
  ///   start of copy
  wxStaticText* outer_range_Text = new wxStaticText(this, wxID_ANY, _("Outer range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(outer_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_outer_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_outer_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_outer_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(NavicoControlsDialog::OnOuter_Range_Value), NULL, this);

  wxStaticText* pStart_Bearing = new wxStaticText(this, wxID_ANY, _("Start bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pStart_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_start_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_start_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_start_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(NavicoControlsDialog::OnStart_Bearing_Value), NULL,
                           this);

  wxStaticText* pEnd_Bearing = new wxStaticText(this, wxID_ANY, _("End bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pEnd_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_end_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_end_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_end_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(NavicoControlsDialog::OnEnd_Bearing_Value), NULL, this);

  // checkbox for ARPA
  m_arpa_box = new wxCheckBox(this, wxID_ANY, _("ARPA On"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
  m_guard_sizer->Add(m_arpa_box, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  m_arpa_box->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(NavicoControlsDialog::OnARPAClick), NULL, this);

  // checkbox for blob alarm
  m_alarm = new wxCheckBox(this, wxID_ANY, _("Alarm On"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
  m_guard_sizer->Add(m_alarm, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  m_alarm->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(NavicoControlsDialog::OnAlarmClick), NULL, this);

  m_top_sizer->Hide(m_guard_sizer);

  //**************** ADJUST BOX ******************//

  m_adjust_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_adjust_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bAdjustBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_adjust_sizer->Add(bAdjustBack, 0, wxALL, BORDER);

  // The RANGE button
  m_range_button = new RadarRangeControlButton(this, m_ri, ID_RANGE, g_buttonSize, _("Range"));
  m_adjust_sizer->Add(m_range_button, 0, wxALL, BORDER);

  // The GAIN button
  m_gain_button = new RadarControlButton(this, ID_GAIN, g_buttonSize, _("Gain"), CT_GAIN, true, m_ri->m_gain.GetButton());
  m_adjust_sizer->Add(m_gain_button, 0, wxALL, BORDER);

  // The SEA button
  sea_clutter_names[0] = _("Harbour");
  sea_clutter_names[1] = _("Offshore");
  m_sea_button = new RadarControlButton(this, ID_SEA, g_buttonSize, _("Sea clutter"), CT_SEA, false, 0);
  m_sea_button->autoNames = sea_clutter_names;
  m_sea_button->autoValues = 2;
  m_sea_button->SetLocalValue(m_ri->m_sea.GetButton());
  m_adjust_sizer->Add(m_sea_button, 0, wxALL, BORDER);

  // The RAIN button
  m_rain_button = new RadarControlButton(this, ID_RAIN, g_buttonSize, _("Rain clutter"), CT_RAIN, false, m_ri->m_rain.GetButton());
  m_adjust_sizer->Add(m_rain_button, 0, wxALL, BORDER);

  m_top_sizer->Hide(m_adjust_sizer);

  //**************** CURSOR BOX ******************//

  m_cursor_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_cursor_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bCursorBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_cursor_sizer->Add(bCursorBack, 0, wxALL, BORDER);

  // The CLEAR CURSOR button
  m_clear_cursor = new RadarButton(this, ID_CLEAR_CURSOR, g_buttonSize, _("Clear cursor"));
  m_cursor_sizer->Add(m_clear_cursor, 0, wxALL, BORDER);

  // The ACQUIRE TARGET button
  m_acquire_target = new RadarButton(this, ID_ACQUIRE_TARGET, g_buttonSize, _("Acquire Target"));
  m_cursor_sizer->Add(m_acquire_target, 0, wxALL, BORDER);

  // The DELETE TARGET button
  m_delete_target = new RadarButton(this, ID_DELETE_TARGET, g_buttonSize, _("Delete target"));
  m_cursor_sizer->Add(m_delete_target, 0, wxALL, BORDER);

  // The DELETE ALL button
  m_delete_all = new RadarButton(this, ID_DELETE_ALL_TARGETS, g_buttonSize, _("Delete all targets"));
  m_cursor_sizer->Add(m_delete_all, 0, wxALL, BORDER);

  for (int b = 0; b < BEARING_LINES; b++) {
    // The BEARING button
    wxString label = _("Place EBL/VRM");
    label << wxString::Format(wxT("%d"), b + 1);
    m_bearing_buttons[b] = new RadarButton(this, ID_BEARING_SET + b, g_buttonSize, label);
    m_cursor_sizer->Add(m_bearing_buttons[b], 0, wxALL, BORDER);
    m_bearing_buttons[b]->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler(NavicoControlsDialog::OnBearingSetButtonClick), 0, this);
  }

  m_top_sizer->Hide(m_cursor_sizer);

  //**************** VIEW BOX ******************//
  // These are the controls that the users sees when the View button is selected

  m_view_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_view_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bMenuBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_view_sizer->Add(bMenuBack, 0, wxALL, BORDER);

  // The Show Targets button
  m_targets_button = new RadarButton(this, ID_TARGETS, g_buttonSize, _("Show AIS/ARPA"));
  m_view_sizer->Add(m_targets_button, 0, wxALL, BORDER);

  // The Trails Motion button
  m_trails_motion_button = new RadarButton(this, ID_TRAILS_MOTION, g_buttonSize, _("Off/Relative/True trails"));
  m_view_sizer->Add(m_trails_motion_button, 0, wxALL, BORDER);

  // The TARGET_TRAIL button
  target_trail_names[TRAIL_15SEC] = _("15 sec");
  target_trail_names[TRAIL_30SEC] = _("30 sec");
  target_trail_names[TRAIL_1MIN] = _("1 min");
  target_trail_names[TRAIL_3MIN] = _("3 min");
  target_trail_names[TRAIL_5MIN] = _("5 min");
  target_trail_names[TRAIL_10MIN] = _("10 min");
  target_trail_names[TRAIL_CONTINUOUS] = _("Continuous");

  m_target_trails_button =
      new RadarControlButton(this, ID_TARGET_TRAILS, g_buttonSize, _("Target trails"), CT_TARGET_TRAILS, false, 0);
  m_view_sizer->Add(m_target_trails_button, 0, wxALL, BORDER);
  m_target_trails_button->minValue = 0;
  m_target_trails_button->maxValue = ARRAY_SIZE(target_trail_names) - 1;
  m_target_trails_button->names = target_trail_names;
  m_target_trails_button->SetLocalValue(m_ri->m_target_trails.GetButton());  // redraw after adding names
  m_target_trails_button->Hide();

  // The Clear Trails button
  m_clear_trails_button = new RadarButton(this, ID_CLEAR_TRAILS, g_buttonSize, _("Clear trails"));
  m_view_sizer->Add(m_clear_trails_button, 0, wxALL, BORDER);
  m_clear_trails_button->Hide();

  // The Rotation button
  m_orientation_button = new RadarButton(this, ID_ORIENTATION, g_buttonSize, _("Orientation"));
  m_view_sizer->Add(m_orientation_button, 0, wxALL, BORDER);
  // Updated when we receive data

  // The REFRESHRATE button
  m_refresh_rate_button = new RadarControlButton(this, ID_REFRESHRATE, g_buttonSize, _("Refresh rate"), CT_REFRESHRATE, false,
                                                 m_pi->m_settings.refreshrate);
  m_view_sizer->Add(m_refresh_rate_button, 0, wxALL, BORDER);
  m_refresh_rate_button->minValue = 1;
  m_refresh_rate_button->maxValue = 5;

  // The TRANSPARENCY button
  m_transparency_button = new RadarControlButton(this, ID_TRANSPARENCY, g_buttonSize, _("Transparency"), CT_TRANSPARENCY, false,
                                                 m_pi->m_settings.overlay_transparency);
  m_view_sizer->Add(m_transparency_button, 0, wxALL, BORDER);
  m_transparency_button->minValue = MIN_OVERLAY_TRANSPARENCY;
  m_transparency_button->maxValue = MAX_OVERLAY_TRANSPARENCY;

  m_top_sizer->Hide(m_view_sizer);

  //***************** POWER BOX *************//

  m_power_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_power_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The <<Back button
  RadarButton* power_back_button = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_power_sizer->Add(power_back_button, 0, wxALL, BORDER);

  m_power_text = new wxStaticText(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
  m_power_sizer->Add(m_power_text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_transmit_button = new RadarButton(this, ID_TRANSMIT, g_buttonSize, _("Transmit"));
  m_power_sizer->Add(m_transmit_button, 0, wxALL, BORDER);

  m_standby_button = new RadarButton(this, ID_STANDBY, g_buttonSize, _("Standby"));
  m_power_sizer->Add(m_standby_button, 0, wxALL, BORDER);

  // The TIMED TRANSMIT button
  timed_idle_times[0] = _("Off");
  timed_idle_times[1] = _("5 min");
  timed_idle_times[2] = _("10 min");
  timed_idle_times[3] = _("15 min");
  timed_idle_times[4] = _("20 min");
  timed_idle_times[5] = _("25 min");
  timed_idle_times[6] = _("30 min");
  timed_idle_times[7] = _("35 min");

  m_timed_idle_button = new RadarControlButton(this, ID_TIMED_IDLE, g_buttonSize, _("Timed Transmit"), CT_TIMED_IDLE, false,
                                               m_pi->m_settings.timed_idle);
  m_power_sizer->Add(m_timed_idle_button, 0, wxALL, BORDER);
  m_timed_idle_button->minValue = 0;
  m_timed_idle_button->maxValue = ARRAY_SIZE(timed_idle_times) - 1;
  m_timed_idle_button->names = timed_idle_times;
  m_timed_idle_button->SetLocalValue(m_pi->m_settings.timed_idle);

  // The TIMED RUNTIME button
  timed_run_times[0] = _("10 s");
  timed_run_times[1] = _("20 s");
  timed_run_times[2] = _("30 s");

  m_timed_run_button = new RadarControlButton(this, ID_TIMED_RUN, g_buttonSize, _("Runtime"), CT_TIMED_RUN, false, 0);
  m_power_sizer->Add(m_timed_run_button, 0, wxALL, BORDER);
  m_timed_run_button->minValue = 0;
  m_timed_run_button->maxValue = ARRAY_SIZE(timed_run_times) - 1;
  m_timed_run_button->names = timed_run_times;
  m_timed_run_button->SetLocalValue(m_pi->m_settings.idle_run_time);

  // The INFO button
  RadarButton* bMessage = new RadarButton(this, ID_MESSAGE, g_buttonSize, _("Info"));
  m_power_sizer->Add(bMessage, 0, wxALL, BORDER);

  m_top_sizer->Hide(m_power_sizer);

  //**************** CONTROL BOX ******************//
  // These are the controls that the users sees when the dialog is started

  // A box sizer to contain RANGE, GAIN etc button
  m_control_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_control_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  //***************** TRANSMIT SIZER, items hidden when not transmitting ****************//

  m_transmit_sizer = new wxBoxSizer(wxVERTICAL);
  m_control_sizer->Add(m_transmit_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The ADJUST button
  m_adjust_button = new RadarButton(this, ID_ADJUST, g_buttonSize, _("Adjust"));
  m_transmit_sizer->Add(m_adjust_button, 0, wxALL, BORDER);

  // The ADVANCED button
  RadarButton* bAdvanced = new RadarButton(this, ID_ADVANCED, g_buttonSize, _("Advanced"));
  m_transmit_sizer->Add(bAdvanced, 0, wxALL, BORDER);

  // The VIEW menu
  RadarButton* bView = new RadarButton(this, ID_VIEW, g_buttonSize, _("View"));
  m_transmit_sizer->Add(bView, 0, wxALL, BORDER);

  // The BEARING button
  m_bearing_button = new RadarButton(this, ID_BEARING, g_buttonSize, _("Cursor"));
  m_transmit_sizer->Add(m_bearing_button, 0, wxALL, BORDER);

  // The GUARD ZONE 1 button
  m_guard_1_button = new RadarButton(this, ID_ZONE1, g_buttonSize, wxT(""));
  m_transmit_sizer->Add(m_guard_1_button, 0, wxALL, BORDER);

  // The GUARD ZONE 2 button
  m_guard_2_button = new RadarButton(this, ID_ZONE2, g_buttonSize, wxT(""));
  m_transmit_sizer->Add(m_guard_2_button, 0, wxALL, BORDER);

  ///// Following three are shown even when radar is not transmitting

  // The SHOW / HIDE RADAR button
  m_window_button = new RadarButton(this, ID_SHOW_RADAR, g_buttonSize, wxT(""));
  m_control_sizer->Add(m_window_button, 0, wxALL, BORDER);

  // The RADAR ONLY / OVERLAY button
  wxString overlay = _("Overlay");
  m_overlay_button = new RadarButton(this, ID_RADAR_OVERLAY, g_buttonSize, overlay);
  m_control_sizer->Add(m_overlay_button, 0, wxALL, BORDER);

  // The Transmit button
  m_power_button = new RadarButton(this, ID_POWER, g_buttonSize, _("Unknown"));
  m_control_sizer->Add(m_power_button, 0, wxALL, BORDER);
  // Updated when we receive data

  m_from_sizer = m_control_sizer;
  m_control_sizer->Hide(m_transmit_sizer);
  m_top_sizer->Hide(m_control_sizer);

  UpdateGuardZoneState();

  DimeWindow(this);  // Call OpenCPN to change colours depending on day/night mode
  Layout();
  Fit();
  // wxSize size_min = GetBestSize();
  // SetMinSize(size_min);
  // SetSize(size_min);
}

void NavicoControlsDialog::UpdateRadarSpecificState() {
  if (m_top_sizer->IsShown(m_advanced_sizer)) {
    if (m_ri->m_radar_type == RT_4G) {
      m_advanced_sizer->Show(m_advanced_4G_sizer);
    } else {
      m_advanced_sizer->Hide(m_advanced_4G_sizer);
    }
  }
}

#if 0
void NavicoControlsDialog::SetTimedIdleIndex(int index) {
  m_timed_idle_button->SetValue(index);  // set and recompute the Timed Idle label
}
#endif

void NavicoControlsDialog::OnZone1ButtonClick(wxCommandEvent& event) { ShowGuardZone(0); }

void NavicoControlsDialog::OnZone2ButtonClick(wxCommandEvent& event) { ShowGuardZone(1); }

void NavicoControlsDialog::OnClose(wxCloseEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void NavicoControlsDialog::OnIdOKClick(wxCommandEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void NavicoControlsDialog::OnPlusTenClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnPlustTenClick for %s value %d"), m_log_name.c_str(), m_from_control->GetLabel().c_str(),
             m_from_control->value + 10);
  m_from_control->AdjustValue(+10);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void NavicoControlsDialog::OnPlusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(+1);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void NavicoControlsDialog::OnBackClick(wxCommandEvent& event) {
  if (m_top_sizer->IsShown(m_edit_sizer)) {
    m_top_sizer->Hide(m_edit_sizer);
    SwitchTo(m_from_sizer, wxT("from (back click)"));
    m_from_control = 0;
  } else if (m_top_sizer->IsShown(m_installation_sizer)) {
    SwitchTo(m_advanced_sizer, wxT("advanced (back click)"));
  } else {
    SwitchTo(m_control_sizer, wxT("main (back click)"));
  }
}

void NavicoControlsDialog::OnAutoClick(wxCommandEvent& event) {
  if (m_from_control->autoValues == 1) {
    m_from_control->SetAuto(1);
    m_auto_button->Disable();
  } else if (m_from_control->autoValue < m_from_control->autoValues) {
    m_from_control->SetAuto(m_from_control->autoValue + 1);
  } else {
    m_from_control->SetAuto(0);
  }
}

void NavicoControlsDialog::OnTrailsMotionClick(wxCommandEvent& event) {
  int value = m_ri->m_trails_motion.GetValue();

  value++;
  if (value > TARGET_MOTION_TRUE) {
    value = 0;
  }
  m_ri->m_trails_motion.Update(value);
  m_ri->ComputeColourMap();
  m_ri->ComputeTargetTrails();
  UpdateTrailsState();
  Fit();
  UpdateControlValues(false);
}

void NavicoControlsDialog::OnMinusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-1);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void NavicoControlsDialog::OnMinusTenClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-10);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void NavicoControlsDialog::OnAdjustButtonClick(wxCommandEvent& event) { SwitchTo(m_adjust_sizer, wxT("adjust")); }

void NavicoControlsDialog::OnAdvancedButtonClick(wxCommandEvent& event) { SwitchTo(m_advanced_sizer, wxT("advanced")); }

void NavicoControlsDialog::OnViewButtonClick(wxCommandEvent& event) { SwitchTo(m_view_sizer, wxT("view")); }

void NavicoControlsDialog::OnInstallationButtonClick(wxCommandEvent& event) { SwitchTo(m_installation_sizer, wxT("installation")); }

void NavicoControlsDialog::OnPowerButtonClick(wxCommandEvent& event) { SwitchTo(m_power_sizer, wxT("power")); }

void NavicoControlsDialog::OnPreferencesButtonClick(wxCommandEvent& event) { m_pi->ShowPreferencesDialog(m_pi->m_parent_window); }

void NavicoControlsDialog::OnBearingButtonClick(wxCommandEvent& event) { SwitchTo(m_cursor_sizer, wxT("bearing")); }

void NavicoControlsDialog::OnMessageButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  if (m_pi->m_pMessageBox) {
    m_pi->m_pMessageBox->UpdateMessage(true);
  }
}

void NavicoControlsDialog::OnTargetsButtonClick(wxCommandEvent& event) {
  M_SETTINGS.show_radar_target[m_ri->m_radar] = !(M_SETTINGS.show_radar_target[m_ri->m_radar]);

  UpdateControlValues(false);
}

void NavicoControlsDialog::EnterEditMode(RadarControlButton* button) {
  m_from_control = button;  // Keep a record of which button was clicked
  m_value_text->SetLabel(button->GetLabel());

  SwitchTo(m_edit_sizer, wxT("edit"));

  if (button->comment.length() > 0) {
    m_comment_text->SetLabel(button->comment);
    m_comment_text->Show();
  } else {
    m_comment_text->Hide();
  }

  if (m_from_control->autoValues > 0) {
    m_auto_button->Show();
    if (m_from_control->autoValue != 0 && m_from_control->autoValues == 1) {
      m_auto_button->Disable();
    } else {
      m_auto_button->Enable();
    }
  } else {
    m_auto_button->Hide();
  }
  if (m_from_control->maxValue > 20) {
    m_plus_ten_button->Show();
    m_minus_ten_button->Show();
  } else {
    m_plus_ten_button->Hide();
    m_minus_ten_button->Hide();
  }
  m_edit_sizer->Layout();
}

void NavicoControlsDialog::OnRadarControlButtonClick(wxCommandEvent& event) {
  EnterEditMode((RadarControlButton*)event.GetEventObject());
}

void NavicoControlsDialog::OnRadarShowButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  bool show = true;

  if (m_pi->m_settings.enable_dual_radar) {
    if (m_pi->m_settings.show_radar[m_ri->m_radar]) {
      int show_other_radar = m_pi->m_settings.show_radar[1 - m_ri->m_radar];
      if (show_other_radar) {
        // Hide both windows
        show = false;
      }
    }
    for (int r = 0; r < RADARS; r++) {
      m_pi->m_settings.show_radar[r] = show;
      if (!show && m_pi->m_settings.chart_overlay != r) {
        m_pi->m_settings.show_radar_control[r] = false;
      }
      LOG_DIALOG(wxT("%s OnRadarShowButton: show_radar[%d]=%d"), m_log_name.c_str(), r, show);
    }
  } else {
    if (m_ri->IsPaneShown()) {
      show = false;
    }
    m_pi->m_settings.show_radar[0] = show;
    LOG_DIALOG(wxT("%s OnRadarShowButton: show_radar[%d]=%d"), m_log_name.c_str(), 0, show);
  }

  m_pi->NotifyRadarWindowViz();
}

void NavicoControlsDialog::OnRadarOverlayButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  int this_radar = m_ri->m_radar;
  int other_radar = 1 - this_radar;

  if (m_pi->m_settings.chart_overlay != this_radar) {
    m_pi->m_settings.chart_overlay = this_radar;
  } else if (m_pi->m_settings.enable_dual_radar && ALL_RADARS(m_pi->m_settings.show_radar, 0)) {
    // If no radar window shown, toggle overlay to different radar
    m_pi->m_settings.chart_overlay = other_radar;

    wxPoint pos = m_pi->m_radar[this_radar]->m_control_dialog->GetPosition();

    // Flip which control is visible.
    m_pi->ShowRadarControl(this_radar, false);

    if (!m_pi->m_radar[other_radar]->m_control_dialog || !m_pi->m_radar[other_radar]->m_control_dialog->IsShown()) {
      m_pi->ShowRadarControl(other_radar, true);
      m_pi->m_radar[other_radar]->m_control_dialog->SetPosition(pos);
    }
  } else {
    // If a radar window is shown, switch overlay off
    m_pi->m_settings.chart_overlay = -1;
  }
  m_ri->m_overlay.Update(m_pi->m_settings.chart_overlay == this_radar);
  UpdateControlValues(true);
}

void NavicoControlsDialog::OnRadarGainButtonClick(wxCommandEvent& event) {
  EnterEditMode((RadarControlButton*)event.GetEventObject());
}

void NavicoControlsDialog::OnTransmitButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();
  m_pi->m_settings.timed_idle = 0;
  m_ri->RequestRadarState(RADAR_TRANSMIT);
}

void NavicoControlsDialog::OnStandbyButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();
  m_pi->m_settings.timed_idle = 0;
  m_ri->RequestRadarState(RADAR_STANDBY);
}

void NavicoControlsDialog::OnClearTrailsButtonClick(wxCommandEvent& event) { m_ri->ClearTrails(); }

void NavicoControlsDialog::OnOrientationButtonClick(wxCommandEvent& event) {
  int value = m_ri->m_orientation.GetValue() + 1;

  if (m_pi->GetHeadingSource() == HEADING_NONE) {
    value = ORIENTATION_HEAD_UP;
  } else {  // There is a heading
    if (value == ORIENTATION_NUMBER) {
      if (M_SETTINGS.developer_mode) {
        value = ORIENTATION_HEAD_UP;
      } else {
        value = ORIENTATION_STABILIZED_UP;
      }
    }
  }

  m_ri->m_orientation.Update(value);
  UpdateControlValues(false);
}

void NavicoControlsDialog::OnBearingSetButtonClick(wxCommandEvent& event) {
  int bearing = event.GetId() - ID_BEARING_SET;
  LOG_DIALOG(wxT("%s OnBearingSetButtonClick for bearing #%d"), m_log_name.c_str(), bearing + 1);

  m_ri->SetBearing(bearing);
}

void NavicoControlsDialog::OnClearCursorButtonClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnClearCursorButtonClick"), m_log_name.c_str());
  m_ri->SetMouseVrmEbl(0., nanl(""));
  SwitchTo(m_control_sizer, wxT("main (clear cursor)"));
}

void NavicoControlsDialog::OnAcquireTargetButtonClick(wxCommandEvent& event) {
  Position target_pos;
  target_pos.lat = m_ri->m_mouse_lat;
  target_pos.lon = m_ri->m_mouse_lon;
  LOG_DIALOG(wxT("%s OnAcquireTargetButtonClick mouse=%f/%f"), m_log_name.c_str(), target_pos.lat, target_pos.lon);
  m_ri->m_arpa->AcquireNewMARPATarget(target_pos);
}

void NavicoControlsDialog::OnDeleteTargetButtonClick(wxCommandEvent& event) {
  Position target_pos;
  target_pos.lat = m_ri->m_mouse_lat;
  target_pos.lon = m_ri->m_mouse_lon;
  LOG_DIALOG(wxT("%s OnDeleteTargetButtonClick mouse=%f/%f"), m_log_name.c_str(), target_pos.lat, target_pos.lon);
  m_ri->m_arpa->DeleteTarget(target_pos);
}

void NavicoControlsDialog::OnDeleteAllTargetsButtonClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnDeleteAllTargetsButtonClick"), m_log_name.c_str());
  for (int i = 0; i < RADARS; i++) {
    if (m_pi->m_radar[i]->m_arpa) {
      m_pi->m_radar[i]->m_arpa->DeleteAllTargets();
    }
  }
}

void NavicoControlsDialog::OnMove(wxMoveEvent& event) {
  m_manually_positioned = true;
  event.Skip();
}

void NavicoControlsDialog::UpdateControlValues(bool refreshAll) {
  wxString o;

  if (m_ri->m_state.IsModified()) {
    refreshAll = true;
  }

  RadarState state = (RadarState)m_ri->m_state.GetButton();

  if (state == RADAR_TRANSMIT) {
    m_standby_button->Enable();
    m_transmit_button->Disable();
  } else {
    m_standby_button->Disable();
    m_transmit_button->Enable();
  }
  o = _("Power Status");
  o << wxT("\n");
  if (m_pi->m_settings.timed_idle == 0) {
    switch (state) {
      case RADAR_OFF:
        o << _("Off");
        break;
      case RADAR_STANDBY:
        o << _("Standby");
        break;
      case RADAR_WAKING_UP:
        o << _("Waking up");
        break;
      case RADAR_TRANSMIT:
        o << _("Transmit");
        break;
    }
    m_timed_idle_button->SetLocalValue(0);
  } else {
    o << m_pi->GetTimedIdleText();
  }
  m_power_button->SetLabel(o);
  m_power_text->SetLabel(o);

  if (m_top_sizer->IsShown(m_power_sizer)) {
    if (m_pi->m_settings.timed_idle) {
      m_power_sizer->Show(m_timed_run_button);
    } else {
      m_power_sizer->Hide(m_timed_run_button);
    }
  }
  if (state == RADAR_TRANSMIT) {
    if (m_top_sizer->IsShown(m_control_sizer)) {
      m_control_sizer->Show(m_transmit_sizer);
      m_control_sizer->Layout();
    }
  } else {
    m_control_sizer->Hide(m_transmit_sizer);
    m_control_sizer->Layout();
  }
  m_top_sizer->Layout();
  Layout();
  Fit();

  if (m_pi->m_settings.enable_dual_radar) {
    int show_other_radar = m_pi->m_settings.show_radar[1 - m_ri->m_radar];
    if (m_pi->m_settings.show_radar[m_ri->m_radar]) {
      if (show_other_radar) {
        o = _("Hide both windows");
      } else {
        o = _("Show other window");
      }
    } else {
      if (show_other_radar) {
        o = _("Show this window");  // can happen if this window hidden but control is for overlay
      } else {
        o = _("Show both windows");
      }
    }
  } else {
    o = (m_pi->m_settings.show_radar[m_ri->m_radar]) ? _("Hide window") : _("Show window");
  }
  m_window_button->SetLabel(o);

  for (int b = 0; b < BEARING_LINES; b++) {
    if (!isnan(m_ri->m_vrm[b])) {
      o = _("Clear EBL/VRM");
      o << wxString::Format(wxT("%d"), b + 1);
    } else {
      o = _("Place EBL/VRM");
      o << wxString::Format(wxT("%d"), b + 1);
    }
    m_bearing_buttons[b]->SetLabel(o);
  }

  if (M_SETTINGS.show_radar_target[m_ri->m_radar]) {
    m_targets_button->SetLabel(_("Hide AIS/ARPA"));
  } else {
    m_targets_button->SetLabel(_("Show AIS/ARPA"));
  }

  if (m_ri->m_target_trails.IsModified() || refreshAll) {
    m_target_trails_button->SetLocalValue(m_ri->m_target_trails.GetButton());
  }

  if (m_ri->m_trails_motion.IsModified() || refreshAll) {
    int trails_motion = m_ri->m_trails_motion.GetButton();
    o = _("Off/Relative/True trails");
    o << wxT("\n");
    if (trails_motion == TARGET_MOTION_TRUE) {
      o << _("True");
    } else if (trails_motion == TARGET_MOTION_RELATIVE) {
      o << _("Relative");
    } else {
      o << _("Off");
    }
    m_trails_motion_button->SetLabel(o);
  }

  if (m_ri->m_orientation.IsModified() || refreshAll) {
    int orientation = m_ri->m_orientation.GetButton();

    o = _("Orientation");
    o << wxT("\n");
    switch (orientation) {
      case ORIENTATION_HEAD_UP:
        o << _("Head up");
        break;
      case ORIENTATION_STABILIZED_UP:
        o << _("Head up");
        o << wxT(" ");
        o << _("(Stabilized)");
        break;
      case ORIENTATION_NORTH_UP:
        o << _("North up");
        break;
      case ORIENTATION_COG_UP:
        o << _("Course up");
        break;
      default:
        o << _("Unknown");
    }
    m_orientation_button->SetLabel(o);
  }
  LOG_DIALOG(wxT("radar_pi: orientation=%d heading source=%d"), m_ri->GetOrientation(), m_pi->GetHeadingSource());
  if (m_pi->GetHeadingSource() == HEADING_NONE) {
    m_orientation_button->Disable();
  } else {
    m_orientation_button->Enable();
  }

  int overlay;
  if (m_ri->m_overlay.GetButton(&overlay) || ((m_pi->m_settings.chart_overlay == m_ri->m_radar) != (overlay != 0)) || refreshAll) {
    o = _("Overlay");
    o << wxT("\n");
    if (m_pi->m_settings.enable_dual_radar && ALL_RADARS(m_pi->m_settings.show_radar, 0)) {
      o << ((overlay > 0) ? m_ri->m_name : _("Off"));
    } else {
      o << ((overlay > 0) ? _("On") : _("Off"));
    }
    m_overlay_button->SetLabel(o);
  }

  if (m_ri->m_range.IsModified() || refreshAll) {
    m_ri->m_range.GetButton();
    m_range_button->SetRangeLabel();
  }

  // gain
  if (m_ri->m_gain.IsModified() || refreshAll) {
    int button = m_ri->m_gain.GetButton();
    m_gain_button->SetLocalValue(button);
  }

  //  rain
  if (m_ri->m_rain.IsModified() || refreshAll) {
    m_rain_button->SetLocalValue(m_ri->m_rain.GetButton());
  }

  //   sea
  if (m_ri->m_sea.IsModified() || refreshAll) {
    int button = m_ri->m_sea.GetButton();
    m_sea_button->SetLocalValue(button);
  }

  //   target_boost
  if (m_ri->m_target_boost.IsModified() || refreshAll) {
    m_target_boost_button->SetLocalValue(m_ri->m_target_boost.GetButton());
  }

  //   target_expansion
  if (m_ri->m_target_expansion.IsModified() || refreshAll) {
    m_target_expansion_button->SetLocalValue(m_ri->m_target_expansion.GetButton());
  }

  //  noise_rejection
  if (m_ri->m_noise_rejection.IsModified() || refreshAll) {
    m_noise_rejection_button->SetLocalValue(m_ri->m_noise_rejection.GetButton());
  }

  //  target_separation
  if (m_ri->m_target_separation.IsModified() || refreshAll) {
    m_target_separation_button->SetLocalValue(m_ri->m_target_separation.GetButton());
  }

  //  interference_rejection
  if (m_ri->m_interference_rejection.IsModified() || refreshAll) {
    m_interference_rejection_button->SetLocalValue(m_ri->m_interference_rejection.GetButton());
  }

  // scanspeed
  if (m_ri->m_scan_speed.IsModified() || refreshAll) {
    m_scan_speed_button->SetLocalValue(m_ri->m_scan_speed.GetButton());
  }

  //   antenna height
  if (m_ri->m_antenna_height.IsModified() || refreshAll) {
    m_antenna_height_button->SetLocalValue(m_ri->m_antenna_height.GetButton());
  }

  //  bearing alignment
  if (m_ri->m_bearing_alignment.IsModified() || refreshAll) {
    m_bearing_alignment_button->SetLocalValue(m_ri->m_bearing_alignment.GetButton());
  }

  //  local interference rejection
  if (m_ri->m_local_interference_rejection.IsModified() || refreshAll) {
    m_local_interference_rejection_button->SetLocalValue(m_ri->m_local_interference_rejection.GetButton());
  }

  // side lobe suppression
  if (m_ri->m_side_lobe_suppression.IsModified() || refreshAll) {
    int button = m_ri->m_side_lobe_suppression.GetButton();
    m_side_lobe_suppression_button->SetLocalValue(button);
  }

  if (refreshAll) {
    // Update all buttons set from plugin settings
    m_transparency_button->SetLocalValue(M_SETTINGS.overlay_transparency);
    m_timed_idle_button->SetLocalValue(M_SETTINGS.timed_idle);
    m_timed_run_button->SetLocalValue(M_SETTINGS.idle_run_time);
    m_refresh_rate_button->SetLocalValue(M_SETTINGS.refreshrate);
    m_main_bang_size_button->SetLocalValue(M_SETTINGS.main_bang_size);
    m_antenna_starboard_button->SetLocalValue(M_SETTINGS.antenna_starboard);
    m_antenna_forward_button->SetLocalValue(M_SETTINGS.antenna_forward);
  }

  int arpa_targets = m_ri->m_arpa->GetTargetCount();
  if (arpa_targets) {
    m_delete_target->Enable();
    m_delete_all->Enable();
  } else {
    m_delete_target->Disable();
    m_delete_all->Disable();
  }

  // Update the text that is currently shown in the edit box, this is a copy of the button itself
  if (m_from_control) {
    wxString label = m_from_control->GetLabel();
    m_value_text->SetLabel(label);
  }
}

void NavicoControlsDialog::UpdateDialogShown() {
  if (m_hide) {
    if (IsShown()) {
      LOG_DIALOG(wxT("%s UpdateDialogShown explicit closed: Hidden"), m_log_name.c_str());
      Hide();
    }
    return;
  }

  if (m_hide_temporarily) {
    if (IsShown()) {
      LOG_DIALOG(wxT("%s UpdateDialogShown temporarily hidden"), m_log_name.c_str());
      Hide();
    }
    return;
  }

  if (m_top_sizer->IsShown(m_control_sizer)) {
    if (m_auto_hide_timeout && TIMED_OUT(time(0), m_auto_hide_timeout)) {
      if (IsShown()) {
        LOG_DIALOG(wxT("%s UpdateDialogShown auto-hide"), m_log_name.c_str());
        Hide();
      }
      return;
    }
  } else {
    // If we're somewhere in the sub-window, don't close the dialog
    m_auto_hide_timeout = 0;
  }

  // Following helps on OSX where the control is SHOW_ON_TOP to not show when no part of OCPN is focused
  wxWindow* focused = FindFocus();
  if (!focused) {
    LOG_DIALOG(wxT("%s UpdateDialogShown app not focused"), m_log_name.c_str());
    return;
  }

  if (!IsShown()) {
    LOG_DIALOG(wxT("%s UpdateDialogShown manually opened"), m_log_name.c_str());
    if (!m_top_sizer->IsShown(m_control_sizer) && !m_top_sizer->IsShown(m_advanced_sizer) && !m_top_sizer->IsShown(m_view_sizer) &&
        !m_top_sizer->IsShown(m_edit_sizer) && !m_top_sizer->IsShown(m_installation_sizer) &&
        !m_top_sizer->IsShown(m_guard_sizer) && !m_top_sizer->IsShown(m_adjust_sizer) && !m_top_sizer->IsShown(m_cursor_sizer) &&
        !m_top_sizer->IsShown(m_power_sizer)) {
      SwitchTo(m_control_sizer, wxT("main (manual open)"));
    }
    m_control_sizer->Layout();
    m_top_sizer->Layout();
    Show();
    Raise();

    m_edit_sizer->Layout();
    m_top_sizer->Layout();

    // If the corresponding radar panel is now in a different position from what we remembered
    // then reset the dialog to the left or right of the radar panel.
    wxPoint panelPos = m_ri->m_radar_panel->GetPos();
    bool controlInitialShow = m_pi->m_settings.control_pos[m_ri->m_radar] == OFFSCREEN_CONTROL;
    bool panelShown = m_ri->m_radar_panel->IsShown();
    bool panelMoved = m_panel_position.IsFullySpecified() && panelPos != m_panel_position;

    if (panelShown                                  // if the radar pane is shown and
        && ((panelMoved && !m_manually_positioned)  // has moved this session and user did not touch pos, or
            || controlInitialShow)) {               // the position has never been set at all, ever
      wxSize panelSize = m_ri->m_radar_panel->GetSize();
      wxSize mySize = this->GetSize();

      wxPoint newPos;
      newPos.x = panelPos.x + panelSize.x - mySize.x;
      newPos.y = panelPos.y;
      SetPosition(newPos);
      LOG_DIALOG(wxT("%s show control menu over menu button"), m_log_name.c_str());
    } else if (controlInitialShow) {  // When all else fails set it to default position
      SetPosition(wxPoint(100 + m_ri->m_radar * 100, 100));
      LOG_DIALOG(wxT("%s show control menu at initial location"), m_log_name.c_str());
    }
    EnsureWindowNearOpenCPNWindow();  // If the position is really weird, move it
    m_pi->m_settings.control_pos[m_ri->m_radar] = GetPosition();
    m_pi->m_settings.show_radar_control[m_ri->m_radar] = true;
    m_panel_position = panelPos;
  }
  if (m_top_sizer->IsShown(m_control_sizer)) {
    Fit();
  }
}

void NavicoControlsDialog::HideTemporarily() {
  m_hide_temporarily = true;
  UpdateDialogShown();
}

void NavicoControlsDialog::UnHideTemporarily() {
  m_hide_temporarily = false;
  SetMenuAutoHideTimeout();
  UpdateDialogShown();
}

void NavicoControlsDialog::ShowDialog() {
  m_hide = false;
  UnHideTemporarily();
  UpdateControlValues(true);
}

void NavicoControlsDialog::HideDialog() {
  m_hide = true;
  m_auto_hide_timeout = 0;
  UpdateDialogShown();
}

void NavicoControlsDialog::OnGuardZoneModeClick(wxCommandEvent& event) { SetGuardZoneVisibility(); }

void NavicoControlsDialog::OnInner_Range_Value(wxCommandEvent& event) {
  wxString temp = m_inner_range->GetValue();
  double t;
  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->SetInnerRange((int)(t * conversionFactor));
}

void NavicoControlsDialog::OnOuter_Range_Value(wxCommandEvent& event) {
  wxString temp = m_outer_range->GetValue();
  double t;
  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->SetOuterRange((int)(t * conversionFactor));
}

void NavicoControlsDialog::OnStart_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_start_bearing->GetValue();
  double t;

  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);
  t = fmod(t, 360.);
  if (t < 0.) {
    t += 360.;
  }
  m_guard_zone->SetStartBearing(SCALE_DEGREES_TO_RAW2048(t));
}

void NavicoControlsDialog::OnEnd_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_end_bearing->GetValue();
  double t;

  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);
  t = fmod(t, 360.);
  if (t < 0.) {
    t += 360.;
  }
  m_guard_zone->SetEndBearing(SCALE_DEGREES_TO_RAW2048(t));
}

void NavicoControlsDialog::OnARPAClick(wxCommandEvent& event) {
  int arpa = m_arpa_box->GetValue();
  m_guard_zone->SetArpaOn(arpa);
}

void NavicoControlsDialog::OnAlarmClick(wxCommandEvent& event) {
  int alarm = m_alarm->GetValue();
  m_guard_zone->SetAlarmOn(alarm);
}

PLUGIN_END_NAMESPACE
