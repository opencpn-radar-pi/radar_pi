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
class br24ControlsDialog: public wxDialog
{
    DECLARE_CLASS(br24ControlsDialog)
    DECLARE_EVENT_TABLE()

public:

    br24ControlsDialog();

    ~br24ControlsDialog();
    void Init();

    bool Create(wxWindow *parent, br24radar_pi *pi, RadarInfo *ri,
                wxWindowID id = wxID_ANY,
                const wxString& caption = _("Radar"),
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX)
                );

    void CreateControls();
    void SetRemoteRangeIndex(size_t index);
    void SetRangeIndex(size_t index);
    void SetTimedIdleIndex(int index);
    void UpdateGuardZoneState();
    void UpdateControl(bool haveOpenGL, bool haveGPS, bool haveHeading, bool haveVariation, bool haveRadar, bool haveData);
    void UpdateControlValues(bool refreshAll);
    void SetErrorMessage(wxString &msg);
    bool wantShowMessage; // If true, don't hide messagebox automatically

    br24RadarControlButton *bRadarAB;
    wxBoxSizer         *topSizer;
    wxBoxSizer         *controlBox;
    RadarInfo          *m_ri;
    br24radar_pi       *m_pi;

private:
    void OnClose(wxCloseEvent& event);
    void OnIdOKClick(wxCommandEvent& event);
    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnPlusTenClick(wxCommandEvent& event);
    void OnPlusClick(wxCommandEvent& event);
    void OnBackClick(wxCommandEvent& event);
    void OnMinusClick(wxCommandEvent& event);
    void OnMinusTenClick(wxCommandEvent& event);
    void OnAutoClick(wxCommandEvent& event);
    void OnMultiSweepClick(wxCommandEvent& event);

    void OnAdvancedBackButtonClick(wxCommandEvent& event);
    void OnInstallationBackButtonClick(wxCommandEvent& event);
    void OnAdvancedButtonClick(wxCommandEvent& event);
    void OnInstallationButtonClick(wxCommandEvent& event);

    void OnRadarGainButtonClick(wxCommandEvent& event);
    void OnRadarABButtonClick(wxCommandEvent& event);

    void OnRadarStateButtonClick(wxCommandEvent& event);
    void OnRadarOverlayButtonClick(wxCommandEvent& event);
    void OnMessageButtonClick(wxCommandEvent& event);

    void OnRadarControlButtonClick(wxCommandEvent& event);

    void OnZone1ButtonClick(wxCommandEvent &event);
    void OnZone2ButtonClick(wxCommandEvent &event);

    void EnterEditMode(br24RadarControlButton * button);

    wxWindow               *pParent;
    wxBoxSizer             *advanced4gBox;
    wxBoxSizer             *advancedBox;
    wxBoxSizer             *editBox;
    wxBoxSizer             *installationBox;
    wxBoxSizer             *fromBox; // If on edit control, this is where the button is from

    // Edit Controls

    br24RadarControlButton *fromControl; // Only set when in edit mode

    // The 'edit' control has these buttons:
    wxButton               *bBack;
    wxButton               *bPlusTen;
    wxButton               *bPlus;
    wxStaticText           *tValue;
    wxButton               *bMinus;
    wxButton               *bMinusTen;
    wxButton               *bAuto;
    wxButton               *bMultiSweep;

    // Advanced controls
    wxButton               *bAdvancedBack;
    wxButton               *bInstallationBack;
    br24RadarControlButton *bTransparency;
    br24RadarControlButton *bInterferenceRejection;
    br24RadarControlButton *bTargetSeparation;
    br24RadarControlButton *bNoiseRejection;
    br24RadarControlButton *bTargetBoost;
    br24RadarControlButton *bRefreshrate;
    br24RadarControlButton *bScanSpeed;
    br24RadarControlButton *bTimedIdle;

    // Installation controls
    br24RadarControlButton *bBearingAlignment;
    br24RadarControlButton *bAntennaHeight;
    br24RadarControlButton *bLocalInterferenceRejection;
    br24RadarControlButton *bSideLobeSuppression;

    // Show Controls

    wxButton               *bRadarState;
    wxButton               *bOverlay;
    br24RadarRangeControlButton *bRange;
    br24RadarControlButton *bGain;
    br24RadarControlButton *bSea;
    br24RadarControlButton *bRain;
    wxButton               *bAdvanced;
    wxButton               *bInstallation;
    wxButton               *bGuard1;
    wxButton               *bGuard2;
    wxButton               *bMessage;
};

#endif

// vim: sw=4:ts=8:
