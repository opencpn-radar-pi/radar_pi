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

#include "br24MessageBox.h"
#include "RadarPanel.h"

PLUGIN_BEGIN_NAMESPACE

enum {  // process ID's
  ID_MSG_CLOSE,
  ID_RADAR,
  ID_DATA,
  ID_HEADING,
  ID_VALUE,
  ID_BPOS,
  ID_OFF
};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(br24MessageBox, wxDialog)

BEGIN_EVENT_TABLE(br24MessageBox, wxDialog)

EVT_CLOSE(br24MessageBox::OnClose)
EVT_BUTTON(ID_MSG_CLOSE, br24MessageBox::OnMessageCloseButtonClick)

EVT_MOVE(br24MessageBox::OnMove)
EVT_SIZE(br24MessageBox::OnSize)

END_EVENT_TABLE()

br24MessageBox::br24MessageBox() { Init(); }

br24MessageBox::~br24MessageBox() {}

void br24MessageBox::Init() {
  // Initialize all members that need initialization
  m_parent = 0;
  m_top_sizer = 0;
  m_nmea_sizer = 0;
  m_info_sizer = 0;
  m_message_sizer = 0;
  m_ip_box = 0;
}

bool br24MessageBox::Create(wxWindow *parent, br24radar_pi *pi, wxWindowID id, const wxString &caption, const wxPoint &pos,
                            const wxSize &size, long style) {
  m_parent = parent;
  m_pi = pi;

#ifdef wxMSW
  long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN | wxSTAY_ON_TOP | wxFRAME_FLOAT_ON_PARENT;
#else
  long wstyle = wxCLOSE_BOX | wxCAPTION | wxRESIZE_BORDER | wxSTAY_ON_TOP | wxFRAME_FLOAT_ON_PARENT;
#endif

  if (!wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle)) {
    return false;
  }

  CreateControls();

  Fit();
  Hide();

  m_message_state = HIDE;
  m_old_radar_seen = false;
  m_allow_auto_hide = true;

  wxLogMessage(wxT("BR24radar_pi: MessageBox created"));

  return true;
}

