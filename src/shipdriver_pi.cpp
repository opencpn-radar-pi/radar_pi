/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  ShipDriver Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2017 by Mike Rossiter                                *
 *   $EMAIL$                                                               *
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

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "config.h"
#include "icons.h"
#include "shipdriver_pi.h"
#include "shipdriver_gui.h"
#include "shipdriver_gui_impl.h"
#include "ocpn_plugin.h"
#include "plug_utils.h"

class Dlg;

using namespace std;

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
  return new ShipDriverPi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) { delete p; }

//---------------------------------------------------------------------------------------------------------
//
//    ShipDriver PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

ShipDriverPi::ShipDriverPi(void* ppimgr)
    : opencpn_plugin_118(ppimgr),
      m_hr_dialog_x(0),
      m_hr_dialog_y(8),
      m_is_grib_valid(false),
      m_grib_lat(0),
      m_grib_lon(0),
      m_tr_spd(0),
      m_tr_dir(0),
      m_cursor_lat(0),
      m_cursor_lon(0),
      m_position_menu_id(-1),
      m_gui_scale_factor(1),
      plugin(nullptr),
      m_dialog(nullptr),
      m_config(nullptr),
      m_parent_window(nullptr),
      m_hr_dialog_width(0),
      m_hr_dialog_height(0),
      m_hr_dialog_sx(0),
      m_hr_dialog_sy(0),
      m_display_width(0),
      m_display_height(0),
      m_leftclick_tool_id(-1),
      m_show_shipdriver_icon(false),
      m_copy_use_ais(false),
      m_copy_use_file(false), 
      m_copy_use_nmea(false) 

{
  // Create the PlugIn icons
  initialize_images();
  auto icon_path = GetPluginIcon("shipdriver_panel_icon", PKG_NAME);
  if (icon_path.type == IconPath::Type::Svg)
    m_panel_bitmap = LoadSvgIcon(icon_path.path.c_str());
  else if (icon_path.type == IconPath::Type::Png)
    m_panel_bitmap = LoadPngIcon(icon_path.path.c_str());
  else  // icon_path.type == NotFound
    wxLogWarning("Cannot find icon for basename: %s", "shipdriver_panel_icon");
  if (m_panel_bitmap.IsOk())
    wxLogDebug("ShipDriverPi::, bitmap OK");
  else
    wxLogDebug("ShipDriverPi::, bitmap fail");
  m_show_shipdriver = false;
}

ShipDriverPi::~ShipDriverPi() {
  delete _img_ShipDriverIcon;

  if (m_dialog) {
    wxFileConfig* pConf = GetOCPNConfigObject();

    if (pConf) {
      pConf->SetPath("/PlugIns/ShipDriver_pi");
      pConf->Write("shipdriverUseAis", m_copy_use_ais);
      pConf->Write("shipdriverUseFile", m_copy_use_file);
      pConf->Write("shipdriverMMSI", m_copy_mmsi);
      pConf->Write("shipdriverUseNMEA", m_copy_use_nmea);
    }
  }
}

int ShipDriverPi::Init() {
  AddLocaleCatalog("opencpn-ShipDriver_pi");

  // Set some default private member parameters
  m_hr_dialog_x = 40;
  m_hr_dialog_y = 80;
  m_hr_dialog_sx = 400;
  m_hr_dialog_sy = 300;
  ::wxDisplaySize(&m_display_width, &m_display_height);

  //    Get a pointer to the opencpn display canvas, to use as a parent for
  //    the POI Manager dialog
  m_parent_window = GetOCPNCanvasWindow();

  //    Get a pointer to the opencpn configuration object
  m_config = GetOCPNConfigObject();

  //    And load the configuration items
  LoadConfig();
  auto icon = GetPluginIcon("shipdriver_pi", PKG_NAME);
  auto toggled_icon = GetPluginIcon("shipdriver_pi_toggled", PKG_NAME);
  //    This PlugIn needs a toolbar icon, so request its insertion
  if (m_show_shipdriver_icon) {
    if (icon.type == IconPath::Type::Svg)
      m_leftclick_tool_id = InsertPlugInToolSVG(
          "ShipDriver", icon.path, icon.path, toggled_icon.path, wxITEM_CHECK,
          "ShipDriver", "", nullptr, ShipDriver_TOOL_POSITION, 0, this);
    else if (icon.type == IconPath::Type::Png) {
      auto bitmap = LoadPngIcon(icon.path.c_str());
      m_leftclick_tool_id =
          InsertPlugInTool("", &bitmap, &bitmap, wxITEM_CHECK, "ShipDriver", "",
                           nullptr, ShipDriver_TOOL_POSITION, 0, this);
    }
  }
  m_dialog = nullptr;

  return (WANTS_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK |
          WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_CURSOR_LATLON |
          WANTS_NMEA_SENTENCES | WANTS_AIS_SENTENCES | WANTS_PREFERENCES |
          WANTS_PLUGIN_MESSAGING | WANTS_CONFIG);
}

bool ShipDriverPi::DeInit() {
  //    Record the dialog position
  if (m_dialog) {
    // Capture dialog position
    wxPoint p = m_dialog->GetPosition();
    wxRect r = m_dialog->GetRect();
    SetShipDriverDialogX(p.x);
    SetShipDriverDialogY(p.y);
    SetShipDriverDialogSizeX(r.GetWidth());
    SetShipDriverDialogSizeY(r.GetHeight());
    if (m_copy_use_nmea) {
      if (m_dialog->nmeastream->IsOpened()) {
        m_dialog->nmeastream->Write();
        m_dialog->nmeastream->Close();
      }
    }
  

    if ((m_dialog->m_timer) && (m_dialog->m_timer->IsRunning())) {
      // need to stop the timer or crash on exit
      m_dialog->m_timer->Stop();
    }
    m_dialog->Close();
    delete m_dialog;
    m_dialog = nullptr;

    m_show_shipdriver = false;
    SetToolbarItemState(m_leftclick_tool_id, m_show_shipdriver);
  }

  SaveConfig();

  RequestRefresh(m_parent_window);  // refresh main window

  return true;
}

int ShipDriverPi::GetAPIVersionMajor() { return atoi(API_VERSION); }

int ShipDriverPi::GetAPIVersionMinor() {
  std::string v(API_VERSION);
  size_t dotpos = v.find('.');
  return atoi(v.substr(dotpos + 1).c_str());
}

int ShipDriverPi::GetPlugInVersionMajor() { return PLUGIN_VERSION_MAJOR; }

int ShipDriverPi::GetPlugInVersionMinor() { return PLUGIN_VERSION_MINOR; }

int ShipDriverPi::GetPlugInVersionPatch() { return PLUGIN_VERSION_PATCH; }

int ShipDriverPi::GetPlugInVersionPost() { return PLUGIN_VERSION_TWEAK; };

const char* ShipDriverPi::GetPlugInVersionPre() { return PKG_PRERELEASE; }

const char* ShipDriverPi::GetPlugInVersionBuild() { return PKG_BUILD_INFO; }

wxBitmap* ShipDriverPi::GetPlugInBitmap() { return &m_panel_bitmap; }

wxString ShipDriverPi::GetCommonName() { return PLUGIN_API_NAME; }

wxString ShipDriverPi::GetShortDescription() { return PKG_SUMMARY; }

wxString ShipDriverPi::GetLongDescription() { return PKG_DESCRIPTION; }

int ShipDriverPi::GetToolbarToolCount() { return 1; }

void ShipDriverPi::SetColorScheme(PI_ColorScheme cs) {
  if (!m_dialog) return;
  DimeWindow(m_dialog);
}

void ShipDriverPi::ShowPreferencesDialog(wxWindow* parent) {
  auto* pref = new shipdriverPreferences(parent);

  pref->m_cbTransmitAis->SetValue(m_copy_use_ais);
  pref->m_cbAisToFile->SetValue(m_copy_use_file);
  pref->m_textCtrlMMSI->SetValue(m_copy_mmsi);
  pref->m_cbNMEAToFile->SetValue(m_copy_use_nmea);

  if (pref->ShowModal() == wxID_OK) {
    bool copy_ais = pref->m_cbTransmitAis->GetValue();
    bool copy_file = pref->m_cbAisToFile->GetValue();
    wxString copyMMSI = pref->m_textCtrlMMSI->GetValue();
    bool copy_nmea = pref->m_cbNMEAToFile->GetValue();


    if (m_copy_use_ais != copy_ais || m_copy_use_file != copy_file ||
        m_copy_mmsi != copyMMSI) {
      m_copy_use_ais = copy_ais;
      m_copy_use_file = copy_file;
      m_copy_mmsi = copyMMSI;
    }

    if (m_copy_use_nmea != copy_nmea) {
      m_copy_use_nmea = copy_nmea;
    }

    if (m_dialog) {
      m_dialog->m_bUseAis = m_copy_use_ais;
      m_dialog->m_bUseFile = m_copy_use_file;
      m_dialog->m_tMMSI = m_copy_mmsi;
      m_dialog->m_bUseNMEA = m_copy_use_nmea;
    }

    SaveConfig();

    RequestRefresh(m_parent_window);  // refresh main window
  }

  delete pref;
  pref = nullptr;
}

