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

#ifndef _BR24CONTROLSDIALOG_H_
#define _BR24CONTROLSDIALOG_H_

#include "br24radar_pi.h"

class br24RadarControlButton;
class br24RadarRangeControlButton;

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
              const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize,
              long style = wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX));

  void CreateControls();
  void SetRemoteRangeIndex(size_t index);
  void SetRangeIndex(size_t index);
  wxString &GetRangeText();
  void SetTimedIdleIndex(int index);
  void UpdateGuardZoneState();
  void UpdateDialogShown();
  void UpdateControlValues(bool refreshAll);
  void SetErrorMessage(wxString &msg);

  void HideTemporarily();  // When a second dialog (which is not a child class) takes over -- aka GuardZone
  void UnHideTemporarily();
  void ShowDialog();
  void HideDialog();

  br24radar_pi *m_pi;
  RadarInfo *m_ri;
  wxBoxSizer *m_top_sizer;
  wxBoxSizer *m_control_sizer;

 private:
  void OnClose(wxCloseEvent &event);
  void OnIdOKClick(wxCommandEvent &event);
  void OnMove(wxMoveEvent &event);
  void OnSize(wxSizeEvent &event);

  void OnPlusTenClick(wxCommandEvent &event);
  void OnPlusClick(wxCommandEvent &event);
  void OnBackClick(wxCommandEvent &event);
  void OnMinusClick(wxCommandEvent &event);
  void OnMinusTenClick(wxCommandEvent &event);
  void OnAutoClick(wxCommandEvent &event);
  void OnMultiSweepClick(wxCommandEvent &event);

  void OnAdvancedBackButtonClick(wxCommandEvent &event);
  void OnInstallationBackButtonClick(wxCommandEvent &event);
  void OnAdvancedButtonClick(wxCommandEvent &event);
  void OnInstallationButtonClick(wxCommandEvent &event);

  void OnRadarGainButtonClick(wxCommandEvent &event);
  void OnRadarABButtonClick(wxCommandEvent &event);

  void OnRadarStateButtonClick(wxCommandEvent &event);
  void OnRotationButtonClick(wxCommandEvent &event);
  void OnRadarOverlayButtonClick(wxCommandEvent &event);
  void OnMessageButtonClick(wxCommandEvent &event);

  void OnRadarControlButtonClick(wxCommandEvent &event);

  void OnZone1ButtonClick(wxCommandEvent &event);
  void OnZone2ButtonClick(wxCommandEvent &event);

  void EnterEditMode(br24RadarControlButton *button);

  wxWindow *m_parent;
  wxBoxSizer *m_advanced_4G_sizer;
  wxBoxSizer *m_advanced_sizer;
  wxBoxSizer *m_edit_sizer;
  wxBoxSizer *m_installation_sizer;
  wxBoxSizer *m_from_sizer;  // If on edit control, this is where the button is from

  bool m_hide;
  bool m_hide_temporarily;

  // Edit Controls

  br24RadarControlButton *m_from_control;  // Only set when in edit mode

  // The 'edit' control has these buttons:
  wxButton *m_plus_ten_button;
  wxButton *m_plus_button;
  wxStaticText *m_value_text;
  wxButton *m_minus_button;
  wxButton *m_minus_ten_button;
  wxButton *m_auto_button;
  wxButton *m_multi_sweep_button;

  // Advanced controls
  br24RadarControlButton *m_interference_rejection_button;
  br24RadarControlButton *m_target_separation_button;
  br24RadarControlButton *m_noise_rejection_button;
  br24RadarControlButton *m_target_boost_button;
  br24RadarControlButton *m_scan_speed_button;
  br24RadarControlButton *m_timed_idle_button;

  // Installation controls
  br24RadarControlButton *m_bearing_alignment_button;
  br24RadarControlButton *m_antenna_height_button;
  br24RadarControlButton *m_local_interference_rejection_button;
  br24RadarControlButton *m_side_lobe_suppression_button;

  // Show Controls

  wxButton *m_radar_state;
  wxButton *m_rotation_button;
  wxButton *m_overlay_button;
  br24RadarRangeControlButton *m_range_button;
  br24RadarControlButton *m_transparency_button;  // TODO: Set it on change
  br24RadarControlButton *m_refresh_rate_button;  // TODO: Set it on change
  br24RadarControlButton *m_gain_button;
  br24RadarControlButton *m_sea_button;
  br24RadarControlButton *m_rain_button;
  wxButton *m_guard_1_button;
  wxButton *m_guard_2_button;
};

#endif
