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

#include "br24radar_pi.h"
#include "GuardZoneBogey.h"
#include "Kalman.h"
#include "RadarMarpa.h"
#include "icons.h"
#include "nmea0183/nmea0183.h"

PLUGIN_BEGIN_NAMESPACE

#undef M_SETTINGS
#define M_SETTINGS m_settings

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin *create_pi(void *ppimgr) { return new br24radar_pi(ppimgr); }

extern "C" DECL_EXP void destroy_pi(opencpn_plugin *p) { delete p; }

/********************************************************************************************************/
//   Distance measurement for simple sphere
/********************************************************************************************************/

double local_distance(double lat1, double lon1, double lat2, double lon2) {
  double s1 = deg2rad(lat1);
  double l1 = deg2rad(lon1);
  double s2 = deg2rad(lat2);
  double l2 = deg2rad(lon2);
  double theta = l2 - l1;

  // Spherical Law of Cosines
  double dist = acos(sin(s1) * sin(s2) + cos(s1) * cos(s2) * cos(theta));

  dist = fabs(rad2deg(dist)) * 60;  // nautical miles/degree
  return dist;
}

double local_bearing(double lat1, double lon1, double lat2, double lon2) {
  double s1 = deg2rad(lat1);
  double l1 = deg2rad(lon1);
  double s2 = deg2rad(lat2);
  double l2 = deg2rad(lon2);
  double theta = l2 - l1;

  double y = sin(theta) * cos(s2);
  double x = cos(s1) * sin(s2) - sin(s1) * cos(s2) * cos(theta);

  double brg = fmod(rad2deg(atan2(y, x)) + 360.0, 360.0);
  return brg;
}

static double radar_distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double dist = local_distance(lat1, lon1, lat2, lon2);

  switch (unit) {
    case 'M':  // statute miles
      dist = dist * 1.1515;
      break;
    case 'K':  // kilometers
      dist = dist * 1.852;
      break;
    case 'm':  // meters
      dist = dist * 1852.0;
      break;
    case 'N':  // nautical miles
      break;
  }
  return dist;
}

//---------------------------------------------------------------------------------------------------------
//
//    BR24Radar PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

enum { TIMER_ID = 51 };

BEGIN_EVENT_TABLE(br24radar_pi, wxEvtHandler)
EVT_TIMER(TIMER_ID, br24radar_pi::OnTimerNotify)
END_EVENT_TABLE()

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

br24radar_pi::br24radar_pi(void *ppimgr) : opencpn_plugin_114(ppimgr) {
  m_boot_time = wxGetUTCTimeMillis();
  m_initialized = false;

  // Create the PlugIn icons
  initialize_images();
  m_pdeficon = new wxBitmap(*_img_radar_blank);

  m_opengl_mode = OPENGL_UNKOWN;
  m_opengl_mode_changed = false;
  m_opencpn_gl_context = 0;
  m_opencpn_gl_context_broken = false;

  m_timer = 0;

  m_first_init = true;
}

br24radar_pi::~br24radar_pi() {}

/*
 * Init() is called -every- time that the plugin is enabled. If a user is being nasty
 * they can enable/disable multiple times in the overview. Grrr!
 *
 */

int br24radar_pi::Init(void) {
  if (m_initialized) {
    // Whoops, shouldn't happen
    return PLUGIN_OPTIONS;
  }

  if (m_first_init) {
#ifdef __WXMSW__
    WSADATA wsaData;

    // Initialize Winsock
    DWORD r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r != 0) {
      wxLogError(wxT("BR24radar_pi: Unable to initialise Windows Sockets, error %d"), r);
      // Might as well give up now
      return 0;
    }
#endif

    AddLocaleCatalog(_T("opencpn-br24radar_pi"));

    m_pconfig = GetOCPNConfigObject();
    m_first_init = false;
  }

  time_t now = time(0);

  // Font can change so initialize every time
  m_font = GetOCPNGUIScaledFont_PlugIn(_T("Dialog"));
  m_fat_font = m_font;
  m_fat_font.SetWeight(wxFONTWEIGHT_BOLD);
  m_fat_font.SetPointSize(m_font.GetPointSize() + 1);

  m_var = 0.0;
  m_var_source = VARIATION_SOURCE_NONE;
  m_bpos_set = false;
  m_ownship_lat = nan("");
  m_ownship_lon = nan("");
  m_radar_lat = nan("");
  m_radar_lon = nan("");
  m_cursor_lat = nan("");
  m_cursor_lon = nan("");

  m_guard_bogey_seen = false;
  m_guard_bogey_confirmed = false;
  m_sent_toolbar_button = TB_NONE;
  m_toolbar_button = TB_NONE;
  m_opengl_mode_changed = false;
  m_notify_radar_window_viz = false;
  m_notify_control_dialog = false;

  m_bogey_dialog = 0;
  m_alarm_sound_timeout = 0;
  m_guard_bogey_timeout = 0;
  m_bpos_timestamp = now;
  m_hdt = 0.0;
  m_hdt_timeout = now + WATCHDOG_TIMEOUT;
  m_hdm_timeout = now + WATCHDOG_TIMEOUT;
  m_var_timeout = now + WATCHDOG_TIMEOUT;
  m_cog_timeout = now;
  m_idle_standby = 0;
  m_idle_transmit = 0;
  m_heading_source = HEADING_NONE;
  m_radar_heading = nanl("");
  m_vp_rotation = 0.;

  // Set default settings before we load config. Prevents random behavior on uninitalized behavior.
  // For instance, LOG_XXX messages before config is loaded.
  m_settings.verbose = 0;
  m_settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;
  m_settings.refreshrate = 1;
  m_settings.timed_idle = 0;
  m_settings.threshold_blue = 255;
  m_settings.threshold_red = 255;
  m_settings.threshold_green = 255;
  m_settings.mcast_address = wxT("");

  // Get a pointer to the opencpn display canvas, to use as a parent for the UI
  // dialog
  m_parent_window = GetOCPNCanvasWindow();
  m_shareLocn = *GetpSharedDataLocation() + _T("plugins") + wxFileName::GetPathSeparator() + _T("br24radar_pi") +
                wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();

  m_pMessageBox = new br24MessageBox;
  m_pMessageBox->Create(m_parent_window, this);

  // Create objects before config, so config can set data in it
  // This does not start any threads or generate any UI.
  for (int r = 0; r < RADARS; r++) {
    m_radar[r] = new RadarInfo(this, r);
  }

  //    And load the configuration items
  if (LoadConfig()) {
    LOG_INFO(wxT("BR24radar_pi: Configuration file values initialised"));
    LOG_INFO(wxT("BR24radar_pi: Log verbosity = %d. To modify, set VerboseLog to sum of:"), m_settings.verbose);
    LOG_INFO(wxT("BR24radar_pi: VERBOSE  = %d"), LOGLEVEL_VERBOSE);
    LOG_INFO(wxT("BR24radar_pi: DIALOG   = %d"), LOGLEVEL_DIALOG);
    LOG_INFO(wxT("BR24radar_pi: TRANSMIT = %d"), LOGLEVEL_TRANSMIT);
    LOG_INFO(wxT("BR24radar_pi: RECEIVE  = %d"), LOGLEVEL_RECEIVE);
    LOG_INFO(wxT("BR24radar_pi: GUARD    = %d"), LOGLEVEL_GUARD);
    LOG_INFO(wxT("BR24radar_pi: ARPA     = %d"), LOGLEVEL_ARPA);
    LOG_VERBOSE(wxT("BR24radar_pi: VERBOSE  log is enabled"));
    LOG_DIALOG(wxT("BR24radar_pi: DIALOG   log is enabled"));
    LOG_TRANSMIT(wxT("BR24radar_pi: TRANSMIT log is enabled"));
    LOG_RECEIVE(wxT("BR24radar_pi: RECEIVE  log is enabled"));
    LOG_GUARD(wxT("BR24radar_pi: GUARD    log is enabled"));
    LOG_ARPA(wxT("BR24radar_pi: ARPA     log is enabled"));
  } else {
    wxLogError(wxT("BR24radar_pi: configuration file values initialisation failed"));
    return 0;  // give up
  }

  // After load config
  m_radar[0]->Init(m_settings.enable_dual_radar ? _("Radar A") : _("Radar"), m_settings.verbose);
  m_radar[1]->Init(_("Radar B"), m_settings.verbose);

  //    This PlugIn needs a toolbar icon

  wxString svg_normal = m_shareLocn + wxT("radar_standby.svg");
  wxString svg_rollover = m_shareLocn + wxT("radar_searching.svg");
  wxString svg_toggled = m_shareLocn + wxT("radar_active.svg");
  m_tool_id = InsertPlugInToolSVG(wxT("Navico"), svg_normal, svg_rollover, svg_toggled, wxITEM_NORMAL, wxT("BR24Radar"),
                                  _("Navico BR24, 3G and 4G RADAR"), NULL, BR24RADAR_TOOL_POSITION, 0, this);

  // CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);

  //    In order to avoid an ASSERT on msw debug builds,
  //    we need to create a dummy menu to act as a surrogate parent of the created MenuItems
  //    The Items will be re-parented when added to the real context meenu
  wxMenu dummy_menu;

  wxMenuItem *mi1 = new wxMenuItem(&dummy_menu, -1, _("Show radar"));
  wxMenuItem *mi2 = new wxMenuItem(&dummy_menu, -1, _("Hide radar"));
  wxMenuItem *mi3 = new wxMenuItem(&dummy_menu, -1, _("Radar Control..."));
  wxMenuItem *mi4 = new wxMenuItem(&dummy_menu, -1, _("Acquire radar target"));
  wxMenuItem *mi5 = new wxMenuItem(&dummy_menu, -1, _("Delete radar target"));
  wxMenuItem *mi6 = new wxMenuItem(&dummy_menu, -1, _("Delete all radar targets"));

#ifdef __WXMSW__
  wxFont *qFont = OCPNGetFont(_("Menu"), 10);
  mi1->SetFont(*qFont);
  mi2->SetFont(*qFont);
  mi3->SetFont(*qFont);
  mi4->SetFont(*qFont);
  mi5->SetFont(*qFont);
  mi6->SetFont(*qFont);
#endif
  m_context_menu_show_id = AddCanvasContextMenuItem(mi1, this);
  m_context_menu_hide_id = AddCanvasContextMenuItem(mi2, this);
  m_context_menu_control_id = AddCanvasContextMenuItem(mi3, this);
  m_context_menu_acquire_radar_target = AddCanvasContextMenuItem(mi4, this);
  m_context_menu_delete_radar_target = AddCanvasContextMenuItem(mi5, this);
  m_context_menu_delete_all_radar_targets = AddCanvasContextMenuItem(mi6, this);
  m_context_menu_show = true;
  m_context_menu_control = false;
  m_context_menu_arpa = false;
  SetCanvasContextMenuItemViz(m_context_menu_show_id, false);

  m_initialized = true;
  LOG_VERBOSE(wxT("BR24radar_pi: Initialized plugin transmit=%d/%d overlay=%d"), m_settings.show_radar[0], m_settings.show_radar[1],
              m_settings.chart_overlay);

  m_notify_time_ms = 0;
  m_timer = new wxTimer(this, TIMER_ID);
  SetRadarWindowViz();
  TimedControlUpdate();
  m_radar[0]->StartReceive();
  if (m_settings.enable_dual_radar) {
    m_radar[1]->StartReceive();
  }
  return PLUGIN_OPTIONS;
}

/**
 * DeInit() is called when OpenCPN is quitting or when the user disables the plugin.
 *
 * This should get rid of all on-screen objects and deallocate memory.
 */

bool br24radar_pi::DeInit(void) {
  if (!m_initialized) {
    return false;
  }

  LOG_VERBOSE(wxT("BR24radar_pi: DeInit of plugin"));

  m_initialized = false;

  if (m_timer) {
    m_timer->Stop();
    delete m_timer;
    m_timer = 0;
  }

  // Stop processing in all radars.
  // This waits for the receive threads to stop and removes the dialog, so that its settings
  // can be saved.
  for (int r = 0; r < RADARS; r++) {
    m_radar[r]->Shutdown();
  }

  if (m_bogey_dialog) {
    delete m_bogey_dialog;  // This will also save its current pos in m_settings
    m_bogey_dialog = 0;
  }

  SaveConfig();

  // Delete the RadarInfo objects. This will call their destructor and delete all data.
  for (int r = 0; r < RADARS; r++) {
    delete m_radar[r];
    m_radar[r] = 0;
  }

  // No need to delete wxWindow stuff, wxWidgets does this for us.
  LOG_VERBOSE(wxT("BR24radar_pi: DeInit of plugin done"));
  return true;
}

int br24radar_pi::GetAPIVersionMajor() { return MY_API_VERSION_MAJOR; }

int br24radar_pi::GetAPIVersionMinor() { return MY_API_VERSION_MINOR; }

int br24radar_pi::GetPlugInVersionMajor() { return PLUGIN_VERSION_MAJOR; }

int br24radar_pi::GetPlugInVersionMinor() { return PLUGIN_VERSION_MINOR; }

wxBitmap *br24radar_pi::GetPlugInBitmap() { return m_pdeficon; }

wxString br24radar_pi::GetCommonName() { return wxT("BR24Radar"); }

wxString br24radar_pi::GetShortDescription() { return _("Navico Radar PlugIn for OpenCPN"); }

wxString br24radar_pi::GetLongDescription() { return _("Navico Broadband BR24/3G/4G Radar PlugIn for OpenCPN\n"); }

void br24radar_pi::SetDefaults(void) {
  // This will be called upon enabling a PlugIn via the user Dialog.
  // We don't need to do anything special here.
}

void br24radar_pi::ShowPreferencesDialog(wxWindow *parent) {
  LOG_DIALOG(wxT("BR24radar_pi: ShowPreferencesDialog"));

  br24OptionsDialog dlg(parent, m_settings, m_radar[0]->m_radar_type);
  if (dlg.ShowModal() == wxID_OK) {
    bool old_emulator = m_settings.emulator_on;
    m_settings = dlg.GetSettings();
    SaveConfig();
    if (!m_settings.emulator_on && old_emulator) {  // If the *OLD* setting had emulator on, re-detect radar type
      m_radar[0]->m_radar_type = RT_UNKNOWN;
      m_radar[1]->m_radar_type = RT_UNKNOWN;
    }
    if (m_settings.enable_dual_radar) {
      m_radar[0]->SetName(_("Radar A"));
      m_radar[1]->StartReceive();
    } else {
      m_radar[1]->ShowRadarWindow(false);
      ShowRadarControl(1, false);
    }
    for (size_t r = 0; r < RADARS; r++) {
      m_radar[r]->ComputeColourMap();
      m_radar[r]->UpdateControlState(true);
    }
    if (!m_guard_bogey_confirmed && m_alarm_sound_timeout && m_settings.guard_zone_timeout) {
      m_alarm_sound_timeout = time(0) + m_settings.guard_zone_timeout;
    }
  }
}

// A different thread (or even the control dialog itself) has changed state and now
// the radar window and control visibility needs to be reset. It can't call SetRadarWindowViz()
// directly so we redirect via flag and main thread.
void br24radar_pi::NotifyRadarWindowViz() { m_notify_radar_window_viz = true; }

// A different thread (or even the control dialog itself) has changed state and now
// the content of the control dialog needs to be reset (but not its visibility, that is what
// NotifyRadarWindowViz() does.)
//
void br24radar_pi::NotifyControlDialog() { m_notify_control_dialog = true; }

void br24radar_pi::SetRadarWindowViz(bool reparent) {
  for (int r = 0; r < RADARS; r++) {
    bool showThisRadar = m_settings.show && m_settings.show_radar[r] && (r == 0 || m_settings.enable_dual_radar);
    bool showThisControl = m_settings.show && m_settings.show_radar_control[r] && (r == 0 || m_settings.enable_dual_radar);
    m_radar[r]->ShowRadarWindow(showThisRadar);
    m_radar[r]->ShowControlDialog(showThisControl, reparent);
    m_radar[r]->UpdateTransmitState();
  }

  LOG_DIALOG(wxT("BR24radar_pi: RadarWindow show = %d window0=%d window1=%d"), m_settings.show, m_settings.show_radar[0],
             m_settings.show_radar[1]);

  UpdateContextMenu();
}

void br24radar_pi::UpdateContextMenu() {
  int arpa_targets = 0;

  for (int r = 0; r < RADARS; r++) {
    arpa_targets += m_radar[r]->m_arpa->GetTargetCount();
  }
  bool show = m_settings.show;
  bool control = false;
  bool arpa = arpa_targets == 0;

  if (m_settings.chart_overlay >= 0) {
    control = m_settings.show_radar_control[m_settings.chart_overlay];
  } else {
    control = true;
    for (int r = 0; r < RADARS; r++) {
      if (!m_settings.show_radar_control[r]) {
        control = false;
      }
    }
  }

  if (m_context_menu_arpa != arpa) {
    SetCanvasContextMenuItemGrey(m_context_menu_delete_radar_target, arpa);
    SetCanvasContextMenuItemGrey(m_context_menu_delete_all_radar_targets, arpa);
    m_context_menu_arpa = arpa;
    LOG_DIALOG(wxT("BR24radar_pi: ContextMenu arpa nr of targets = %d"), arpa_targets);
  }
  if (m_context_menu_control != control) {
    SetCanvasContextMenuItemGrey(m_context_menu_control_id, control);
    m_context_menu_control = control;
    LOG_DIALOG(wxT("BR24radar_pi: ContextMenu control = %d"), control);
  }

  if (m_context_menu_show != show) {
    SetCanvasContextMenuItemViz(m_context_menu_show_id, !show);
    SetCanvasContextMenuItemViz(m_context_menu_hide_id, show);
    SetCanvasContextMenuItemViz(m_context_menu_control_id, show);
    SetCanvasContextMenuItemViz(m_context_menu_acquire_radar_target, show);
    SetCanvasContextMenuItemViz(m_context_menu_delete_radar_target, show);
    SetCanvasContextMenuItemViz(m_context_menu_delete_all_radar_targets, show);
    m_context_menu_show = show;
    LOG_DIALOG(wxT("BR24radar_pi: ContextMenu show = %d"), show);
  }
}

//********************************************************************************
// Operation Dialogs - Control, Manual, and Options

void br24radar_pi::ShowRadarControl(int radar, bool show, bool reparent) {
  LOG_DIALOG(wxT("BR24radar_pi: ShowRadarControl(%d, %d)"), radar, (int)show);
  m_settings.show_radar_control[radar] = show;
  m_radar[radar]->ShowControlDialog(show, reparent);
}

void br24radar_pi::OnControlDialogClose(RadarInfo *ri) {
  if (ri->m_control_dialog) {
    m_settings.control_pos[ri->m_radar] = ri->m_control_dialog->GetPosition();
  }
  m_settings.show_radar_control[ri->m_radar] = false;
  if (ri->m_control_dialog) {
    ri->m_control_dialog->HideDialog();
  }
}

void br24radar_pi::ConfirmGuardZoneBogeys() {
  m_guard_bogey_confirmed = true;  // This will stop the sound being repeated
}

//*******************************************************************************
// ToolBar Actions

int br24radar_pi::GetToolbarToolCount(void) { return 1; }

/*
 * The radar icon is clicked. In previous versions all sorts of behavior was linked to clicking on the button, which wasn't very
 * 'discoverable' -- hard to find out what your options are.
 * In this version there are two behaviors.
 * - If the radar windows are not shown:
 *    - If the radar overlay is active and the radar control dialog is not shown, we show that control dialog only.
 *    - Else show the radar windows.
 * - Else close all radar windows and control dialogs.
 *
 * that way all state decisions are visual, without extra timers.
 *
 */
