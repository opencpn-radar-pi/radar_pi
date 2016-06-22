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

#define M_SETTINGS m_settings

#include "br24radar_pi.h"
#include "icons.h"
#include "nmea0183/nmea0183.h"

PLUGIN_BEGIN_NAMESPACE

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin *create_pi(void *ppimgr) { return new br24radar_pi(ppimgr); }

extern "C" DECL_EXP void destroy_pi(opencpn_plugin *p) { delete p; }

/********************************************************************************************************/
//   Distance measurement for simple sphere
/********************************************************************************************************/

double local_distance(double lat1, double lon1, double lat2, double lon2) {
  // Spherical Law of Cosines
  double theta, dist;

  theta = lon2 - lon1;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);  // radians
  dist = rad2deg(dist);
  dist = fabs(dist) * 60;  // nautical miles/degree
  return (dist);
}

double local_bearing(double lat1, double lon1, double lat2, double lon2) {
  double s1 = deg2rad(lat1);
  double l1 = deg2rad(lon1);
  double s2 = deg2rad(lat2);
  double l2 = deg2rad(lon2);

  double y = sin(l2 - l1) * cos(s2);
  double x = cos(s1) * sin(s2) - sin(s1) * cos(s2) * cos(l2 - l1);

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

//#include "default_pi.xpm"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

br24radar_pi::br24radar_pi(void *ppimgr) : opencpn_plugin_112(ppimgr) {
  m_initialized = false;
  // Create the PlugIn icons
  initialize_images();
  m_pdeficon = new wxBitmap(*_img_radar_blank);

  m_opengl_mode = false;
  m_opengl_mode_changed = false;
  m_opencpn_gl_context = 0;
  m_opencpn_gl_context_broken = false;

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

  // Font can change so initialize every time
  m_font = *OCPNGetFont(_("Dialog"), 12);
  m_fat_font = m_font;
  m_fat_font.SetWeight(wxFONTWEIGHT_BOLD);
  m_fat_font.SetPointSize(m_font.GetPointSize() + 1);

  m_var = 0.0;
  m_var_source = VARIATION_SOURCE_NONE;
  m_bpos_set = false;
  m_auto_range_meters = 0;
  m_previous_auto_range_meters = 0;
  m_guard_bogey_confirmed = false;

  m_alarm_sound_timeout = 0;
  m_bpos_timestamp = 0;
  m_hdt = 0.0;
  m_hdt_timeout = 0;
  m_var_timeout = 0;
  m_idle_timeout = 0;

  m_radar[0] = new RadarInfo(this, _("Radar"), 0);
  m_radar[1] = new RadarInfo(this, _("Radar B"), 1);

  m_ptemp_icon = NULL;
  m_sent_bm_id_normal = -1;
  m_sent_bm_id_rollover = -1;

  m_heading_source = HEADING_NONE;

  m_settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;
  m_settings.refreshrate = 1;
  m_settings.timed_idle = 0;

  m_settings.mcast_address = wxT("");

  ::wxDisplaySize(&m_display_width, &m_display_height);

  //    And load the configuration items
  if (LoadConfig()) {
    LOG_INFO(wxT("BR24radar_pi: Configuration file values initialised"));
    LOG_INFO(wxT("BR24radar_pi: Log verbosity = %d (to modify, set VerboseLog to sum of:"), m_settings.verbose);
    LOG_INFO(wxT("BR24radar_pi: VERBOSE  = %d"), LOGLEVEL_VERBOSE);
    LOG_INFO(wxT("BR24radar_pi: DIALOG   = %d"), LOGLEVEL_DIALOG);
    LOG_INFO(wxT("BR24radar_pi: TRANSMIT = %d"), LOGLEVEL_TRANSMIT);
    LOG_INFO(wxT("BR24radar_pi: RECEIVE  = %d"), LOGLEVEL_RECEIVE);
    LOG_INFO(wxT("BR24radar_pi: GUARD    = %d"), LOGLEVEL_GUARD);
    LOG_VERBOSE(wxT("BR24radar_pi: VERBOSE  log is enabled"));
    LOG_DIALOG(wxT("BR24radar_pi: DIALOG   log is enabled"));
    LOG_TRANSMIT(wxT("BR24radar_pi: TRANSMIT log is enabled"));
    LOG_RECEIVE(wxT("BR24radar_pi: RECEIVE  log is enabled"));
    LOG_GUARD(wxT("BR24radar_pi: GUARD    log is enabled"));
  } else {
    wxLogError(wxT("BR24radar_pi: configuration file values initialisation failed"));
    return 0;  // give up
  }
  ComputeColorMap();  // After load config

  for (size_t r = 0; r < RADARS; r++) {
    if (!m_radar[r]->Init(m_settings.verbose)) {
      wxLogError(wxT("BR24radar_pi: initialisation failed"));
      return 0;
    }
  }

  // Get a pointer to the opencpn display canvas, to use as a parent for the UI
  // dialog
  m_parent_window = GetOCPNCanvasWindow();

  //    This PlugIn needs a toolbar icon

  wxString svg_normal = wxT("/tmp/radar_active.svg");
  wxString svg_rollover = wxT("/tmp/radar_standby.svg");
  wxString svg_toggled = wxT("/tmp/radar_toggled.svg");
  m_tool_id = InsertPlugInToolSVG(wxT("Navico"), svg_normal, svg_rollover, svg_toggled, wxITEM_NORMAL, wxT("BR24Radar"),
                                  _("Navico BR24, 3G and 4G RADAR"), NULL,
                               BR24RADAR_TOOL_POSITION, 0, this);

  // CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);

  m_pMessageBox = new br24MessageBox;
  m_pMessageBox->Create(m_parent_window, this);

  m_radar[0]->StartReceive();

  if (m_settings.enable_dual_radar) {
    m_radar[0]->SetName(_("Radar A"));
    m_radar[1]->StartReceive();
  }

  m_initialized = true;

  m_context_menu = new wxMenu();
  wxMenuItem *mi1 = new wxMenuItem(m_context_menu, -1, _("Show radar"));
  m_context_menu_show_id = AddCanvasContextMenuItem(mi1, this);
  wxMenuItem *mi2 = new wxMenuItem(m_context_menu, -1, _("Hide radar"));
  m_context_menu_hide_id = AddCanvasContextMenuItem(mi2, this);
  wxMenuItem *mi3 = new wxMenuItem(m_context_menu, -1, _("Radar Control..."));
  m_context_menu_control_id = AddCanvasContextMenuItem(mi3, this);

  LOG_VERBOSE(wxT("BR24radar_pi: Initialized plugin transmit=%d/%d overlay=%d"), m_settings.chart_overlay);

  Start(1000, wxTIMER_CONTINUOUS);  // inherited from wxTimer

  SetRadarWindowViz(m_settings.show != 0);

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

  Stop();  // inherited from wxTimer

  // Save our config, first, as it contains state regarding what is open.
  SaveConfig();

  m_initialized = false;
  LOG_VERBOSE(wxT("BR24radar_pi: DeInit of plugin"));

  // First close everything that the user can have open
  m_pMessageBox->Close();
  for (int r = 0; r < RADARS; r++) {
    OnControlDialogClose(m_radar[r]);
    m_radar[r]->ShowRadarWindow(false);
  }

  // Delete all 'new'ed objects
  for (int r = 0; r < RADARS; r++) {
    if (m_radar[r]) {
      delete m_radar[r];
      m_radar[r] = 0;
    }
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

  br24OptionsDialog dlg(parent, m_settings);
  if (dlg.ShowModal() == wxID_OK)
  {
    m_settings = dlg.GetSettings();
    ComputeColorMap();
    SaveConfig();
    if (m_settings.enable_dual_radar) {
      m_radar[0]->SetName(_("Radar A"));
      m_radar[1]->StartReceive();
    }
    else {
      m_radar[1]->ShowRadarWindow(false);
      ShowRadarControl(1, false);
    }
  }
}

/**
 * Set the radar window visibility.
 *
 * @param show        desired visibility state
 */
void br24radar_pi::SetRadarWindowViz(bool show) {
  int r;
  for (r = 0; r <= (int)m_settings.enable_dual_radar; r++) {
    bool showThisRadar = show && (m_settings.show_radar[r] != 0);
    m_radar[r]->ShowRadarWindow(showThisRadar);
    if (!showThisRadar && m_settings.chart_overlay) {
      ShowRadarControl(r, false);
    }
    if (show && m_radar[r]->wantedState == RADAR_TRANSMIT) {
      m_radar[r]->transmit->RadarTxOn();
    }
  }

  SetCanvasContextMenuItemViz(m_context_menu_show_id, !show);
  SetCanvasContextMenuItemViz(m_context_menu_hide_id, show);
  SetCanvasContextMenuItemGrey(m_context_menu_control_id, !show);
  LOG_DIALOG(wxT("BR24radar_pi: RadarWindow visibility = %d"), (int)show);
}

//********************************************************************************
// Operation Dialogs - Control, Manual, and Options

void br24radar_pi::ShowRadarControl(int radar, bool show) {
  LOG_DIALOG(wxT("BR24radar_pi: ShowRadarControl(%d, %d)"), radar, (int)show);

  if (show) {
    if (!m_radar[radar]->control_dialog) {
      m_radar[radar]->control_dialog = new br24ControlsDialog;
      m_radar[radar]->control_dialog->Create(m_parent_window, this, m_radar[radar], wxID_ANY, m_radar[radar]->name);
      m_radar[radar]->control_dialog->Fit();
      m_radar[radar]->control_dialog->Hide();
    }
    m_radar[radar]->control_dialog->ShowDialog();
  } else {
    if (m_radar[radar]->control_dialog) {
      m_radar[radar]->control_dialog->HideDialog();
    }
  }

  m_radar[radar]->UpdateControlState(true);
}

void br24radar_pi::OnControlDialogClose(RadarInfo *ri) {
  if (ri->control_dialog) {
    ri->control_dialog->HideDialog();
  }
}

void br24radar_pi::ConfirmGuardZoneBogeys() {
  m_guard_bogey_confirmed = true;  // This will stop the sound being repeated
}

void br24radar_pi::ComputeColorMap() {
  switch (m_settings.display_option) {
    case 0:
      for (int i = 0; i <= UINT8_MAX; i++) {
        m_color_map[i] = (i >= m_settings.threshold_blue) ? BLOB_RED : BLOB_NONE;
      }
      break;
    case 1:
      for (int i = 0; i <= UINT8_MAX; i++) {
        m_color_map[i] =
            (i >= m_settings.threshold_red) ? BLOB_RED : (i >= m_settings.threshold_green)
                                                             ? BLOB_GREEN
                                                             : (i >= m_settings.threshold_blue) ? BLOB_BLUE : BLOB_NONE;
      }
      break;
  }

  memset(m_color_map_red, 0, sizeof(m_color_map_red));
  memset(m_color_map_green, 0, sizeof(m_color_map_green));
  memset(m_color_map_blue, 0, sizeof(m_color_map_blue));
  m_color_map_red[BLOB_RED] = 255;
  m_color_map_green[BLOB_GREEN] = 255;
  m_color_map_blue[BLOB_BLUE] = 255;

  if (m_settings.display_option == 1) {

  for (BlobColor history = BLOB_HISTORY_0; history <= BLOB_HISTORY_9; history = (BlobColor)(history + 1)) {
    m_color_map[history] = history;
    m_color_map_red[history] = 255;
    m_color_map_green[history] = 255;
    m_color_map_blue[history] = 255;
  }
}
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
    SetRadarWindowViz(false);
    m_settings.show = 0;
    return;
  }

  if (m_settings.show) {
    if (m_settings.chart_overlay >= 0 &&
        (!m_radar[m_settings.chart_overlay]->control_dialog || !m_radar[m_settings.chart_overlay]->control_dialog->IsShown())) {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Show control"));
      ShowRadarControl(m_settings.chart_overlay, true);
    } else {
      LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Hide radar windows"));
      m_settings.show = 0;
      SetRadarWindowViz(false);
    }
  } else {
    LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: Show radar windows"));
    m_settings.show = 1;
    SetRadarWindowViz(true);
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
      for (int r = 0; r < RADARS; r++) {
        m_settings.show_radar[r] = 1;
        LOG_DIALOG(wxT("BR24radar_pi: OnToolbarToolCallback: nothing visible, show_radar[%d]=%d"), r, 1);
      }
      SetRadarWindowViz(true);
    }
  } else if (id == m_context_menu_hide_id) {
    m_settings.show = 0;
    SetRadarWindowViz(false);
  } else if (id == m_context_menu_show_id) {
    m_settings.show = 1;
    SetRadarWindowViz(true);
  } else {
    wxLogError(wxT("BR24radar_pi: Unknown context menu item callback"));
  }
}

void br24radar_pi::PassHeadingToOpenCPN() {
  wxString nmea;
  char sentence[40];
  char checksum = 0;
  char *p;

  snprintf(sentence, sizeof(sentence), "APHDT,%.1f,M", m_hdt);

  for (p = sentence; *p; p++) {
    checksum ^= *p;
  }

  nmea.Printf(wxT("$%s*%02X\r\n"), sentence, (unsigned)checksum);
  LOG_TRANSMIT(wxT("BR24radar_pi: Passing heading '%s'"), nmea.c_str());
  PushNMEABuffer(nmea);
}

wxString br24radar_pi::GetGuardZoneText(RadarInfo *ri, bool withTimeout) {
  wxString text;

  if (m_settings.timed_idle) {
    time_t now = time(0);
    int left = m_idle_timeout - now;
    if (left > 0) {
      if (m_radar[0]->state.value == RADAR_TRANSMIT || m_radar[1]->state.value == RADAR_TRANSMIT) {
        text << wxString::Format(_("Standby in %02dm %02ds"), left / 60, left % 60);
      } else {
        text << wxString::Format(_("Transmit in %02dm %02ds"), left / 60, left % 60);
        return text;
      }
    }
  }

  for (int z = 0; z < GUARD_ZONES; z++) {
    int bogeys = ri->guard_zone[z]->m_bogey_count;
    if (bogeys >= 0) {
      if (text.length() > 0) {
        text << wxT("\n");
      }
      text << _("Zone") << wxT(" ") << z + 1 << wxT(": ") << ri->guard_zone[z]->m_bogey_count;
    }
  }
  if (withTimeout) {
    time_t now = time(0);

    if (m_alarm_sound_timeout > 0) {
      if (text.length() > 0) {
        text << wxT("\n");
      }
      text << wxString::Format(_("Next alarm in %d s"), m_alarm_sound_timeout + ALARM_TIMEOUT - now);
    }
  }

  return text;
}

/**
 * Check any guard zones
 *
 */
void br24radar_pi::CheckGuardZoneBogeys(void) {
  SpokeBearing current_hdt = SCALE_DEGREES_TO_RAW2048(m_hdt);
  bool bogeys_found = false;
  time_t now = time(0);

  for (size_t r = 0; r < RADARS; r++) {
    if (m_radar[r]->state.value == RADAR_TRANSMIT) {
      bool bogeys_found_this_radar = false;

      for (size_t z = 0; z < GUARD_ZONES; z++) {
        int bogeys = m_radar[r]->guard_zone[z]->GetBogeyCount(current_hdt);
        if (bogeys > m_settings.guard_zone_threshold) {
          bogeys_found = true;
          bogeys_found_this_radar = true;
        }
      }
      if (bogeys_found_this_radar && !m_guard_bogey_confirmed) {
        ShowRadarControl(r);
        m_radar[r]->control_dialog->ShowBogeys(GetGuardZoneText(m_radar[r], true));
      }
    }
  }

  if (bogeys_found) {
    if (!m_guard_bogey_confirmed && TIMED_OUT(now, m_alarm_sound_timeout)) {
      // If the last time is 10 seconds ago we ping a sound, unless the user
      // confirmed
      m_alarm_sound_timeout = now + ALARM_TIMEOUT;

      if (!m_settings.alert_audio_file.IsEmpty()) {
        PlugInPlaySound(m_settings.alert_audio_file);
      } else {
        wxBell();
      }
    }
  } else {
    m_guard_bogey_confirmed = false;  // Reset for next time we see bogeys
  }
}

void br24radar_pi::SetDesiredStateAllRadars(RadarState desiredState) {
  for (size_t r = 0; r < RADARS; r++) {
    RadarState state = (RadarState)m_radar[r]->state.value;
    if (state != RADAR_OFF) {
      if (state != desiredState) {
        m_radar[r]->FlipRadarState();
      }
    }
  }
}

/**
 * See how TimedTransmit is doing.
 *
 * If the ON timer is running and has run out, start the radar and start an OFF timer.
 * If the OFF timer is running and has run out, stop the radar and start an ON timer.
 */
void br24radar_pi::CheckTimedTransmit(RadarState state) {
  static const int SECONDS_PER_TIMED_IDLE_SETTING = 5 * 60;  // 5 minutes increment for each setting
  static const int SECONDS_PER_TRANSMIT_BURST = 30;

  if (m_settings.timed_idle == 0) {
    return;  // User does not want timed idle
  }

  if (state == RADAR_OFF) {
    return;  // Timers are just stuck at existing value if radar is off.
  }

  time_t now = time(0);

  if (state == RADAR_TRANSMIT) {
    if (TIMED_OUT(now, m_idle_timeout)) {
      SetDesiredStateAllRadars(RADAR_STANDBY);
      m_idle_timeout = now + m_settings.timed_idle * SECONDS_PER_TIMED_IDLE_SETTING;
    }
  } else {
    if (TIMED_OUT(now, m_idle_timeout)) {
      SetDesiredStateAllRadars(RADAR_TRANSMIT);
      int burst = wxMax(m_settings.idle_run_time, SECONDS_PER_TRANSMIT_BURST);
      m_idle_timeout = now + burst;
    }
  }
}