void br24MessageBox::CreateControls() {
  static int BORDER = 0;

  // A top-level sizer
  m_top_sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(m_top_sizer);

  //**************** MESSAGE BOX ******************//
  // A box sizer to contain warnings

  wxString label;

  m_message_sizer = new wxBoxSizer(wxVERTICAL);
  m_top_sizer->Add(m_message_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

  m_radar_off = new wxStaticText(this, ID_OFF, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_message_sizer->Add(m_radar_off, 0, wxALL, 2);
  m_radar_off->SetLabel(_("Cannot switch radar on as\nit is not connected or off"));
  m_radar_off->SetFont(m_pi->m_font);

  m_error_message = new wxStaticText(this, ID_BPOS, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_message_sizer->Add(m_error_message, 0, wxALL, 2);
  m_error_message->SetLabel(_("Radar requires the following"));
  m_error_message->SetFont(m_pi->m_font);

  wxStaticBox *optionsBox = new wxStaticBox(this, wxID_ANY, _("OpenCPN options"));
  optionsBox->SetFont(m_pi->m_font);
  wxStaticBoxSizer *optionsSizer = new wxStaticBoxSizer(optionsBox, wxVERTICAL);
  m_message_sizer->Add(optionsSizer, 0, wxEXPAND | wxALL, BORDER * 2);

  m_have_open_gl = new wxCheckBox(this, ID_BPOS, _("Accelerated Graphics (OpenGL)"), wxDefaultPosition, wxDefaultSize,
                                  wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  optionsSizer->Add(m_have_open_gl, 0, wxALL, BORDER);
  m_have_open_gl->SetFont(m_pi->m_font);
  m_have_open_gl->Disable();

  m_ip_box = new wxStaticBox(this, wxID_ANY, _("ZeroConf via Ethernet"));
  m_ip_box->SetFont(m_pi->m_font);
  wxStaticBoxSizer *ipSizer = new wxStaticBoxSizer(m_ip_box, wxVERTICAL);
  m_message_sizer->Add(ipSizer, 0, wxEXPAND | wxALL, BORDER * 2);

  m_have_radar =
      new wxCheckBox(this, ID_RADAR, _("Radar present"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  ipSizer->Add(m_have_radar, 0, wxALL, BORDER);
  m_have_radar->SetFont(m_pi->m_font);
  m_have_radar->Disable();

  m_have_data =
      new wxCheckBox(this, ID_DATA, _("Radar sending data"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  ipSizer->Add(m_have_data, 0, wxALL, BORDER);
  m_have_data->SetFont(m_pi->m_font);
  m_have_data->Disable();

  wxStaticBox *nmeaBox = new wxStaticBox(this, wxID_ANY, _("For radar overlay also required"));
  nmeaBox->SetFont(m_pi->m_font);

  m_nmea_sizer = new wxStaticBoxSizer(nmeaBox, wxVERTICAL);
  m_message_sizer->Add(m_nmea_sizer, 0, wxEXPAND | wxALL, BORDER * 2);
  m_message_sizer->Hide(m_nmea_sizer);

  m_have_boat_pos =
      new wxCheckBox(this, ID_BPOS, _("Boat position"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_nmea_sizer->Add(m_have_boat_pos, 0, wxALL, BORDER);
  m_have_boat_pos->SetFont(m_pi->m_font);
  m_have_boat_pos->Disable();

  m_have_heading =
      new wxCheckBox(this, ID_HEADING, _("Heading"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_nmea_sizer->Add(m_have_heading, 0, wxALL, BORDER);
  m_have_heading->SetFont(m_pi->m_font);
  m_have_heading->Disable();

  m_have_variation =
      new wxCheckBox(this, ID_HEADING, _("Variation"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
  m_nmea_sizer->Add(m_have_variation, 0, wxALL, BORDER);
  m_have_variation->SetFont(m_pi->m_font);
  m_have_variation->Disable();

  wxStaticBox *infoBox = new wxStaticBox(this, wxID_ANY, _("Statistics"));
  infoBox->SetFont(m_pi->m_font);
  m_info_sizer = new wxStaticBoxSizer(infoBox, wxVERTICAL);
  m_message_sizer->Add(m_info_sizer, 0, wxEXPAND | wxALL, BORDER * 2);

  m_statistics = new wxStaticText(this, ID_VALUE, _("Statistics"), wxDefaultPosition, wxDefaultSize, 0);
  m_statistics->SetFont(*OCPNGetFont(_("Dialog"), 8));
  m_info_sizer->Add(m_statistics, 0, wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE, BORDER);

  // The <Close> button
  m_close_button = new wxButton(this, ID_MSG_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
  m_message_sizer->Add(m_close_button, 0, wxALL, BORDER);
  m_close_button->SetFont(m_pi->m_font);
  m_message_sizer->Hide(m_close_button);
}

void br24MessageBox::OnMove(wxMoveEvent &event) { event.Skip(); }

void br24MessageBox::OnSize(wxSizeEvent &event) { event.Skip(); }

void br24MessageBox::OnClose(wxCloseEvent &event) {
  m_allow_auto_hide = true;
  Hide();
}

bool br24MessageBox::Show() {
  // Come up with a good message box location: straight in the center of the chart window

  wxPoint parentPos = m_parent->GetPosition();
  wxSize parentSize = m_parent->GetSize();
  wxSize mySize = this->GetSize();
  wxPoint newPos;

  newPos.x = parentPos.x + parentSize.x / 2 - mySize.x / 2;
  newPos.y = parentPos.y + parentSize.y / 2 - mySize.y / 2;
  SetPosition(newPos);

  return wxDialog::Show();
}

void br24MessageBox::UpdateMessage(bool force) {
  message_status new_message_state = HIDE;

  bool haveOpenGL = m_pi->m_opengl_mode;
  bool haveGPS = m_pi->m_bpos_set;
  bool haveHeading = m_pi->m_heading_source != HEADING_NONE;
  bool haveVariation = m_pi->m_var_source != VARIATION_SOURCE_NONE;
  bool radarSeen = false;
  bool haveData = false;

  if (force) {
    m_allow_auto_hide = false;
  }

  for (int r = 0; r < RADARS; r++) {
    if (m_pi->m_radar[r]->state.value != RADAR_OFF) {
      radarSeen = true;
    }
    if (m_pi->m_radar[r]->state.value == RADAR_TRANSMIT) {
      haveData = true;
    }
  }

  bool radarOn = haveOpenGL && radarSeen;
  bool navOn = haveGPS && haveHeading && haveVariation;
  bool no_overlay = m_pi->m_settings.chart_overlay < 0;

  wxLogMessage(wxT("BR24radar_pi: messagebox decision: show=%d overlay=%d auto_hide=%d opengl=%d radarOn=%d navOn=%d"),
               m_pi->m_settings.show_radar, m_pi->m_settings.chart_overlay, m_allow_auto_hide, haveOpenGL, radarOn, navOn);

  /*
    if (m_pi->m_settings.show_radar == RADAR_OFF && no_overlay && m_allow_hide) {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox show_radar = RADAR_OFF"));
      }
      new_message_state = HIDE;
    } else
  */

  if (!m_allow_auto_hide) {
    if (m_pi->m_settings.verbose >= 2) {
      wxLogMessage(wxT("BR24radar_pi: messagebox explicit wanted"));
    }
    new_message_state = SHOW_CLOSE;
  } else if (!haveOpenGL) {
    if (m_pi->m_settings.verbose >= 2) {
      wxLogMessage(wxT("BR24radar_pi: messagebox no OpenGL"));
    }
    m_pi->m_settings.chart_overlay = -1;
    m_pi->m_settings.show_radar = RADAR_OFF;
    new_message_state = SHOW_CLOSE;
  } else if (no_overlay) {
    if (m_pi->m_settings.show_radar == RADAR_OFF) {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox no radar wanted"));
      }
      new_message_state = HIDE;
    } else if (radarOn) {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox radar window needs met"));
      }
      new_message_state = HIDE;
    } else {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox radar window needs not met"));
      }
      new_message_state = SHOW_NO_NMEA;
    }
  } else {  // overlay
    if (navOn && radarOn) {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox overlay needs met"));
      }
      new_message_state = HIDE;
    } else {
      if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: messagebox overlay needs not met"));
      }
      new_message_state = SHOW;
    }
  }

  m_have_open_gl->SetValue(haveOpenGL);
  m_have_boat_pos->SetValue(haveGPS);
  m_have_heading->SetValue(haveHeading);
  m_have_variation->SetValue(haveVariation);
  m_have_radar->SetValue(radarSeen);
  m_have_data->SetValue(haveData);
  if (m_pi->m_settings.verbose >= 2) {
    wxLogMessage(wxT("BR24radar_pi: messagebox switch, case=%d"), new_message_state);
  }

  if (m_message_state != new_message_state || m_old_radar_seen != radarSeen) {
    if (!radarSeen) {
      m_radar_off->Show();
    } else {
      m_radar_off->Hide();
    }

    switch (new_message_state) {
      case HIDE:
        Hide();
        break;

      case SHOW:
        Show();
        m_message_sizer->Show(m_nmea_sizer);
        m_close_button->Hide();
        break;

      case SHOW_NO_NMEA:
        Show();
        m_message_sizer->Hide(m_nmea_sizer);
        m_close_button->Hide();
        break;

      case SHOW_CLOSE:
        Show();
        m_message_sizer->Show(m_nmea_sizer);
        m_close_button->Show();
        break;
    }
  }
  Fit();
  m_top_sizer->Layout();
  Fit();
  m_old_radar_seen = radarSeen;
  m_message_state = new_message_state;
}

void br24MessageBox::OnMessageCloseButtonClick(wxCommandEvent &event) { Hide(); }

void br24MessageBox::SetRadarIPAddress(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  if (m_have_radar) {
    wxString label;

    label << _("Radar IP") << wxT(" ") << msg;
    m_have_radar->SetLabel(label);
  }
}

void br24MessageBox::SetErrorMessage(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  m_error_message->SetLabel(msg);
  m_top_sizer->Show(m_message_sizer);
  m_message_sizer->Layout();
  Fit();
  m_top_sizer->Layout();
}

void br24MessageBox::SetMcastIPAddress(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  if (m_ip_box) {
    wxString label;

    label << _("Ethernet card") << wxT(" ") << msg;
    m_ip_box->SetLabel(label);
  }
}

void br24MessageBox::SetHeadingInfo(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  if (m_have_heading && m_top_sizer->IsShown(m_message_sizer)) {
    wxString label;

    label << _("Heading") << wxT(" ") << msg;
    m_have_heading->SetLabel(label);
  }
}

void br24MessageBox::SetVariationInfo(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  if (m_have_variation && m_top_sizer->IsShown(m_message_sizer)) {
    wxString label;

    label << _("Variation") << wxT(" ") << msg;
    m_have_variation->SetLabel(label);
  }
}

void br24MessageBox::SetRadarInfo(wxString &msg) {
  wxMutexLocker lock(m_mutex);

  if (m_statistics && m_top_sizer->IsShown(m_message_sizer)) {
    m_statistics->SetLabel(msg);
  }
}

PLUGIN_END_NAMESPACE
