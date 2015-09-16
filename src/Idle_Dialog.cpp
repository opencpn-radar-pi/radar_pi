/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif                          //precompiled headers

#include <wx/socket.h>
#include "wx/apptrait.h"
#include <wx/glcanvas.h>
#include "wx/sckaddr.h"
#include "wx/datetime.h"
#include <wx/fileconf.h>

#include <math.h>
#include <fstream>

using namespace std;

#include "br24radar_pi.h"

enum {
    ID_STOPIDLE
};

IMPLEMENT_CLASS(Idle_Dialog, wxDialog)

BEGIN_EVENT_TABLE(Idle_Dialog, wxDialog)

EVT_CLOSE(Idle_Dialog::OnClose)
EVT_BUTTON(ID_STOPIDLE, Idle_Dialog::OnIdStopIdleClick)

END_EVENT_TABLE()

Idle_Dialog::Idle_Dialog()
{
    Init();
}
Idle_Dialog::~Idle_Dialog()
{
}

void Idle_Dialog::Init()
{
}

bool Idle_Dialog::Create(wxWindow *parent, br24radar_pi *pPI, wxWindowID id,
                         const wxString  &m_caption, const wxPoint   &pos,
                         const wxSize    &size, long style)
{
    pParent = parent;
    pPlugIn = pPI;
#ifdef wxMSW
    long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#else
    long wstyle =                 wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#endif

    wxSize  size_min = size;

    if (!wxDialog::Create(parent, id, m_caption, pos, size_min, wstyle)) return false;

    CreateControls();

    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;


}

//Foresee translated text to fit into the Idle_Dialog
wxString Timelabel_1  = _("Idle time is set to");
wxString Timelabel_2 = _("minutes");
wxString Timeleftlabel_1 = _T("<");
wxString Timeleftlabel_2 = _("minutes until next run");

void Idle_Dialog::CreateControls()
{
    const int border = 5;
    wxString Label_1, Label_2;
    Label_1 << Timelabel_1 << _T(" 15 ") << Timelabel_2;
    Label_2 << Timeleftlabel_1 << _T(" 15 ") << Timeleftlabel_2;

    wxBoxSizer *sbIdleDialogSizer;
    sbIdleDialogSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Radar scanner is Idling") ), wxVERTICAL );
    this->SetSizer( sbIdleDialogSizer);

    p_Idle_Mode = new wxStaticText( this, wxID_ANY, Label_1 , wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    p_Idle_Mode->Wrap( -1 );
    sbIdleDialogSizer->Add( p_Idle_Mode, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, border );

    p_IdleTimeLeft = new wxStaticText( this, wxID_ANY, Label_2, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    p_IdleTimeLeft->Wrap( -1 );
    sbIdleDialogSizer->Add( p_IdleTimeLeft, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, border );

    m_Idle_gauge = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,12 ), wxGA_HORIZONTAL);
    m_Idle_gauge->SetValue( 0 );
    sbIdleDialogSizer->Add( m_Idle_gauge, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, border);

    m_btnStopIdle = new wxButton( this, ID_STOPIDLE, _("Turn Radar always ON"), wxDefaultPosition, wxDefaultSize, 0 );
    sbIdleDialogSizer->Add( m_btnStopIdle, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, border );

    this->Layout();
    sbIdleDialogSizer->Fit( this );
}

void Idle_Dialog::SetIdleTimes(int IdleTime, int IdleTimeLeft)
{
    wxString Timelabel, t, Timeleftlabel, t2;
    //wxString t, t2;
    t.Printf(_T("%d"), IdleTime);
    t2.Printf(_T("%d"), IdleTimeLeft + 1);
    Timelabel << Timelabel_1 << _T(" ") << t << _T(" ") << Timelabel_2;
    Timeleftlabel << Timeleftlabel_1 << _T(" ") << t2 << _T(" ") << Timeleftlabel_2;
    int GaugeValue = 100 - (IdleTimeLeft >= 4 ? 99 : (100 * (IdleTimeLeft+1)/IdleTime));
    p_Idle_Mode->SetLabel(Timelabel);
    p_IdleTimeLeft->SetLabel(Timeleftlabel);
    m_Idle_gauge->SetValue(GaugeValue);
}

void Idle_Dialog::OnClose(wxCloseEvent &event)
{
    event.Skip();
}

void Idle_Dialog::OnIdStopIdleClick(wxCommandEvent &event)
{
    Idle_Dialog::Close();
    event.Skip();
    pPlugIn->br24radar_pi::OnToolbarToolCallback(1);    //Start radar scanning and set Timed Idle off
}
