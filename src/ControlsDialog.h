/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
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

#ifndef _CONTROLSDIALOG_H_
#define _CONTROLSDIALOG_H_

#include "radar_control_item.h"
#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

#define OFFSCREEN_CONTROL_X (-10000)
#define OFFSCREEN_CONTROL_Y (-10000)

#define AUTO_RANGE (-20000)  // Auto values are -20000 - auto_index

const static wxPoint OFFSCREEN_CONTROL = wxPoint(OFFSCREEN_CONTROL_X, OFFSCREEN_CONTROL_Y);

class RadarControlButton;
class RadarRangeControlButton;
class RadarButton;

extern wxString guard_zone_names[2];

extern string ControlTypeNames[CT_MAX];

extern wxSize g_buttonSize;

struct ControlInfo {
  ControlType type;
  int autoValues;
  wxString *autoNames;
  int defaultValue;
  int minValue;
  int maxValue;
  int stepValue;
  int nameCount;
  wxString *names;
};

struct ControlSet {
  ControlInfo control[CT_MAX];
};

//----------------------------------------------------------------------------------------------------------
//    Radar Control Dialog Specification
//----------------------------------------------------------------------------------------------------------
class ControlsDialog : public wxDialog {
  DECLARE_CLASS(ControlsDialog)
  DECLARE_EVENT_TABLE()

 public:
  ControlsDialog() {
    // Initialize all members that need initialization
    m_hide = false;
    m_hide_temporarily = true;

    m_from_control = 0;

    m_panel_position = wxDefaultPosition;
    m_manually_positioned = false;

    m_pi = 0;
    m_ri = 0;
    m_top_sizer = 0;
    m_control_sizer = 0;
    m_parent = 0;
    m_advanced_sizer = 0;
    m_view_sizer = 0;
    m_edit_sizer = 0;
    m_guard_sizer = 0;
    m_adjust_sizer = 0;
    m_cursor_sizer = 0;
    m_installation_sizer = 0;
    m_power_sizer = 0;
    m_transmit_sizer = 0;  // Controls disabled if not transmitting
    m_from_sizer = 0;      // If on edit control, this is where the button is from
    m_from_control = 0;    // Only set when in edit mode
    m_plus_ten_button = 0;
    m_plus_button = 0;
    m_value_text = 0;
    m_comment_text = 0;
    m_minus_button = 0;
    m_minus_ten_button = 0;
    m_auto_button = 0;
    m_power_button = 0;
    m_guard_zone = 0;
    m_guard_zone_text = 0;
    m_guard_zone_type = 0;
    m_outer_range = 0;
    m_inner_range = 0;
    m_start_bearing = 0;
    m_end_bearing = 0;
    m_arpa_box = 0;
    m_alarm = 0;
    CLEAR_STRUCT(m_bearing_buttons);
    m_clear_cursor = 0;
    m_acquire_target = 0;
    m_delete_target = 0;
    m_delete_all = 0;
    m_target_trails_button = 0;
    m_targets_button = 0;
    m_trails_motion_button = 0;
    m_clear_trails_button = 0;
    m_orientation_button = 0;
    m_power_text = 0;
    m_transmit_button = 0;
    m_standby_button = 0;
    m_timed_idle_button = 0;
    m_timed_run_button = 0;
    m_interference_rejection_button = 0;
    m_target_separation_button = 0;
    m_noise_rejection_button = 0;
    m_target_boost_button = 0;
    m_target_expansion_button = 0;
    m_scan_speed_button = 0;
    m_bearing_alignment_button = 0;
    m_antenna_height_button = 0;
    m_antenna_forward_button = 0;
    m_antenna_starboard_button = 0;
    m_local_interference_rejection_button = 0;
    m_side_lobe_suppression_button = 0;
    m_main_bang_size_button = 0;
    m_overlay_button = 0;
    m_window_button = 0;
    m_range_button = 0;
    m_transparency_button = 0;  // TODO: Set it on change
    m_refresh_rate_button = 0;  // TODO: Set it on change
    m_gain_button = 0;
    m_sea_button = 0;
    m_rain_button = 0;
    m_adjust_button = 0;
    m_bearing_button = 0;

    CLEAR_STRUCT(m_ctrl);
  };
  ~ControlsDialog();

  bool Create(wxWindow *parent, radar_pi *pi, RadarInfo *ri, wxWindowID id = wxID_ANY, const wxString &caption = _("Radar"),
              const wxPoint &pos = OFFSCREEN_CONTROL);

  void AdjustRange(int adjustment);
  wxString &GetRangeText();
  void SetTimedIdleIndex(int index);
  void UpdateGuardZoneState();
  void UpdateDialogShown();
  void UpdateControlValues(bool force);
  void SetErrorMessage(wxString &msg);
  void ShowBogeys(wxString text, bool confirmed);
  void HideBogeys();

