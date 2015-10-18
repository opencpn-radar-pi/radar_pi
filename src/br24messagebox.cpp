/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
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

#include "br24radar_pi.h"


enum {                                      // process ID's
    ID_MSG_BACK,
    ID_RADAR,
    ID_DATA,
    ID_HEADING,
    ID_VALUE,
    ID_BPOS,
    ID_OFF
};

enum message_status {HIDE, SHOW, SHOW_NO_NMEA, SHOW_BACK};
//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(BR24MessageBox, wxDialog)

BEGIN_EVENT_TABLE(BR24MessageBox, wxDialog)

EVT_CLOSE(BR24MessageBox::OnClose)
EVT_BUTTON(ID_MSG_BACK, BR24MessageBox::OnMessageBackButtonClick)

EVT_MOVE(BR24MessageBox::OnMove)
EVT_SIZE(BR24MessageBox::OnSize)

END_EVENT_TABLE()



BR24MessageBox::BR24MessageBox()
{
    Init();
}

BR24MessageBox::~BR24MessageBox()
{
}


void BR24MessageBox::Init()
{
    // Initialize all members that need initialization
}

void BR24MessageBox::OnClose(wxCloseEvent& event)
{
    pPlugIn->OnBR24MessageBoxClose();
}

bool BR24MessageBox::Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id,
                                const wxString& caption,
                                const wxPoint& pos, const wxSize& size, long style)
{

    pParent = parent;
    pPlugIn = ppi;

#ifdef wxMSW
    long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#else
    long wstyle =                 wxCLOSE_BOX | wxCAPTION | wxRESIZE_BORDER;
#endif

    if (!wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle)) {
        return false;
    }
    g_font = *OCPNGetFont(_("Dialog"), 12);

    CreateControls();
    return true;
}