void ShipDriverPi::OnToolbarToolCallback(int id) {
  if (!m_dialog) {
    m_dialog = new Dlg(m_parent_window);
    m_dialog->plugin = this;
    m_dialog->m_timer = new wxTimer(m_dialog);
    m_dialog->Move(wxPoint(m_hr_dialog_x, m_hr_dialog_y));
    m_dialog->SetSize(m_hr_dialog_sx, m_hr_dialog_sy);

    wxMenu dummy_menu;
    m_position_menu_id = AddCanvasContextMenuItem(
        new wxMenuItem(&dummy_menu, -1, _("Select Vessel Start Position")),
        this);
    SetCanvasContextMenuItemViz(m_position_menu_id, true);
  }

  // m_pDialog->Fit();
  // Toggle
  m_show_shipdriver = !m_show_shipdriver;

  //    Toggle dialog?
  if (m_show_shipdriver) {
    m_dialog->Move(wxPoint(m_hr_dialog_x, m_hr_dialog_y));
    m_dialog->SetSize(m_hr_dialog_sx, m_hr_dialog_sy);
    m_dialog->Show();

  } else {
    m_dialog->Hide();
  }

  // Toggle is handled by the toolbar but we must keep plugin manager b_toggle
  // updated to actual status to ensure correct status upon toolbar rebuild
  SetToolbarItemState(m_leftclick_tool_id, m_show_shipdriver);

  // Capture dialog position
  wxPoint p = m_dialog->GetPosition();
  wxRect r = m_dialog->GetRect();
  SetShipDriverDialogX(p.x);
  SetShipDriverDialogY(p.y);
  SetShipDriverDialogSizeX(r.GetWidth());
  SetShipDriverDialogSizeY(r.GetHeight());

  RequestRefresh(m_parent_window);  // refresh main window
}

bool ShipDriverPi::LoadConfig() {
  auto* conf = (wxFileConfig*)m_config;

  if (conf) {
    if (conf->HasGroup(_T("/Settings/ShipDriver_pi"))) {
      // Read the existing settings

      conf->SetPath("/Settings/ShipDriver_pi");
      conf->Read("ShowShipDriverIcon", &m_show_shipdriver_icon, true);
      conf->Read("shipdriverUseAis", &m_copy_use_ais, false);
      conf->Read("shipdriverUseNMEA", &m_copy_use_nmea, false);
      conf->Read("shipdriverUseFile", &m_copy_use_file, false);
      m_copy_mmsi = conf->Read("shipdriverMMSI", "123456789");

      m_hr_dialog_x = conf->Read("DialogPosX", 40L);
      m_hr_dialog_y = conf->Read("DialogPosY", 140L);
      m_hr_dialog_sx = conf->Read("DialogSizeX", 330L);
#ifdef __WXOSX__
      m_hr_dialog_sy = conf->Read("DialogSizeY", 250L);
#else
      m_hr_dialog_sy = conf->Read("DialogSizeY", 300L);
#endif
      conf->DeleteGroup(_T("/Settings/ShipDriver_pi"));
    } else {
      conf->SetPath("/PlugIns/ShipDriver_pi");
      conf->Read("ShowShipDriverIcon", &m_show_shipdriver_icon, true);
      conf->Read("shipdriverUseAis", &m_copy_use_ais, false);
      conf->Read("shipdriverUseNMEA", &m_copy_use_nmea, false);
      conf->Read("shipdriverUseFile", &m_copy_use_file, false);
      m_copy_mmsi = conf->Read("shipdriverMMSI", "123456789");

      m_hr_dialog_x = conf->Read("DialogPosX", 40L);
      m_hr_dialog_y = conf->Read("DialogPosY", 140L);
      m_hr_dialog_sx = conf->Read("DialogSizeX", 330L);
#ifdef __WXOSX__
      m_hr_dialog_sy = conf->Read("DialogSizeY", 250L);
#else
      m_hr_dialog_sy = conf->Read("DialogSizeY", 300L);
#endif
    }
    if ((m_hr_dialog_x < 0) || (m_hr_dialog_x > m_display_width))
      m_hr_dialog_x = 40;
    if ((m_hr_dialog_y < 0) || (m_hr_dialog_y > m_display_height))
      m_hr_dialog_y = 140;

    return true;
  } else
    return false;
}

bool ShipDriverPi::SaveConfig() {
  auto* conf = (wxFileConfig*)m_config;

  if (conf) {
    bool is_digits = m_copy_mmsi.IsNumber();
    if (m_copy_mmsi.length() < 9 || !is_digits) {
      wxMessageBox(_("MMSI must be 9 digits.\nEdit using Preferences"));
      return false;
    }

    conf->SetPath("/PlugIns/ShipDriver_pi");
    conf->Write("ShowShipDriverIcon", m_show_shipdriver_icon);
    conf->Write("shipdriverUseAis", m_copy_use_ais);
    conf->Write("shipdriverUseNMEA", m_copy_use_nmea);
    conf->Write("shipdriverUseFile", m_copy_use_file);
    conf->Write("shipdriverMMSI", m_copy_mmsi);

    conf->Write("DialogPosX", m_hr_dialog_x);
    conf->Write("DialogPosY", m_hr_dialog_y);
    conf->Write("DialogSizeX", m_hr_dialog_sx);
    conf->Write("DialogSizeY", m_hr_dialog_sy);

    return true;
  } else
    return false;
}

