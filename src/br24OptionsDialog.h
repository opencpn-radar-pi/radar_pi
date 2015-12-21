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

#ifndef _BR24OPTIONSDIALOG_H_
#define _BR24OPTIONSDIALOG_H_

#include "br24radar_pi.h"

class br24OptionsDialog : public wxDialog
{
    DECLARE_CLASS(br24OptionsDialog)
    DECLARE_EVENT_TABLE()

   public:
    br24OptionsDialog();

    ~br24OptionsDialog();
    void Init();

    bool Create(wxWindow * parent, br24radar_pi * pi);

    void CreateDisplayOptions();

   private:
    void OnClose(wxCloseEvent & event);
    void OnIdOKClick(wxCommandEvent & event);
    void OnRangeUnitsClick(wxCommandEvent & event);
    void OnDisplayOptionClick(wxCommandEvent & event);
    void OnIntervalSlider(wxCommandEvent & event);
    void OnDisplayModeClick(wxCommandEvent & event);
    void OnGuardZoneStyleClick(wxCommandEvent & event);
    void OnHeading_Calibration_Value(wxCommandEvent & event);
    void OnSelectSoundClick(wxCommandEvent & event);
    void OnTestSoundClick(wxCommandEvent & event);
    void OnPassHeadingClick(wxCommandEvent & event);
    void OnDrawingMethodClick(wxCommandEvent & event);
    void OnEnableDualRadarClick(wxCommandEvent & event);
    void OnEmulatorClick(wxCommandEvent & event);

    wxWindow * m_parent;
    br24radar_pi * m_pi;

    // DisplayOptions
    wxRadioBox * pRangeUnits;
    wxRadioBox * pOverlayDisplayOptions;
    wxRadioBox * pDisplayMode;
    wxRadioBox * pGuardZoneStyle;
    wxSlider * pIntervalSlider;
    wxTextCtrl * pText_Heading_Correction_Value;
    wxCheckBox * cbPassHeading;
    wxComboBox * cbDrawingMethod;
    wxCheckBox * cbEnableDualRadar;
    wxCheckBox * cbEmulator;
};

#endif /* _BR24OPTIONSDIALOG_H_ */

// vim: sw=4:ts=8:
