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

#ifndef _BR24OPTIONSDIALOG_H_
#define _BR24OPTIONSDIALOG_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class br24OptionsDialog : public wxDialog {
 public:
  br24OptionsDialog(wxWindow* parent, PersistentSettings& settings, RadarType radar_type);
  PersistentSettings GetSettings() { return m_settings; };

 private:
  void OnClose(wxCloseEvent& event);
  void OnIdOKClick(wxCommandEvent& event);
  void OnRangeUnitsClick(wxCommandEvent& event);
  void OnDisplayOptionClick(wxCommandEvent& event);
  void OnDisplayModeClick(wxCommandEvent& event);
  void OnGuardZoneStyleClick(wxCommandEvent& event);
  void OnGuardZoneOnOverlayClick(wxCommandEvent& event);
  void OnGuardZoneTimeoutClick(wxCommandEvent& event);
  void OnTrailsOnOverlayClick(wxCommandEvent& event);
  void OnSelectSoundClick(wxCommandEvent& event);
  void OnTestSoundClick(wxCommandEvent& event);
  void OnPassHeadingClick(wxCommandEvent& event);
  void OnDrawingMethodClick(wxCommandEvent& event);
  void OnMenuAutoHideClick(wxCommandEvent& event);
  void OnEnableCOGHeadingClick(wxCommandEvent& event);
  void OnEnableDualRadarClick(wxCommandEvent& event);
  void OnEmulatorClick(wxCommandEvent& event);
  void OnReverseZoomClick(wxCommandEvent& event);

  PersistentSettings m_settings;

  // DisplayOptions
  wxRadioBox* m_RangeUnits;
  wxRadioBox* m_OverlayDisplayOptions;
  wxRadioBox* m_DisplayMode;
  wxRadioBox* m_GuardZoneStyle;
  wxRadioBox* m_GuardZoneOnOverlay;
  wxTextCtrl* m_GuardZoneTimeout;
  wxRadioBox* m_TrailsOnOverlay;
  wxCheckBox* m_PassHeading;
  wxCheckBox* m_COGHeading;
  wxComboBox* m_DrawingMethod;
  wxComboBox* m_MenuAutoHide;
  wxCheckBox* m_EnableDualRadar;
  wxCheckBox* m_Emulator;
  wxCheckBox* m_ReverseZoom;
};

PLUGIN_END_NAMESPACE

#endif /* _BR24OPTIONSDIALOG_H_ */