// Notify
// ------
// Called once a second by the timer.
//
// This checks if we need to ping the radar to keep it alive (or make it alive)

void br24radar_pi::Notify(void) {
  time_t now = time(0);

  if (m_radar[0]->radar_type == RT_BR24) {
    m_settings.enable_dual_radar = 0;
  }

  if (m_opengl_mode_changed) {
    m_opengl_mode_changed = false;
    SetRadarWindowViz(m_settings.show != 0);
  }

  // Move this
  if (m_heading_source == HEADING_RADAR) {
    if (m_pMessageBox->IsShown()) {
      wxString info = _("Radar");
      info << wxT(" ") << m_hdt;
      m_pMessageBox->SetHeadingInfo(info);
    }
  }

  if (m_bpos_set && TIMED_OUT(now, m_bpos_timestamp + WATCHDOG_TIMEOUT)) {
    // If the position data is 10s old reset our heading.
    // Note that the watchdog is continuously reset every time we receive a
    // heading.
    m_bpos_set = false;
    LOG_VERBOSE(wxT("BR24radar_pi: Lost Boat Position data"));
  }

  if (m_heading_source != HEADING_NONE && TIMED_OUT(now, m_hdt_timeout)) {
    // If the position data is 10s old reset our heading.
    // Note that the watchdog is continuously reset every time we receive a
    // heading
    m_heading_source = HEADING_NONE;
    LOG_VERBOSE(wxT("BR24radar_pi: Lost Heading data"));
  }

  if (m_var_source != VARIATION_SOURCE_NONE && TIMED_OUT(now, m_var_timeout)) {
    m_var_source = VARIATION_SOURCE_NONE;
    LOG_VERBOSE(wxT("BR24radar_pi: Lost Variation source"));
  }

  // Check the age of "radar_seen", if too old radar_seen = false
  bool any_data_seen = false;
  for (size_t r = 0; r < RADARS; r++) {
    if (m_radar[r]->state.value == RADAR_STANDBY && TIMED_OUT(now, m_radar[r]->m_radar_timeout)) {
      static wxString empty;

      m_radar[r]->state.Update(RADAR_OFF);
      m_pMessageBox->SetRadarIPAddress(empty);
      LOG_INFO(wxT("BR24radar_pi: Lost %s presence"), m_radar[r]->name);
    }
    if (m_radar[r]->state.value == RADAR_TRANSMIT && TIMED_OUT(now, m_radar[r]->m_data_timeout)) {
      m_radar[r]->state.Update(RADAR_STANDBY);
      LOG_INFO(wxT("BR24radar_pi: Data Lost %s "), m_radar[r]->name);
    }

    if (m_radar[r]->state.value == RADAR_TRANSMIT) {
      if (IsRadarOnScreen(r) && TIMED_OUT(now, m_radar[r]->m_stayalive_timeout)) {
        m_radar[r]->m_stayalive_timeout = now + STAYALIVE_TIMEOUT;
        m_radar[r]->transmit->RadarStayAlive();
      }
      any_data_seen = true;
    }
  }

  if (!any_data_seen) {  // Something coming from radar unit?
    if (m_heading_source == HEADING_RADAR) {
      m_heading_source = HEADING_NONE;
    }
  } else if (m_settings.show) {
    CheckGuardZoneBogeys();
  }

  if (m_settings.pass_heading_to_opencpn && m_heading_source == HEADING_RADAR) {
    PassHeadingToOpenCPN();
  }

  if (m_pMessageBox->IsShown() || (m_settings.verbose != 0)) {
    wxString t;
    for (size_t r = 0; r < RADARS; r++) {
      if (m_radar[r]->state.value != RADAR_OFF) {
        t << wxString::Format(wxT("%s\npackets %d/%d\nspokes %d/%d/%d\n"), m_radar[r]->name, m_radar[r]->statistics.packets,
                              m_radar[r]->statistics.broken_packets, m_radar[r]->statistics.spokes,
                              m_radar[r]->statistics.broken_spokes, m_radar[r]->statistics.missing_spokes);
      }
    }
    if (m_pMessageBox->IsShown()) {
      m_pMessageBox->SetRadarInfo(t);
    }
    if (t.length() > 0) {
      t.Replace(wxT("\n"), wxT(" "));
      LOG_RECEIVE(wxT("BR24radar_pi: %s"), t.c_str());
    }
  }
  m_pMessageBox->UpdateMessage(false);

  for (size_t r = 0; r < RADARS; r++) {
    m_radar[r]->UpdateControlState(false);
  }

  for (int r = 0; r < RADARS; r++) {
    m_radar[r]->statistics.broken_packets = 0;
    m_radar[r]->statistics.broken_spokes = 0;
    m_radar[r]->statistics.missing_spokes = 0;
    m_radar[r]->statistics.packets = 0;
    m_radar[r]->statistics.spokes = 0;
  }

  UpdateState();
}

// UpdateState
// run by RenderGLOverlay  updates the color of the toolbar button
void br24radar_pi::UpdateState(void) {
  RadarState state = RADAR_OFF;

  for (int r = 0; r < RADARS; r++) {
    state = wxMax(state, (RadarState)m_radar[r]->state.value);
  }
  if (state == RADAR_OFF) {
    m_toolbar_button = TB_RED;
    //CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_RED);
  } else if (state == RADAR_TRANSMIT) {
    m_toolbar_button = TB_GREEN;
    //CacheSetToolbarToolBitmaps(BM_ID_GREEN, BM_ID_GREEN);
  } else {
    m_toolbar_button = TB_AMBER;
    //CacheSetToolbarToolBitmaps(BM_ID_AMBER, BM_ID_AMBER);
  }

  CheckTimedTransmit(state);
}

//**************************************************************************************************
// Radar Image Graphic Display Processes
//**************************************************************************************************

bool br24radar_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) {
  if (!m_initialized) {
    return true;
  }

  LOG_DIALOG(wxT("BR24radar_pi: RenderOverlay"));

  if (m_opengl_mode) {
    m_opengl_mode = false;
    // Can't hide/show the windows from here, this becomes recursive because the Chart display
    // is managed by wxAuiManager as well.
    m_opengl_mode_changed = true;
  }
  return true;
}

// Called by Plugin Manager on main system process cycle

bool br24radar_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp) {
  if (!m_initialized) {
    return true;
  }

  LOG_DIALOG(wxT("BR24radar_pi: RenderGLOverlay"));
  m_opencpn_gl_context = pcontext;
  if (!m_opencpn_gl_context && !m_opencpn_gl_context_broken) {
    LOG_INFO(wxT("BR24radar_pi: OpenCPN does not pass OpenGL context. Resize of OpenCPN window may be broken!"));
  }
  m_opencpn_gl_context_broken = m_opencpn_gl_context == 0;

  if (!m_opengl_mode) {
    m_opengl_mode = true;
    // Can't hide/show the windows from here, this becomes recursive because the Chart display
    // is managed by wxAuiManager as well.
    m_opengl_mode_changed = true;
  }

  if (!m_settings.show || m_settings.chart_overlay < 0 ||
      m_radar[m_settings.chart_overlay]->state.value != RADAR_TRANSMIT) {  // No overlay desired
    return true;
  }
  if (!m_bpos_set) {  // No overlay possible (yet)
    return true;
  }

  // Always compute m_auto_range_meters, possibly needed by SendState() called
  // from DoTick().
  double max_distance = radar_distance(vp->lat_min, vp->lon_min, vp->lat_max, vp->lon_max, 'm');
  // max_distance is the length of the diagonal of the viewport. If the boat
  // were centered, the
  // max length to the edge of the screen is exactly half that.
  double edge_distance = max_distance / 2.0;
  m_auto_range_meters = (int)edge_distance;
  if (m_auto_range_meters < 50) {
    m_auto_range_meters = 50;
  }

  wxPoint boat_center;
  GetCanvasPixLL(vp, &boat_center, m_ownship_lat, m_ownship_lon);

  m_radar[m_settings.chart_overlay]->SetAutoRangeMeters(m_auto_range_meters);

  //    Calculate image scale factor

  GetCanvasLLPix(vp, wxPoint(0, vp->pix_height - 1), &ulat, &ulon);  // is pix_height a mapable coordinate?
  GetCanvasLLPix(vp, wxPoint(0, 0), &llat, &llon);
  dist_y = radar_distance(llat, llon, ulat, ulon, 'm');  // Distance of height of display - meters
  pix_y = vp->pix_height;
  v_scale_ppm = 1.0;
  if (dist_y > 0.) {
    // v_scale_ppm = vertical pixels per meter
    v_scale_ppm = vp->pix_height / dist_y;  // pixel height of screen div by equivalent meters
  }

  if (m_radar[m_settings.chart_overlay]->state.value == RADAR_TRANSMIT) {
    double rotation = fmod(rad2deg(vp->rotation + vp->skew * m_settings.skew_factor) + 720.0, 360);

    LOG_DIALOG(wxT("BR24radar_pi: RenderRadarOverlay lat=%g lon=%g v_scale_ppm=%g vp_rotation=%g skew=%g scale=%f rot=%g"),
               vp->clat, vp->clon, vp->view_scale_ppm, vp->rotation, vp->skew, vp->chart_scale, rotation);
    RenderRadarOverlay(boat_center, v_scale_ppm, rotation);
  }
  return true;
}

void br24radar_pi::RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, double rotation) {
  RadarInfo *ri = m_radar[m_settings.chart_overlay];

  ri->RenderRadarImage(radar_center, v_scale_ppm, rotation, true);
}

//****************************************************************************

