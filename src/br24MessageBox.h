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

#ifndef _BR24MESSAGEBOX_H_
#define _BR24MESSAGEBOX_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

enum message_status { HIDE, SHOW, SHOW_NO_NMEA, SHOW_CLOSE };

class radar_info_item {
 public:
  void Update(wxString &v) {
    wxCriticalSectionLocker lock(m_exclusive);

    mod = true;
    value = v;
  };

  bool GetNewValue(wxString *str) {
    if (mod) {
      wxCriticalSectionLocker lock(m_exclusive);

      mod = false;
      *str = value;
      return true;
    }
    return false;
  }

  void GetValue(wxString *str) {
    wxCriticalSectionLocker lock(m_exclusive);

    *str = value;
  }

  radar_info_item() { mod = false; }

 private:
  wxCriticalSection m_exclusive;
  wxString value;
  bool mod;
};

class br24MessageBox : public wxDialog {
  DECLARE_CLASS(br24MessageBox)
  DECLARE_EVENT_TABLE()

 public:
  br24MessageBox();

  ~br24MessageBox();
  void Init();
  bool Show(bool show = true);

  bool Create(wxWindow *parent, br24radar_pi *pi, wxWindowID id = wxID_ANY, const wxString &caption = _("Radar"),
              const wxPoint &pos = wxDefaultPosition);

  void CreateControls();
  bool UpdateMessage(bool force);  // Check whether message box needs to be visible, return true if shown
  // void SetErrorMessage(wxString &msg);
  void SetRadarBuildInfo(wxString &msg);
  void SetRadarIPAddress(wxString &msg);
  void SetRadarType(RadarType radar_type);
  void SetMcastIPAddress(wxString &msg);
  void SetTrueHeadingInfo(wxString &msg);
  void SetMagHeadingInfo(wxString &msg);
  void SetVariationInfo(wxString &msg);
  void SetStatisticsInfo(wxString &msg);

  void OnClose(wxCloseEvent &event);

 private:
  void OnIdOKClick(wxCommandEvent &event);
  void OnMove(wxMoveEvent &event);
  void OnSize(wxSizeEvent &event);

  void OnMessageCloseButtonClick(wxCommandEvent &event);
  void OnMessageHideRadarClick(wxCommandEvent &event);

  bool IsModalDialogShown();

  wxWindow *m_parent;
  br24radar_pi *m_pi;

  radar_info_item m_build_info;
  radar_info_item m_radar_type_info;
  // radar_info_item m_error_message_info;
  radar_info_item m_radar_addr_info;
  radar_info_item m_mcast_addr_info;
  radar_info_item m_true_heading_info;
  radar_info_item m_mag_heading_info;
  radar_info_item m_variation_info;
  radar_info_item m_statistics_info;

  message_status m_message_state;
  bool m_old_radar_seen;
  bool m_allow_auto_hide;

  wxBoxSizer *m_top_sizer;
  wxBoxSizer *m_nmea_sizer;
  wxBoxSizer *m_info_sizer;

  wxBoxSizer *m_message_sizer;  // Contains NO HDG and/or NO GPS
  wxStaticBox *m_ip_box;

  // MessageBox
  wxButton *m_close_button;
  wxButton *m_hide_radar;
  wxStaticText *m_error_message;
  wxStaticText *m_radar_off;
  wxCheckBox *m_have_open_gl;
  wxCheckBox *m_have_boat_pos;
  wxCheckBox *m_have_true_heading;
  wxCheckBox *m_have_mag_heading;
  wxCheckBox *m_have_variation;
  wxCheckBox *m_have_radar;
  wxCheckBox *m_have_data;
  wxStaticText *m_statistics;
};

PLUGIN_END_NAMESPACE

#endif
