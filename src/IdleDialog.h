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

#ifndef _IDLEDIALOG_H_
#define _IDLEDIALOG_H_

#include "br24radar_pi.h"

class IdleDialog : public wxDialog
{
    DECLARE_CLASS(IdleDialog)
    DECLARE_EVENT_TABLE()

public:
    IdleDialog();

    ~IdleDialog();

    void    Init();

    bool Create
    (
     wxWindow        *parent,
     br24radar_pi    *pi,
     wxWindowID      id = wxID_ANY,
     const wxString  &m_caption = _("Timed Transmit"),
     const wxPoint   &pos = wxPoint(0 ,0),
     const wxSize    &size = wxDefaultSize,
     long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
     );

    void    CreateControls();
    void    SetIdleTimes(int IdleMode, int IdleTime, int IdleTimeLeft);

private:

    void            OnClose(wxCloseEvent &event);
    void            OnMove(wxMoveEvent &event);
    void            OnSize(wxSizeEvent &event);
    void            OnIdStopIdleClick(wxCommandEvent &event);

    wxWindow        *m_parent;

    br24radar_pi    *m_pi;

    /* Controls  */
    wxStaticText *p_Idle_Mode;
    wxStaticText *p_IdleTimeLeft;
    wxGauge *m_Idle_gauge;
    wxButton *m_btnStopIdle;

};

#endif

// vim: sw=4:ts=8:
