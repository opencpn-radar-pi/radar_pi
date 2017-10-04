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

#ifndef _NAVICOCONTROLSDIALOG_H_
#define _NAVICOCONTROLSDIALOG_H_

#include "ControlsDialog.h"

PLUGIN_BEGIN_NAMESPACE

//----------------------------------------------------------------------------------------------------------
//    Radar Control Dialog Specification
//----------------------------------------------------------------------------------------------------------
class NavicoControlsDialog : public ControlsDialog {
  DECLARE_CLASS(NavicoControlsDialog)
  DECLARE_EVENT_TABLE()

 public:
  NavicoControlsDialog(){};

  ~NavicoControlsDialog(){};
  void Init();

  bool Create(wxWindow *parent, radar_pi *pi, RadarInfo *ri, wxWindowID id = wxID_ANY, const wxString &caption = _("Radar"),
              const wxPoint &pos = OFFSCREEN_CONTROL);

  void AdjustRange(int adjustment);
  wxString &GetRangeText();
  void SetTimedIdleIndex(int index);
  void SetErrorMessage(wxString &msg);
  void ShowBogeys(wxString text, bool confirmed);
  void HideBogeys();

  void HideTemporarily();  // When a second dialog (which is not a child class) takes over -- aka GuardZone
  void UnHideTemporarily();
  void ShowDialog();
  void HideDialog();

  void UpdateDialogShown();
  void UpdateControlValues(bool force);

 private:
  void CreateControls();

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

  void UpdateRadarSpecificState();

  wxBoxSizer *m_advanced_4G_sizer;
  wxBoxSizer *m_installation_sizer;

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

  // Show Controls

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