  void HideTemporarily();  // When a second dialog (which is not a child class) takes over -- aka GuardZone
  void UnHideTemporarily();
  void ShowDialog();
  void HideDialog();
  void ShowCursorPane() { SwitchTo(m_cursor_sizer, wxT("cursor")); }

  radar_pi *m_pi;
  RadarInfo *m_ri;
  wxString m_log_name;
  wxBoxSizer *m_top_sizer;
  wxBoxSizer *m_control_sizer;
  wxPoint m_panel_position;
  bool m_manually_positioned;

 protected:
  ControlSet m_ctrl;
  void DefineControl(ControlType ct, int autoValues, wxString auto_names[], int defaultValue, int minValue, int maxValue,
                     int stepValue, int nameCount, wxString names[]) {
    m_ctrl.control[ct].type = ct;
    m_ctrl.control[ct].autoValues = autoValues;
    m_ctrl.control[ct].defaultValue = defaultValue;
    m_ctrl.control[ct].minValue = minValue;
    m_ctrl.control[ct].maxValue = maxValue;
    m_ctrl.control[ct].stepValue = stepValue;
    m_ctrl.control[ct].nameCount = nameCount;
    if (autoValues > 0) {
      m_ctrl.control[ct].autoNames = new wxString[autoValues];
      for (int i = 0; i < autoValues; i++) {
        m_ctrl.control[ct].autoNames[i] = auto_names[i];
      }
    }
    if (nameCount > 0) {
      m_ctrl.control[ct].names = new wxString[nameCount];
      for (int i = 0; i < nameCount; i++) {
        m_ctrl.control[ct].names[i] = names[i];
      }
    }
  }

  bool m_hide;
  bool m_hide_temporarily;
  time_t m_auto_hide_timeout;  // At what time do we hide the dialog

  wxWindow *m_parent;
  wxBoxSizer *m_advanced_sizer;
  wxBoxSizer *m_view_sizer;
  wxBoxSizer *m_edit_sizer;
  wxBoxSizer *m_guard_sizer;
  wxBoxSizer *m_adjust_sizer;
  wxBoxSizer *m_cursor_sizer;
  wxBoxSizer *m_installation_sizer;
  wxBoxSizer *m_power_sizer;
  wxBoxSizer *m_transmit_sizer;  // Controls disabled if not transmitting
  wxBoxSizer *m_from_sizer;      // If on edit control, this is where the button is from

  // Edit Controls
  RadarControlButton *m_from_control;  // Only set when in edit mode

  // The 'edit' control has these buttons:
  wxButton *m_plus_ten_button;
  wxButton *m_plus_button;
  wxStaticText *m_value_text;
  wxStaticText *m_comment_text;
  wxButton *m_minus_button;
  wxButton *m_minus_ten_button;
  wxButton *m_auto_button;

  // Main control
  wxButton *m_guard_1_button;
  wxButton *m_guard_2_button;
  wxButton *m_power_button;

  // Guard Zone Edit

  GuardZone *m_guard_zone;
  wxStaticText *m_guard_zone_text;
  wxRadioBox *m_guard_zone_type;
  wxTextCtrl *m_outer_range;
  wxTextCtrl *m_inner_range;
  wxTextCtrl *m_start_bearing;
  wxTextCtrl *m_end_bearing;
  wxCheckBox *m_arpa_box;
  wxCheckBox *m_alarm;

  // Cursor controls
  wxButton *m_bearing_buttons[BEARING_LINES];
  wxButton *m_clear_cursor;
  wxButton *m_acquire_target;
  wxButton *m_delete_target;
  wxButton *m_delete_all;

  // View controls
  RadarControlButton *m_target_trails_button;
  wxButton *m_targets_button;
  wxButton *m_trails_motion_button;
  wxButton *m_clear_trails_button;
  wxButton *m_orientation_button;

  // Power controls
  wxStaticText *m_power_text;
  wxButton *m_transmit_button;
  wxButton *m_standby_button;
  RadarControlButton *m_timed_idle_button;
  RadarControlButton *m_timed_run_button;

  // Advanced controls
  RadarControlButton *m_interference_rejection_button;
  RadarControlButton *m_target_separation_button;
  RadarControlButton *m_noise_rejection_button;
  RadarControlButton *m_target_boost_button;
  RadarControlButton *m_target_expansion_button;
  RadarControlButton *m_scan_speed_button;

  // Installation controls
  RadarControlButton *m_bearing_alignment_button;
  RadarControlButton *m_antenna_height_button;
  RadarControlButton *m_antenna_forward_button;
  RadarControlButton *m_antenna_starboard_button;
  RadarControlButton *m_local_interference_rejection_button;
  RadarControlButton *m_side_lobe_suppression_button;
  RadarControlButton *m_main_bang_size_button;

