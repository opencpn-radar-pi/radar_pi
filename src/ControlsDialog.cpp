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

// The following are only for logging, so don't care about translations.
string ControlTypeNames[CT_MAX] = {
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

ControlsDialog::ControlsDialog() { Init(); }

ControlsDialog::~ControlsDialog() {
  wxPoint pos = GetPosition();

  LOG_DIALOG(wxT("%s saved position %d,%d"), m_log_name.c_str(), pos.x, pos.y);
  m_pi->m_settings.control_pos[m_ri->m_radar] = pos;
}

void ControlsDialog::Init() {
  // Initialize all members that need initialization
  m_hide = false;
  m_hide_temporarily = true;

  m_from_control = 0;

  m_panel_position = wxDefaultPosition;
  m_manually_positioned = false;
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

  return wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle);
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

  double bearing = SCALE_RAW_TO_DEGREES2048(m_guard_zone->m_start_bearing);
  if (bearing >= 180.0) {
    bearing -= 360.;
  }
  bearing = round(bearing);
  m_start_bearing->SetValue(wxString::Format(wxT("%3.0f"), bearing));

  bearing = SCALE_RAW_TO_DEGREES2048(m_guard_zone->m_end_bearing);
  if (bearing >= 180.0) {
    bearing -= 360.;
  }
  bearing = round(bearing);
  m_end_bearing->SetValue(wxString::Format(wxT("%3.0f"), bearing));
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

PLUGIN_END_NAMESPACE