void br24radar_pi::OnToolbarToolCallback(int id) {
  if (!m_initialized) {
    return;
  }

  LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback"));

  if (m_pMessageBox->UpdateMessage(false)) {
    // Conditions for radar not satisfied, hide radar windows
    m_settings.show = 0;
    SetRadarWindowViz();
    return;
  }

  if (m_settings.show) {
    if (m_settings.chart_overlay >= 0 &&
        (!m_radar[m_settings.chart_overlay]->m_control_dialog || !m_radar[m_settings.chart_overlay]->m_control_dialog->IsShown())) {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Show control"));
      ShowRadarControl(m_settings.chart_overlay, true);
    } else {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Hide radar windows"));
      m_settings.show = 0;
      SetRadarWindowViz();
    }
  } else {
    LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Show radar windows"));
    m_settings.show = 1;
    SetRadarWindowViz();
  }

  UpdateState();
}

void br24radar_pi::OnContextMenuItemCallback(int id) {
  if (id == m_context_menu_control_id) {
    bool done = false;
    if (m_settings.chart_overlay >= 0) {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: overlay is active -> show control"));
      ShowRadarControl(m_settings.chart_overlay, true);
      done = true;
    } else {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: show controls of visible radars"));
      for (int r = 0; r < RADARS; r++) {
        if (m_settings.show_radar[r]) {
          ShowRadarControl(r, true);
          done = true;
        }
      }
    }
    if (!done) {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: nothing visible, make radar A overlay"));
      m_settings.chart_overlay = 0;
      ShowRadarControl(m_settings.chart_overlay, true);
    }
  } else if (id == m_context_menu_hide_id) {
    m_settings.show = false;
    SetRadarWindowViz();
  } else if (id == m_context_menu_show_id) {
    m_settings.show = true;
    SetRadarWindowViz();
  } else if (id == m_context_menu_acquire_radar_target) {
    if (m_settings.show                                                             // radar shown
        && m_settings.chart_overlay >= 0                                            // overlay desired
        && m_radar[m_settings.chart_overlay]->m_state.GetValue() == RADAR_TRANSMIT  // Radar  transmitting
        && !isnan(m_cursor_lat) && !isnan(m_cursor_lon)) {
      Position target_pos;
      target_pos.lat = m_cursor_lat;
      target_pos.lon = m_cursor_lon;
      m_radar[m_settings.chart_overlay]->m_arpa->AcquireNewMARPATarget(target_pos);
    }
  } else if (id == m_context_menu_delete_radar_target) {
    // Targets can also be deleted when the overlay is not shown
    // In this case targets can be made by a guard zone in a radarwindow
    if (m_settings.show && m_settings.chart_overlay >= 0) {
      Position target_pos;
      target_pos.lat = m_cursor_lat;
      target_pos.lon = m_cursor_lon;
      if (m_radar[m_settings.chart_overlay]->m_arpa) {
        m_radar[m_settings.chart_overlay]->m_arpa->DeleteTarget(target_pos);
      }
    }
  } else if (id == m_context_menu_delete_all_radar_targets) {
    for (int i = 0; i < RADARS; i++) {
      if (m_radar[i]->m_arpa) {
        m_radar[i]->m_arpa->DeleteAllTargets();
      }
    }
  } else {
    wxLogError(wxT("BR24radar_pi: Unknown context menu item callback"));
  }
}

void br24radar_pi::PassHeadingToOpenCPN() {
  wxString nmea;
  char sentence[40];
  char checksum = 0;
  char *p;

  snprintf(sentence, sizeof(sentence), "RAHDT,%.1f,T", m_hdt);

  for (p = sentence; *p; p++) {
    checksum ^= *p;
  }

  nmea.Printf(wxT("$%s*%02X\r\n"), sentence, (unsigned)checksum);
  LOG_TRANSMIT(wxT("BR24radar_pi: Passing heading '%s'"), nmea.c_str());
  PushNMEABuffer(nmea);
}

wxString br24radar_pi::GetTimedIdleText() {
  wxString text;

  if (m_settings.timed_idle > 0) {
    time_t now = time(0);
    int left = m_idle_standby - now;
    if (left >= 0) {
      text = _("Standby in");
      text << wxString::Format(wxT(" %d:%02d"), left / 60, left % 60);
    } else {
      left = m_idle_transmit - now;
      if (left >= 0) {
        text = _("Transmit in");
        text << wxString::Format(wxT(" %d:%02d"), left / 60, left % 60);
      }
    }
  }
  return text;
}

wxString br24radar_pi::GetGuardZoneText(RadarInfo *ri) {
  wxString text = GetTimedIdleText();

  for (int z = 0; z < GUARD_ZONES; z++) {
    int bogeys = ri->m_guard_zone[z]->GetBogeyCount();
    if (bogeys > 0 || (m_guard_bogey_confirmed && bogeys == 0)) {
      if (text.length() > 0) {
        text << wxT("\n");
      }
      text << _("Zone") << wxT(" ") << z + 1 << wxT(": ") << bogeys;
      if (m_guard_bogey_confirmed) {
        text << wxT(" ") << _("(Confirmed)");
      }
    }
  }

  return text;
}

/**
 * Check any guard zones
 *
 */
void br24radar_pi::CheckGuardZoneBogeys(void) {
  bool bogeys_found = false;
  time_t now = time(0);
  wxString text;

  for (size_t r = 0; r < RADARS; r++) {
    if (m_settings.enable_dual_radar) {
      text << m_radar[r]->m_name;
      text << wxT(":\n");
    }
    if (m_radar[r]->m_state.GetValue() == RADAR_TRANSMIT) {
      bool bogeys_found_this_radar = false;

      for (size_t z = 0; z < GUARD_ZONES; z++) {
        int bogeys = m_radar[r]->m_guard_zone[z]->GetBogeyCount();
        if (bogeys > m_settings.guard_zone_threshold) {
          bogeys_found = true;
          bogeys_found_this_radar = true;
          m_settings.timed_idle = 0;  // reset timed idle to off
        }
        text << _(" Zone") << wxT(" ") << z + 1 << wxT(": ");
        if (bogeys > m_settings.guard_zone_threshold) {
          text << bogeys;
        } else if (bogeys >= 0) {
          text << wxT("(");
          text << bogeys;
          text << wxT(")");
        } else {
          text << wxT("-");
        }
        text << wxT("\n");
      }
      LOG_GUARD(wxT("BR24radar_pi: Radar %c: CheckGuardZoneBogeys found=%d confirmed=%d"), r + 'A', bogeys_found_this_radar,
                m_guard_bogey_confirmed);
    }
  }

  if (bogeys_found) {
    if (!m_guard_bogey_confirmed && TIMED_OUT(now, m_alarm_sound_timeout) && m_settings.guard_zone_timeout) {
      // If the last time is 10 seconds ago we ping a sound, unless the user
      // confirmed
      m_alarm_sound_timeout = now + m_settings.guard_zone_timeout;

      if (!m_settings.alert_audio_file.IsEmpty()) {
        PlugInPlaySound(m_settings.alert_audio_file);
      } else {
        wxBell();
      }
    }
    m_guard_bogey_seen = true;
    m_guard_bogey_timeout = 0;
  } else if (m_guard_bogey_seen) {  // First time here after bogey disappeared. Start timer.
    m_guard_bogey_timeout = now + CONFIRM_RESET_TIMEOUT;
    m_guard_bogey_seen = false;
    m_alarm_sound_timeout = 0;
  } else if (TIMED_OUT(now, m_guard_bogey_timeout)) {  // No bogeys and timer elapsed, now reset confirmed
    m_guard_bogey_confirmed = false;                   // Reset for next time we see bogeys
    m_alarm_sound_timeout = 0;
  }

  if (!m_guard_bogey_confirmed && m_alarm_sound_timeout > 0) {
    if (text.length() > 0) {
      text << wxT("\n");
    }
    text << _("Next alarm in");
    text << wxString::Format(wxT(" %d s"), m_alarm_sound_timeout - now);
  }

  if (bogeys_found && !m_bogey_dialog) {
    m_bogey_dialog = new GuardZoneBogey;
    m_bogey_dialog->Create(m_parent_window, this);
  }
  if (m_bogey_dialog) {
    m_bogey_dialog->ShowBogeys(text, bogeys_found, m_guard_bogey_confirmed);
  }
}

void br24radar_pi::RequestStateAllRadars(RadarState state) {
  for (size_t r = 0; r < RADARS; r++) {
    m_radar[r]->RequestRadarState(state);
  }
}

/**
 * See how TimedTransmit is doing.
 *
 * If the ON timer is running and has run out, start the radar and start an OFF timer.
 * If the OFF timer is running and has run out, stop the radar and start an ON timer.
 */
void br24radar_pi::CheckTimedTransmit(RadarState state) {
  if (m_settings.timed_idle == 0) {
    return;  // User does not want timed idle
  }

  if (state == RADAR_OFF) {
    return;  // Timers are just stuck at existing value if radar is off.
  }

  time_t now = time(0);

  if (m_idle_standby > 0 && TIMED_OUT(now, m_idle_standby) && state == RADAR_TRANSMIT) {
    RequestStateAllRadars(RADAR_STANDBY);
    m_idle_transmit = now + m_settings.timed_idle * SECONDS_PER_TIMED_IDLE_SETTING -
                      (m_settings.idle_run_time + 1) * SECONDS_PER_TIMED_RUN_SETTING;
    m_idle_standby = 0;
  } else if (m_idle_transmit > 0 && TIMED_OUT(now, m_idle_transmit) && state == RADAR_STANDBY) {
    RequestStateAllRadars(RADAR_TRANSMIT);
    m_idle_standby = now + (m_settings.idle_run_time + 1) * SECONDS_PER_TIMED_RUN_SETTING;
    m_idle_transmit = 0;
  }
}

void br24radar_pi::SetRadarHeading(double heading, bool isTrue) {
  wxCriticalSectionLocker lock(m_exclusive);
  m_radar_heading = heading;
  m_radar_heading_true = isTrue;
  time_t now = time(0);
  if (!wxIsNaN(m_radar_heading)) {
    if (m_radar_heading_true) {
      if (m_heading_source != HEADING_RADAR_HDT) {
        m_heading_source = HEADING_RADAR_HDT;
      }
      if (m_heading_source == HEADING_RADAR_HDT) {
        m_hdt = m_radar_heading;
        m_hdt_timeout = now + HEADING_TIMEOUT;
      }
    } else {
      if (m_heading_source != HEADING_RADAR_HDM) {
        m_heading_source = HEADING_RADAR_HDM;
      }
      if (m_heading_source == HEADING_RADAR_HDM) {
        m_hdm = m_radar_heading;
        m_hdt = m_radar_heading + m_var;
        m_hdm_timeout = now + HEADING_TIMEOUT;
      }
    }
  } else if (m_heading_source == HEADING_RADAR_HDM || m_heading_source == HEADING_RADAR_HDT) {
    // no heading on radar and heading source is still radar
    m_heading_source = HEADING_NONE;
  }
}

void br24radar_pi::UpdateHeadingPositionState() {
  wxCriticalSectionLocker lock(m_exclusive);
  time_t now = time(0);

  if (m_bpos_set && TIMED_OUT(now, m_bpos_timestamp + WATCHDOG_TIMEOUT)) {
    // If the position data is 10s old reset our position.
    // Note that the watchdog is reset every time we receive a position.
    m_bpos_set = false;
    LOG_VERBOSE(wxT("BR24radar_pi: Lost Boat Position data"));
  }

  switch (m_heading_source) {
    case HEADING_NONE:
      break;
    case HEADING_FIX_COG:
    case HEADING_FIX_HDT:
    case HEADING_NMEA_HDT:
    case HEADING_RADAR_HDT:
      if (TIMED_OUT(now, m_hdt_timeout)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is reset every time we receive a heading.
        m_heading_source = HEADING_NONE;
        LOG_VERBOSE(wxT("BR24radar_pi: Lost Heading data"));
      }
      break;
    case HEADING_FIX_HDM:
    case HEADING_NMEA_HDM:
    case HEADING_RADAR_HDM:
      if (TIMED_OUT(now, m_hdm_timeout)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is continuously reset every time we receive a
        // heading
        m_heading_source = HEADING_NONE;
        LOG_VERBOSE(wxT("BR24radar_pi: Lost Heading data"));
      }
      break;
  }

  if (m_var_source != VARIATION_SOURCE_NONE && TIMED_OUT(now, m_var_timeout)) {
    m_var_source = VARIATION_SOURCE_NONE;
    LOG_VERBOSE(wxT("BR24radar_pi: Lost Variation source"));
  }

  // Update radar position offset from GPS
  if (m_heading_source != HEADING_NONE && !wxIsNaN(m_hdt) &&
      (m_settings.antenna_starboard != 0 || m_settings.antenna_forward != 0)) {
    double sine = sin(deg2rad(m_hdt));
    double cosine = cos(deg2rad(m_hdt));
    double dist_forward = (double)m_settings.antenna_forward / 1852 / 60;
    double dist_starboard = (double)m_settings.antenna_starboard / 1852 / 60;
    m_radar_lat = dist_forward * cosine - dist_starboard * sine + m_ownship_lat;
    m_radar_lon = (dist_forward * sine + dist_starboard * cosine) / cos(deg2rad(m_ownship_lat)) + m_ownship_lon;
  } else {
    m_radar_lat = m_ownship_lat;
    m_radar_lon = m_ownship_lon;
  }
}

/**
 * This is called whenever OpenCPN is drawing the chart, about halfway through its
 * process, e.g. as the last part of RenderGLOverlay(), and by the timer.
 *
 * This happens on the main (GUI) thread.
 */
void br24radar_pi::ScheduleWindowRefresh() {
  int drawTime = 0;
  int millis;

  TimedControlUpdate();  // Update the controls. Method is self-limiting if called too often.

  for (int r = 0; r < RADARS; r++) {
    drawTime += m_radar[r]->GetDrawTime();
    m_radar[r]->RefreshDisplay();
  }

  if (m_settings.refreshrate > 1 && drawTime < 500) {  // 1 = 1 per s, 1000ms between draws, no additional refreshes
    // 2 = 2 per s,  500ms
    // 3 = 4 per s,  250ms
    // 4 = 8 per s,  125ms
    // 5 = 16 per s,  64ms
    millis = (1000 - drawTime) / (1 << (m_settings.refreshrate - 1)) + drawTime;

    m_timer->StartOnce(millis);
    LOG_VERBOSE(wxT("BR24radar_pi: rendering PPI window(s) took %dms, next extra render is in %dms"), drawTime, millis);
  } else {
    LOG_VERBOSE(wxT("BR24radar_pi: rendering PPI window(s) took %dms, refreshrate=%d, no next extra render"), drawTime,
                m_settings.refreshrate);
  }
}

void br24radar_pi::OnTimerNotify(wxTimerEvent &event) {
  if (m_settings.show) {  // Is radar enabled?
    LOG_INFO(wxT("BR24radar_pi: TIMER"));

    if (m_settings.chart_overlay >= 0) {
      // If overlay is enabled schedule another chart draw. Note this will cause another call to RenderGLOverlay,
      // which will then call ScheduleWindowRefresh again itself.
      GetOCPNCanvasWindow()->Refresh(false);
    } else {
      ScheduleWindowRefresh();
    }
  }
}

// Called between 1 and 10 times per second by timer or RenderGLOverlay call
void br24radar_pi::TimedControlUpdate() {
  wxLongLong now = wxGetUTCTimeMillis();
  if (!m_notify_control_dialog && !TIMED_OUT(now, m_notify_time_ms + 200)) {
    return;  // Don't run this more often than 5 times per second
  }
  m_notify_time_ms = now;

  bool updateAllControls = m_notify_control_dialog;
  m_notify_control_dialog = false;
  if (m_opengl_mode_changed || m_notify_radar_window_viz) {
    m_opengl_mode_changed = false;
    m_notify_radar_window_viz = false;
    SetRadarWindowViz(true);
    updateAllControls = true;
  } else {
    UpdateContextMenu();
  }

  UpdateHeadingPositionState();

  // Check the age of "radar_seen", if too old radar_seen = false
  bool any_data_seen = false;
  for (size_t r = 0; r < RADARS; r++) {
    int state = m_radar[r]->m_state.GetValue();  // Safe, protected by lock
    if (state == RADAR_TRANSMIT) {
      any_data_seen = true;
    }
    if (!m_settings.show            // No radar shown
        || state != RADAR_TRANSMIT  // Radar not transmitting
        || !m_bpos_set) {           // No overlay possible (yet)
                                    // Conditions for ARPA not fulfilled, delete all targets
      m_radar[r]->m_arpa->RadarLost();
    }
    m_radar[r]->UpdateTransmitState();
  }

  if (any_data_seen && m_settings.show) {
    CheckGuardZoneBogeys();
  }

  if (m_settings.pass_heading_to_opencpn && m_heading_source >= HEADING_RADAR_HDM) {
    PassHeadingToOpenCPN();
  }

  if (m_pMessageBox->IsShown() || (m_settings.verbose != 0)) {
    wxString t;
    for (size_t r = 0; r < RADARS; r++) {
      if (m_radar[r]->m_state.GetValue() != RADAR_OFF) {
        wxCriticalSectionLocker lock(m_radar[r]->m_exclusive);

        t << wxString::Format(wxT("%s\npackets %d/%d\nspokes %d/%d/%d\n"), m_radar[r]->m_name.c_str(),
                              m_radar[r]->m_statistics.packets, m_radar[r]->m_statistics.broken_packets,
                              m_radar[r]->m_statistics.spokes, m_radar[r]->m_statistics.broken_spokes,
                              m_radar[r]->m_statistics.missing_spokes);
      }
    }
    m_pMessageBox->SetStatisticsInfo(t);
    if (t.length() > 0) {
      t.Replace(wxT("\n"), wxT(" "));
      LOG_RECEIVE(wxT("BR24radar_pi: %s"), t.c_str());
    }
  }

  // Always reset the counters, so they don't show huge numbers after IsShown changes
  for (int r = 0; r < RADARS; r++) {
    wxCriticalSectionLocker lock(m_radar[r]->m_exclusive);

    m_radar[r]->m_statistics.broken_packets = 0;
    m_radar[r]->m_statistics.broken_spokes = 0;
    m_radar[r]->m_statistics.missing_spokes = 0;
    m_radar[r]->m_statistics.packets = 0;
    m_radar[r]->m_statistics.spokes = 0;
  }

  wxString info;
  switch (m_heading_source) {
    case HEADING_NONE:
    case HEADING_FIX_HDM:
    case HEADING_NMEA_HDM:
    case HEADING_RADAR_HDM:
      info = wxT("");
      break;
    case HEADING_FIX_COG:
      info = _("COG");
      break;
    case HEADING_FIX_HDT:
    case HEADING_NMEA_HDT:
      info = _("HDT");
      break;
    case HEADING_RADAR_HDT:
      info = _("RADAR");
      break;
  }
  if (info.Len() > 0 && !wxIsNaN(m_hdt)) {
    info << wxString::Format(wxT(" %3.1f"), m_hdt);
  }
  m_pMessageBox->SetTrueHeadingInfo(info);
  switch (m_heading_source) {
    case HEADING_NONE:
    case HEADING_FIX_COG:
    case HEADING_FIX_HDT:
    case HEADING_NMEA_HDT:
    case HEADING_RADAR_HDT:
      info = wxT("");
      break;
    case HEADING_FIX_HDM:
    case HEADING_NMEA_HDM:
      info = _("HDM");
      break;
    case HEADING_RADAR_HDM:
      info = _("RADAR");
      break;
  }
  if (info.Len() > 0 && !wxIsNaN(m_hdm)) {
    info << wxString::Format(wxT(" %3.1f"), m_hdm);
  }
  m_pMessageBox->SetMagHeadingInfo(info);
  m_pMessageBox->UpdateMessage(false);

  for (int r = 0; r < RADARS; r++) {
    m_radar[r]->UpdateControlState(updateAllControls);
  }

  UpdateState();
}

void br24radar_pi::UpdateState(void) {
  RadarState state = RADAR_OFF;

  for (int r = 0; r < RADARS; r++) {
    state = wxMax(state, (RadarState)m_radar[r]->m_state.GetValue());
  }
  if (state == RADAR_OFF) {
    m_toolbar_button = TB_SEARCHING;
  } else if (m_settings.show == false) {
    m_toolbar_button = TB_HIDDEN;
  } else if (state == RADAR_TRANSMIT) {
    m_toolbar_button = TB_ACTIVE;
  } else if (m_settings.timed_idle) {
    m_toolbar_button = TB_SEEN;
  } else {
    m_toolbar_button = TB_STANDBY;
  }
  CacheSetToolbarToolBitmaps();

  CheckTimedTransmit(state);
}

void br24radar_pi::SetOpenGLMode(OpenGLMode mode) {
  if (m_opengl_mode != mode) {
    m_opengl_mode = mode;
    // Can't hide/show the windows from here, this becomes recursive because the Chart display
    // is managed by wxAuiManager as well.
    m_opengl_mode_changed = true;
  }
}

wxGLContext *br24radar_pi::GetChartOpenGLContext() { return m_opencpn_gl_context; }

//**************************************************************************************************
// Radar Image Graphic Display Processes
//**************************************************************************************************

bool br24radar_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) {
  if (!m_initialized) {
    return true;
  }

  LOG_DIALOG(wxT("BR24radar_pi: RenderOverlay"));

  SetOpenGLMode(OPENGL_OFF);
  return true;
}

// Called by Plugin Manager on main system process cycle

bool br24radar_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp) {
  double radar_lat, radar_lon;

  if (!m_initialized) {
    return true;
  }

  LOG_DIALOG(wxT("BR24radar_pi: RenderGLOverlay context=%p"), pcontext);
  m_opencpn_gl_context = pcontext;
  if (!m_opencpn_gl_context && !m_opencpn_gl_context_broken) {
    LOG_INFO(wxT("BR24radar_pi: OpenCPN does not pass OpenGL context. Resize of OpenCPN window may be broken!"));
  }
  m_opencpn_gl_context_broken = m_opencpn_gl_context == 0;

  SetOpenGLMode(OPENGL_ON);

  if (vp->rotation != m_vp_rotation) {
    wxCriticalSectionLocker lock(m_exclusive);

    m_cog_timeout = time(0) + m_COGAvgSec;
    m_cog = m_COGAvg;
    m_vp_rotation = vp->rotation;
  }

  if (m_settings.show                                                             // Radar shown
      && m_settings.chart_overlay >= 0                                            // Overlay desired
      && m_radar[m_settings.chart_overlay]->m_state.GetValue() == RADAR_TRANSMIT  // Radar transmitting
      && GetRadarPosition(&radar_lat, &radar_lon)) {                              // Boat position known

    // Always compute m_auto_range_meters, possibly needed by SendState() called
    // from DoTick().
    double max_distance = radar_distance(vp->lat_min, vp->lon_min, vp->lat_max, vp->lon_max, 'm');
    // max_distance is the length of the diagonal of the viewport. If the boat
    // were centered, the max length to the edge of the screen is exactly half that.
    double edge_distance = max_distance / 2.0;
    int auto_range_meters = (int)edge_distance;
    if (auto_range_meters < 50) {
      auto_range_meters = 50;
    }

    wxPoint boat_center;
    GetCanvasPixLL(vp, &boat_center, radar_lat, radar_lon);

    m_radar[m_settings.chart_overlay]->SetAutoRangeMeters(auto_range_meters);

    //    Calculate image scale factor
    double llat, llon, ulat, ulon, dist_y, v_scale_ppm;

    GetCanvasLLPix(vp, wxPoint(0, vp->pix_height - 1), &ulat, &ulon);  // is pix_height a mapable coordinate?
    GetCanvasLLPix(vp, wxPoint(0, 0), &llat, &llon);
    dist_y = radar_distance(llat, llon, ulat, ulon, 'm');  // Distance of height of display - meters
    v_scale_ppm = 1.0;
    if (dist_y > 0.) {
      // v_scale_ppm = vertical pixels per meter
      v_scale_ppm = vp->pix_height / dist_y;  // pixel height of screen div by equivalent meters
    }

    double rotation = fmod(rad2deg(vp->rotation + vp->skew * m_settings.skew_factor) + 720.0, 360);

    LOG_DIALOG(wxT("BR24radar_pi: RenderRadarOverlay lat=%g lon=%g v_scale_ppm=%g vp_rotation=%g skew=%g scale=%f rot=%g"),
               vp->clat, vp->clon, vp->view_scale_ppm, vp->rotation, vp->skew, vp->chart_scale, rotation);
    m_radar[m_settings.chart_overlay]->RenderRadarImage(boat_center, v_scale_ppm, rotation, true);
  }

  ScheduleWindowRefresh();

  return true;
}

//****************************************************************************

bool br24radar_pi::LoadConfig(void) {
  wxFileConfig *pConf = m_pconfig;
  int v, x, y;
  wxString s;

  if (pConf) {
    pConf->SetPath(wxT("Settings"));
    pConf->Read(wxT("COGUPAvgSeconds"), &m_COGAvgSec, 15);
    m_COGAvgSec = wxMin(m_COGAvgSec, MAX_COG_AVERAGE_SECONDS);  // Bound the array size
    for (int i = 0; i < m_COGAvgSec; i++) {
      m_COGTable[i] = NAN;
    }

    pConf->SetPath(wxT("/Plugins/BR24Radar"));

    // Valgrind: This needs to be set before we set range, since that uses this
    pConf->Read(wxT("RangeUnits"), &v, 0);
    m_settings.range_units = (RangeUnits)wxMax(wxMin(v, 1), 0);
    m_settings.range_unit_meters = (m_settings.range_units == RANGE_METRIC) ? 1000 : 1852;

    if (pConf->Read(wxT("DisplayMode"), &v, 0)) {  // v1.3
      wxLogMessage(wxT("BR24radar_pi: Upgrading settings from v1.3 or lower"));
      pConf->Read(wxT("VerboseLog"), &m_settings.verbose, 0);
      m_settings.verbose = wxMax(m_settings.verbose, 1);  // Values over 1 are different now
      pConf->Read(wxT("RunTimeOnIdle"), &m_settings.idle_run_time, 2);
      m_settings.idle_run_time = wxMax(m_settings.idle_run_time, 2);

      for (int r = 0; r < RADARS; r++) {
        m_radar[r]->m_orientation.Update(ORIENTATION_HEAD_UP);
        m_radar[r]->m_boot_state.Update(0);
        SetControlValue(r, CT_TARGET_TRAILS, 0, 0);
        m_settings.show_radar[r] = true;
        LOG_DIALOG(wxT("BR24radar_pi: LoadConfig: show_radar[%d]=%d"), r, v);
        wxString s = (r) ? wxT("B") : wxT("");

        for (int i = 0; i < GUARD_ZONES; i++) {
          double bearing;
          pConf->Read(wxString::Format(wxT("Zone%dStBrng%s"), i + 1, s), &bearing, 0.0);
          m_radar[r]->m_guard_zone[i]->m_start_bearing = SCALE_DEGREES_TO_RAW2048(bearing);
          pConf->Read(wxString::Format(wxT("Zone%dEndBrng%s"), i + 1, s), &bearing, 0.0);
          m_radar[r]->m_guard_zone[i]->m_end_bearing = SCALE_DEGREES_TO_RAW2048(bearing);
          pConf->Read(wxString::Format(wxT("Zone%dOuterRng%s"), i + 1, s), &m_radar[r]->m_guard_zone[i]->m_outer_range, 0);
          pConf->Read(wxString::Format(wxT("Zone%dInnerRng%s"), i + 1, s), &m_radar[r]->m_guard_zone[i]->m_inner_range, 0);
          pConf->Read(wxString::Format(wxT("Zone%dArcCirc%s"), i + 1, s), &v, 0);
          m_radar[r]->m_guard_zone[i]->SetType((GuardZoneType)v);
        }
      }
      pConf->Read(wxT("GuardZonePosX"), &x, 20);
      pConf->Read(wxT("GuardZonePosY"), &y, 170);
      m_settings.alarm_pos = wxPoint(x, y);
      pConf->Read(wxT("Enable_COG_heading"), &m_settings.enable_cog_heading, false);
    } else {
      pConf->Read(wxT("VerboseLog"), &m_settings.verbose, 0);
      pConf->Read(wxT("RunTimeOnIdle"), &m_settings.idle_run_time, 1);
      m_settings.idle_run_time = wxMax(m_settings.idle_run_time, 2);

      for (int r = 0; r < RADARS; r++) {
        pConf->Read(wxString::Format(wxT("Radar%dRange"), r), &v, 2000);
        m_radar[r]->m_range.Update(v);
        pConf->Read(wxString::Format(wxT("Radar%dRotation"), r), &v, 0);
        if (v == ORIENTATION_HEAD_UP) {
          v = ORIENTATION_STABILIZED_UP;
        }
        m_radar[r]->m_orientation.Update(v);
        pConf->Read(wxString::Format(wxT("Radar%dTransmit"), r), &v, 0);
        m_radar[r]->m_boot_state.Update(v);
        pConf->Read(wxString::Format(wxT("Radar%dMinContourLength"), r), &m_radar[r]->m_min_contour_length, 6);

        pConf->Read(wxString::Format(wxT("Radar%dTrails"), r), &v, 0);
        SetControlValue(r, CT_TARGET_TRAILS, v, 0);
        pConf->Read(wxString::Format(wxT("Radar%dTrueMotion"), r), &v, 0);
        SetControlValue(r, CT_TRAILS_MOTION, v, 0);
        pConf->Read(wxString::Format(wxT("Radar%dWindowShow"), r), &m_settings.show_radar[r], r ? false : true);
        pConf->Read(wxString::Format(wxT("Radar%dWindowPosX"), r), &x, 30 + 540 * r);
        pConf->Read(wxString::Format(wxT("Radar%dWindowPosY"), r), &y, 120);
        m_settings.window_pos[r] = wxPoint(x, y);
        pConf->Read(wxString::Format(wxT("Radar%dControlShow"), r), &m_settings.show_radar_control[r], false);
        pConf->Read(wxString::Format(wxT("Radar%dTargetShow"), r), &m_settings.show_radar_target[r], true);
        pConf->Read(wxString::Format(wxT("Radar%dControlPosX"), r), &x, OFFSCREEN_CONTROL_X);
        pConf->Read(wxString::Format(wxT("Radar%dControlPosY"), r), &y, OFFSCREEN_CONTROL_Y);
        m_settings.control_pos[r] = wxPoint(x, y);
        LOG_DIALOG(wxT("BR24radar_pi: LoadConfig: show_radar[%d]=%d control=%d,%d"), r, v, x, y);
        for (int i = 0; i < GUARD_ZONES; i++) {
          pConf->Read(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), &m_radar[r]->m_guard_zone[i]->m_start_bearing, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), &m_radar[r]->m_guard_zone[i]->m_end_bearing, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), &m_radar[r]->m_guard_zone[i]->m_outer_range, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), &m_radar[r]->m_guard_zone[i]->m_inner_range, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dType"), r, i), &v, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dAlarmOn"), r, i), &m_radar[r]->m_guard_zone[i]->m_alarm_on, 0);
          pConf->Read(wxString::Format(wxT("Radar%dZone%dArpaOn"), r, i), &m_radar[r]->m_guard_zone[i]->m_arpa_on, 0);
          m_radar[r]->m_guard_zone[i]->SetType((GuardZoneType)v);
        }
      }
      pConf->Read(wxT("AlarmPosX"), &x, 25);
      pConf->Read(wxT("AlarmPosY"), &y, 175);
      m_settings.alarm_pos = wxPoint(x, y);
      pConf->Read(wxT("EnableCOGHeading"), &m_settings.enable_cog_heading, false);
      pConf->Read(wxT("AISatARPAoffset"), &m_settings.AISatARPAoffset, 40);
      if (m_settings.AISatARPAoffset < 10 || m_settings.AISatARPAoffset > 200) m_settings.AISatARPAoffset = 40;
    }

    pConf->Read(wxT("AlertAudioFile"), &m_settings.alert_audio_file, m_shareLocn + wxT("alarm.wav"));
    pConf->Read(wxT("ChartOverlay"), &m_settings.chart_overlay, 0);
    pConf->Read(wxT("ColourStrong"), &s, "red");
    m_settings.strong_colour = wxColour(s);
    pConf->Read(wxT("ColourIntermediate"), &s, "green");
    m_settings.intermediate_colour = wxColour(s);
    pConf->Read(wxT("ColourWeak"), &s, "blue");
    m_settings.weak_colour = wxColour(s);
    pConf->Read(wxT("ColourArpaEdge"), &s, "white");
    m_settings.arpa_colour = wxColour(s);
    pConf->Read(wxT("ColourAISText"), &s, "rgb(100,100,100)");
    m_settings.ais_text_colour = wxColour(s);
    pConf->Read(wxT("ColourPPIBackground"), &s, "rgb(0,0,50)");
    m_settings.ppi_background_colour = wxColour(s);
    pConf->Read(wxT("DeveloperMode"), &m_settings.developer_mode, false);
    pConf->Read(wxT("DrawingMethod"), &m_settings.drawing_method, 0);
    pConf->Read(wxT("EmulatorOn"), &m_settings.emulator_on, false);
    pConf->Read(wxT("EnableDualRadar"), &m_settings.enable_dual_radar, false);
    pConf->Read(wxT("GuardZoneDebugInc"), &m_settings.guard_zone_debug_inc, 0);
    pConf->Read(wxT("GuardZoneOnOverlay"), &m_settings.guard_zone_on_overlay, true);
    pConf->Read(wxT("GuardZoneTimeout"), &m_settings.guard_zone_timeout, 30);
    pConf->Read(wxT("GuardZonesRenderStyle"), &m_settings.guard_zone_render_style, 0);
    pConf->Read(wxT("GuardZonesThreshold"), &m_settings.guard_zone_threshold, 5L);
    pConf->Read(wxT("IgnoreRadarHeading"), &m_settings.ignore_radar_heading, 0);
    pConf->Read(wxT("MainBangSize"), &m_settings.main_bang_size, 0);
    pConf->Read(wxT("ShowExtremeRange"), &m_settings.show_extreme_range, false);
    pConf->Read(wxT("AntennaForward"), &m_settings.antenna_forward, 0);
    pConf->Read(wxT("AntennaStarboard"), &m_settings.antenna_starboard, 0);
    pConf->Read(wxT("MenuAutoHide"), &m_settings.menu_auto_hide, 0);
    pConf->Read(wxT("PassHeadingToOCPN"), &m_settings.pass_heading_to_opencpn, false);
    pConf->Read(wxT("RadarInterface"), &m_settings.mcast_address);
    pConf->Read(wxT("Refreshrate"), &m_settings.refreshrate, 3);
    pConf->Read(wxT("ReverseZoom"), &m_settings.reverse_zoom, false);
    pConf->Read(wxT("ScanMaxAge"), &m_settings.max_age, 6);
    pConf->Read(wxT("Show"), &m_settings.show, true);
    pConf->Read(wxT("SkewFactor"), &m_settings.skew_factor, 1);
    pConf->Read(wxT("ThresholdBlue"), &m_settings.threshold_blue, 50);
    // Make room for BLOB_HISTORY_MAX history values
    m_settings.threshold_blue = MAX(m_settings.threshold_blue, BLOB_HISTORY_MAX + 1);
    pConf->Read(wxT("ThresholdGreen"), &m_settings.threshold_green, 100);
    pConf->Read(wxT("ThresholdRed"), &m_settings.threshold_red, 200);
    pConf->Read(wxT("TrailColourStart"), &s, "rgb(255,255,255)");
    m_settings.trail_start_colour = wxColour(s);
    pConf->Read(wxT("TrailColourEnd"), &s, "rgb(63,63,63)");
    m_settings.trail_end_colour = wxColour(s);
    pConf->Read(wxT("TrailsOnOverlay"), &m_settings.trails_on_overlay, false);
    pConf->Read(wxT("Transparency"), &m_settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY);

    m_settings.max_age = wxMax(wxMin(m_settings.max_age, MAX_AGE), MIN_AGE);
    m_settings.refreshrate = wxMax(wxMin(m_settings.refreshrate, 5), 1);

    SaveConfig();
    return true;
  }
  return false;
}

bool br24radar_pi::SaveConfig(void) {
  wxFileConfig *pConf = m_pconfig;

  if (pConf) {
    pConf->DeleteGroup(wxT("/Plugins/BR24Radar"));
    pConf->SetPath(wxT("/Plugins/BR24Radar"));

    pConf->Write(wxT("AlarmPosX"), m_settings.alarm_pos.x);
    pConf->Write(wxT("AlarmPosY"), m_settings.alarm_pos.y);
    pConf->Write(wxT("AlertAudioFile"), m_settings.alert_audio_file);
    pConf->Write(wxT("ChartOverlay"), m_settings.chart_overlay);
    pConf->Write(wxT("DeveloperMode"), m_settings.developer_mode);
    pConf->Write(wxT("DrawingMethod"), m_settings.drawing_method);
    pConf->Write(wxT("EmulatorOn"), m_settings.emulator_on);
    pConf->Write(wxT("EnableCOGHeading"), m_settings.enable_cog_heading);
    pConf->Write(wxT("EnableDualRadar"), m_settings.enable_dual_radar);
    pConf->Write(wxT("GuardZoneDebugInc"), m_settings.guard_zone_debug_inc);
    pConf->Write(wxT("GuardZoneOnOverlay"), m_settings.guard_zone_on_overlay);
    pConf->Write(wxT("GuardZoneTimeout"), m_settings.guard_zone_timeout);
    pConf->Write(wxT("GuardZonesRenderStyle"), m_settings.guard_zone_render_style);
    pConf->Write(wxT("GuardZonesThreshold"), m_settings.guard_zone_threshold);
    pConf->Write(wxT("IgnoreRadarHeading"), m_settings.ignore_radar_heading);
    pConf->Write(wxT("MainBangSize"), m_settings.main_bang_size);
    pConf->Write(wxT("ShowExtremeRange"), m_settings.show_extreme_range);
    pConf->Write(wxT("AntennaForward"), m_settings.antenna_forward);
    pConf->Write(wxT("AntennaStarboard"), m_settings.antenna_starboard);
    pConf->Write(wxT("MenuAutoHide"), m_settings.menu_auto_hide);
    pConf->Write(wxT("PassHeadingToOCPN"), m_settings.pass_heading_to_opencpn);
    pConf->Write(wxT("RadarInterface"), m_settings.mcast_address);
    pConf->Write(wxT("RangeUnits"), (int)m_settings.range_units);
    pConf->Write(wxT("Refreshrate"), m_settings.refreshrate);
    pConf->Write(wxT("ReverseZoom"), m_settings.reverse_zoom);
    pConf->Write(wxT("RunTimeOnIdle"), m_settings.idle_run_time);
    pConf->Write(wxT("ScanMaxAge"), m_settings.max_age);
    pConf->Write(wxT("Show"), m_settings.show);
    pConf->Write(wxT("SkewFactor"), m_settings.skew_factor);
    pConf->Write(wxT("ThresholdBlue"), m_settings.threshold_blue);
    pConf->Write(wxT("ThresholdGreen"), m_settings.threshold_green);
    pConf->Write(wxT("ThresholdRed"), m_settings.threshold_red);
    pConf->Write(wxT("TrailColourStart"), m_settings.trail_start_colour.GetAsString());
    pConf->Write(wxT("TrailColourEnd"), m_settings.trail_end_colour.GetAsString());
    pConf->Write(wxT("TrailsOnOverlay"), m_settings.trails_on_overlay);
    pConf->Write(wxT("Transparency"), m_settings.overlay_transparency);
    pConf->Write(wxT("VerboseLog"), m_settings.verbose);
    pConf->Write(wxT("AISatARPAoffset"), m_settings.AISatARPAoffset);
    pConf->Write(wxT("ColourStrong"), m_settings.strong_colour.GetAsString());
    pConf->Write(wxT("ColourIntermediate"), m_settings.intermediate_colour.GetAsString());
    pConf->Write(wxT("ColourWeak"), m_settings.weak_colour.GetAsString());
    pConf->Write(wxT("ColourArpaEdge"), m_settings.arpa_colour.GetAsString());
    pConf->Write(wxT("ColourAISText"), m_settings.ais_text_colour.GetAsString());
    pConf->Write(wxT("ColourPPIBackground"), m_settings.ppi_background_colour.GetAsString());

    for (int r = 0; r < RADARS; r++) {
      pConf->Write(wxString::Format(wxT("Radar%dRange"), r), m_radar[r]->m_range.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dRotation"), r), m_radar[r]->m_orientation.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dTransmit"), r), m_radar[r]->m_state.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dWindowShow"), r), m_settings.show_radar[r]);
      pConf->Write(wxString::Format(wxT("Radar%dControlShow"), r), m_settings.show_radar_control[r]);
      pConf->Write(wxString::Format(wxT("Radar%dTargetShow"), r), m_settings.show_radar_target[r]);
      pConf->Write(wxString::Format(wxT("Radar%dTrails"), r), m_radar[r]->m_target_trails.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dTrueMotion"), r), m_radar[r]->m_trails_motion.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dWindowPosX"), r), m_settings.window_pos[r].x);
      pConf->Write(wxString::Format(wxT("Radar%dWindowPosY"), r), m_settings.window_pos[r].y);
      pConf->Write(wxString::Format(wxT("Radar%dControlPosX"), r), m_settings.control_pos[r].x);
      pConf->Write(wxString::Format(wxT("Radar%dControlPosY"), r), m_settings.control_pos[r].y);
      pConf->Write(wxString::Format(wxT("Radar%dMinContourLength"), r), m_radar[r]->m_min_contour_length);

      // LOG_DIALOG(wxT("BR24radar_pi: SaveConfig: show_radar[%d]=%d"), r, m_settings.show_radar[r]);
      for (int i = 0; i < GUARD_ZONES; i++) {
        pConf->Write(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), m_radar[r]->m_guard_zone[i]->m_start_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), m_radar[r]->m_guard_zone[i]->m_end_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), m_radar[r]->m_guard_zone[i]->m_outer_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), m_radar[r]->m_guard_zone[i]->m_inner_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dType"), r, i), (int)m_radar[r]->m_guard_zone[i]->m_type);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dAlarmOn"), r, i), m_radar[r]->m_guard_zone[i]->m_alarm_on);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dArpaOn"), r, i), m_radar[r]->m_guard_zone[i]->m_arpa_on);
      }
    }

    pConf->Flush();
    // LOG_VERBOSE(wxT("BR24radar_pi: Saved settings"));
    return true;
  }

  return false;
}

void br24radar_pi::SetMcastIPAddress(wxString &address) {
  wxCriticalSectionLocker lock(m_exclusive);

  m_settings.mcast_address = address;
  if (m_pMessageBox) {
    m_pMessageBox->SetMcastIPAddress(address);
  }
}

// Positional Data passed from NMEA to plugin
void br24radar_pi::SetPositionFix(PlugIn_Position_Fix &pfix) {}

void br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix) {
  wxCriticalSectionLocker lock(m_exclusive);

  time_t now = time(0);
  wxString info;
  if (m_var_source <= VARIATION_SOURCE_FIX && !wxIsNaN(pfix.Var) && (fabs(pfix.Var) > 0.0 || m_var == 0.0)) {
    if (m_var_source < VARIATION_SOURCE_FIX || fabs(pfix.Var - m_var) > 0.05) {
      LOG_VERBOSE(wxT("BR24radar_pi: Position fix provides new magnetic variation %f"), pfix.Var);
      if (m_pMessageBox->IsShown()) {
        info = _("GPS");
        info << wxT(" ") << wxString::Format(wxT("%2.1f"), m_var);
        m_pMessageBox->SetVariationInfo(info);
      }
    }
    m_var = pfix.Var;
    m_var_source = VARIATION_SOURCE_FIX;
    m_var_timeout = now + WATCHDOG_TIMEOUT;
  }

  LOG_VERBOSE(wxT("BR24radar_pi: SetPositionFixEx var=%f var_wd=%d"), pfix.Var, NOT_TIMED_OUT(now, m_var_timeout));

  if (!wxIsNaN(pfix.Hdt)) {
    if (m_heading_source < HEADING_FIX_HDT) {
      LOG_VERBOSE(wxT("BR24radar_pi: Heading source is now HDT from OpenCPN (%d->%d)"), m_heading_source, HEADING_FIX_HDT);
      m_heading_source = HEADING_FIX_HDT;
    }
    if (m_heading_source == HEADING_FIX_HDT) {
      m_hdt = pfix.Hdt;
      m_hdt_timeout = now + HEADING_TIMEOUT;
    }
  } else if (!wxIsNaN(pfix.Hdm) && NOT_TIMED_OUT(now, m_var_timeout)) {
    if (m_heading_source < HEADING_FIX_HDM) {
      LOG_VERBOSE(wxT("BR24radar_pi: Heading source is now HDM from OpenCPN + VAR (%d->%d)"), m_heading_source, HEADING_FIX_HDM);
      m_heading_source = HEADING_FIX_HDM;
    }
    if (m_heading_source == HEADING_FIX_HDM) {
      m_hdm = pfix.Hdm;
      m_hdt = pfix.Hdm + m_var;
      m_hdm_timeout = now + HEADING_TIMEOUT;
    }
  } else if (!wxIsNaN(pfix.Cog) && m_settings.enable_cog_heading) {
    if (m_heading_source < HEADING_FIX_COG) {
      LOG_VERBOSE(wxT("BR24radar_pi: Heading source is now COG from OpenCPN (%d->%d)"), m_heading_source, HEADING_FIX_COG);
      m_heading_source = HEADING_FIX_COG;
    }
    if (m_heading_source == HEADING_FIX_COG) {
      m_hdt = pfix.Cog;
      m_hdt_timeout = now + HEADING_TIMEOUT;
    }
  }

  if (pfix.FixTime > 0 && NOT_TIMED_OUT(now, pfix.FixTime + WATCHDOG_TIMEOUT)) {
    m_ownship_lat = pfix.Lat;
    m_ownship_lon = pfix.Lon;

    if (!m_bpos_set) {
      LOG_VERBOSE(wxT("BR24radar_pi: GPS position is now known"));
    }
    m_bpos_set = true;
    m_bpos_timestamp = now;
  }

  if (!wxIsNaN(pfix.Cog)) {
    UpdateCOGAvg(pfix.Cog);
  }
  if (TIMED_OUT(now, m_cog_timeout)) {
    m_cog_timeout = now + m_COGAvgSec;
    m_cog = m_COGAvg;
  }
}

void br24radar_pi::UpdateCOGAvg(double cog) {
  // This is a straight copy (except for formatting) of the code in
  // OpenCPN/src/chart1.cpp MyFrame::PostProcessNNEA
  if (m_COGAvgSec > 0) {
    //    Make a hole
    for (int i = m_COGAvgSec - 1; i > 0; i--) {
      m_COGTable[i] = m_COGTable[i - 1];
    }
    m_COGTable[0] = cog;

    double sum = 0., count = 0;
    for (int i = 0; i < m_COGAvgSec; i++) {
      double adder = m_COGTable[i];
      if (wxIsNaN(adder)) {
        continue;
      }
      if (fabs(adder - m_COGAvg) > 180.) {
        if ((adder - m_COGAvg) > 0.) {
          adder -= 360.;
        } else {
          adder += 360.;
        }
      }

      sum += adder;
      count++;
    }
    sum /= count;

    if (sum < 0.) {
      sum += 360.;
    } else if (sum >= 360.) {
      sum -= 360.;
    }
    m_COGAvg = sum;
  } else {
    m_COGAvg = cog;
  }
}

void br24radar_pi::SetPluginMessage(wxString &message_id, wxString &message_body) {
  static const wxString WMM_VARIATION_BOAT = wxString(_T("WMM_VARIATION_BOAT"));
  wxString info;
  if (message_id.Cmp(WMM_VARIATION_BOAT) == 0) {
    wxJSONReader reader;
    wxJSONValue message;
    if (!reader.Parse(message_body, &message)) {
      wxCriticalSectionLocker lock(m_exclusive);
      wxJSONValue defaultValue(360);
      double variation = message.Get(_T("Decl"), defaultValue).AsDouble();

      if (variation != 360.0) {
        if (m_var_source != VARIATION_SOURCE_WMM) {
          LOG_VERBOSE(wxT("BR24radar_pi: WMM plugin provides new magnetic variation %f"), variation);
        }
        m_var = variation;
        m_var_source = VARIATION_SOURCE_WMM;
        m_var_timeout = time(0) + WATCHDOG_TIMEOUT;
        if (m_pMessageBox->IsShown()) {
          info = _("WMM");
          info << wxT(" ") << wxString::Format(wxT("%2.1f"), m_var);
          m_pMessageBox->SetVariationInfo(info);
        }
      }
    }
  } else if (message_id == wxS("AIS") || m_ais_in_arpa_zone.size() > 0) {
    // Check if any Radar and ARPA zone is active
    double ArpaMaxRange = 0.0;
    bool ArpaGuardOn = false;
    for (size_t r = 0; r < RADARS; r++) {
      if (m_radar[r]->m_state.GetValue() != RADAR_OFF) {  // One radar is on. Check for guardzones
        for (int i = 0; i < RADARS; i++) {
          for (int z = 0; z < GUARD_ZONES; z++) {
            if (m_radar[i]->m_guard_zone[z]->m_arpa_on) {
              ArpaGuardOn = true;
              int t = m_radar[i]->m_guard_zone[z]->m_outer_range;
              if (t > ArpaMaxRange) ArpaMaxRange = t;
            }
          }
        }
        break;
      }
    }
    if (ArpaGuardOn) {
      wxJSONReader reader;
      wxJSONValue message;
      if (!reader.Parse(message_body, &message)) {
        wxJSONValue defaultValue(999);
        long json_ais_mmsi = message.Get(_T("mmsi"), defaultValue).AsLong();
        if (json_ais_mmsi > 200000000) {  // Neither ARPA targets nor SAR_aircraft
          wxJSONValue defaultValue("90.0");
          double f_AISLat = wxAtof(message.Get(_T("lat"), defaultValue).AsString());
          double f_AISLon = wxAtof(message.Get(_T("lon"), defaultValue).AsString());
          // Rectangle around own ship to look for AIS targets.
          double d_side = ArpaMaxRange / 1852.0 / 60.0;
          if (f_AISLat < (m_radar_lat + d_side) && f_AISLat > (m_radar_lat - d_side) && f_AISLon < (m_radar_lon + d_side * 2) &&
              f_AISLon > (m_radar_lon - d_side * 2)) {
            bool updated = false;
            for (size_t i = 0; i < m_ais_in_arpa_zone.size(); i++) {  // Check for existing mmsi
              if (m_ais_in_arpa_zone[i].ais_mmsi == json_ais_mmsi) {
                m_ais_in_arpa_zone[i].ais_time_upd = time(0);
                m_ais_in_arpa_zone[i].ais_lat = f_AISLat;
                m_ais_in_arpa_zone[i].ais_lon = f_AISLon;
                updated = true;
                break;
              }
            }
            if (!updated) {  // Add a new target
              AisArpa m_new_ais_target;
              m_new_ais_target.ais_mmsi = json_ais_mmsi;
              m_new_ais_target.ais_time_upd = time(0);
              m_new_ais_target.ais_lat = f_AISLat;
              m_new_ais_target.ais_lon = f_AISLon;
              m_ais_in_arpa_zone.push_back(m_new_ais_target);
            }
          }
        }
      }
    }
    // Delete > 3 min old AIS items or at once if neither active ARPA zone nor Radar
    if (m_ais_in_arpa_zone.size() > 0) {
      for (size_t i = 0; i < m_ais_in_arpa_zone.size(); i++) {
        if (m_ais_in_arpa_zone[i].ais_mmsi > 0 && ((time(0) - m_ais_in_arpa_zone[i].ais_time_upd) > (3 * 60) || !ArpaGuardOn)) {
          m_ais_in_arpa_zone.erase(m_ais_in_arpa_zone.begin() + i);
        }
      }
    }
  }
}

bool br24radar_pi::FindAIS_at_arpaPos(const double &lat, const double &lon, const double &dist) {
  if (m_ais_in_arpa_zone.size() < 1) return false;
  bool hit = false;
  double offset = dist / 1852. / 60.;
  for (size_t i = 0; i < m_ais_in_arpa_zone.size(); i++) {
    if (m_ais_in_arpa_zone[i].ais_mmsi != 0) {  // Avtive post
      if (lat + offset > m_ais_in_arpa_zone[i].ais_lat && lat - offset < m_ais_in_arpa_zone[i].ais_lat &&
          lon + (offset * 1.75) > m_ais_in_arpa_zone[i].ais_lon && lon - (offset * 1.75) < m_ais_in_arpa_zone[i].ais_lon) {
        hit = true;
        break;
      }
    }
  }
  return hit;
}

bool br24radar_pi::SetControlValue(int radar, ControlType controlType, int value,
                                   int autoValue) {  // sends the command to the radar
  LOG_TRANSMIT(wxT("BR24radar_pi: %s set %s = %d"), m_radar[radar]->m_name.c_str(), ControlTypeNames[controlType].c_str(), value);
  switch (controlType) {
    case CT_TRANSPARENCY: {
      m_settings.overlay_transparency = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }
    case CT_SCAN_AGE: {
      m_settings.max_age = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }
    case CT_TIMED_IDLE: {
      m_settings.timed_idle = value;
      m_idle_standby = 0;
      m_idle_transmit = 0;
      if (m_radar[0]->m_state.GetValue() == RADAR_TRANSMIT || m_radar[1]->m_state.GetValue() == RADAR_TRANSMIT) {
        m_idle_standby = time(0) + 10;
      } else {
        m_idle_transmit = time(0) + 10;
      }
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }
    case CT_TIMED_RUN: {
      m_settings.idle_run_time = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }
    case CT_REFRESHRATE: {
      m_settings.refreshrate = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }
    case CT_TARGET_TRAILS: {
      m_radar[radar]->m_target_trails.Update(value);
      m_radar[radar]->ComputeColourMap();
      m_radar[radar]->ComputeTargetTrails();
      return true;
    }
    case CT_TRAILS_MOTION: {
      m_radar[radar]->m_trails_motion.Update(value);
      m_radar[radar]->ComputeColourMap();
      m_radar[radar]->ComputeTargetTrails();
      return true;
    }
    case CT_MAIN_BANG_SIZE: {
      m_settings.main_bang_size = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }

    case CT_ANTENNA_FORWARD: {
      m_settings.antenna_forward = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }

    case CT_ANTENNA_STARBOARD: {
      m_settings.antenna_starboard = value;
      m_radar[1 - radar]->UpdateControlState(true);  // Update the controls in the other radar
      return true;
    }

    default: {
      if (m_radar[radar]->SetControlValue(controlType, value, autoValue)) {
        return true;
      }
    }
  }
  wxLogError(wxT("BR24radar_pi: %s unhandled control setting for control %s"), m_radar[radar]->m_name.c_str(),
             ControlTypeNames[controlType].c_str());
  return false;
}

//*****************************************************************************************************
void br24radar_pi::CacheSetToolbarToolBitmaps() {
  if (m_toolbar_button == m_sent_toolbar_button) {
    return;  // no change needed
  }

  wxString icon;

  switch (m_toolbar_button) {
    case TB_NONE:
    case TB_HIDDEN:
      icon = m_shareLocn + wxT("radar_hidden.svg");
      break;

    case TB_SEEN:
      icon = m_shareLocn + wxT("radar_seen.svg");
      break;

    case TB_STANDBY:
      icon = m_shareLocn + wxT("radar_standby.svg");
      break;

    case TB_SEARCHING:
      icon = m_shareLocn + wxT("radar_searching.svg");
      break;

    case TB_ACTIVE:
      icon = m_shareLocn + wxT("radar_active.svg");
      break;
  }
  SetToolbarToolBitmapsSVG(m_tool_id, icon, icon, icon);
  m_sent_toolbar_button = m_toolbar_button;
}

/*
   SetNMEASentence is used to speed up rotation and variation
   detection if SetPositionEx() is not called very often. This will
   be the case if you have a high speed heading sensor (for instance, 2 to 20
   Hz)
   but only a 1 Hz GPS update.
*/

void br24radar_pi::SetNMEASentence(wxString &sentence) {
  m_NMEA0183 << sentence;
  time_t now = time(0);
  double hdm = nan("");
  double hdt = nan("");
  double var;

  LOG_RECEIVE(wxT("BR24radar_pi: SetNMEASentence %s"), sentence.c_str());

  if (m_NMEA0183.PreParse()) {
    if (m_NMEA0183.LastSentenceIDReceived == _T("HDG") && m_NMEA0183.Parse()) {
      if (!wxIsNaN(m_NMEA0183.Hdg.MagneticVariationDegrees)) {
        if (m_NMEA0183.Hdg.MagneticVariationDirection == East) {
          var = +m_NMEA0183.Hdg.MagneticVariationDegrees;
        } else {
          var = -m_NMEA0183.Hdg.MagneticVariationDegrees;
        }
        if (fabs(var - m_var) >= 0.05 && m_var_source <= VARIATION_SOURCE_NMEA) {
          //        LOG_INFO(wxT("BR24radar_pi: NMEA provides new magnetic variation %f from %s"), var, sentence.c_str());
          m_var = var;
          m_var_source = VARIATION_SOURCE_NMEA;
          m_var_timeout = now + WATCHDOG_TIMEOUT;
          wxString info = _("NMEA");
          info << wxT(" ") << wxString::Format(wxT("%2.1f"), m_var);
          m_pMessageBox->SetVariationInfo(info);
        }
      }

      if (!wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees)) {
        hdm = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees;
      }
    } else if (m_NMEA0183.LastSentenceIDReceived == _T("HDM") && m_NMEA0183.Parse() && !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic)) {
      hdm = m_NMEA0183.Hdm.DegreesMagnetic;
    } else if (m_NMEA0183.LastSentenceIDReceived == _T("HDT") && m_NMEA0183.Parse() && !wxIsNaN(m_NMEA0183.Hdt.DegreesTrue)) {
      hdt = m_NMEA0183.Hdt.DegreesTrue;
    }
  }

  if (!wxIsNaN(hdt)) {
    if (m_heading_source < HEADING_NMEA_HDT) {
      //   LOG_INFO(wxT("BR24radar_pi: Heading source is now HDT %d from NMEA %s (%d->%d)"), m_hdt, sentence.c_str(),
      //   m_heading_source,
      //           HEADING_NMEA_HDT);    Crashes!!!
      m_heading_source = HEADING_NMEA_HDT;
    }
    if (m_heading_source == HEADING_NMEA_HDT) {
      m_hdt = hdt;
      m_hdt_timeout = now + HEADING_TIMEOUT;
    }
  } else if (!wxIsNaN(hdm) && NOT_TIMED_OUT(now, m_var_timeout)) {
    if (m_heading_source < HEADING_NMEA_HDM) {
      //   LOG_INFO(wxT("BR24radar_pi: Heading source is now HDM %f + VAR %f from NMEA %s (%d->%d)"), hdm, m_var, sentence.c_str(),
      //            m_heading_source, HEADING_NMEA_HDT);
      m_heading_source = HEADING_NMEA_HDM;
    }
    if (m_heading_source == HEADING_NMEA_HDM) {
      m_hdm = hdm;
      m_hdt = hdm + m_var;
      m_hdm_timeout = now + HEADING_TIMEOUT;
    }
  }
}

void br24radar_pi::SetCursorLatLon(double lat, double lon) {
  m_cursor_lat = lat;
  m_cursor_lon = lon;
}

bool br24radar_pi::MouseEventHook(wxMouseEvent &event) {
  if (event.LeftDown()) {
    for (int r = 0; r < RADARS; r++) {
      m_radar[r]->SetMouseLatLon(m_cursor_lat, m_cursor_lon);
    }
  }
  return false;
}

PLUGIN_END_NAMESPACE
