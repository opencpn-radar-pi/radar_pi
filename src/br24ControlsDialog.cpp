/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico br24 Radar Plugin
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

#include "br24ControlsDialog.h"
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
  ID_MULTISWEEP,

  ID_TRANSPARENCY,
  ID_INTERFERENCE_REJECTION,
  ID_TARGET_BOOST,
  ID_TARGET_EXPANSION,
  ID_NOISE_REJECTION,
  ID_TARGET_SEPARATION,
  ID_REFRESHRATE,
  ID_SCAN_SPEED,
  ID_INSTALLATION,
  ID_TIMED_IDLE,

  ID_BEARING_ALIGNMENT,
  ID_ANTENNA_HEIGHT,
  ID_LOCAL_INTERFERENCE_REJECTION,
  ID_SIDE_LOBE_SUPPRESSION,

  ID_RANGE,
  ID_GAIN,
  ID_SEA,
  ID_RAIN,

  ID_CLEAR_CURSOR,

  ID_TARGET_TRAILS,
  ID_CLEAR_TRAILS,
  ID_ORIENTATION,

  ID_RADAR_STATE,
  ID_SHOW_RADAR,
  ID_RADAR_OVERLAY,
  ID_ADJUST,
  ID_ADVANCED,
  ID_VIEW,
  ID_BEARING,
  ID_ZONE1,
  ID_ZONE2,

  ID_CONFIRM_BOGEY,

  ID_MESSAGE,
  ID_BPOS,
  ID_HEADING,
  ID_RADAR,
  ID_DATA,

  ID_BEARING_SET = 1000,  // next one should be BEARING_LINES higher

};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(br24ControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(br24ControlsDialog, wxDialog)

EVT_CLOSE(br24ControlsDialog::OnClose)
EVT_BUTTON(ID_BACK, br24ControlsDialog::OnBackClick)
EVT_BUTTON(ID_PLUS_TEN, br24ControlsDialog::OnPlusTenClick)
EVT_BUTTON(ID_PLUS, br24ControlsDialog::OnPlusClick)
EVT_BUTTON(ID_MINUS, br24ControlsDialog::OnMinusClick)
EVT_BUTTON(ID_MINUS_TEN, br24ControlsDialog::OnMinusTenClick)
EVT_BUTTON(ID_AUTO, br24ControlsDialog::OnAutoClick)
EVT_BUTTON(ID_MULTISWEEP, br24ControlsDialog::OnMultiSweepClick)

EVT_BUTTON(ID_TRANSPARENCY, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INTERFERENCE_REJECTION, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_BOOST, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_EXPANSION, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_NOISE_REJECTION, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_SEPARATION, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_REFRESHRATE, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SCAN_SPEED, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INSTALLATION, br24ControlsDialog::OnInstallationButtonClick)
EVT_BUTTON(ID_TIMED_IDLE, br24ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_BEARING_ALIGNMENT, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_HEIGHT, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_LOCAL_INTERFERENCE_REJECTION, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SIDE_LOBE_SUPPRESSION, br24ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_RADAR_STATE, br24ControlsDialog::OnRadarStateButtonClick)
EVT_BUTTON(ID_SHOW_RADAR, br24ControlsDialog::OnRadarShowButtonClick)
EVT_BUTTON(ID_RADAR_OVERLAY, br24ControlsDialog::OnRadarOverlayButtonClick)
EVT_BUTTON(ID_RANGE, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_GAIN, br24ControlsDialog::OnRadarGainButtonClick)
EVT_BUTTON(ID_SEA, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_RAIN, br24ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_TARGET_TRAILS, br24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_CLEAR_TRAILS, br24ControlsDialog::OnClearTrailsButtonClick)
EVT_BUTTON(ID_ORIENTATION, br24ControlsDialog::OnOrientationButtonClick)

EVT_BUTTON(ID_ADJUST, br24ControlsDialog::OnAdjustButtonClick)
EVT_BUTTON(ID_ADVANCED, br24ControlsDialog::OnAdvancedButtonClick)
EVT_BUTTON(ID_VIEW, br24ControlsDialog::OnViewButtonClick)

EVT_BUTTON(ID_BEARING, br24ControlsDialog::OnBearingButtonClick)
EVT_BUTTON(ID_ZONE1, br24ControlsDialog::OnZone1ButtonClick)
EVT_BUTTON(ID_ZONE2, br24ControlsDialog::OnZone2ButtonClick)

EVT_BUTTON(ID_MESSAGE, br24ControlsDialog::OnMessageButtonClick)
EVT_BUTTON(ID_CONFIRM_BOGEY, br24ControlsDialog::OnConfirmBogeyButtonClick)

EVT_BUTTON(ID_BEARING_SET, br24ControlsDialog::OnBearingSetButtonClick)
EVT_BUTTON(ID_CLEAR_CURSOR, br24ControlsDialog::OnClearCursorButtonClick)

EVT_MOVE(br24ControlsDialog::OnMove)
EVT_SIZE(br24ControlsDialog::OnSize)
EVT_CLOSE(br24ControlsDialog::OnClose)

END_EVENT_TABLE()

static wxSize g_buttonSize;
static wxSize g_smallButtonSize;

class br24RadarControlButton : public wxButton {
 public:
  br24RadarControlButton(){

  };

  br24RadarControlButton(br24ControlsDialog* parent, wxWindowID id, const wxString& label, ControlType ct, bool newHasAuto,
                         int newValue) {
    Create(parent, id, label + wxT("\n"), wxDefaultPosition, g_buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;
    minValue = 0;
    maxValue = 100;
    value = 0;
    if (ct == CT_GAIN) {
      value = 50;
    }
    hasAuto = newHasAuto;
    firstLine = label;
    names = 0;
    controlType = ct;
    if (hasAuto) {
      SetLocalAuto();
    } else {
      SetLocalValue(newValue);
    }

    this->SetFont(m_parent->m_pi->m_font);
  }

  virtual void AdjustValue(int adjustment);
  virtual void SetAuto();
  virtual void SetLocalValue(int newValue);
  virtual void SetLocalAuto();
  const wxString* names;

  wxString firstLine;

  br24ControlsDialog* m_parent;
  br24radar_pi* m_pi;

  int value;

  int minValue;
  int maxValue;
  bool hasAuto;
  ControlType controlType;
};

class br24RadarRangeControlButton : public br24RadarControlButton {
 public:
  br24RadarRangeControlButton(br24ControlsDialog* parent, RadarInfo* ri, wxWindowID id, const wxString& label) {
    Create(parent, id, label + wxT("\n"), wxDefaultPosition, g_buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;
    m_ri = ri;
    minValue = 0;
    maxValue = 0;
    value = -1;  // means: never set
    hasAuto = true;
    firstLine = label;
    names = 0;
    controlType = CT_RANGE;

    this->SetFont(m_parent->m_pi->m_font);
  }

  virtual void AdjustValue(int adjustment);
  virtual void SetAuto();
  void SetRangeLabel();

 private:
  RadarInfo* m_ri;
};

wxString interference_rejection_names[4];
wxString target_separation_names[4];
wxString noise_rejection_names[3];
wxString target_boost_names[3];
wxString target_expansion_names[2];
wxString scan_speed_names[2];
wxString timed_idle_times[8];
wxString guard_zone_names[3];
wxString target_trail_names[6];

void br24RadarControlButton::AdjustValue(int adjustment) {
  int newValue = value + adjustment;

  if (newValue < minValue) {
    newValue = minValue;
  } else if (newValue > maxValue) {
    newValue = maxValue;
  }
  if (newValue != value) {
    LOG_VERBOSE(wxT("BR24radar_pi: Adjusting %s by %d from %d to %d"), GetName(), adjustment, value, newValue);
    if (m_pi->SetControlValue(m_parent->m_ri->radar, controlType, newValue)) {
      SetLocalValue(newValue);
    }
  }
}

void br24RadarControlButton::SetLocalValue(int newValue) {  // sets value in the button without sending new value to the radar
  if (newValue < minValue) {
    value = minValue;
  } else if (newValue > maxValue) {
    value = maxValue;
  } else {
    value = newValue;
  }

  wxString label;

  if (names) {
    label.Printf(wxT("%s\n%s"), firstLine.c_str(), names[value].c_str());
  } else {
    label.Printf(wxT("%s\n%d"), firstLine.c_str(), value);
  }

  this->SetLabel(label);
}

void br24RadarControlButton::SetAuto() {
  SetLocalAuto();
  m_parent->m_ri->SetControlValue(controlType, -1);
}

void br24RadarControlButton::SetLocalAuto() {  // sets auto in the button without sending new value
                                               // to the radar
  wxString label;

  label << firstLine << wxT("\n") << _("Auto");
  this->SetLabel(label);
}

void br24RadarRangeControlButton::SetRangeLabel() {
  wxString text = m_ri->GetRangeText();
  this->SetLabel(firstLine + wxT("\n") + text);
}

void br24RadarRangeControlButton::AdjustValue(int adjustment) {
  LOG_VERBOSE(wxT("BR24radar_pi: Adjusting %s by %d"), GetName(), adjustment);

  m_parent->m_ri->AdjustRange(adjustment);  // send new value to the radar
}

void br24RadarRangeControlButton::SetAuto() { m_parent->m_ri->auto_range_mode = true; }

br24ControlsDialog::br24ControlsDialog() { Init(); }

br24ControlsDialog::~br24ControlsDialog() {}

void br24ControlsDialog::Init() {
  // Initialize all members that need initialization
}

void br24ControlsDialog::BindLeftDown(wxWindow* component) {
  wxWindowListNode* node;

  if (component) {
    component->Bind(wxEVT_LEFT_DOWN, &br24ControlsDialog::OnMouseLeftDown, this);
    for (node = component->GetChildren().GetFirst(); node; node = node->GetNext()) {
      wxWindow* child = node->GetData();
      BindLeftDown(child);
    }
  }
}

bool br24ControlsDialog::Create(wxWindow* parent, br24radar_pi* ppi, RadarInfo* ri, wxWindowID id, const wxString& caption,
                                const wxPoint& pos) {
  m_parent = parent;
  m_pi = ppi;
  m_ri = ri;

  m_hide = false;
  m_hide_temporarily = true;
  m_panel_position = wxPoint(0, 0);

  m_from_control = 0;

  long wstyle = wxCLOSE_BOX | wxCAPTION | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxCLIP_CHILDREN;
#ifdef __WXMAC__
  wstyle |= wxSTAY_ON_TOP;  // FLOAT_ON_PARENT is broken on Mac, I know this is not optimal
#endif

  if (!wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle)) {
    return false;
  }

  CreateControls();

  BindLeftDown(this);

  return true;
}

void br24ControlsDialog::CreateControls() {
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
  label << _("Transparency") << wxT("\n");
  label << _("Interference rejection") << wxT("\n");
  label << _("Target separation") << wxT("\n");
  label << _("Noise rejection") << wxT("\n");
  label << _("Target boost") << wxT("\n");
  label << _("Target expansion") << wxT("\n");
  label << _("Scan speed") << wxT("\n");
  label << _("Scan age") << wxT("\n");
  label << _("Timed Transmit") << wxT("\n");
  label << _("Gain") << wxT("\n");
  label << _("Sea clutter") << wxT("\n");
  label << _("Rain clutter") << wxT("\n");

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
  int width = m_top_sizer->GetSize().GetWidth() + 10;

  wxSize bestSize = GetBestSize();
  if (width < bestSize.GetWidth()) {
    width = bestSize.GetWidth();
  }
  if (width < 100) {
    width = 100;
  }
  if (width > 300) {
    width = 300;
  }

#define BUTTON_BORDER 4
#ifdef __WXMAC__
#define BUTTON_HEIGTH_FUDGE 1 + BUTTON_BORDER
#else
#define BUTTON_HEIGTH_FUDGE 1 + 2 * BUTTON_BORDER
#endif

  g_smallButtonSize = wxSize(width, testButtonText->GetSize().y + BUTTON_BORDER);
  g_buttonSize = wxSize(width, testButton2Text->GetSize().y * BUTTON_HEIGTH_FUDGE);

  LOG_DIALOG(wxT("BR24radar_pi: Dynamic button width = %d height = %d, %d"), g_buttonSize.x, g_buttonSize.y, g_smallButtonSize.y);

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
  wxButton* back_button = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(back_button, 0, wxALL, BORDER);
  back_button->SetFont(m_pi->m_font);

  // The +10 button
  m_plus_ten_button = new wxButton(this, ID_PLUS_TEN, _("+10"), wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_plus_ten_button, 0, wxALL, BORDER);
  m_plus_ten_button->SetFont(m_pi->m_font);

  // The + button
  m_plus_button = new wxButton(this, ID_PLUS, _("+"), wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_plus_button, 0, wxALL, BORDER);
  m_plus_button->SetFont(m_pi->m_font);

  // The VALUE button
  wxSize valueSize =  wxSize(g_buttonSize.x, g_buttonSize.y + 20);
  m_value_text = new wxStaticText(this, ID_VALUE, _("Value"), wxDefaultPosition, valueSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_edit_sizer->Add(m_value_text, 0, wxALL, BORDER);
  m_value_text->SetFont(m_pi->m_fat_font);
  m_value_text->SetBackgroundColour(*wxLIGHT_GREY);

  // The - button
  m_minus_button = new wxButton(this, ID_MINUS, _("-"), wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_minus_button, 0, wxALL, BORDER);
  m_minus_button->SetFont(m_pi->m_font);

  // The -10 button
  m_minus_ten_button = new wxButton(this, ID_MINUS_TEN, _("-10"), wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_minus_ten_button, 0, wxALL, BORDER);
  m_minus_ten_button->SetFont(m_pi->m_font);

  // The Auto button
  m_auto_button = new wxButton(this, ID_AUTO, _("Auto"), wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_auto_button, 0, wxALL, BORDER);
  m_auto_button->SetFont(m_pi->m_font);

  // The Multi Sweep Filter button
  wxString labelMS;
  labelMS << _("Multi Sweep Filter") << wxT("\n") << _("Off");
  m_multi_sweep_button = new wxButton(this, ID_MULTISWEEP, labelMS, wxDefaultPosition, g_buttonSize, 0);
  m_edit_sizer->Add(m_multi_sweep_button, 0, wxALL, BORDER);
  m_multi_sweep_button->SetFont(m_pi->m_font);

  m_top_sizer->Hide(m_edit_sizer);

  //**************** ADVANCED BOX ******************//
  // These are the controls that the users sees when the Advanced button is selected

  m_advanced_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_advanced_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  wxButton* bAdvancedBack = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_advanced_sizer->Add(bAdvancedBack, 0, wxALL, BORDER);
  bAdvancedBack->SetFont(m_pi->m_font);

  // The NOISE REJECTION button
  noise_rejection_names[0] = _("Off");
  noise_rejection_names[1] = _("Low");
  noise_rejection_names[2] = _("High");

  m_noise_rejection_button = new br24RadarControlButton(this, ID_NOISE_REJECTION, _("Noise rejection"), CT_NOISE_REJECTION, false,
                                                        m_ri->noise_rejection.button);
  m_advanced_sizer->Add(m_noise_rejection_button, 0, wxALL, BORDER);
  m_noise_rejection_button->minValue = 0;
  m_noise_rejection_button->maxValue = ARRAY_SIZE(noise_rejection_names) - 1;
  m_noise_rejection_button->names = noise_rejection_names;
  m_noise_rejection_button->SetLocalValue(m_ri->noise_rejection.button);  // redraw after adding names

  m_advanced_4G_sizer = new wxBoxSizer(wxVERTICAL);
  m_advanced_sizer->Add(m_advanced_4G_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  // The TARGET EXPANSION button
  target_expansion_names[0] = _("Off");
  target_expansion_names[1] = _("On");
  m_target_expansion_button = new br24RadarControlButton(this, ID_TARGET_EXPANSION, _("Target expansion"), CT_TARGET_EXPANSION,
                                                         false, m_ri->target_expansion.button);
  m_advanced_sizer->Add(m_target_expansion_button, 0, wxALL, BORDER);
  m_target_expansion_button->minValue = 0;
  m_target_expansion_button->maxValue = ARRAY_SIZE(target_expansion_names) - 1;
  m_target_expansion_button->names = target_expansion_names;
  m_target_expansion_button->SetLocalValue(m_ri->target_expansion.button);  // redraw after adding names

  // The REJECTION button

  interference_rejection_names[0] = _("Off");
  interference_rejection_names[1] = _("Low");
  interference_rejection_names[2] = _("Medium");
  interference_rejection_names[3] = _("High");

  m_interference_rejection_button =
      new br24RadarControlButton(this, ID_INTERFERENCE_REJECTION, _("Interference rej."), CT_INTERFERENCE_REJECTION, false,
                                 m_ri->interference_rejection.button);
  m_advanced_sizer->Add(m_interference_rejection_button, 0, wxALL, BORDER);
  m_interference_rejection_button->minValue = 0;
  m_interference_rejection_button->maxValue = ARRAY_SIZE(interference_rejection_names) - 1;
  m_interference_rejection_button->names = interference_rejection_names;
  m_interference_rejection_button->SetLocalValue(m_ri->interference_rejection.button);  // redraw after adding names

  // The TARGET SEPARATION button

  target_separation_names[0] = _("Off");
  target_separation_names[1] = _("Low");
  target_separation_names[2] = _("Medium");
  target_separation_names[3] = _("High");

  m_target_separation_button = new br24RadarControlButton(this, ID_TARGET_SEPARATION, _("Target separation"), CT_TARGET_SEPARATION,
                                                          false, m_ri->target_separation.button);
  m_advanced_4G_sizer->Add(m_target_separation_button, 0, wxALL, BORDER);
  m_target_separation_button->minValue = 0;
  m_target_separation_button->maxValue = ARRAY_SIZE(target_separation_names) - 1;
  m_target_separation_button->names = target_separation_names;
  m_target_separation_button->SetLocalValue(m_ri->target_separation.button);  // redraw after adding names

  // The SCAN SPEED button
  scan_speed_names[0] = _("Normal");
  scan_speed_names[1] = _("Fast");
  m_scan_speed_button =
      new br24RadarControlButton(this, ID_SCAN_SPEED, _("Scan speed"), CT_SCAN_SPEED, false, m_ri->scan_speed.button);
  m_advanced_sizer->Add(m_scan_speed_button, 0, wxALL, BORDER);
  m_scan_speed_button->minValue = 0;
  m_scan_speed_button->maxValue = ARRAY_SIZE(scan_speed_names) - 1;
  m_scan_speed_button->names = scan_speed_names;
  m_scan_speed_button->SetLocalValue(m_ri->scan_speed.button);  // redraw after adding names

  // The TARGET BOOST button
  target_boost_names[0] = _("Off");
  target_boost_names[1] = _("Low");
  target_boost_names[2] = _("High");
  m_target_boost_button =
      new br24RadarControlButton(this, ID_TARGET_BOOST, _("Target boost"), CT_TARGET_BOOST, false, m_ri->target_boost.button);
  m_advanced_sizer->Add(m_target_boost_button, 0, wxALL, BORDER);
  m_target_boost_button->minValue = 0;
  m_target_boost_button->maxValue = ARRAY_SIZE(target_boost_names) - 1;
  m_target_boost_button->names = target_boost_names;
  m_target_boost_button->SetLocalValue(m_ri->target_boost.button);  // redraw after adding names

  // The REFRESHRATE button
  m_refresh_rate_button =
      new br24RadarControlButton(this, ID_REFRESHRATE, _("Refresh rate"), CT_REFRESHRATE, false, m_pi->m_settings.refreshrate);
  m_advanced_sizer->Add(m_refresh_rate_button, 0, wxALL, BORDER);
  m_refresh_rate_button->minValue = 1;
  m_refresh_rate_button->maxValue = 5;

  // The INSTALLATION button
  wxButton* bInstallation = new wxButton(this, ID_INSTALLATION, _("Installation"), wxDefaultPosition, g_smallButtonSize, 0);
  m_advanced_sizer->Add(bInstallation, 0, wxALL, BORDER);
  bInstallation->SetFont(m_pi->m_font);

  m_top_sizer->Hide(m_advanced_sizer);

  //**************** Installation BOX ******************//
  // These are the controls that the users sees when the Installation button is selected

  m_installation_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_installation_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  wxButton* bInstallationBack = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_installation_sizer->Add(bInstallationBack, 0, wxALL, BORDER);
  bInstallationBack->SetFont(m_pi->m_font);

  // The TRANSPARENCY button
  m_transparency_button = new br24RadarControlButton(this, ID_TRANSPARENCY, _("Transparency"), CT_TRANSPARENCY, false,
                                                     m_pi->m_settings.overlay_transparency);
  m_installation_sizer->Add(m_transparency_button, 0, wxALL, BORDER);
  m_transparency_button->minValue = MIN_OVERLAY_TRANSPARENCY;
  m_transparency_button->maxValue = MAX_OVERLAY_TRANSPARENCY;

  // The BEARING ALIGNMENT button
  m_bearing_alignment_button = new br24RadarControlButton(this, ID_BEARING_ALIGNMENT, _("Bearing alignment"), CT_BEARING_ALIGNMENT,
                                                          false, m_ri->bearing_alignment.button);
  m_installation_sizer->Add(m_bearing_alignment_button, 0, wxALL, BORDER);
  m_bearing_alignment_button->SetFont(m_pi->m_font);  // this bearing alignment work opposite to the one defined in the pi!
  m_bearing_alignment_button->minValue = -179;
  m_bearing_alignment_button->maxValue = 180;

  // The ANTENNA HEIGHT button
  m_antenna_height_button = new br24RadarControlButton(this, ID_ANTENNA_HEIGHT, _("Antenna height"), CT_ANTENNA_HEIGHT, false,
                                                       m_ri->antenna_height.button);
  m_installation_sizer->Add(m_antenna_height_button, 0, wxALL, BORDER);
  m_antenna_height_button->minValue = 0;
  m_antenna_height_button->maxValue = 30;

  // The LOCAL INTERFERENCE REJECTION button
  m_local_interference_rejection_button =
      new br24RadarControlButton(this, ID_LOCAL_INTERFERENCE_REJECTION, _("Local interference rejection"),
                                 CT_LOCAL_INTERFERENCE_REJECTION, false, m_ri->local_interference_rejection.button);
  m_installation_sizer->Add(m_local_interference_rejection_button, 0, wxALL, BORDER);
  m_local_interference_rejection_button->minValue = 0;
  m_local_interference_rejection_button->maxValue =
      ARRAY_SIZE(target_separation_names) - 1;  // off, low, medium, high, same as target separation
  m_local_interference_rejection_button->names = target_separation_names;
  m_local_interference_rejection_button->SetLocalValue(m_ri->local_interference_rejection.button);

  // The SIDE LOBE SUPPRESSION button
  m_side_lobe_suppression_button = new br24RadarControlButton(this, ID_SIDE_LOBE_SUPPRESSION, _("Side lobe suppression"),
                                                              CT_SIDE_LOBE_SUPPRESSION, true, m_ri->side_lobe_suppression.button);
  m_installation_sizer->Add(m_side_lobe_suppression_button, 0, wxALL, BORDER);
  m_side_lobe_suppression_button->minValue = 0;
  m_side_lobe_suppression_button->maxValue = 100;
  m_side_lobe_suppression_button->SetLocalValue(m_ri->side_lobe_suppression.button);  // redraw after adding names

  m_top_sizer->Hide(m_installation_sizer);

  //***************** GUARD ZONE BOGEY BOX *************//
  m_bogey_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_bogey_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_bogey_text =
      new wxStaticText(this, wxID_ANY, _("Zone 1: unknown\nZone 2: unknown\nTimeout\n"), wxDefaultPosition, wxDefaultSize, 0);
  m_bogey_sizer->Add(m_bogey_text, 0, wxALIGN_LEFT | wxALL, BORDER);

  m_bogey_confirm = new wxButton(this, ID_CONFIRM_BOGEY, _("&Confirm"), wxDefaultPosition, wxDefaultSize, 0);
  m_bogey_sizer->Add(m_bogey_confirm, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);

  m_top_sizer->Hide(m_bogey_sizer);

  //***************** GUARD ZONE EDIT BOX *************//

  m_guard_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_guard_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The <<Back button
  wxButton* guard_back_button = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_guard_sizer->Add(guard_back_button, 0, wxALL, BORDER);
  guard_back_button->SetFont(m_pi->m_font);

  m_guard_zone_text = new wxStaticText(this, wxID_ANY, _("Guard zones"));
  m_guard_sizer->Add(m_guard_zone_text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

  wxStaticText* type_Text = new wxStaticText(this, wxID_ANY, _("Zone type"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(type_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  guard_zone_names[0] = _("Off");
  guard_zone_names[1] = _("Arc");
  guard_zone_names[2] = _("Circle");
  m_guard_zone_type = new wxRadioBox(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, ARRAY_SIZE(guard_zone_names),
                                     guard_zone_names, 1, wxRA_SPECIFY_COLS);
  m_guard_sizer->Add(m_guard_zone_type, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_guard_zone_type->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(br24ControlsDialog::OnGuardZoneModeClick), NULL,
                             this);

  // Inner and Outer Ranges
  wxStaticText* inner_range_Text = new wxStaticText(this, wxID_ANY, _("Inner range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(inner_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_inner_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_inner_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_inner_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(br24ControlsDialog::OnInner_Range_Value), NULL, this);
  ///   start of copy
  wxStaticText* outer_range_Text = new wxStaticText(this, wxID_ANY, _("Outer range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(outer_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_outer_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_outer_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_outer_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(br24ControlsDialog::OnOuter_Range_Value), NULL, this);

  wxStaticText* pStart_Bearing = new wxStaticText(this, wxID_ANY, _("Start bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pStart_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_start_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_start_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_start_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(br24ControlsDialog::OnStart_Bearing_Value), NULL,
                           this);

  wxStaticText* pEnd_Bearing = new wxStaticText(this, wxID_ANY, _("End bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pEnd_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_end_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_end_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_end_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(br24ControlsDialog::OnEnd_Bearing_Value), NULL, this);

  // added check box to control multi swep filtering
  m_filter = new wxCheckBox(this, wxID_ANY, _("Sweep Filter"), wxDefaultPosition, wxDefaultSize,
                            wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);
  m_guard_sizer->Add(m_filter, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  m_filter->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(br24ControlsDialog::OnFilterClick), NULL, this);

  m_top_sizer->Hide(m_guard_sizer);

  //**************** ADJUST BOX ******************//

  m_adjust_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_adjust_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  wxButton* bAdjustBack = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_adjust_sizer->Add(bAdjustBack, 0, wxALL, BORDER);
  bAdjustBack->SetFont(m_pi->m_font);

  // The RANGE button
  m_range_button = new br24RadarRangeControlButton(this, m_ri, ID_RANGE, _("Range"));
  m_adjust_sizer->Add(m_range_button, 0, wxALL, BORDER);

  // The GAIN button
  m_gain_button = new br24RadarControlButton(this, ID_GAIN, _("Gain"), CT_GAIN, true, m_ri->gain.button);
  m_adjust_sizer->Add(m_gain_button, 0, wxALL, BORDER);

  // The SEA button
  m_sea_button = new br24RadarControlButton(this, ID_SEA, _("Sea clutter"), CT_SEA, true, m_ri->sea.button);
  m_adjust_sizer->Add(m_sea_button, 0, wxALL, BORDER);

  // The RAIN button
  m_rain_button = new br24RadarControlButton(this, ID_RAIN, _("Rain clutter"), CT_RAIN, false, m_ri->rain.button);
  m_adjust_sizer->Add(m_rain_button, 0, wxALL, BORDER);

  m_top_sizer->Hide(m_adjust_sizer);

  //**************** BEARING BOX ******************//

  m_bearing_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_bearing_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  wxButton* bBearingBack = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_bearing_sizer->Add(bBearingBack, 0, wxALL, BORDER);
  bBearingBack->SetFont(m_pi->m_font);

  // The CLEAR CURSOR button
  m_clear_cursor = new wxButton(this, ID_CLEAR_CURSOR, _("Clear cursor"), wxDefaultPosition, g_smallButtonSize, 0);
  m_bearing_sizer->Add(m_clear_cursor, 0, wxALL, BORDER);
  m_clear_cursor->SetFont(m_pi->m_font);

  for (int b = 0; b < BEARING_LINES; b++) {
    // The BEARING button
    wxString label = wxString::Format(_("Place EBL/VRM%d"), b + 1);
    m_bearing_buttons[b] = new wxButton(this, ID_BEARING_SET + b, label, wxDefaultPosition, g_smallButtonSize, 0);
    m_bearing_sizer->Add(m_bearing_buttons[b], 0, wxALL, BORDER);
    m_bearing_buttons[b]->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(br24ControlsDialog::OnBearingSetButtonClick),
                                  0, this);
  }

  m_top_sizer->Hide(m_bearing_sizer);

  //**************** VIEW BOX ******************//
  // These are the controls that the users sees when the View button is selected

  m_view_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_view_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  wxButton* bMenuBack = new wxButton(this, ID_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
  m_view_sizer->Add(bMenuBack, 0, wxALL, BORDER);
  bMenuBack->SetFont(m_pi->m_font);

  // The TARGET_TRAIL button
  target_trail_names[0] = _("Off");
  target_trail_names[1] = _("15 sec");
  target_trail_names[2] = _("30 sec");
  target_trail_names[3] = _("1 min");
  target_trail_names[4] = _("3 min");
  target_trail_names[5] = _("Continuous");

  m_target_trails_button =
      new br24RadarControlButton(this, ID_TARGET_TRAILS, _("Target trails"), CT_TARGET_TRAILS, false, m_ri->target_trails.button);
  m_view_sizer->Add(m_target_trails_button, 0, wxALL, BORDER);
  m_target_trails_button->minValue = 0;
  m_target_trails_button->maxValue = ARRAY_SIZE(target_trail_names) - 1;
  m_target_trails_button->names = target_trail_names;
  m_target_trails_button->SetLocalValue(m_ri->target_trails.button);  // redraw after adding names

  // The Clear Trails button
  m_clear_trails_button = new wxButton(this, ID_CLEAR_TRAILS, _("Clear trails"), wxDefaultPosition, g_smallButtonSize, 0);
  m_view_sizer->Add(m_clear_trails_button, 0, wxALL, BORDER);
  m_clear_trails_button->SetFont(m_pi->m_font);

  // The Rotation button
  m_orientation_button = new wxButton(this, ID_ORIENTATION, _("Orientation"), wxDefaultPosition, g_buttonSize, 0);
  m_view_sizer->Add(m_orientation_button, 0, wxALL, BORDER);
  m_orientation_button->SetFont(m_pi->m_font);
  // Updated when we receive data

  m_top_sizer->Hide(m_view_sizer);

  //**************** CONTROL BOX ******************//
  // These are the controls that the users sees when the dialog is started

  // A box sizer to contain RANGE, GAIN etc button
  m_control_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_control_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Transmit button
  m_radar_state = new wxButton(this, ID_RADAR_STATE, _("Unknown"), wxDefaultPosition, g_smallButtonSize, 0);
  m_control_sizer->Add(m_radar_state, 0, wxALL, BORDER);
  m_radar_state->SetFont(m_pi->m_font);
  // Updated when we receive data

  // The SHOW / HIDE RADAR button
  m_window_button = new wxButton(this, ID_SHOW_RADAR, wxT(""), wxDefaultPosition, g_smallButtonSize, 0);
  m_control_sizer->Add(m_window_button, 0, wxALL, BORDER);
  m_window_button->SetFont(m_pi->m_font);

  // The RADAR ONLY / OVERLAY button
  wxString overlay = _("Overlay");
  m_overlay_button = new wxButton(this, ID_RADAR_OVERLAY, overlay, wxDefaultPosition, g_buttonSize, 0);
  m_control_sizer->Add(m_overlay_button, 0, wxALL, BORDER);
  m_overlay_button->SetFont(m_pi->m_font);

  //***************** TRANSMIT SIZER, items hidden when not transmitting ****************//

  m_transmit_sizer = new wxBoxSizer(wxVERTICAL);
  m_control_sizer->Add(m_transmit_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The ADJUST button
  m_adjust_button = new wxButton(this, ID_ADJUST, _("Adjust"), wxDefaultPosition, g_smallButtonSize, 0);
  m_transmit_sizer->Add(m_adjust_button, 0, wxALL, BORDER);
  m_adjust_button->SetFont(m_pi->m_font);

  // The ADVANCED button
  wxButton* bAdvanced = new wxButton(this, ID_ADVANCED, _("Advanced"), wxDefaultPosition, g_smallButtonSize, 0);
  m_transmit_sizer->Add(bAdvanced, 0, wxALL, BORDER);
  bAdvanced->SetFont(m_pi->m_font);

  // The VIEW menu
  wxButton* bView = new wxButton(this, ID_VIEW, _("View"), wxDefaultPosition, g_smallButtonSize, 0);
  m_transmit_sizer->Add(bView, 0, wxALL, BORDER);
  bView->SetFont(m_pi->m_font);

  // The BEARING button
  m_bearing_button = new wxButton(this, ID_BEARING, _("EBL/VRM"), wxDefaultPosition, g_smallButtonSize, 0);
  m_transmit_sizer->Add(m_bearing_button, 0, wxALL, BORDER);
  m_bearing_button->SetFont(m_pi->m_font);

  // The GUARD ZONE 1 button
  m_guard_1_button = new wxButton(this, ID_ZONE1, wxT(""), wxDefaultPosition, g_buttonSize, 0);
  m_transmit_sizer->Add(m_guard_1_button, 0, wxALL, BORDER);
  m_guard_1_button->SetFont(m_pi->m_font);

  // The GUARD ZONE 2 button
  m_guard_2_button = new wxButton(this, ID_ZONE2, wxT(""), wxDefaultPosition, g_buttonSize, 0);
  m_transmit_sizer->Add(m_guard_2_button, 0, wxALL, BORDER);
  m_guard_2_button->SetFont(m_pi->m_font);

  // The TIMED TRANSMIT button
  timed_idle_times[0] = _("Off");
  timed_idle_times[1] = _("5 min");
  timed_idle_times[2] = _("10 min");
  timed_idle_times[3] = _("15 min");
  timed_idle_times[4] = _("20 min");
  timed_idle_times[5] = _("25 min");
  timed_idle_times[6] = _("30 min");
  timed_idle_times[7] = _("35 min");

  m_timed_idle_button =
      new br24RadarControlButton(this, ID_TIMED_IDLE, _("Timed Transmit"), CT_TIMED_IDLE, false, m_pi->m_settings.timed_idle);
  m_control_sizer->Add(m_timed_idle_button, 0, wxALL, BORDER);
  m_timed_idle_button->minValue = 0;
  m_timed_idle_button->maxValue = ARRAY_SIZE(timed_idle_times) - 1;
  m_timed_idle_button->names = timed_idle_times;
  m_timed_idle_button->value = m_pi->m_settings.timed_idle;

  // The INFO button
  wxButton* bMessage = new wxButton(this, ID_MESSAGE, _("Info"), wxDefaultPosition, g_smallButtonSize, 0);
  m_control_sizer->Add(bMessage, 0, wxALL, BORDER);
  bMessage->SetFont(m_pi->m_font);

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

void br24ControlsDialog::SwitchTo(wxBoxSizer* to) {
  m_top_sizer->Hide(m_from_sizer);
  m_top_sizer->Show(to);

  UpdateAdvanced4GState();
  UpdateGuardZoneState();

  if (to != m_edit_sizer) {
    m_from_sizer = to;
  }

  to->Layout();
  m_top_sizer->Layout();
  Fit();
}

void br24ControlsDialog::UpdateAdvanced4GState() {
  if (m_top_sizer->IsShown(m_advanced_sizer)) {
    if (m_ri->radar_type == RT_4G) {
      m_advanced_sizer->Show(m_advanced_4G_sizer);
    } else {
      m_advanced_sizer->Hide(m_advanced_4G_sizer);
    }
  }
}

void br24ControlsDialog::UpdateGuardZoneState() {
  wxString label1, label2;

  label1 << _("Guard zone") << wxT(" 1\n") << guard_zone_names[m_ri->guard_zone[0]->type];
  m_guard_1_button->SetLabel(label1);

  label2 << _("Guard zone") << wxT(" 2\n") << guard_zone_names[m_ri->guard_zone[1]->type];
  m_guard_2_button->SetLabel(label2);
}

#if 0
void br24ControlsDialog::SetTimedIdleIndex(int index) {
  m_timed_idle_button->SetValue(index);  // set and recompute the Timed Idle label
}
#endif

void br24ControlsDialog::OnZone1ButtonClick(wxCommandEvent& event) { ShowGuardZone(0); }

void br24ControlsDialog::OnZone2ButtonClick(wxCommandEvent& event) { ShowGuardZone(1); }

void br24ControlsDialog::OnClose(wxCloseEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void br24ControlsDialog::OnIdOKClick(wxCommandEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void br24ControlsDialog::OnPlusTenClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("br24radar_pi: OnPlustTenClick for %s value %d"), m_from_control->GetLabel().c_str(), m_from_control->value + 10);
  m_from_control->AdjustValue(+10);

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void br24ControlsDialog::OnPlusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(+1);

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void br24ControlsDialog::OnBackClick(wxCommandEvent& event) {
  if (m_top_sizer->IsShown(m_edit_sizer)) {
    m_top_sizer->Hide(m_edit_sizer);
    SwitchTo(m_from_sizer);
    m_from_control = 0;
  } else if (m_top_sizer->IsShown(m_installation_sizer)) {
    SwitchTo(m_advanced_sizer);
  } else {
    SwitchTo(m_control_sizer);
  }
}

void br24ControlsDialog::OnAutoClick(wxCommandEvent& event) {
  m_from_control->SetAuto();
  OnBackClick(event);
}

void br24ControlsDialog::OnMultiSweepClick(wxCommandEvent& event) {
  wxString labelSweep;
  if (m_ri->multi_sweep_filter == 0) {
    labelSweep << _("Multi Sweep Filter") << wxT("\n") << _("ON");
    m_ri->multi_sweep_filter = 1;
  } else {
    labelSweep << _("Multi Sweep Filter") << wxT("\n") << _("Off");
    m_ri->multi_sweep_filter = 0;
  }
  m_multi_sweep_button->SetLabel(labelSweep);
}

void br24ControlsDialog::OnMinusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-1);

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void br24ControlsDialog::OnMinusTenClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-10);

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void br24ControlsDialog::OnAdjustButtonClick(wxCommandEvent& event) { SwitchTo(m_adjust_sizer); }

void br24ControlsDialog::OnAdvancedButtonClick(wxCommandEvent& event) { SwitchTo(m_advanced_sizer); }

void br24ControlsDialog::OnViewButtonClick(wxCommandEvent& event) { SwitchTo(m_view_sizer); }

void br24ControlsDialog::OnInstallationButtonClick(wxCommandEvent& event) { SwitchTo(m_installation_sizer); }

void br24ControlsDialog::OnBearingButtonClick(wxCommandEvent& event) { SwitchTo(m_bearing_sizer); }

void br24ControlsDialog::OnMessageButtonClick(wxCommandEvent& event) {
  if (m_pi->m_pMessageBox) {
    m_pi->m_pMessageBox->UpdateMessage(true);
  }
}

void br24ControlsDialog::EnterEditMode(br24RadarControlButton* button) {
  m_from_control = button;  // Keep a record of which button was clicked
  m_value_text->SetLabel(button->GetLabel());

  SwitchTo(m_edit_sizer);

  if (m_from_control->hasAuto) {
    m_auto_button->Show();
  } else {
    m_auto_button->Hide();
  }
  if (m_from_control == m_gain_button) {
    m_multi_sweep_button->Show();
  } else {
    m_multi_sweep_button->Hide();
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

void br24ControlsDialog::OnRadarControlButtonClick(wxCommandEvent& event) {
  EnterEditMode((br24RadarControlButton*)event.GetEventObject());
}

void br24ControlsDialog::OnRadarShowButtonClick(wxCommandEvent& event) {
  int show = 1;

  if (m_pi->m_settings.enable_dual_radar) {
    if (m_pi->m_settings.show_radar[m_ri->radar]) {
      int show_other_radar = m_pi->m_settings.show_radar[1 - m_ri->radar];
      if (show_other_radar) {
        // Hide both windows
        show = 0;
      }
    }
    m_pi->m_settings.show_radar[0] = show;
    m_pi->m_settings.show_radar[1] = show;
    LOG_DIALOG(wxT("BR24radar_pi: OnRadarShowButton: show_radar[%d]=%d"), 0, show);
    LOG_DIALOG(wxT("BR24radar_pi: OnRadarShowButton: show_radar[%d]=%d"), 1, show);
  } else {
    if (m_ri->IsPaneShown()) {
      show = 0;
    }
    m_pi->m_settings.show_radar[0] = show;
    LOG_DIALOG(wxT("BR24radar_pi: OnRadarShowButton: show_radar[%d]=%d"), 0, show);
  }

  m_pi->SetRadarWindowViz();
}

void br24ControlsDialog::OnRadarOverlayButtonClick(wxCommandEvent& event) {
  if (m_pi->m_settings.chart_overlay != m_ri->radar) {
    m_pi->m_settings.chart_overlay = m_ri->radar;
  } else {
    m_pi->m_settings.chart_overlay = -1;
  }
  m_ri->overlay.Update(m_pi->m_settings.chart_overlay == m_ri->radar);
  UpdateControlValues(true);
}

void br24ControlsDialog::OnRadarGainButtonClick(wxCommandEvent& event) {
  EnterEditMode((br24RadarControlButton*)event.GetEventObject());
}

void br24ControlsDialog::OnRadarStateButtonClick(wxCommandEvent& event) {
  m_pi->m_settings.timed_idle = 0;
  m_ri->FlipRadarState();
}

void br24ControlsDialog::OnClearTrailsButtonClick(wxCommandEvent& event) {
  m_ri->ClearTrails();
}


void br24ControlsDialog::OnOrientationButtonClick(wxCommandEvent& event) {
  m_ri->orientation.Update(1 - m_ri->orientation.value);
  UpdateControlValues(false);
}

void br24ControlsDialog::OnConfirmBogeyButtonClick(wxCommandEvent& event) {
  m_pi->ConfirmGuardZoneBogeys();
  SwitchTo(m_control_sizer);
  UpdateDialogShown();
}

void br24ControlsDialog::OnBearingSetButtonClick(wxCommandEvent& event) {
  int bearing = event.GetId() - ID_BEARING_SET;
  LOG_DIALOG(wxT("br24radar_pi: OnBearingSetButtonClick for bearing #%d"), bearing + 1);

  m_ri->SetBearing(bearing);
}

void br24ControlsDialog::OnClearCursorButtonClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("br24radar_pi: OnClearCursorButtonClick"));
  m_ri->SetMouseVrmEbl(0., 0.);
  SwitchTo(m_control_sizer);
}

void br24ControlsDialog::OnMove(wxMoveEvent& event) { event.Skip(); }

void br24ControlsDialog::OnSize(wxSizeEvent& event) { event.Skip(); }

void br24ControlsDialog::UpdateControlValues(bool refreshAll) {
  wxString o;

  if (m_ri->state.mod) {
    refreshAll = true;
  }

  RadarState state = (RadarState)m_ri->state.GetButton();

  o = (state == RADAR_TRANSMIT) ? _("Standby") : _("Transmit");
  if (m_pi->m_settings.timed_idle == 0)
  {
    m_timed_idle_button->SetLocalValue(0);
  }
  else
  {
    time_t now = time(0);
    int left = m_pi->m_idle_standby - now;
    if (left > 0) {
      o = wxString::Format(_("Standby in %d:%02d"), left / 60, left % 60);
    }
    else {
      left = m_pi->m_idle_transmit - now;
      if (left >= 0) {
        o = wxString::Format(_("Transmit in %d:%02d"), left / 60, left % 60);
      }
    }
  }
  m_radar_state->SetLabel(o);

  if (state == RADAR_TRANSMIT) {
    if (m_top_sizer->IsShown(m_control_sizer)) {
      m_control_sizer->Show(m_transmit_sizer);
      m_top_sizer->Show(m_timed_idle_button);
      m_control_sizer->Layout();
    }
  } else {
    m_control_sizer->Hide(m_transmit_sizer);
    if (m_pi->m_settings.timed_idle) {
      m_top_sizer->Show(m_timed_idle_button);
    } else {
      m_top_sizer->Hide(m_timed_idle_button);
    }
    m_control_sizer->Layout();
  }
  m_top_sizer->Layout();
  Layout();

  if (m_pi->m_settings.enable_dual_radar) {
    int show_other_radar = m_pi->m_settings.show_radar[1 - m_ri->radar];
    if (m_pi->m_settings.show_radar[m_ri->radar]) {
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
    o = (m_pi->m_settings.show_radar[m_ri->radar]) ? _("Hide window") : _("Show window");
  }
  m_window_button->SetLabel(o);

  for (int b = 0; b < BEARING_LINES; b++) {
    if (m_ri->m_vrm[b] != 0.0) {
      o = wxString::Format(_("Clear EBL/VRM%d"), b + 1);
    } else {
      o = wxString::Format(_("Place EBL/VRM%d"), b + 1);
    }
    m_bearing_buttons[b]->SetLabel(o);
  }

  if (m_ri->target_trails.mod || refreshAll) {
    m_target_trails_button->SetLocalValue(m_ri->target_trails.GetButton());
  }

  if (m_ri->orientation.mod || refreshAll) {
    o = _("Orientation");
    o << wxT("\n");
    o << ((m_ri->orientation.GetButton()) ? _("North Up") : _("Head Up"));
    m_orientation_button->SetLabel(o);
  }

  if (m_ri->overlay.mod || ((m_pi->m_settings.chart_overlay == m_ri->radar) != (m_ri->overlay.button != 0)) || refreshAll) {
    o = _("Overlay");
    o << wxT("\n");
    o << ((m_ri->overlay.GetButton() > 0) ? _("on") : _("off"));
    m_overlay_button->SetLabel(o);
  }

  if (m_ri->range.mod || refreshAll) {
    m_ri->range.GetButton();
    m_range_button->SetRangeLabel();
  }

  // gain
  if (m_ri->gain.mod || refreshAll) {
    int button = m_ri->gain.GetButton();
    if (button == -1) {
      m_gain_button->SetLocalAuto();
    } else {
      m_gain_button->SetLocalValue(button);
    }
  }

  //  rain
  if (m_ri->rain.mod || refreshAll) {
    m_rain_button->SetLocalValue(m_ri->rain.GetButton());
  }

  //   sea
  if (m_ri->sea.mod || refreshAll) {
    int button = m_ri->sea.GetButton();
    if (button == -1) {
      m_sea_button->SetLocalAuto();
    } else {
      m_sea_button->SetLocalValue(button);
    }
  }

  //   target_boost
  if (m_ri->target_boost.mod || refreshAll) {
    m_target_boost_button->SetLocalValue(m_ri->target_boost.GetButton());
  }

  //   target_expansion
  if (m_ri->target_expansion.mod || refreshAll) {
    m_target_expansion_button->SetLocalValue(m_ri->target_expansion.GetButton());
  }

  //  noise_rejection
  if (m_ri->noise_rejection.mod || refreshAll) {
    m_noise_rejection_button->SetLocalValue(m_ri->noise_rejection.GetButton());
  }

  //  target_separation
  if (m_ri->target_separation.mod || refreshAll) {
    m_target_separation_button->SetLocalValue(m_ri->target_separation.GetButton());
  }

  //  interference_rejection
  if (m_ri->interference_rejection.mod || refreshAll) {
    m_interference_rejection_button->SetLocalValue(m_ri->interference_rejection.GetButton());
  }

  // scanspeed
  if (m_ri->scan_speed.mod || refreshAll) {
    m_scan_speed_button->SetLocalValue(m_ri->scan_speed.GetButton());
  }

  //   antenna height
  if (m_ri->antenna_height.mod || refreshAll) {
    m_antenna_height_button->SetLocalValue(m_ri->antenna_height.GetButton());
  }

  //  bearing alignment
  if (m_ri->bearing_alignment.mod || refreshAll) {
    m_bearing_alignment_button->SetLocalValue(m_ri->bearing_alignment.GetButton());
  }

  //  local interference rejection
  if (m_ri->local_interference_rejection.mod || refreshAll) {
    m_local_interference_rejection_button->SetLocalValue(m_ri->local_interference_rejection.GetButton());
  }

  // side lobe suppression
  if (m_ri->side_lobe_suppression.mod || refreshAll) {
    int button = m_ri->side_lobe_suppression.GetButton();
    if (button == -1) {
      m_side_lobe_suppression_button->SetLocalAuto();
    } else {
      m_side_lobe_suppression_button->SetLocalValue(button);
    }
  }

  if (m_pi->m_settings.display_option == 1) {
    m_target_trails_button->Enable();
    m_clear_trails_button->Enable();
  } else {
    m_target_trails_button->Disable();
    m_clear_trails_button->Disable();
  }

  // Update the text that is currently shown in the edit box, this is a copy of the button itself
  if (m_from_control) {
    wxString label = m_from_control->GetLabel();
    m_value_text->SetLabel(label);
  }
}

void br24ControlsDialog::UpdateDialogShown() {
  if (m_hide) {
    if (IsShown()) {
      LOG_DIALOG(wxT("br24radar_pi: %s ControlsDialog::UpdateDialogShown explicit closed: Hidden"), m_ri->name.c_str());
      Hide();
    }
    return;
  }

  if (m_hide_temporarily) {
    if (IsShown()) {
      LOG_DIALOG(wxT("br24radar_pi: %s ControlsDialog::UpdateDialogShown temporarily hidden"), m_ri->name.c_str());
      Hide();
    }
    return;
  }

  if (m_auto_hide_timeout && TIMED_OUT(time(0), m_auto_hide_timeout)) {
    if (!m_top_sizer->IsShown(m_control_sizer)) {
      // If we're somewhere in the sub-window, don't close the dialog
      SetMenuAutoHideTimeout();
    } else {
      if (IsShown()) {
        LOG_DIALOG(wxT("br24radar_pi: %s ControlsDialog::UpdateDialogShown auto-hide"), m_ri->name.c_str());
        Hide();
      }
      return;
    }
  }

  // Following helps on OSX where the control is SHOW_ON_TOP to not show when no part of OCPN is focused
  wxWindow* focused = FindFocus();
  if (!focused) {
    LOG_DIALOG(wxT("br24radar_pi: %s ControlsDialog::UpdateDialogShown app not focused"), m_ri->name.c_str());
    return;
  }

  if (!IsShown()) {
    LOG_DIALOG(wxT("br24radar_pi: %s ControlsDialog::UpdateDialogShown manually opened"), m_ri->name.c_str());
    if (!m_top_sizer->IsShown(m_control_sizer) && !m_top_sizer->IsShown(m_advanced_sizer) && !m_top_sizer->IsShown(m_view_sizer) &&
        !m_top_sizer->IsShown(m_edit_sizer) && !m_top_sizer->IsShown(m_installation_sizer) &&
        !m_top_sizer->IsShown(m_bogey_sizer) && !m_top_sizer->IsShown(m_guard_sizer) && !m_top_sizer->IsShown(m_adjust_sizer) &&
        !m_top_sizer->IsShown(m_bearing_sizer)) {
      SwitchTo(m_control_sizer);
    }
    m_control_sizer->Layout();
    m_top_sizer->Layout();
    Show();

    m_edit_sizer->Layout();
    m_top_sizer->Layout();
  }
  if (m_top_sizer->IsShown(m_control_sizer)) {
    Fit();
  }
}

void br24ControlsDialog::HideTemporarily() {
  m_hide_temporarily = true;
  UpdateDialogShown();
}

void br24ControlsDialog::UnHideTemporarily() {
  m_hide_temporarily = false;
  SetMenuAutoHideTimeout();
  UpdateDialogShown();
}

void br24ControlsDialog::ShowDialog() {
  m_hide = false;

  if (!IsShown()) {
    // If the corresponding radar panel is now in a different position from what we remembered
    // then reset the dialog to the left or right of the radar panel.

    wxPoint panelPos = m_ri->radar_panel->GetPos();
    if (panelPos != m_panel_position) {
      wxSize mySize = this->GetSize();

      bool showOnLeft = (panelPos.x > mySize.x);

      wxPoint newPos = panelPos;

      if (showOnLeft) {
        newPos.x = panelPos.x - mySize.x;
      } else {
        newPos.x = panelPos.x + m_ri->radar_panel->GetSize().x;
      }
      SetPosition(newPos);

      m_panel_position = panelPos;
    }
  }
  UnHideTemporarily();
}

void br24ControlsDialog::ShowBogeys(wxString text) {
  if (m_top_sizer->IsShown(m_control_sizer)) {
    SwitchTo(m_bogey_sizer);
    if (!m_hide) {
      UnHideTemporarily();
    } else {
      ShowDialog();
    }
  }
  if (m_top_sizer->IsShown(m_bogey_sizer)) {
    m_bogey_text->SetLabel(text);
    m_bogey_sizer->Layout();
  }
}

void br24ControlsDialog::HideDialog() {
  m_hide = true;
  m_auto_hide_timeout = 0;
  UpdateDialogShown();
}

void br24ControlsDialog::SetMenuAutoHideTimeout() {
  switch (m_pi->m_settings.menu_auto_hide) {
    case 1:
      m_auto_hide_timeout = time(0) + 10;
      break;
    case 2:
      m_auto_hide_timeout = time(0) + 30;
      break;
    default:
      m_auto_hide_timeout = 0;
      break;
  }
}

void br24ControlsDialog::OnMouseLeftDown(wxMouseEvent& event) {
  SetMenuAutoHideTimeout();
  event.Skip();
}

void br24ControlsDialog::ShowGuardZone(int zone) {
  double conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone = m_ri->guard_zone[zone];

  wxString GuardZoneText;
  GuardZoneText << _("Guard Zone") << wxString::Format(wxT(" %d"), zone + 1);
  m_guard_zone_text->SetLabel(GuardZoneText);

  m_guard_zone_type->SetSelection(m_guard_zone->type);
  m_inner_range->SetValue(wxString::Format(wxT("%2.2f"), m_guard_zone->inner_range / conversionFactor));
  m_outer_range->SetValue(wxString::Format(wxT("%2.2f"), m_guard_zone->outer_range / conversionFactor));

  m_start_bearing->SetValue(wxString::Format(wxT("%3.1f"), SCALE_RAW_TO_DEGREES2048(m_guard_zone->start_bearing)));
  m_end_bearing->SetValue(wxString::Format(wxT("%3.1f"), SCALE_RAW_TO_DEGREES2048(m_guard_zone->end_bearing)));
  m_filter->SetValue(m_guard_zone->multi_sweep_filter ? 1 : 0);

  m_top_sizer->Hide(m_control_sizer);
  SwitchTo(m_guard_sizer);
  SetGuardZoneVisibility();
  UpdateDialogShown();
}

void br24ControlsDialog::SetGuardZoneVisibility() {
  GuardZoneType zoneType = (GuardZoneType)m_guard_zone_type->GetSelection();

  m_guard_zone->type = zoneType;

  if (zoneType == GZ_OFF) {
    m_start_bearing->Disable();
    m_end_bearing->Disable();
    m_inner_range->Disable();
    m_outer_range->Disable();

  } else if (zoneType == GZ_CIRCLE) {
    m_start_bearing->Disable();
    m_end_bearing->Disable();
    m_inner_range->Enable();
    m_outer_range->Enable();

  } else {
    m_start_bearing->Enable();
    m_end_bearing->Enable();
    m_inner_range->Enable();
    m_outer_range->Enable();
  }
  m_guard_sizer->Layout();
}

void br24ControlsDialog::OnGuardZoneModeClick(wxCommandEvent& event) { SetGuardZoneVisibility(); }

void br24ControlsDialog::OnInner_Range_Value(wxCommandEvent& event) {
  wxString temp = m_inner_range->GetValue();
  double t;
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->inner_range = (int)(t * conversionFactor);
}

void br24ControlsDialog::OnOuter_Range_Value(wxCommandEvent& event) {
  wxString temp = m_outer_range->GetValue();
  double t;
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->outer_range = (int)(t * conversionFactor);
}

void br24ControlsDialog::OnStart_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_start_bearing->GetValue();
  double t;

  temp.ToDouble(&t);
  m_guard_zone->start_bearing = SCALE_DEGREES_TO_RAW2048(t);
}

void br24ControlsDialog::OnEnd_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_end_bearing->GetValue();
  double t;

  temp.ToDouble(&t);
  m_guard_zone->end_bearing = SCALE_DEGREES_TO_RAW2048(t);
}

void br24ControlsDialog::OnFilterClick(wxCommandEvent& event) {
  int filt = m_filter->GetValue();
  m_guard_zone->multi_sweep_filter = filt;
}

PLUGIN_END_NAMESPACE
