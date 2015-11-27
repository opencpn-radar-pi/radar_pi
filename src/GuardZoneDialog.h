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

#ifndef _GUARDZONEDIALOG_H_
#define _GUARDZONEDIALOG_H_

#include "br24radar_pi.h"

class GuardZoneDialog :    public wxDialog
{
    DECLARE_CLASS(GuardZoneDialog)
    DECLARE_EVENT_TABLE()

public:
    GuardZoneDialog();

    ~GuardZoneDialog();
    void    Init();

    bool    Create
    (
     wxWindow        *parent,
     br24radar_pi    *pi,
     wxWindowID      id = wxID_ANY,
     const wxString  &m_caption = _(" Guard Zone Control"),
     const wxPoint   &pos = wxDefaultPosition,
     const wxSize    &size = wxDefaultSize,
     long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
    );


    void            CreateControls();
    void            OnContextMenuGuardCallback(double mark_rng, double mark_brg);
    void            OnGuardZoneDialogShow(RadarInfo * ri, int zone);

private:
    void            SetVisibility();
    void            OnGuardZoneModeClick(wxCommandEvent &event);
    void            OnInner_Range_Value(wxCommandEvent &event);
    void            OnOuter_Range_Value(wxCommandEvent &event);
    void            OnStart_Bearing_Value(wxCommandEvent &event);
    void            OnEnd_Bearing_Value(wxCommandEvent &event);
    void            OnFilterClick(wxCommandEvent &event);
    void            OnClose(wxCloseEvent &event);
    void            OnIdOKClick(wxCommandEvent &event);

    wxWindow        *m_parent;
    br24radar_pi    *m_pi;
    RadarInfo       *m_ri;
    GuardZone       *m_guard_zone;

    /* Controls */
    wxStaticBox     *pBoxGuardZone;
    wxRadioBox      *pGuardZoneType;
    wxTextCtrl      *pInner_Range;
    wxTextCtrl      *pOuter_Range;
    wxTextCtrl      *pStart_Bearing_Value;
    wxTextCtrl      *pEnd_Bearing_Value;
    wxCheckBox      *cbFilter;
};

#endif /* _GUARDZONEDIALOG_H_ */

// vim: sw=4:ts=8:
