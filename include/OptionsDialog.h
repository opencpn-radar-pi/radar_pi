/******************************************************************************
 *
 * Project:  Shore radar
 * Authors:  Kees Verruijt
 *           Douwe Fokkema
 ***************************************************************************
 *   Copyright (C) 2012-2023 by Kees Verruijt         canboat@verruijt.net *
 *   Copyright (C) 2013-2023 by Douwe Fokkema         df@percussion.nl     *
 *                                                                         *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *                                                                         *
 ***************************************************************************
 */

#ifndef _OPTIONSDIALOG_H_
#define _OPTIONSDIALOG_H_

#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class OptionsDialog : public wxDialog {
public:
    OptionsDialog(wxWindow* parent, radar_pi* pi, PersistentSettings& settings,
        RadarType radar_type);
    PersistentSettings GetSettings() { return m_settings; };
    radar_pi* m_pi;

private:
    void OnClose(wxCloseEvent& event);
    void OnIdOKClick(wxCommandEvent& event);
    void OnRangeUnitsClick(wxCommandEvent& event);
    void OnDisplayOptionClick(wxCommandEvent& event);
    void OnDisplayModeClick(wxCommandEvent& event);
    void OnGuardZoneStyleClick(wxCommandEvent& event);
    void OnGuardZoneOnOverlayClick(wxCommandEvent& event);
    void OnOverlayOnStandbyClick(wxCommandEvent& event);
    void OnGuardZoneTimeoutClick(wxCommandEvent& event);
    void OnFixedHeadingClick(wxCommandEvent& event);
    void OnFixedPositionClick(wxCommandEvent& event);
    void OnFixedLonTextClick(wxCommandEvent& event);
    void OnRadarDescriptionTextClick(wxCommandEvent& event);
    void OnFixedLatTextClick(wxCommandEvent& event);
    void OnCopyOCPNPositionClick(wxCommandEvent& event);
    void OnFixedHeadingValueClick(wxCommandEvent& event);
    void OnShowExtremeRangeClick(wxCommandEvent& event);
    void OnTrailsOnOverlayClick(wxCommandEvent& event);
    void OnTrailStartColourClick(wxCommandEvent& event);
    void OnTrailEndColourClick(wxCommandEvent& event);
    void OnWeakColourClick(wxCommandEvent& event);
    void OnArpaColourClick(wxCommandEvent& event);
    void OnDopplerApproachingColourClick(wxCommandEvent& event);
    void OnDopplerRecedingColourClick(wxCommandEvent& event);
    void OnPPIBackgroundColourClick(wxCommandEvent& event);
    void OnAisTextColourClick(wxCommandEvent& event);
    void OnIntermediateColourClick(wxCommandEvent& event);
    void OnStrongColourClick(wxCommandEvent& event);
    void OnSelectSoundClick(wxCommandEvent& event);
    void OnTestSoundClick(wxCommandEvent& event);
    void OnIgnoreHeadingClick(wxCommandEvent& event);
    void OnPassHeadingClick(wxCommandEvent& event);
    void OnDrawingMethodClick(wxCommandEvent& event);
    void OnMenuAutoHideClick(wxCommandEvent& event);
    void OnEnableCOGHeadingClick(wxCommandEvent& event);
    void OnReverseZoomClick(wxCommandEvent& event);
    void OnResetButtonClick(wxCommandEvent& event);
    void OnLoggingClick(wxCommandEvent& event);

    PersistentSettings m_settings;

    // DisplayOptions
    wxRadioBox* m_RangeUnits;
    wxRadioBox* m_OverlayDisplayOptions;
    wxRadioBox* m_DisplayMode;
    wxRadioBox* m_GuardZoneStyle;
    wxTextCtrl* m_GuardZoneTimeout;
    wxTextCtrl* m_FixedHeadingValue;
    wxTextCtrl* m_FixedLatValue;
    wxTextCtrl* m_FixedLonValue;
    wxTextCtrl* m_RadarDescriptionText;
    wxColourPickerCtrl* m_TrailStartColour;
    wxColourPickerCtrl* m_TrailEndColour;
    wxColourPickerCtrl* m_WeakColour;
    wxColourPickerCtrl* m_IntermediateColour;
    wxColourPickerCtrl* m_StrongColour;
    wxColourPickerCtrl* m_ArpaColour;
    wxColourPickerCtrl* m_AisTextColour;
    wxColourPickerCtrl* m_PPIBackgroundColour;
    wxColourPickerCtrl* m_DopplerApproachingColour;
    wxColourPickerCtrl* m_DopplerRecedingColour;
    wxCheckBox* m_ShowExtremeRange;
    wxCheckBox* m_GuardZoneOnOverlay;
    wxCheckBox* m_TrailsOnOverlay;
    wxCheckBox* m_OverlayStandby;
    wxCheckBox* m_IgnoreHeading;
    wxCheckBox* m_FixedPosition;
    wxButton* m_CopyOCPNPosition;
    wxCheckBox* m_FixedHeading;
    wxCheckBox* m_PassHeading;
    wxCheckBox* m_COGHeading;
    wxComboBox* m_DrawingMethod;
    wxComboBox* m_MenuAutoHide;
    wxCheckBox* m_EnableDualRadar;
    wxCheckBox* m_ReverseZoom;
    wxCheckBox* m_Verbose;
    wxCheckBox* m_Dialog;
    wxCheckBox* m_Transmit;
    wxCheckBox* m_Receive;
    wxCheckBox* m_Guard;
    wxCheckBox* m_ARPA;
    wxCheckBox* m_Reports;
    wxCheckBox* m_Inter;
};

PLUGIN_END_NAMESPACE

#endif /* _OPTIONSDIALOG_H_ */