bool br24radar_pi::LoadConfig(void) {
  wxFileConfig *pConf = m_pconfig;
  int intValue;

  if (pConf) {
    pConf->SetPath(wxT("/Plugins/BR24Radar"));
    pConf->Read(wxT("DisplayOption"), &m_settings.display_option, 0);
    pConf->Read(wxT("RangeUnits"), &m_settings.range_units, 0);  // 0 = "Nautical miles"), 1 = "Kilometers"
    if (m_settings.range_units >= 2) {
      m_settings.range_units = 1;
    }
    m_settings.range_unit_meters = (m_settings.range_units == 1) ? 1000 : 1852;
    pConf->Read(wxT("ChartOverlay"), &m_settings.chart_overlay, -1);
    pConf->Read(wxT("EmulatorOn"), &intValue, 0);
    m_settings.emulator_on = intValue != 0;
    pConf->Read(wxT("VerboseLog"), &m_settings.verbose, 0);
    pConf->Read(wxT("Transparency"), &m_settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY);
    pConf->Read(wxT("ScanMaxAge"), &m_settings.max_age, 6);  // default 6
    if (m_settings.max_age < MIN_AGE) {
      m_settings.max_age = MIN_AGE;
    } else if (m_settings.max_age > MAX_AGE) {
      m_settings.max_age = MAX_AGE;
    }
    pConf->Read(wxT("RunTimeOnIdle"), &m_settings.idle_run_time, 2);

    pConf->Read(wxT("GuardZoneOnOverlay"), &m_settings.guard_zone_on_overlay, 1);
    pConf->Read(wxT("GuardZonesThreshold"), &m_settings.guard_zone_threshold, 5L);
    pConf->Read(wxT("GuardZonesRenderStyle"), &m_settings.guard_zone_render_style, 0);
    pConf->Read(wxT("Refreshrate"), &m_settings.refreshrate, 1);
    m_settings.refreshrate = wxMin(m_settings.refreshrate, 5);
    m_settings.refreshrate = wxMax(m_settings.refreshrate, 1);

    pConf->Read(wxT("PassHeadingToOCPN"), &intValue, 0);
    m_settings.pass_heading_to_opencpn = intValue != 0;
    pConf->Read(wxT("DrawingMethod"), &m_settings.drawing_method, 0);
    pConf->Read(wxT("IgnoreRadarHeading"), &m_settings.ignore_radar_heading, 0);

    for (int r = 0; r < RADARS; r++) {
      int v;

      pConf->Read(wxString::Format(wxT("Radar%dRotation"), r), &v, 0);
      m_radar[r]->orientation.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dTransmit"), r), &v, 0);
      m_radar[r]->wantedState = (RadarState)v;
      pConf->Read(wxString::Format(wxT("Radar%dTrails"), r), &v, 0);
      SetControlValue(r, CT_TARGET_TRAILS, v);
      pConf->Read(wxString::Format(wxT("Radar%dWindowShow"), r), &v, 0);
      m_settings.show_radar[r] = v;
      LOG_DIALOG(wxT("BR24radar_pi: LoadConfig: show_radar[%d]=%d"), r, v);
      for (int i = 0; i < GUARD_ZONES; i++) {
        pConf->Read(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), &m_radar[r]->guard_zone[i]->start_bearing, 0.0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), &m_radar[r]->guard_zone[i]->end_bearing, 0.0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), &m_radar[r]->guard_zone[i]->outer_range, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), &m_radar[r]->guard_zone[i]->inner_range, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dType"), r, i), &v, 0);
        m_radar[r]->guard_zone[i]->type = (GuardZoneType)v;
        pConf->Read(wxString::Format(wxT("Radar%dZone%dFilter"), r, i), &m_radar[r]->guard_zone[i]->multi_sweep_filter, 0);
      }
    }

    pConf->Read(wxT("RadarAlertAudioFile"), &m_settings.alert_audio_file);
    pConf->Read(wxT("Show"), &m_settings.show, 0);
    pConf->Read(wxT("MenuAutoHide"), &m_settings.menu_auto_hide, 0);

    pConf->Read(wxT("EnableDualRadar"), &m_settings.enable_dual_radar, 0);
    pConf->Read(wxT("RadarInterface"), &m_settings.mcast_address);

    pConf->Read(wxT("SkewFactor"), &m_settings.skew_factor, 1);

    pConf->Read(wxT("ThresholdRed"), &m_settings.threshold_red, 200);
    pConf->Read(wxT("ThresholdGreen"), &m_settings.threshold_green, 100);
    pConf->Read(wxT("ThresholdBlue"), &m_settings.threshold_blue, 50);
    pConf->Read(wxT("ThresholdMultiSweep"), &m_settings.threshold_multi_sweep, 20);

    pConf->Read(wxT("ReverseZoom"), &m_settings.reverse_zoom, 0);

    SaveConfig();
    return true;
  }
  return false;
}

bool br24radar_pi::SaveConfig(void) {
  wxFileConfig *pConf = m_pconfig;

  if (pConf) {
    pConf->SetPath(wxT("/Plugins/BR24Radar"));
    pConf->Write(wxT("DisplayOption"), m_settings.display_option);
    pConf->Write(wxT("RangeUnits"), m_settings.range_units);
    pConf->Write(wxT("ChartOverlay"), m_settings.chart_overlay);
    pConf->Write(wxT("VerboseLog"), m_settings.verbose);
    pConf->Write(wxT("Transparency"), m_settings.overlay_transparency);
    pConf->Write(wxT("GuardZonesThreshold"), m_settings.guard_zone_threshold);
    pConf->Write(wxT("GuardZonesRenderStyle"), m_settings.guard_zone_render_style);
    pConf->Write(wxT("ScanMaxAge"), m_settings.max_age);
    pConf->Write(wxT("RunTimeOnIdle"), m_settings.idle_run_time);
    pConf->Write(wxT("GuardZoneOnOverlay"), m_settings.guard_zone_on_overlay);
    pConf->Write(wxT("Refreshrate"), m_settings.refreshrate);
    pConf->Write(wxT("PassHeadingToOCPN"), m_settings.pass_heading_to_opencpn);
    pConf->Write(wxT("DrawingMethod"), m_settings.drawing_method);
    pConf->Write(wxT("EmulatorOn"), (int)m_settings.emulator_on);
    pConf->Write(wxT("Show"), m_settings.show);
    pConf->Write(wxT("MenuAutoHide"), m_settings.menu_auto_hide);
    pConf->Write(wxT("RadarAlertAudioFile"), m_settings.alert_audio_file);
    pConf->Write(wxT("EnableDualRadar"), m_settings.enable_dual_radar);
    pConf->Write(wxT("RadarInterface"), m_settings.mcast_address);

    for (int r = 0; r < RADARS; r++) {
      pConf->Write(wxString::Format(wxT("Radar%dRotation"), r), m_radar[r]->orientation.value);
      pConf->Write(wxString::Format(wxT("Radar%dTransmit"), r), m_radar[r]->state.value);
      pConf->Write(wxString::Format(wxT("Radar%dWindowShow"), r), m_settings.show_radar[r]);
      pConf->Write(wxString::Format(wxT("Radar%dTrails"), r), m_radar[r]->target_trails.value);

      LOG_DIALOG(wxT("BR24radar_pi: SaveConfig: show_radar[%d]=%d"), r, m_settings.show_radar[r]);
      for (int i = 0; i < GUARD_ZONES; i++) {
        pConf->Write(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), m_radar[r]->guard_zone[i]->start_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), m_radar[r]->guard_zone[i]->end_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), m_radar[r]->guard_zone[i]->outer_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), m_radar[r]->guard_zone[i]->inner_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dType"), r, i), (int)m_radar[r]->guard_zone[i]->type);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dFilter"), r, i), m_radar[r]->guard_zone[i]->multi_sweep_filter);
      }
    }

    pConf->Write(wxT("SkewFactor"), m_settings.skew_factor);

    pConf->Write(wxT("ThresholdRed"), m_settings.threshold_red);
    pConf->Write(wxT("ThresholdGreen"), m_settings.threshold_green);
    pConf->Write(wxT("ThresholdBlue"), m_settings.threshold_blue);
    pConf->Write(wxT("ThresholdMultiSweep"), m_settings.threshold_multi_sweep);

    pConf->Write(wxT("ReverseZoom"), m_settings.reverse_zoom);

    pConf->Flush();
    LOG_VERBOSE(wxT("BR24radar_pi: Saved settings"));
    return true;
  }

  return false;
}