  // Adjust controls
  wxButton *m_overlay_button;
  wxButton *m_window_button;
  RadarRangeControlButton *m_range_button;
  RadarControlButton *m_transparency_button;  // TODO: Set it on change
  RadarControlButton *m_refresh_rate_button;  // TODO: Set it on change
  RadarControlButton *m_gain_button;
  RadarControlButton *m_sea_button;
  RadarControlButton *m_rain_button;
  wxButton *m_adjust_button;
  wxButton *m_bearing_button;

  // Methods common to any radar
  void EnsureWindowNearOpenCPNWindow();
  void ShowGuardZone(int zone);
  void SetGuardZoneVisibility();
  void SetMenuAutoHideTimeout();
  void SwitchTo(wxBoxSizer *to, const wxChar *name);
  void UpdateTrailsState();

  void CreateControls();

  // Methods that we know that every radar must or may implement its own way
  virtual void UpdateRadarSpecificState(){};

  // Callbacks when a button is pressed

  void OnClose(wxCloseEvent &event);
  void OnIdOKClick(wxCommandEvent &event);
  void OnMove(wxMoveEvent &event);

  void OnPlusTenClick(wxCommandEvent &event);
  void OnPlusClick(wxCommandEvent &event);
  void OnBackClick(wxCommandEvent &event);
  void OnMinusClick(wxCommandEvent &event);
  void OnMinusTenClick(wxCommandEvent &event);
  void OnAutoClick(wxCommandEvent &event);
  void OnTrailsMotionClick(wxCommandEvent &event);

  void OnAdjustButtonClick(wxCommandEvent &event);
  void OnAdvancedButtonClick(wxCommandEvent &event);
  void OnViewButtonClick(wxCommandEvent &event);
  void OnInstallationButtonClick(wxCommandEvent &event);
  void OnPreferencesButtonClick(wxCommandEvent &event);

  void OnRadarGainButtonClick(wxCommandEvent &event);

  void OnPowerButtonClick(wxCommandEvent &event);
  void OnRadarShowButtonClick(wxCommandEvent &event);
  void OnRadarOverlayButtonClick(wxCommandEvent &event);
  void OnMessageButtonClick(wxCommandEvent &event);

  void OnTargetsButtonClick(wxCommandEvent &event);
  void OnClearTrailsButtonClick(wxCommandEvent &event);
  void OnOrientationButtonClick(wxCommandEvent &event);

  void OnRadarControlButtonClick(wxCommandEvent &event);

  void OnZone1ButtonClick(wxCommandEvent &event);
  void OnZone2ButtonClick(wxCommandEvent &event);

  void OnClearCursorButtonClick(wxCommandEvent &event);
  void OnAcquireTargetButtonClick(wxCommandEvent &event);
  void OnDeleteTargetButtonClick(wxCommandEvent &event);
  void OnDeleteAllTargetsButtonClick(wxCommandEvent &event);
  void OnBearingSetButtonClick(wxCommandEvent &event);
  void OnBearingButtonClick(wxCommandEvent &event);

  void OnConfirmBogeyButtonClick(wxCommandEvent &event);

  void OnTransmitButtonClick(wxCommandEvent &event);
  void OnStandbyButtonClick(wxCommandEvent &event);

  void EnterEditMode(RadarControlButton *button);

  void OnGuardZoneModeClick(wxCommandEvent &event);
  void OnInner_Range_Value(wxCommandEvent &event);
  void OnOuter_Range_Value(wxCommandEvent &event);
  void OnStart_Bearing_Value(wxCommandEvent &event);
  void OnEnd_Bearing_Value(wxCommandEvent &event);
  void OnARPAClick(wxCommandEvent &event);
  void OnAlarmClick(wxCommandEvent &event);
};

class RadarButton : public wxButton {
 public:
  RadarButton(){

  };

  RadarButton(ControlsDialog *parent, wxWindowID id, wxSize buttonSize, const wxString &label) {
    Create(parent, id, label, wxDefaultPosition, buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;
    SetFont(m_parent->m_pi->m_font);
    SetLabel(label);  // Use the \n on Mac to enforce double height button
  }

  ControlsDialog *m_parent;
  radar_pi *m_pi;

  void SetLabel(const wxString &label) {
    wxString newLabel;

#ifdef __WXOSX__
    newLabel << wxT("\n");
#endif
    newLabel << label;
#ifdef __WXOSX__
    newLabel << wxT("\n");
#endif
    wxButton::SetLabel(newLabel);
  }
};

class DynamicStaticText : public wxStaticText {
 public:
  DynamicStaticText() {}

