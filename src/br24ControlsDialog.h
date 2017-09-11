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

#ifndef _BR24CONTROLSDIALOG_H_
#define _BR24CONTROLSDIALOG_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class br24RadarControlButton;
class br24RadarRangeControlButton;

#define OFFSCREEN_CONTROL_X (-10000)
#define OFFSCREEN_CONTROL_Y (-10000)

#define AUTO_RANGE (-20000)  // Auto values are -20000 - auto_index

const static wxPoint OFFSCREEN_CONTROL = wxPoint(OFFSCREEN_CONTROL_X, OFFSCREEN_CONTROL_Y);

//----------------------------------------------------------------------------------------------------------
//    BR24Radar Control Dialog Specification
//----------------------------------------------------------------------------------------------------------
class br24ControlsDialog : public wxDialog {
  DECLARE_CLASS(br24ControlsDialog)
  DECLARE_EVENT_TABLE()

 public:
  br24ControlsDialog();

  ~br24ControlsDialog();
  void Init();

  bool Create(wxWindow *parent, br24radar_pi *pi, RadarInfo *ri, wxWindowID id = wxID_ANY, const wxString &caption = _("Radar"),
              const wxPoint &pos = OFFSCREEN_CONTROL);

  void CreateControls();
  void AdjustRange(int adjustment);
  wxString &GetRangeText();
  void SetTimedIdleIndex(int index);
  void UpdateGuardZoneState();
  void UpdateDialogShown();
  void UpdateControlValues(bool refreshAll);
  void SetErrorMessage(wxString &msg);
  void ShowBogeys(wxString text, bool confirmed);
  void HideBogeys();

  void HideTemporarily();  // When a second dialog (which is not a child class) takes over -- aka GuardZone
  void UnHideTemporarily();
  void ShowDialog();
  void HideDialog();
  void ShowCursorPane() { SwitchTo(m_cursor_sizer, wxT("cursor")); }

  br24radar_pi *m_pi;
  RadarInfo *m_ri;
  wxString m_log_name;
  wxBoxSizer *m_top_sizer;
  wxBoxSizer *m_control_sizer;
  wxPoint m_panel_position;
  bool m_manually_positioned;

 private:
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

  void EnterEditMode(br24RadarControlButton *button);

  void SwitchTo(wxBoxSizer *to, const wxChar *name);
  void UpdateAdvanced4GState();
  void UpdateTrailsState();

  void SetMenuAutoHideTimeout();

  void EnsureWindowNearOpenCPNWindow();

  wxWindow *m_parent;
  wxBoxSizer *m_advanced_4G_sizer;
  wxBoxSizer *m_advanced_sizer;
  wxBoxSizer *m_view_sizer;
  wxBoxSizer *m_edit_sizer;
  wxBoxSizer *m_installation_sizer;
  wxBoxSizer *m_guard_sizer;
  wxBoxSizer *m_adjust_sizer;
  wxBoxSizer *m_cursor_sizer;
  wxBoxSizer *m_power_sizer;
  wxBoxSizer *m_transmit_sizer;  // Controls disabled if not transmitting
  wxBoxSizer *m_from_sizer;      // If on edit control, this is where the button is from

  bool m_hide;
  bool m_hide_temporarily;
  time_t m_auto_hide_timeout;  // At what time do we hide the dialog

  // Edit Controls

  br24RadarControlButton *m_from_control;  // Only set when in edit mode

  // The 'edit' control has these buttons:
  wxButton *m_plus_ten_button;
  wxButton *m_plus_button;
  wxStaticText *m_value_text;
  wxStaticText *m_comment_text;
  wxButton *m_minus_button;
  wxButton *m_minus_ten_button;
  wxButton *m_auto_button;

  // Advanced controls
  br24RadarControlButton *m_interference_rejection_button;
  br24RadarControlButton *m_target_separation_button;
  br24RadarControlButton *m_noise_rejection_button;
  br24RadarControlButton *m_target_boost_button;
  br24RadarControlButton *m_target_expansion_button;
  br24RadarControlButton *m_scan_speed_button;

  // Installation controls
  br24RadarControlButton *m_bearing_alignment_button;
  br24RadarControlButton *m_antenna_height_button;
  br24RadarControlButton *m_antenna_forward_button;
  br24RadarControlButton *m_antenna_starboard_button;
  br24RadarControlButton *m_local_interference_rejection_button;
  br24RadarControlButton *m_side_lobe_suppression_button;
  br24RadarControlButton *m_main_bang_size_button;

  // Bearing controls
  wxButton *m_bearing_buttons[BEARING_LINES];
  wxButton *m_clear_cursor;
  wxButton *m_acquire_target;
  wxButton *m_delete_target;
  wxButton *m_delete_all;

  // View controls
  br24RadarControlButton *m_target_trails_button;
  wxButton *m_targets_button;
  wxButton *m_trails_motion_button;
  wxButton *m_clear_trails_button;
  wxButton *m_orientation_button;

  // Power controls
  wxStaticText *m_power_text;
  wxButton *m_transmit_button;
  wxButton *m_standby_button;
  br24RadarControlButton *m_timed_idle_button;
  br24RadarControlButton *m_timed_run_button;

  // Show Controls

  wxButton *m_overlay_button;
  wxButton *m_window_button;
  br24RadarRangeControlButton *m_range_button;
  br24RadarControlButton *m_transparency_button;  // TODO: Set it on change
  br24RadarControlButton *m_refresh_rate_button;  // TODO: Set it on change
  br24RadarControlButton *m_gain_button;
  br24RadarControlButton *m_sea_button;
  br24RadarControlButton *m_rain_button;
  wxButton *m_adjust_button;
  wxButton *m_bearing_button;
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

  void ShowGuardZone(int zone);
  void SetGuardZoneVisibility();
  void OnGuardZoneModeClick(wxCommandEvent &event);
  void OnInner_Range_Value(wxCommandEvent &event);
  void OnOuter_Range_Value(wxCommandEvent &event);
  void OnStart_Bearing_Value(wxCommandEvent &event);
  void OnEnd_Bearing_Value(wxCommandEvent &event);
  void OnARPAClick(wxCommandEvent &event);
  void OnAlarmClick(wxCommandEvent &event);
};

PLUGIN_END_NAMESPACE

#endif