void BR24MessageBox::CreateControls()
{
    static int BORDER = 0;
    wxFont fatFont = g_font;
    fatFont.SetWeight(wxFONTWEIGHT_BOLD);
    fatFont.SetPointSize(g_font.GetPointSize() + 1);

    // A top-level sizer
    topSizeM = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizeM);

    //**************** MESSAGE BOX ******************//
    // A box sizer to contain warnings

    wxString label;

    messageBox = new wxBoxSizer(wxVERTICAL);
    topSizeM->Add(messageBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    offMessage = new wxStaticText(this, ID_OFF, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    messageBox->Add(offMessage, 0, wxALL, 2);
    offMessage->SetLabel(_("Can not switch radar on as\nit is not connected or off\n Switch radar on when button is amber"));
    offMessage->SetFont(g_font);

    tMessage = new wxStaticText(this, ID_BPOS, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    messageBox->Add(tMessage, 0, wxALL, 2);
    tMessage->SetLabel(_("Radar requires the following"));
    tMessage->SetFont(g_font);

    wxStaticBox* optionsBox = new wxStaticBox(this, wxID_ANY, _("OpenCPN options"));
    optionsBox->SetFont(g_font);
    wxStaticBoxSizer* optionsSizer = new wxStaticBoxSizer(optionsBox, wxVERTICAL);
    messageBox->Add(optionsSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    cbOpenGL = new wxCheckBox(this, ID_BPOS, _("Accelerated Graphics (OpenGL)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    optionsSizer->Add(cbOpenGL, 0, wxALL, BORDER);
    cbOpenGL->SetFont(g_font);
    cbOpenGL->Disable();

    ipBox = new wxStaticBox(this, wxID_ANY, _("ZeroConf via Ethernet"));
    ipBox->SetFont(g_font);
    wxStaticBoxSizer* ipSizer = new wxStaticBoxSizer(ipBox, wxVERTICAL);
    messageBox->Add(ipSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    cbRadar = new wxCheckBox(this, ID_RADAR, _("Radar present"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    ipSizer->Add(cbRadar, 0, wxALL, BORDER);
    cbRadar->SetFont(g_font);
    cbRadar->Disable();

    cbData = new wxCheckBox(this, ID_DATA, _("Radar sending data"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    ipSizer->Add(cbData, 0, wxALL, BORDER);
    cbData->SetFont(g_font);
    cbData->Disable();

    nmeaBox = new wxStaticBox(this, wxID_ANY, _("For radar overlay also required"));
    nmeaBox->SetFont(g_font);

    nmeaSizer = new wxStaticBoxSizer(nmeaBox, wxVERTICAL);
    messageBox->Add(nmeaSizer, 0, wxEXPAND | wxALL, BORDER * 2);
    messageBox->Hide(nmeaSizer);

    cbBoatPos = new wxCheckBox(this, ID_BPOS, _("Boat position"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbBoatPos, 0, wxALL, BORDER);
    cbBoatPos->SetFont(g_font);
    cbBoatPos->Disable();

    cbHeading = new wxCheckBox(this, ID_HEADING, _("Heading"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbHeading, 0, wxALL, BORDER);
    cbHeading->SetFont(g_font);
    cbHeading->Disable();

    cbVariation = new wxCheckBox(this, ID_HEADING, _("Variation"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbVariation, 0, wxALL, BORDER);
    cbVariation->SetFont(g_font);
    cbVariation->Disable();

    infoBox = new wxStaticBox(this, wxID_ANY, _("Statistics"));
    infoBox->SetFont(g_font);

    infoSizer = new wxStaticBoxSizer(infoBox, wxVERTICAL);
    messageBox->Add(infoSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    tStatistics = new wxStaticText(this, ID_VALUE, _("Statistics"), wxDefaultPosition, wxDefaultSize, 0);
    tStatistics->SetFont(*OCPNGetFont(_("Dialog"), 8));
    infoSizer->Add(tStatistics, 0, wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE, BORDER);

    // The <Close> button
    bMsgBack = new wxButton(this, ID_MSG_BACK, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    messageBox->Add(bMsgBack, 0, wxALL, BORDER);
    bMsgBack->SetFont(g_font);
    messageBox->Hide(bMsgBack);
}


void BR24MessageBox::OnMove(wxMoveEvent& event)
{
    //    Record the dialog position
    wxPoint p =  GetPosition();
    pPlugIn->SetBR24MessageBoxX(p.x);
    pPlugIn->SetBR24MessageBoxY(p.y);

    event.Skip();
}

void BR24MessageBox::OnSize(wxSizeEvent& event)
{
    //    Record the dialog size
    wxSize p = event.GetSize();
    pPlugIn->SetBR24MessageBoxSizeX(p.GetWidth());
    pPlugIn->SetBR24MessageBoxSizeY(p.GetHeight());
    event.Skip();
}

void BR24MessageBox::UpdateMessage(bool haveOpenGL, bool haveGPS, bool haveHeading, bool haveVariation, bool radarSeen, bool haveData)
{
    static message_status message_state = HIDE;
    message_status new_message_state = HIDE;
    static bool old_radarSeen = false;
    if (!pPlugIn->m_pMessageBox) {
        wxLogMessage(wxT("BR24radar_pi: ERROR UpdateMessage m_pMessageBox not existent"));
        return;
    }

    bool radarOn = haveOpenGL && radarSeen;
    bool navOn = haveGPS && haveHeading && haveVariation;
    bool black = pPlugIn->settings.display_mode[pPlugIn->settings.selectRadarB] == DM_CHART_BLACKOUT;
    bool want_message = false;
    if (pPlugIn->m_pControlDialog) {
        if (pPlugIn->m_pControlDialog->wantShowMessage) {
            want_message = true;
        }
    }


    /*
    Decision table to select the state of the message box
    - means not relevant

    case nr        1   2   3   4   5   6   7   8   9   10  11
    box type       m   m   m1  m1  m   H   H   H   mb  mb  mb
    _________________________________________________________
    radarOn        0   0   0   0   1   1   1   1   1   1   1
    navOn          0   1   0   1   0   1   0   1   1   0   1
    black          0   0   1   1   0   0   1   1   0   1   1
    want_message   -   -   -   -   -   0   0   0   1   1   1

    m     message box
    m1    message box without NMEA (no buttons)
    H     hide messagebox
    mb    message box with back button

    */

    if (!pPlugIn->settings.showRadar) {
        new_message_state = HIDE;
    }
    else if (!haveOpenGL) {
        new_message_state = SHOW;
    }
    else if (want_message) {
        new_message_state = SHOW_BACK;
    }
    else if (!black && (!navOn || !radarOn)) {
        new_message_state = SHOW;
    }
    else if (black && !radarOn) {
        new_message_state = SHOW_NO_NMEA;
    }
    else if ((black || navOn) && radarOn) {
        new_message_state = HIDE;
    }
    else {
        new_message_state = SHOW;
    }

    cbOpenGL->SetValue(haveOpenGL);
    cbBoatPos->SetValue(haveGPS);
    cbHeading->SetValue(haveHeading);
    cbVariation->SetValue(haveVariation);
    cbRadar->SetValue(radarSeen);
    cbData->SetValue(haveData);
    if (pPlugIn->settings.verbose)wxLogMessage(wxT("BR24radar_pi: messagebox switch, case=%d"), new_message_state);
    if (message_state != new_message_state || old_radarSeen != radarSeen) {
        switch (new_message_state) {

        case HIDE:
            if(pPlugIn->settings.verbose) wxLogMessage(wxT("BR24radar_pi: case hide"));
            if (pPlugIn->m_pMessageBox->IsShown()) {
                pPlugIn->m_pMessageBox->Hide();
            }
            break;

        case SHOW:
            if (pPlugIn->settings.verbose)wxLogMessage(wxT("BR24radar_pi: case show"));
            if (!pPlugIn->m_pMessageBox-> IsShown()) {
                pPlugIn->m_pMessageBox->Show();
            }
            if (!radarSeen) {
                offMessage->Show();
            }
            else{
                offMessage->Hide();
            }
            messageBox->Show(nmeaSizer);
            bMsgBack->Hide();
            break;

        case SHOW_NO_NMEA:
            if (pPlugIn->settings.verbose)wxLogMessage(wxT("BR24radar_pi: case show no nmea"));
            if (!pPlugIn->m_pMessageBox->IsShown()) {
                pPlugIn->m_pMessageBox->Show();
            }
            if (!radarSeen) {
                offMessage->Show();
            }
            else{
                offMessage->Hide();
            }
            messageBox->Hide(nmeaSizer);
            messageBox->Hide(bMsgBack);
            break;

        case SHOW_BACK:
            if (pPlugIn->settings.verbose)wxLogMessage(wxT("BR24radar_pi: case show back"));
            if (!pPlugIn->m_pMessageBox->IsShown()) {
                pPlugIn->m_pMessageBox->Show();
            }
            offMessage->Hide();
            messageBox->Show(nmeaSizer);
            bMsgBack->Show();
            break;
        }
    }
    Fit();
    topSizeM->Layout();
    old_radarSeen = radarSeen;
    message_state = new_message_state;
}


void BR24MessageBox::OnMessageBackButtonClick(wxCommandEvent& event)
{
    pPlugIn->m_pControlDialog->wantShowMessage = false;

    if (pPlugIn->m_pMessageBox)
    {
        pPlugIn->m_pMessageBox->Hide();
    }
    Fit();
    topSizeM->Layout();
}

void BR24MessageBox::SetRadarIPAddress(wxString &msg)
{
    if (cbRadar) {
        wxString label;

        label << _("Radar IP") << wxT(" ") << msg;
        cbRadar->SetLabel(label);
    }
}

void BR24MessageBox::SetErrorMessage(wxString &msg)
{
    tMessage->SetLabel(msg);
    topSizeM->Show(messageBox);
    messageBox->Layout();
    Fit();
    topSizeM->Layout();
}


void BR24MessageBox::SetMcastIPAddress(wxString &msg)
{
    if (ipBox) {
        wxString label;

        label << _("ZeroConf E'net") << wxT(" ") << msg;
        ipBox->SetLabel(label);
    }
}

void BR24MessageBox::SetHeadingInfo(wxString &msg)
{
    if (cbHeading && topSizeM->IsShown(messageBox)) {
        wxString label;

        label << _("Heading") << wxT(" ") << msg;
        cbHeading->SetLabel(label);
    }
}

void BR24MessageBox::SetVariationInfo(wxString &msg)
{
    if (cbVariation && topSizeM->IsShown(messageBox)) {
        wxString label;

        label << _("Variation") << wxT(" ") << msg;
        cbVariation->SetLabel(label);
    }
}

void BR24MessageBox::SetRadarInfo(wxString &msg)
{
    if (tStatistics && topSizeM->IsShown(messageBox)) {
        tStatistics->SetLabel(msg);
    }
}