void br24radar_pi::SetMcastIPAddress(wxString &address) {
  wxMutexLocker lock(m_mutex);

  m_settings.mcast_address = address;
  if (m_pMessageBox) {
    m_pMessageBox->SetMcastIPAddress(address);
  }
}

// Positional Data passed from NMEA to plugin
void br24radar_pi::SetPositionFix(PlugIn_Position_Fix &pfix) {}

void br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix) {
  time_t now = time(0);
  wxString info;

  if (m_var_source <= VARIATION_SOURCE_FIX && !wxIsNaN(pfix.Var) && (fabs(pfix.Var) > 0.0 || m_var == 0.0)) {
    if (m_var_source < VARIATION_SOURCE_FIX || fabs(pfix.Var - m_var) > 0.05) {
      LOG_VERBOSE(wxT("BR24radar_pi: Position fix provides new magnetic variation %f"), pfix.Var);
      if (m_pMessageBox->IsShown()) {
        info = _("GPS");
        info << wxT(" ") << m_var;
        m_pMessageBox->SetVariationInfo(info);
      }
    }
    m_var = pfix.Var;
    m_var_source = VARIATION_SOURCE_FIX;
    m_var_timeout = now + WATCHDOG_TIMEOUT;
  }

  LOG_VERBOSE(wxT("BR24radar_pi: SetPositionFixEx var=%f var_wd=%d"), pfix.Var, NOT_TIMED_OUT(now, m_var_timeout));

  if (m_heading_source != HEADING_RADAR) {
    if (!wxIsNaN(pfix.Hdm) && NOT_TIMED_OUT(now, m_var_timeout)) {
      m_hdt = pfix.Hdm + m_var;
      if (m_heading_source != HEADING_HDM) {
        LOG_INFO(wxT("BR24radar_pi: Heading source is now HDM %f"), m_hdt);
        m_heading_source = HEADING_HDM;
      }
      if (m_pMessageBox->IsShown()) {
        info = _("HDM");
        info << wxT(" ") << m_hdt;
        m_pMessageBox->SetHeadingInfo(info);
      }
      m_hdt_timeout = now + HEADING_TIMEOUT;
    } else if (!wxIsNaN(pfix.Hdt)) {
      m_hdt = pfix.Hdt;
      if (m_heading_source != HEADING_HDT) {
        LOG_INFO(wxT("BR24radar_pi: Heading source is now HDT"));
        m_heading_source = HEADING_HDT;
      }
      if (m_pMessageBox->IsShown()) {
        info = _("HDT");
        info << wxT(" ") << m_hdt;
        m_pMessageBox->SetHeadingInfo(info);
      }
      m_hdt_timeout = now + HEADING_TIMEOUT;
    } else if (!wxIsNaN(pfix.Cog)) {
      m_hdt = pfix.Cog;
      if (m_heading_source != HEADING_COG) {
        LOG_INFO(wxT("BR24radar_pi: Heading source is now COG"));
        m_heading_source = HEADING_COG;
      }
      if (m_pMessageBox->IsShown()) {
        info = _("COG");
        info << wxT(" ") << m_hdt;
        m_pMessageBox->SetHeadingInfo(info);
      }
      m_hdt_timeout = now + HEADING_TIMEOUT;
    }
  }

  if (pfix.FixTime > 0 && NOT_TIMED_OUT(now, pfix.FixTime + WATCHDOG_TIMEOUT)) {
    m_ownship_lat = pfix.Lat;
    m_ownship_lon = pfix.Lon;
    if (!m_bpos_set) {
      LOG_INFO(wxT("BR24radar_pi: GPS position is now known"));
    }
    m_bpos_set = true;
    m_bpos_timestamp = now;
  }
}  // end of br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix)

void br24radar_pi::SetPluginMessage(wxString &message_id, wxString &message_body) {
  static const wxString WMM_VARIATION_BOAT = wxString(_T("WMM_VARIATION_BOAT"));
  wxString info;

  if (message_id.Cmp(WMM_VARIATION_BOAT) == 0) {
    wxJSONReader reader;
    wxJSONValue message;
    if (!reader.Parse(message_body, &message)) {
      wxJSONValue defaultValue(360);
      double variation = message.Get(_T("Decl"), defaultValue).AsDouble();

      if (variation != 360.0) {
        if (m_var_source != VARIATION_SOURCE_WMM) {
          LOG_INFO(wxT("BR24radar_pi: WMM plugin provides new magnetic variation %f"), variation);
        }
        m_var = variation;
        m_var_source = VARIATION_SOURCE_WMM;
        m_var_timeout = time(0) + WATCHDOG_TIMEOUT;
        if (m_pMessageBox->IsShown()) {
          info = _("WMM");
          info << wxT(" ") << m_var;
          m_pMessageBox->SetVariationInfo(info);
        }
      }
    }
  }
}

