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

#include "ControlsDialog.h"
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
IMPLEMENT_CLASS(ControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(ControlsDialog, wxDialog)

EVT_CLOSE(ControlsDialog::OnClose)
EVT_BUTTON(ID_BACK, ControlsDialog::OnBackClick)
EVT_BUTTON(ID_PLUS_TEN, ControlsDialog::OnPlusTenClick)
EVT_BUTTON(ID_PLUS, ControlsDialog::OnPlusClick)
EVT_BUTTON(ID_MINUS, ControlsDialog::OnMinusClick)
EVT_BUTTON(ID_MINUS_TEN, ControlsDialog::OnMinusTenClick)
EVT_BUTTON(ID_AUTO, ControlsDialog::OnAutoClick)
EVT_BUTTON(ID_TRAILS_MOTION, ControlsDialog::OnTrailsMotionClick)

EVT_BUTTON(ID_TRANSPARENCY, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INTERFERENCE_REJECTION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_BOOST, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_EXPANSION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_NOISE_REJECTION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_SEPARATION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_REFRESHRATE, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SCAN_SPEED, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INSTALLATION, ControlsDialog::OnInstallationButtonClick)
EVT_BUTTON(ID_PREFERENCES, ControlsDialog::OnPreferencesButtonClick)

EVT_BUTTON(ID_BEARING_ALIGNMENT, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_HEIGHT, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_FORWARD, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ANTENNA_STARBOARD, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_LOCAL_INTERFERENCE_REJECTION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SIDE_LOBE_SUPPRESSION, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_MAIN_BANG_SIZE, ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_POWER, ControlsDialog::OnPowerButtonClick)
EVT_BUTTON(ID_SHOW_RADAR, ControlsDialog::OnRadarShowButtonClick)
EVT_BUTTON(ID_RADAR_OVERLAY, ControlsDialog::OnRadarOverlayButtonClick)
EVT_BUTTON(ID_RANGE, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_GAIN, ControlsDialog::OnRadarGainButtonClick)
EVT_BUTTON(ID_SEA, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_RAIN, ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_TARGETS, ControlsDialog::OnTargetsButtonClick)
EVT_BUTTON(ID_TARGET_TRAILS, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_CLEAR_TRAILS, ControlsDialog::OnClearTrailsButtonClick)
EVT_BUTTON(ID_ORIENTATION, ControlsDialog::OnOrientationButtonClick)

EVT_BUTTON(ID_ADJUST, ControlsDialog::OnAdjustButtonClick)
EVT_BUTTON(ID_ADVANCED, ControlsDialog::OnAdvancedButtonClick)
EVT_BUTTON(ID_VIEW, ControlsDialog::OnViewButtonClick)

EVT_BUTTON(ID_BEARING, ControlsDialog::OnBearingButtonClick)
EVT_BUTTON(ID_ZONE1, ControlsDialog::OnZone1ButtonClick)
EVT_BUTTON(ID_ZONE2, ControlsDialog::OnZone2ButtonClick)

EVT_BUTTON(ID_MESSAGE, ControlsDialog::OnMessageButtonClick)

EVT_BUTTON(ID_BEARING_SET, ControlsDialog::OnBearingSetButtonClick)
EVT_BUTTON(ID_CLEAR_CURSOR, ControlsDialog::OnClearCursorButtonClick)
EVT_BUTTON(ID_ACQUIRE_TARGET, ControlsDialog::OnAcquireTargetButtonClick)
EVT_BUTTON(ID_DELETE_TARGET, ControlsDialog::OnDeleteTargetButtonClick)
EVT_BUTTON(ID_DELETE_ALL_TARGETS, ControlsDialog::OnDeleteAllTargetsButtonClick)

EVT_BUTTON(ID_TRANSMIT, ControlsDialog::OnTransmitButtonClick)
EVT_BUTTON(ID_STANDBY, ControlsDialog::OnStandbyButtonClick)
EVT_BUTTON(ID_TIMED_IDLE, ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TIMED_RUN, ControlsDialog::OnRadarControlButtonClick)

EVT_MOVE(ControlsDialog::OnMove)
EVT_CLOSE(ControlsDialog::OnClose)

END_EVENT_TABLE()

// The following are only for logging, so don't care about translations.
string ControlTypeNames[CT_MAX] = {
    "Unused",
#define CONTROL_TYPE(x, y) y,
#include "ControlType.inc"
#undef CONTROL_TYPE
};

wxString guard_zone_names[2];

void RadarControlButton::AdjustValue(int adjustment) {
  int newValue = value + adjustment;

  autoValue = 0;  // Disable Auto

  if (newValue < minValue) {
    newValue = minValue;
  } else if (newValue > maxValue) {
    newValue = maxValue;
  }
  if (newValue != value) {
    LOG_VERBOSE(wxT("%s Adjusting %s by %d from %d to %d"), m_parent->m_log_name.c_str(), GetName(), adjustment, value, newValue);
    if (m_pi->SetControlValue(m_parent->m_ri->m_radar, controlType, newValue, 0)) {
      SetLocalValue(newValue);
    }
  }
}

void RadarControlButton::SetLocalValue(int newValue) {  // sets value in the button without sending new value to the radar
  if (newValue <= AUTO_RANGE) {
    SetLocalAuto(AUTO_RANGE - newValue);
    return;
  }
  if (newValue != value) {
    LOG_VERBOSE(wxT("%s Set %s value %d -> %d, range=%d..%d"), m_parent->m_log_name.c_str(), ControlTypeNames[controlType], value,
                newValue, minValue, maxValue);
  }
  if (newValue < minValue) {
    value = minValue;
  } else if (newValue > maxValue) {
    value = maxValue;
  } else {
    value = newValue;
  }
  autoValue = 0;

  wxString label;

  if (names) {
    label.Printf(wxT("%s\n%s"), firstLine.c_str(), names[value].c_str());
  } else {
    label.Printf(wxT("%s\n%d"), firstLine.c_str(), value);
  }
  if (unit.length() > 0) {
    label << wxT(" ") << unit;
  }

  this->SetLabel(label);
}

void RadarControlButton::SetAuto(int newAutoValue) {
  SetLocalAuto(newAutoValue);
  m_parent->m_ri->SetControlValue(controlType, value, newAutoValue);
}

void RadarControlButton::SetLocalAuto(int newValue) {  // sets auto in the button without sending new value
                                                       // to the radar
  wxString label;

  autoValue = newValue;
  LOG_VERBOSE(wxT("%s Set %s to auto value %d, max=%d"), m_parent->m_log_name.c_str(), ControlTypeNames[controlType], autoValue,
              autoValues);
  if (autoValue == 0) {
    SetLocalValue(value);  // To update label to old non-auto value
    return;
  }
  label << firstLine << wxT("\n");
  if (autoNames && autoValue > 0 && autoValue <= autoValues) {
    label << autoNames[autoValue - 1];
  } else {
    label << _("Auto");
  }
  this->SetLabel(label);
}

void RadarRangeControlButton::SetRangeLabel() {
  wxString text = m_ri->GetRangeText();
  this->SetLabel(firstLine + wxT("\n") + text);
}

void RadarRangeControlButton::AdjustValue(int adjustment) {
  LOG_VERBOSE(wxT("%s Adjusting %s by %d"), m_parent->m_log_name.c_str(), GetName(), adjustment);
  autoValue = 0;
  m_parent->m_ri->AdjustRange(adjustment);  // send new value to the radar
}

void RadarRangeControlButton::SetAuto(int newValue) {
  autoValue = newValue;
  m_parent->m_ri->m_auto_range_mode = true;
}

ControlsDialog::~ControlsDialog() {
  wxPoint pos = GetPosition();

  LOG_DIALOG(wxT("%s saved position %d,%d"), m_log_name.c_str(), pos.x, pos.y);
  m_pi->m_settings.control_pos[m_ri->m_radar] = pos;
}

bool ControlsDialog::Create(wxWindow* parent, radar_pi* ppi, RadarInfo* ri, wxWindowID id, const wxString& caption,
                            const wxPoint& pos) {
  m_parent = parent;
  m_pi = ppi;
  m_ri = ri;

  m_log_name = wxString::Format(wxT("radar_pi: Radar %c ControlDialog:"), ri->m_radar + 'A');

#ifdef __WXMSW__
  long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#endif
#ifdef __WXMAC__
  long wstyle = wxCLOSE_BOX | wxCAPTION;
  // long wstyle = wxCLOSE_BOX | wxCAPTION | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP;

  wstyle |= wxSTAY_ON_TOP;  // Radar AUI windows, when float, are already FLOAT_ON_PARENT and we don't seem to be on top of those.
  wstyle |= wxFRAME_FLOAT_ON_PARENT;  // Float on our parent
  wstyle |= wxFRAME_TOOL_WINDOW;      // This causes window to hide when OpenCPN is not activated, but this doesn't wo
#endif
#ifdef __WXGTK__
  long wstyle = wxCLOSE_BOX | wxCAPTION | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR;
#endif

  /*guard_zone_names[0] = _("Off");*/
  guard_zone_names[0] = _("Arc");
  guard_zone_names[1] = _("Circle");

  if (!wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle)) {
    return false;
  }

  CreateControls();
  return true;
}