void ShipDriverPi::OnShipDriverDialogClose() {
  m_show_shipdriver = false;
  SetToolbarItemState(m_leftclick_tool_id, m_show_shipdriver);
  m_dialog->Hide();
  SaveConfig();

  RequestRefresh(m_parent_window);  // refresh main window
}

void ShipDriverPi::OnContextMenuItemCallback(int id) {
  if (!m_dialog) return;

  if (id == m_position_menu_id) {
    m_cursor_lat = GetCursorLat();
    m_cursor_lon = GetCursorLon();

    m_dialog->OnContextMenu(m_cursor_lat, m_cursor_lon);
  }
}

void ShipDriverPi::SetCursorLatLon(double lat, double lon) {
  m_cursor_lat = lat;
  m_cursor_lon = lon;
}

void ShipDriverPi::SetPluginMessage(wxString& message_id,
                                    wxString& message_body) {
  if (message_id == "GRIB_TIMELINE") {
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    Json::Value value;
    string errors;

    bool parsingSuccessful = reader->parse(
        message_body.c_str(), message_body.c_str() + message_body.size(),
        &value, &errors);
    delete reader;

    if (!parsingSuccessful) {
      wxLogMessage("Grib_TimeLine error");
      return;
    }

    // int day = value["Day"].asInt();

    wxDateTime time;
    time.Set(value["Day"].asInt(), (wxDateTime::Month)value["Month"].asInt(),
             value["Year"].asInt(), value["Hour"].asInt(),
             value["Minute"].asInt(), value["Second"].asInt());

    if (m_dialog) {
      m_dialog->m_GribTimelineTime = time.ToUTC();
      // m_pDialog->m_textCtrl1->SetValue(dt);
    }
  }
  if (message_id == "GRIB_TIMELINE_RECORD") {
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    Json::Value value;
    string errors;

    bool parsingSuccessful = reader->parse(
        message_body.c_str(), message_body.c_str() + message_body.size(),
        &value, &errors);
    delete reader;

    if (!parsingSuccessful) {
      wxLogMessage("Grib_TimeLine_Record error");
      return;
    }

    static bool shown_warnings;
    if (!shown_warnings) {
      shown_warnings = true;

      int grib_version_major = value["GribVersionMajor"].asInt();
      int grib_version_minor = value["GribVersionMinor"].asInt();

      int grib_version = 1000 * grib_version_major + grib_version_minor;
      int grib_min = 1000 * GRIB_MIN_MAJOR + GRIB_MIN_MINOR;
      int grib_max = 1000 * GRIB_MAX_MAJOR + GRIB_MAX_MINOR;

      if (grib_version < grib_min || grib_version > grib_max) {
        wxMessageDialog mdlg(
            m_parent_window,
            _("Grib plugin version not supported.") + "\n\n" +
                wxString::Format(_("Use versions %d.%d to %d.%d"),
                                 GRIB_MIN_MAJOR, GRIB_MIN_MINOR, GRIB_MAX_MAJOR,
                                 GRIB_MAX_MINOR),
            _("Weather Routing"), wxOK | wxICON_WARNING);
        mdlg.ShowModal();
      }
    }

    wxString sptr = value["TimelineSetPtr"].asString();

    wxCharBuffer bptr = sptr.To8BitData();
    const char* ptr = bptr.data();

    GribRecordSet* gptr;
    sscanf(ptr, "%p", &gptr);

    double dir, spd;

    m_is_grib_valid = GribCurrent(gptr, m_grib_lat, m_grib_lon, dir, spd);

    m_tr_spd = spd;
    m_tr_dir = dir;
  }
}

[[maybe_unused]] bool ShipDriverPi::GribWind(GribRecordSet* grib, double lat,
                                             double lon, double& wg,
                                             double& vwg) {
  if (!grib) return false;

  if (!GribRecord::getInterpolatedValues(
          vwg, wg, grib->m_GribRecordPtrArray[Idx_WIND_VX],
          grib->m_GribRecordPtrArray[Idx_WIND_VY], lon, lat))
    return false;

  vwg *= 3.6 / 1.852;  // knots

#if 0
// test
	VWG = 0.;
	WG = 0.;
#endif

  return true;
}

void ShipDriverPi::SetNMEASentence(wxString& sentence) {
  if (m_dialog) {
    m_dialog->SetNMEAMessage(sentence);
  }
}