bool br24radar_pi::SetControlValue(int radar, ControlType controlType, int value) {  // sends the command to the radar
  LOG_TRANSMIT(wxT("BR24radar_pi: %s set %s = %d"), m_radar[radar]->name.c_str(), ControlTypeNames[controlType].c_str(), value);
  switch (controlType) {
    case CT_TRANSPARENCY: {
      m_settings.overlay_transparency = value;
      return true;
    }
    case CT_SCAN_AGE: {
      m_settings.max_age = value;
      return true;
    }
    case CT_TIMED_IDLE: {
      m_settings.timed_idle = value;
      return true;
    }
    case CT_REFRESHRATE: {
      m_settings.refreshrate = value;
      return true;
    }
    case CT_TARGET_TRAILS: {
      m_radar[radar]->target_trails.Update(value);
      m_radar[radar]->ComputeTargetTrails();
    }

    default: {
      if (!m_radar[radar]->SetControlValue(controlType, value)) {
        wxLogError(wxT("BR24radar_pi: %s unhandled control setting for control %d"), m_radar[radar]->name, controlType);
      }
    }
  }
  return false;
}

//*****************************************************************************************************
void br24radar_pi::CacheSetToolbarToolBitmaps(int bm_id_normal, int bm_id_rollover) {
  if ((bm_id_normal == m_sent_bm_id_normal) && (bm_id_rollover == m_sent_bm_id_rollover)) {
    return;  // no change needed
  }

  if ((bm_id_normal == -1) || (bm_id_rollover == -1)) {  // don't do anything, caller's responsibility
    m_sent_bm_id_normal = bm_id_normal;
    m_sent_bm_id_rollover = bm_id_rollover;
    return;
  }

  m_sent_bm_id_normal = bm_id_normal;
  m_sent_bm_id_rollover = bm_id_rollover;

  wxBitmap *pnormal = NULL;
  wxBitmap *prollover = NULL;

  switch (bm_id_normal) {
    case BM_ID_RED:
      pnormal = _img_radar_red;
      break;
    case BM_ID_RED_SLAVE:
      pnormal = _img_radar_red_slave;
      break;
    case BM_ID_GREEN:
      pnormal = _img_radar_green;
      break;
    case BM_ID_GREEN_SLAVE:
      pnormal = _img_radar_green_slave;
      break;
    case BM_ID_AMBER:
      pnormal = _img_radar_amber;
      break;
    case BM_ID_AMBER_SLAVE:
      pnormal = _img_radar_amber_slave;
      break;
    case BM_ID_BLANK:
      pnormal = _img_radar_blank;
      break;
    case BM_ID_BLANK_SLAVE:
      pnormal = _img_radar_blank_slave;
      break;
    default:
      break;
  }

  switch (bm_id_rollover) {
    case BM_ID_RED:
      prollover = _img_radar_red;
      break;
    case BM_ID_RED_SLAVE:
      prollover = _img_radar_red_slave;
      break;
    case BM_ID_GREEN:
      prollover = _img_radar_green;
      break;
    case BM_ID_GREEN_SLAVE:
      prollover = _img_radar_green_slave;
      break;
    case BM_ID_AMBER:
      prollover = _img_radar_amber;
      break;
    case BM_ID_AMBER_SLAVE:
      prollover = _img_radar_amber_slave;
      break;
    case BM_ID_BLANK:
      prollover = _img_radar_blank;
      break;
    case BM_ID_BLANK_SLAVE:
      prollover = _img_radar_blank_slave;
      break;
    default:
      break;
  }

  if ((pnormal) && (prollover)) {
    SetToolbarToolBitmaps(m_tool_id, pnormal, prollover);
  }
}

/*
   SetNMEASentence is used to speed up rotation and variation
   detection if SetPositionEx() is not called very often. This will
   be the case if you have a high speed heading sensor (for instance, 2 to 20
   Hz)
   but only a 1 Hz GPS update.

   Note that the type of heading source is only updated in SetPositionEx().
*/

void br24radar_pi::SetNMEASentence(wxString &sentence) {
  m_NMEA0183 << sentence;
  time_t now = time(0);

  if (m_NMEA0183.PreParse()) {
    if (m_NMEA0183.LastSentenceIDReceived == _T("HDG") && m_NMEA0183.Parse()) {
      LOG_RECEIVE(wxT("BR24radar_pi: received HDG variation=%f var_source=%d m_var=%f"), m_NMEA0183.Hdg.MagneticVariationDegrees,
                  m_var_source, m_var);
      if (!wxIsNaN(m_NMEA0183.Hdg.MagneticVariationDegrees) &&
          (m_var_source <= VARIATION_SOURCE_NMEA || (m_var == 0.0 && m_NMEA0183.Hdg.MagneticVariationDegrees > 0.0))) {
        double newVar;
        if (m_NMEA0183.Hdg.MagneticVariationDirection == East) {
          newVar = +m_NMEA0183.Hdg.MagneticVariationDegrees;
        } else {
          newVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
        }
        if (fabs(newVar - m_var) >= 0.1) {
          LOG_VERBOSE(wxT("BR24radar_pi: NMEA provides new magnetic variation %f"), newVar);
        }
        m_var = newVar;
        m_var_source = VARIATION_SOURCE_NMEA;
        m_var_timeout = now + WATCHDOG_TIMEOUT;
        if (m_pMessageBox->IsShown()) {
          wxString info = _("NMEA");
          info << wxT(" ") << m_var;
          m_pMessageBox->SetVariationInfo(info);
        }
      }
      if (m_heading_source == HEADING_HDM && !wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees)) {
        m_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + m_var;
        m_hdt_timeout = now + HEADING_TIMEOUT;
      }
    } else if (m_heading_source == HEADING_HDM && m_NMEA0183.LastSentenceIDReceived == _T("HDM") && m_NMEA0183.Parse() &&
               !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic)) {
      m_hdt = m_NMEA0183.Hdm.DegreesMagnetic + m_var;
      m_hdt_timeout = now + HEADING_TIMEOUT;
    } else if (m_heading_source == HEADING_HDT && m_NMEA0183.LastSentenceIDReceived == _T("HDT") && m_NMEA0183.Parse() &&
               !wxIsNaN(m_NMEA0183.Hdt.DegreesTrue)) {
      m_hdt = m_NMEA0183.Hdt.DegreesTrue;
      m_hdt_timeout = now + HEADING_TIMEOUT;
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