void ControlsDialog::EnsureWindowNearOpenCPNWindow() {
#define PROXIMITY_MARGIN 32

  wxWindow* parent = m_pi->m_parent_window;
  while (parent->GetParent()) {
    parent = parent->GetParent();
  }
  wxPoint oPos = parent->GetScreenPosition();
  wxSize oSize = parent->GetSize();
  oSize.x += PROXIMITY_MARGIN;
  oSize.y += PROXIMITY_MARGIN;

  wxPoint mPos = GetPosition();
  wxSize mSize = GetSize();
  mSize.x += PROXIMITY_MARGIN;
  mSize.y += PROXIMITY_MARGIN;

  bool move = false;

  // LOG_DIALOG(wxT("%s control %d,%d is near OpenCPN at %d,%d to %d,%d?"), m_log_name.c_str(), mPos.x, mPos.y, oPos.x, oPos.y
  // , oPos.x + oSize.x, oPos.y + oSize.y);

  if (mPos.x + mSize.x < oPos.x) {
    mPos.x = oPos.x;
    move = true;
  }
  if (oPos.x + oSize.x < mPos.x) {
    mPos.x = oPos.x + oSize.x - mSize.x;
    move = true;
  }
  if (mPos.y + mSize.y < oPos.y) {
    mPos.y = oPos.y;
    move = true;
  }
  if (oPos.y + oSize.y < mPos.y) {
    mPos.y = oPos.y + oSize.y - mSize.y;
    move = true;
  }
  if (move) {
    LOG_DIALOG(wxT("%s Move control dialog to %d,%d to be near OpenCPN at %d,%d to %d,%d"), m_log_name.c_str(), mPos.x, mPos.y,
               oPos.x, oPos.y, oPos.x + oSize.x, oPos.y + oSize.y);
  }
  SetPosition(mPos);
}

void ControlsDialog::SetMenuAutoHideTimeout() {
  if (m_top_sizer->IsShown(m_control_sizer)) {
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
  } else {
    m_auto_hide_timeout = 0;
  }
}

void ControlsDialog::ShowGuardZone(int zone) {
  double conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone = m_ri->m_guard_zone[zone];

  wxString GuardZoneText;
  if (zone == 0) {
    GuardZoneText << _("Guard Zone 1 Green");
  }
  if (zone == 1) {
    GuardZoneText << _("Guard Zone 2 Blue");
  }
  m_guard_zone_text->SetLabel(GuardZoneText);

  m_guard_zone_type->SetSelection(m_guard_zone->m_type);
  m_inner_range->SetValue(wxString::Format(wxT("%2.2f"), m_guard_zone->m_inner_range / conversionFactor));
  m_outer_range->SetValue(wxString::Format(wxT("%2.2f"), m_guard_zone->m_outer_range / conversionFactor));

  AngleDegrees bearing = m_guard_zone->m_start_bearing;
  m_start_bearing->SetValue(wxString::Format(wxT("%d"), bearing));

  bearing = m_guard_zone->m_end_bearing;
  while (bearing >= 180.0) {
    bearing -= 360.;
  }
  bearing = round(bearing);
  m_end_bearing->SetValue(wxString::Format(wxT("%d"), bearing));
  m_alarm->SetValue(m_guard_zone->m_alarm_on ? 1 : 0);
  m_arpa_box->SetValue(m_guard_zone->m_arpa_on ? 1 : 0);
  m_guard_zone->m_show_time = time(0);

  m_top_sizer->Hide(m_control_sizer);
  SwitchTo(m_guard_sizer, wxT("guard"));
  SetGuardZoneVisibility();
  UpdateDialogShown();
}