  DynamicStaticText(wxWindow *parent, wxWindowID id, const wxString &label, const wxPoint &pos = wxDefaultPosition,
                    const wxSize &size = wxDefaultSize, long style = 0, const wxString &name = wxStaticTextNameStr) {
    Create(parent, id, label, pos, size, style, name);
  }

  void SetLabel(const wxString &label) {
    wxStaticText::SetLabel(label);
    SetSize(GetTextExtent(label));
  }
};

class RadarControlButton : public wxButton {
 public:
  RadarControlButton(){

  };

  RadarControlButton(ControlsDialog *parent, wxWindowID id, wxSize buttonSize, const wxString &label, ControlType ct,
                     bool newHasAuto, int newValue, const wxString &newUnit = wxT(""), const wxString &newComment = wxT("")) {
    Create(parent, id, label + wxT("\n"), wxDefaultPosition, buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;
    minValue = 0;
    maxValue = 100;
    value = 0;
    if (ct == CT_GAIN) {
      value = 50;
    }
    autoValue = 0;
    autoValues = newHasAuto ? 1 : 0;
    autoNames = 0;
    firstLine = label;
    unit = newUnit;
    comment = newComment;
    names = 0;
    controlType = ct;
    if (autoValues > 0) {
      SetLocalAuto(AUTO_RANGE - 1);  // Not sent to radar, radar will update state
    } else {
      SetLocalValue(newValue);
    }

    this->SetFont(m_parent->m_pi->m_font);
  }

  RadarControlButton(ControlsDialog *parent, wxWindowID id, const wxString &label, ControlInfo &ctrl, radar_control_item &item,
                     const wxString &newUnit = wxT(""), const wxString &newComment = wxT("")) {
    Create(parent, id, label + wxT("\n"), wxDefaultPosition, g_buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;

    value = ctrl.defaultValue;
    autoValues = ctrl.autoValues;
    autoNames = ctrl.autoNames;
    minValue = ctrl.minValue;
    maxValue = ctrl.maxValue;
    if (ctrl.nameCount > 1) {
      names = ctrl.names;
    } else if (ctrl.nameCount == 1 && ctrl.names[0].length() > 0) {
      unit = ctrl.names[0];
      names = 0;
    } else {
      names = 0;
    }
    autoValue = 0;
    firstLine = label;
    if (newUnit.length() > 0) {
      unit = newUnit;
    }
    comment = newComment;
    controlType = ctrl.type;

    SetLocalValue(item.GetButton());

    this->SetFont(m_parent->m_pi->m_font);
    // SetLocalValue(item.GetButton());
  }

  virtual void AdjustValue(int adjustment);
  virtual void SetAuto(int newValue);
  virtual void SetLocalValue(int newValue);
  virtual void SetLocalAuto(int newValue);
  const wxString *names;
  const wxString *autoNames;
  wxString unit;
  wxString comment;

  wxString firstLine;

  ControlsDialog *m_parent;
  radar_pi *m_pi;

  int value;
  int autoValue;   // 0 = not auto mode, 1 = normal auto value, 2... etc special, auto_names is set
  int autoValues;  // 0 = none, 1 = normal auto value, 2.. etc special, auto_names is set

  int minValue;
  int maxValue;
  ControlType controlType;
};

class RadarRangeControlButton : public RadarControlButton {
 public:
  RadarRangeControlButton(ControlsDialog *parent, RadarInfo *ri, wxWindowID id, wxSize buttonSize, const wxString &label) {
    Create(parent, id, label + wxT("\n"), wxDefaultPosition, buttonSize, 0, wxDefaultValidator, label);

    m_parent = parent;
    m_pi = m_parent->m_pi;
    m_ri = ri;
    minValue = 0;
    maxValue = 0;
    value = -1;  // means: never set
    autoValue = 0;
    autoValues = 1;
    autoNames = 0;
    unit = wxT("");
    firstLine = label;
    names = 0;
    controlType = CT_RANGE;

    this->SetFont(m_parent->m_pi->m_font);
  }

  virtual void AdjustValue(int adjustment);
  virtual void SetAuto(int newValue);
  void SetRangeLabel();

 private:
  RadarInfo *m_ri;
};

// This sets up the initializer macro in the constructor of the
// derived control dialogs.
#define HAVE_CONTROL(a, b, c, d, e, f, g) \
  wxString a##_auto_names[] = b;          \
  wxString a##_names[] = g;               \
  DefineControl(a, ARRAY_SIZE(a##_auto_names), a##_auto_names, c, d, e, f, ARRAY_SIZE(a##_names), a##_names);
#define SKIP_CONTROL(a)

PLUGIN_END_NAMESPACE

#endif