void ControlsDialog::SetGuardZoneVisibility() {
  GuardZoneType zoneType = (GuardZoneType)m_guard_zone_type->GetSelection();

  m_guard_zone->SetType(zoneType);

  if (zoneType == GZ_CIRCLE) {
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

void ControlsDialog::UpdateGuardZoneState() {
  wxString label1, label2, label3, label4;
  if (m_ri->m_guard_zone[0]->m_alarm_on) {
    label3 << _(" + Alarm");
  }
  if (m_ri->m_guard_zone[0]->m_arpa_on) {
    label3 << _(" + Arpa");
  }
  if (!m_ri->m_guard_zone[0]->m_alarm_on && !m_ri->m_guard_zone[0]->m_arpa_on) {
    label3 << _(" Off");
  }

  if (m_ri->m_guard_zone[1]->m_alarm_on) {
    label4 << _(" + Alarm");
  }
  if (m_ri->m_guard_zone[1]->m_arpa_on) {
    label4 << _(" + Arpa");
  }
  if (!m_ri->m_guard_zone[1]->m_alarm_on && !m_ri->m_guard_zone[1]->m_arpa_on) {
    label4 << _(" Off");
  }

  label1 << _("Guard zone") << wxT(" 1 Green\n") << guard_zone_names[m_ri->m_guard_zone[0]->m_type] << label3;
  m_guard_1_button->SetLabel(label1);

  label2 << _("Guard zone") << wxT(" 2 Blue\n") << guard_zone_names[m_ri->m_guard_zone[1]->m_type] << label4;
  m_guard_2_button->SetLabel(label2);
}

void ControlsDialog::UpdateTrailsState() {
  if (m_top_sizer->IsShown(m_view_sizer)) {
    int value = m_ri->m_trails_motion.GetValue();

    if (value == TARGET_MOTION_OFF) {
      m_target_trails_button->Hide();
      m_clear_trails_button->Hide();
    } else {
      m_target_trails_button->Show();
      m_clear_trails_button->Show();
    }
  }
}

void ControlsDialog::SwitchTo(wxBoxSizer* to, const wxChar* name) {
  if (!m_top_sizer || !m_from_sizer) {
    return;
  }
  m_top_sizer->Hide(m_from_sizer);
  m_top_sizer->Show(to);
  LOG_VERBOSE(wxT("%s switch to control view %s"), m_log_name.c_str(), name);

  UpdateRadarSpecificState();
  UpdateTrailsState();
  UpdateGuardZoneState();
  SetMenuAutoHideTimeout();

  if (to != m_edit_sizer) {
    m_from_sizer = to;
  }

  to->Layout();
  m_top_sizer->Layout();
  Fit();
}

wxSize g_buttonSize;

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

void ControlsDialog::CreateControls() {
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
  label << _("Scan speed") << wxT("\n");
  label << _("Installation") << wxT("\n");
  label << _("Antenna height") << wxT("\n");
  label << _("Antenna forward of GPS") << wxT("\n");
  label << _("Antenna starboard of GPS") << wxT("\n");
  label << _("Local interference rej.") << wxT("\n");
  label << _("Guard zones") << wxT("\n");
  label << _("Zone type") << wxT("\n");
  label << _("Guard zones") << wxT("\n");
  label << _("Inner range") << wxT("\n");
  label << _("Outer range") << wxT("\n");
  label << _("Start bearing") << wxT("\n");
  label << _("End bearing") << wxT("\n");
  label << _("Clear cursor") << wxT("\n");
  label << _("Place EBL/VRM") << wxT("\n");
  label << _("Off/Relative/True trails") << wxT("\n");
  label << _("Clear trails") << wxT("\n");
  label << _("Orientation") << wxT("\n");
  label << _("Transparency") << wxT("\n");
  label << _("Overlay") << wxT("\n");
  label << _("Adjust") << wxT("\n");
  label << _("Advanced") << wxT("\n");
  label << _("View") << wxT("\n");
  label << _("EBL/VRM") << wxT("\n");
  label << _("Timed Transmit") << wxT("\n");
  label << _("Info") << wxT("\n");

  for (int i = 0; i < CT_MAX; i++) {
    label << _(ControlTypeNames[i]) << wxT("\n");
  }

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

  if (m_ctrl.control[CT_NOISE_REJECTION].type) {
    // The NOISE REJECTION button
    m_noise_rejection_button = new RadarControlButton(this, ID_NOISE_REJECTION, _("Noise rejection"),
                                                      m_ctrl.control[CT_NOISE_REJECTION], m_ri->m_noise_rejection);
    m_advanced_sizer->Add(m_noise_rejection_button, 0, wxALL, BORDER);
  }

  // The TARGET EXPANSION button
  if (m_ctrl.control[CT_TARGET_EXPANSION].type) {
    m_target_expansion_button = new RadarControlButton(this, ID_TARGET_EXPANSION, _("Target expansion"),
                                                       m_ctrl.control[CT_TARGET_EXPANSION], m_ri->m_target_expansion);
    m_advanced_sizer->Add(m_target_expansion_button, 0, wxALL, BORDER);
  }

  // The REJECTION button

  if (m_ctrl.control[CT_INTERFERENCE_REJECTION].type) {
    m_interference_rejection_button =
        new RadarControlButton(this, ID_INTERFERENCE_REJECTION, _("Interference rejection"),
                               m_ctrl.control[CT_INTERFERENCE_REJECTION], m_ri->m_interference_rejection);
    m_advanced_sizer->Add(m_interference_rejection_button, 0, wxALL, BORDER);
  }

  // The TARGET SEPARATION button
  if (m_ctrl.control[CT_TARGET_SEPARATION].type) {
    m_target_separation_button = new RadarControlButton(this, ID_TARGET_SEPARATION, _("Target separation"),
                                                        m_ctrl.control[CT_TARGET_SEPARATION], m_ri->m_target_separation);
    m_advanced_sizer->Add(m_target_separation_button, 0, wxALL, BORDER);
  }

  // The SCAN SPEED button
  if (m_ctrl.control[CT_SCAN_SPEED].type) {
    m_scan_speed_button =
        new RadarControlButton(this, ID_SCAN_SPEED, _("Scan speed"), m_ctrl.control[CT_SCAN_SPEED], m_ri->m_scan_speed);
    m_advanced_sizer->Add(m_scan_speed_button, 0, wxALL, BORDER);
  }

  // The TARGET BOOST button
  if (m_ctrl.control[CT_TARGET_BOOST].type) {
    m_target_boost_button =
        new RadarControlButton(this, ID_SCAN_SPEED, _("Target boost"), m_ctrl.control[CT_TARGET_BOOST], m_ri->m_target_boost);
    m_advanced_sizer->Add(m_target_boost_button, 0, wxALL, BORDER);
  }

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
  if (m_ctrl.control[CT_BEARING_ALIGNMENT].type) {
    m_bearing_alignment_button = new RadarControlButton(this, ID_BEARING_ALIGNMENT, _("Bearing alignment"),
                                                        m_ctrl.control[CT_BEARING_ALIGNMENT], m_ri->m_bearing_alignment);
    m_installation_sizer->Add(m_bearing_alignment_button, 0, wxALL, BORDER);
  }

  // The ANTENNA HEIGHT button
  if (m_ctrl.control[CT_ANTENNA_HEIGHT].type) {
    m_antenna_height_button = new RadarControlButton(this, ID_ANTENNA_HEIGHT, _("Bearing alignment"),
                                                     m_ctrl.control[CT_ANTENNA_HEIGHT], m_ri->m_antenna_height);
    m_installation_sizer->Add(m_antenna_height_button, 0, wxALL, BORDER);
  }

  // The ANTENNA FORWARD button
  if (m_ctrl.control[CT_ANTENNA_FORWARD].type) {
    m_antenna_forward_button =
        new RadarControlButton(this, ID_ANTENNA_FORWARD, _("Antenna forward"), m_ctrl.control[CT_ANTENNA_FORWARD],
                               m_ri->m_antenna_forward, _("m"), _("relative to GPS") + wxT("\n") + _("negative = behind"));
    m_installation_sizer->Add(m_antenna_forward_button, 0, wxALL, BORDER);
  }

  // The ANTENNA STARBOARD button
  if (m_ctrl.control[CT_ANTENNA_STARBOARD].type) {
    m_antenna_starboard_button =
        new RadarControlButton(this, ID_ANTENNA_STARBOARD, _("Antenna forward"), m_ctrl.control[CT_ANTENNA_STARBOARD],
                               m_ri->m_antenna_starboard, _("m"), _("relative to GPS") + wxT("\n") + _("negative = port"));
    m_installation_sizer->Add(m_antenna_starboard_button, 0, wxALL, BORDER);
  }

  // The LOCAL INTERFERENCE REJECTION button
  if (m_ctrl.control[CT_LOCAL_INTERFERENCE_REJECTION].type) {
    m_local_interference_rejection_button =
        new RadarControlButton(this, ID_LOCAL_INTERFERENCE_REJECTION, _("Local interference rej."),
                               m_ctrl.control[CT_LOCAL_INTERFERENCE_REJECTION], m_ri->m_local_interference_rejection);
    m_installation_sizer->Add(m_local_interference_rejection_button, 0, wxALL, BORDER);
  }

  // The SIDE LOBE SUPPRESSION button
  if (m_ctrl.control[CT_SIDE_LOBE_SUPPRESSION].type) {
    m_side_lobe_suppression_button =
        new RadarControlButton(this, ID_SIDE_LOBE_SUPPRESSION, _("Side lobe suppression"), m_ctrl.control[CT_SIDE_LOBE_SUPPRESSION],
                               m_ri->m_side_lobe_suppression);
    m_installation_sizer->Add(m_side_lobe_suppression_button, 0, wxALL, BORDER);
  }

  // The MAIN BANG SIZE button
  if (m_ctrl.control[CT_MAIN_BANG_SIZE].type) {
    m_main_bang_size_button = new RadarControlButton(this, ID_MAIN_BANG_SIZE, _("Main bang size"),
                                                     m_ctrl.control[CT_MAIN_BANG_SIZE], m_ri->m_main_bang_size, _("pixels"));
    m_installation_sizer->Add(m_main_bang_size_button, 0, wxALL, BORDER);
  }

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

  m_guard_zone_type->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(ControlsDialog::OnGuardZoneModeClick), NULL,
                             this);

  // Inner and Outer Ranges
  wxStaticText* inner_range_Text = new wxStaticText(this, wxID_ANY, _("Inner range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(inner_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_inner_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_inner_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_inner_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ControlsDialog::OnInner_Range_Value), NULL, this);
  ///   start of copy
  wxStaticText* outer_range_Text = new wxStaticText(this, wxID_ANY, _("Outer range"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(outer_range_Text, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_outer_range = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_outer_range, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_outer_range->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ControlsDialog::OnOuter_Range_Value), NULL, this);

  wxStaticText* pStart_Bearing = new wxStaticText(this, wxID_ANY, _("Start bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pStart_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_start_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_start_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_start_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ControlsDialog::OnStart_Bearing_Value), NULL, this);

  wxStaticText* pEnd_Bearing = new wxStaticText(this, wxID_ANY, _("End bearing"), wxDefaultPosition, wxDefaultSize, 0);
  m_guard_sizer->Add(pEnd_Bearing, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

  m_end_bearing = new wxTextCtrl(this, wxID_ANY);
  m_guard_sizer->Add(m_end_bearing, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);
  m_end_bearing->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ControlsDialog::OnEnd_Bearing_Value), NULL, this);

  // checkbox for ARPA
  m_arpa_box = new wxCheckBox(this, wxID_ANY, _("ARPA On"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
  m_guard_sizer->Add(m_arpa_box, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  m_arpa_box->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(ControlsDialog::OnARPAClick), NULL, this);

  // checkbox for blob alarm
  m_alarm = new wxCheckBox(this, wxID_ANY, _("Alarm On"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
  m_guard_sizer->Add(m_alarm, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
  m_alarm->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(ControlsDialog::OnAlarmClick), NULL, this);

  m_top_sizer->Hide(m_guard_sizer);

  //**************** ADJUST BOX ******************//

  m_adjust_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_adjust_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  // The Back button
  RadarButton* bAdjustBack = new RadarButton(this, ID_BACK, g_buttonSize, backButtonStr);
  m_adjust_sizer->Add(bAdjustBack, 0, wxALL, BORDER);

  // The RANGE button
  if (m_ctrl.control[CT_RANGE].type) {
    m_range_button = new RadarRangeControlButton(this, m_ri, ID_RANGE, g_buttonSize, _("Range"));
    m_adjust_sizer->Add(m_range_button, 0, wxALL, BORDER);
  }

  // The GAIN button
  if (m_ctrl.control[CT_GAIN].type) {
    m_gain_button = new RadarControlButton(this, ID_GAIN, _("Gain"), m_ctrl.control[CT_GAIN], m_ri->m_gain);
    m_adjust_sizer->Add(m_gain_button, 0, wxALL, BORDER);
  }

  // The SEA button
  if (m_ctrl.control[CT_SEA].type) {
    m_sea_button = new RadarControlButton(this, ID_SEA, _("Sea clutter"), m_ctrl.control[CT_SEA], m_ri->m_sea);
    m_adjust_sizer->Add(m_sea_button, 0, wxALL, BORDER);
  }

  // The RAIN button
  if (m_ctrl.control[CT_RAIN].type) {
    m_rain_button = new RadarControlButton(this, ID_RAIN, _("Rain clutter"), m_ctrl.control[CT_RAIN], m_ri->m_rain);
    m_adjust_sizer->Add(m_rain_button, 0, wxALL, BORDER);
  }

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
    m_bearing_buttons[b]->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ControlsDialog::OnBearingSetButtonClick), 0,
                                  this);
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
  if (m_ctrl.control[CT_TRAILS_MOTION].type) {
    m_trails_motion_button = new RadarControlButton(this, ID_TRAILS_MOTION, _("Off/Relative/True trails"),
                                                    m_ctrl.control[CT_TRAILS_MOTION], m_ri->m_trails_motion);
    m_view_sizer->Add(m_trails_motion_button, 0, wxALL, BORDER);
  }

  // The TARGET_TRAIL button
  if (m_ctrl.control[CT_TARGET_TRAILS].type) {
    m_target_trails_button =
        new RadarControlButton(this, ID_TARGET_TRAILS, _("Target trails"), m_ctrl.control[CT_TARGET_TRAILS], m_ri->m_target_trails);
    m_view_sizer->Add(m_target_trails_button, 0, wxALL, BORDER);
    m_target_trails_button->Hide();
  }

  // The Clear Trails button
  m_clear_trails_button = new RadarButton(this, ID_CLEAR_TRAILS, g_buttonSize, _("Clear trails"));
  m_view_sizer->Add(m_clear_trails_button, 0, wxALL, BORDER);
  m_clear_trails_button->Hide();

  // The Rotation button
  m_orientation_button = new RadarButton(this, ID_ORIENTATION, g_buttonSize, _("Orientation"));
  m_view_sizer->Add(m_orientation_button, 0, wxALL, BORDER);
  // Updated when we receive data

  // The REFRESHRATE button
  if (m_ctrl.control[CT_REFRESHRATE].type) {
    m_refresh_rate_button = new RadarControlButton(this, ID_REFRESHRATE, _("Refresh rate"), m_ctrl.control[CT_REFRESHRATE],
                                                   m_pi->m_settings.refreshrate);
    m_view_sizer->Add(m_refresh_rate_button, 0, wxALL, BORDER);
  }

  // The TRANSPARENCY button
  if (m_ctrl.control[CT_TRANSPARENCY].type) {
    m_transparency_button = new RadarControlButton(this, ID_TRANSPARENCY, _("Transparency"), m_ctrl.control[CT_TRANSPARENCY],
                                                   m_pi->m_settings.overlay_transparency);
    m_view_sizer->Add(m_transparency_button, 0, wxALL, BORDER);
  }

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
  if (m_ctrl.control[CT_TIMED_IDLE].type) {
    m_timed_idle_button = new RadarControlButton(this, ID_TIMED_IDLE, _("Timed Transmit"), m_ctrl.control[CT_TIMED_IDLE],
                                                 m_pi->m_settings.timed_idle);
    m_power_sizer->Add(m_timed_idle_button, 0, wxALL, BORDER);
  }

  // The TIMED RUNTIME button
  if (m_ctrl.control[CT_TIMED_IDLE].type) {
    m_timed_run_button =
        new RadarControlButton(this, ID_TIMED_RUN, _("Runtime"), m_ctrl.control[CT_TIMED_RUN], m_pi->m_settings.idle_run_time);
    m_power_sizer->Add(m_timed_run_button, 0, wxALL, BORDER);
  }

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

void ControlsDialog::OnZone1ButtonClick(wxCommandEvent& event) { ShowGuardZone(0); }

void ControlsDialog::OnZone2ButtonClick(wxCommandEvent& event) { ShowGuardZone(1); }

void ControlsDialog::OnClose(wxCloseEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void ControlsDialog::OnIdOKClick(wxCommandEvent& event) { m_pi->OnControlDialogClose(m_ri); }

void ControlsDialog::OnPlusTenClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnPlustTenClick for %s value %d"), m_log_name.c_str(), m_from_control->GetLabel().c_str(),
             m_from_control->value + 10);
  m_from_control->AdjustValue(+10);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void ControlsDialog::OnPlusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(+1);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void ControlsDialog::OnBackClick(wxCommandEvent& event) {
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

void ControlsDialog::OnAutoClick(wxCommandEvent& event) {
  if (m_from_control->autoValues == 1) {
    m_from_control->SetAuto(1);
    m_auto_button->Disable();
  } else if (m_from_control->autoValue < m_from_control->autoValues) {
    m_from_control->SetAuto(m_from_control->autoValue + 1);
  } else {
    m_from_control->SetAuto(0);
  }
}

void ControlsDialog::OnTrailsMotionClick(wxCommandEvent& event) {
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

void ControlsDialog::OnMinusClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-1);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void ControlsDialog::OnMinusTenClick(wxCommandEvent& event) {
  m_from_control->AdjustValue(-10);
  m_auto_button->Enable();

  wxString label = m_from_control->GetLabel();
  m_value_text->SetLabel(label);
}

void ControlsDialog::OnAdjustButtonClick(wxCommandEvent& event) { SwitchTo(m_adjust_sizer, wxT("adjust")); }

void ControlsDialog::OnAdvancedButtonClick(wxCommandEvent& event) { SwitchTo(m_advanced_sizer, wxT("advanced")); }

void ControlsDialog::OnViewButtonClick(wxCommandEvent& event) { SwitchTo(m_view_sizer, wxT("view")); }

void ControlsDialog::OnInstallationButtonClick(wxCommandEvent& event) { SwitchTo(m_installation_sizer, wxT("installation")); }

void ControlsDialog::OnPowerButtonClick(wxCommandEvent& event) { SwitchTo(m_power_sizer, wxT("power")); }

void ControlsDialog::OnPreferencesButtonClick(wxCommandEvent& event) { m_pi->ShowPreferencesDialog(m_pi->m_parent_window); }

void ControlsDialog::OnBearingButtonClick(wxCommandEvent& event) { SwitchTo(m_cursor_sizer, wxT("bearing")); }

void ControlsDialog::OnMessageButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  if (m_pi->m_pMessageBox) {
    m_pi->m_pMessageBox->UpdateMessage(true);
  }
}

void ControlsDialog::OnTargetsButtonClick(wxCommandEvent& event) {
  M_SETTINGS.show_radar_target[m_ri->m_radar] = !(M_SETTINGS.show_radar_target[m_ri->m_radar]);

  UpdateControlValues(false);
}

void ControlsDialog::EnterEditMode(RadarControlButton* button) {
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

void ControlsDialog::OnRadarControlButtonClick(wxCommandEvent& event) {
  EnterEditMode((RadarControlButton*)event.GetEventObject());
}

void ControlsDialog::OnRadarShowButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  bool show = true;

  if (M_SETTINGS.radar_count > 1) {
    if (m_pi->m_settings.show_radar[m_ri->m_radar]) {
      for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
        if (r != m_ri->m_radar && M_SETTINGS.show_radar[r] == true) {
          show = false;
        }
      }
    }
    for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
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

void ControlsDialog::OnRadarOverlayButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();

  int this_radar = m_ri->m_radar;
  int next_radar = (this_radar + 1) % M_SETTINGS.radar_count;

  int other_radar = -1;
  if (M_SETTINGS.radar_count > 1 && !M_SETTINGS.show_radar[this_radar]) {
    for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
      if (next_radar != this_radar && M_SETTINGS.show_radar[r] == false) {
        other_radar = next_radar;
        break;
      }
      next_radar = (next_radar + 1) % M_SETTINGS.radar_count;
    }
  }

  if (m_pi->m_settings.chart_overlay != this_radar) {
    m_pi->m_settings.chart_overlay = this_radar;
  } else if (other_radar > -1) {
    // If no radar window shown, toggle overlay to different radar with hidden window
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

void ControlsDialog::OnRadarGainButtonClick(wxCommandEvent& event) { EnterEditMode((RadarControlButton*)event.GetEventObject()); }

void ControlsDialog::OnTransmitButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();
  m_pi->m_settings.timed_idle.Update(0);
  m_ri->RequestRadarState(RADAR_TRANSMIT);
}

void ControlsDialog::OnStandbyButtonClick(wxCommandEvent& event) {
  SetMenuAutoHideTimeout();
  m_pi->m_settings.timed_idle.Update(0);
  m_ri->RequestRadarState(RADAR_STANDBY);
}

void ControlsDialog::OnClearTrailsButtonClick(wxCommandEvent& event) { m_ri->ClearTrails(); }

void ControlsDialog::OnOrientationButtonClick(wxCommandEvent& event) {
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

void ControlsDialog::OnBearingSetButtonClick(wxCommandEvent& event) {
  int bearing = event.GetId() - ID_BEARING_SET;
  LOG_DIALOG(wxT("%s OnBearingSetButtonClick for bearing #%d"), m_log_name.c_str(), bearing + 1);

  m_ri->SetBearing(bearing);
}

void ControlsDialog::OnClearCursorButtonClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnClearCursorButtonClick"), m_log_name.c_str());
  m_ri->SetMouseVrmEbl(0., nanl(""));
  SwitchTo(m_control_sizer, wxT("main (clear cursor)"));
}

void ControlsDialog::OnAcquireTargetButtonClick(wxCommandEvent& event) {
  Position target_pos;
  target_pos.pos = m_ri->m_mouse_pos;
  LOG_DIALOG(wxT("%s OnAcquireTargetButtonClick mouse=%f/%f"), m_log_name.c_str(), target_pos.pos.lat, target_pos.pos.lon);
  m_ri->m_arpa->AcquireNewMARPATarget(target_pos);
}

void ControlsDialog::OnDeleteTargetButtonClick(wxCommandEvent& event) {
  Position target_pos;
  target_pos.pos = m_ri->m_mouse_pos;
  LOG_DIALOG(wxT("%s OnDeleteTargetButtonClick mouse=%f/%f"), m_log_name.c_str(), target_pos.pos.lat, target_pos.pos.lon);
  m_ri->m_arpa->DeleteTarget(target_pos);
}

void ControlsDialog::OnDeleteAllTargetsButtonClick(wxCommandEvent& event) {
  LOG_DIALOG(wxT("%s OnDeleteAllTargetsButtonClick"), m_log_name.c_str());
  for (size_t i = 0; i < M_SETTINGS.radar_count; i++) {
    if (m_pi->m_radar[i]->m_arpa) {
      m_pi->m_radar[i]->m_arpa->DeleteAllTargets();
    }
  }
}

void ControlsDialog::OnMove(wxMoveEvent& event) {
  m_manually_positioned = true;
  event.Skip();
}

void ControlsDialog::UpdateControlValues(bool refreshAll) {
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
  if (m_pi->m_settings.timed_idle.GetValue() == 0) {
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
    if (m_pi->m_settings.timed_idle.GetValue() > 0) {
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

  if (M_SETTINGS.radar_count > 1) {
    bool show_other_radar = false;
    if (m_pi->m_settings.show_radar[m_ri->m_radar]) {
      for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
        if (r != m_ri->m_radar && M_SETTINGS.show_radar[r] == true) {
          show_other_radar = true;
        }
      }
    }

    if (m_pi->m_settings.show_radar[m_ri->m_radar]) {
      if (show_other_radar) {
        o = _("Hide all PPI windows");
      } else {
        o = _("Show other PPI windows");
      }
    } else {
      if (show_other_radar) {
        o = _("Show PPI window");  // can happen if this window hidden but control is for overlay
      } else {
        o = _("Show all PPI windows");
      }
    }
  } else {
    o = (m_pi->m_settings.show_radar[m_ri->m_radar]) ? _("Hide PPI") : _("Show PPI");
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
    if (overlay) {
      if (M_SETTINGS.show_radar[m_ri->m_radar]) {
        o << _("On");
      } else {
        o << m_ri->m_name;
      }
    } else {
      o << _("Off");
    }
    m_overlay_button->SetLabel(o);
  }

  if (m_range_button && (m_ri->m_range.IsModified() || refreshAll)) {
    m_ri->m_range.GetButton();
    m_range_button->SetRangeLabel();
  }

  // gain
  if (m_gain_button && (m_ri->m_gain.IsModified() || refreshAll)) {
    int button = m_ri->m_gain.GetButton();
    m_gain_button->SetLocalValue(button);
  }

  //  rain
  if (m_rain_button && (m_ri->m_rain.IsModified() || refreshAll)) {
    m_rain_button->SetLocalValue(m_ri->m_rain.GetButton());
  }

  //   sea
  if (m_sea_button && (m_ri->m_sea.IsModified() || refreshAll)) {
    int button = m_ri->m_sea.GetButton();
    m_sea_button->SetLocalValue(button);
  }

  //   target_boost
  if (m_target_boost_button && (m_ri->m_target_boost.IsModified() || refreshAll)) {
    m_target_boost_button->SetLocalValue(m_ri->m_target_boost.GetButton());
  }

  //   target_expansion
  if (m_target_expansion_button && (m_ri->m_target_expansion.IsModified() || refreshAll)) {
    m_target_expansion_button->SetLocalValue(m_ri->m_target_expansion.GetButton());
  }

  //  noise_rejection
  if (m_noise_rejection_button && (m_ri->m_noise_rejection.IsModified() || refreshAll)) {
    m_noise_rejection_button->SetLocalValue(m_ri->m_noise_rejection.GetButton());
  }

  //  target_separation
  if (m_target_separation_button && (m_ri->m_target_separation.IsModified() || refreshAll)) {
    m_target_separation_button->SetLocalValue(m_ri->m_target_separation.GetButton());
  }

  //  interference_rejection
  if (m_interference_rejection_button && (m_ri->m_interference_rejection.IsModified() || refreshAll)) {
    m_interference_rejection_button->SetLocalValue(m_ri->m_interference_rejection.GetButton());
  }

  // scanspeed
  if (m_scan_speed_button && (m_ri->m_scan_speed.IsModified() || refreshAll)) {
    m_scan_speed_button->SetLocalValue(m_ri->m_scan_speed.GetButton());
  }

  //   antenna height
  if (m_antenna_height_button && (m_ri->m_antenna_height.IsModified() || refreshAll)) {
    m_antenna_height_button->SetLocalValue(m_ri->m_antenna_height.GetButton());
  }

  //  bearing alignment
  if (m_bearing_alignment_button && (m_ri->m_bearing_alignment.IsModified() || refreshAll)) {
    m_bearing_alignment_button->SetLocalValue(m_ri->m_bearing_alignment.GetButton());
  }

  //  local interference rejection
  if (m_local_interference_rejection_button && (m_ri->m_local_interference_rejection.IsModified() || refreshAll)) {
    m_local_interference_rejection_button->SetLocalValue(m_ri->m_local_interference_rejection.GetButton());
  }

  // side lobe suppression
  if (m_side_lobe_suppression_button && (m_ri->m_side_lobe_suppression.IsModified() || refreshAll)) {
    int button = m_ri->m_side_lobe_suppression.GetButton();
    m_side_lobe_suppression_button->SetLocalValue(button);
  }

  if (m_main_bang_size_button && (m_ri->m_main_bang_size.IsModified() || refreshAll)) {
    m_main_bang_size_button->SetLocalValue(m_ri->m_main_bang_size.GetButton());
  }
  if (m_antenna_starboard_button && (m_ri->m_antenna_starboard.IsModified() || refreshAll)) {
    m_antenna_starboard_button->SetLocalValue(m_ri->m_antenna_starboard.GetButton());
  }
  if (m_antenna_forward_button && (m_ri->m_antenna_forward.IsModified() || refreshAll)) {
    m_antenna_forward_button->SetLocalValue(m_ri->m_antenna_forward.GetButton());
  }

  if (refreshAll) {
    // Update all buttons set from plugin settings
    if (m_transparency_button) {
      m_transparency_button->SetLocalValue(M_SETTINGS.overlay_transparency.GetButton());
    }
    if (m_timed_idle_button) {
      m_timed_idle_button->SetLocalValue(M_SETTINGS.timed_idle.GetButton());
    }
    if (m_timed_run_button) {
      m_timed_run_button->SetLocalValue(M_SETTINGS.idle_run_time.GetButton());
    }
    if (m_refresh_rate_button) {
      m_refresh_rate_button->SetLocalValue(M_SETTINGS.refreshrate.GetButton());
    }
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

void ControlsDialog::UpdateDialogShown() {
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

void ControlsDialog::HideTemporarily() {
  m_hide_temporarily = true;
  UpdateDialogShown();
}

void ControlsDialog::UnHideTemporarily() {
  m_hide_temporarily = false;
  SetMenuAutoHideTimeout();
  UpdateDialogShown();
}

void ControlsDialog::ShowDialog() {
  m_hide = false;
  UnHideTemporarily();
  UpdateControlValues(true);
}

void ControlsDialog::HideDialog() {
  m_hide = true;
  m_auto_hide_timeout = 0;
  UpdateDialogShown();
}

void ControlsDialog::OnGuardZoneModeClick(wxCommandEvent& event) { SetGuardZoneVisibility(); }

void ControlsDialog::OnInner_Range_Value(wxCommandEvent& event) {
  wxString temp = m_inner_range->GetValue();
  double t;
  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->SetInnerRange((int)(t * conversionFactor));
}

void ControlsDialog::OnOuter_Range_Value(wxCommandEvent& event) {
  wxString temp = m_outer_range->GetValue();
  double t;
  m_guard_zone->m_show_time = time(0);
  temp.ToDouble(&t);

  int conversionFactor = RangeUnitsToMeters[m_pi->m_settings.range_units];

  m_guard_zone->SetOuterRange((int)(t * conversionFactor));
}

void ControlsDialog::OnStart_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_start_bearing->GetValue();
  long t;

  m_guard_zone->m_show_time = time(0);

  temp.ToLong(&t);
  t = MOD_DEGREES(t);
  while (t < 0) {
    t += 360;
  }
  m_guard_zone->SetStartBearing(t);
}

void ControlsDialog::OnEnd_Bearing_Value(wxCommandEvent& event) {
  wxString temp = m_end_bearing->GetValue();
  long t;

  m_guard_zone->m_show_time = time(0);

  temp.ToLong(&t);
  t = MOD_DEGREES(t);
  while (t < 0) {
    t += 360;
  }
  m_guard_zone->SetEndBearing(t);
}

void ControlsDialog::OnARPAClick(wxCommandEvent& event) {
  int arpa = m_arpa_box->GetValue();
  m_guard_zone->SetArpaOn(arpa);
}

void ControlsDialog::OnAlarmClick(wxCommandEvent& event) {
  int alarm = m_alarm->GetValue();
  m_guard_zone->SetAlarmOn(alarm);
}

PLUGIN_END_NAMESPACE
