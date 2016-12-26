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

#ifndef _BR24RADARPI_H_
#define _BR24RADARPI_H_

#define MY_API_VERSION_MAJOR 1
#define MY_API_VERSION_MINOR 12

#include "pi_common.h"
#include "jsonreader.h"
#include "version.h"
#include "nmea0183/nmea0183.h"

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class GuardZone;
class RadarInfo;

class br24ControlsDialog;
class br24MessageBox;
class br24OptionsDialog;
class br24Receive;
class br24Transmit;
class br24radar_pi;
class GuardZoneBogey;
class RadarArpa;

#define SPOKES (4096)               // BR radars can generate up to 4096 spokes per rotation,
#define LINES_PER_ROTATION (2048)   // but use only half that in practice
#define RETURNS_PER_LINE (512)      // BR radars generate 512 separate values per range, at 8 bits each
#define DEGREES_PER_ROTATION (360)  // Classical math
#define RADARS (2)                  // Number of radars supported by this PI. 2 since 4G supports 2. More work
                                    // needed if you intend to add multiple radomes to network!
#define GUARD_ZONES (2)             // Could be increased if wanted

#define BEARING_LINES (2)           // And these as well

static const int SECONDS_PER_TIMED_IDLE_SETTING = 5 * 60;  // 5 minutes increment for each setting
static const int SECONDS_PER_TRANSMIT_BURST = 30;

#define OPENGL_ROTATION (-90.0)  // Difference between 'up' and OpenGL 'up'...

#define ALL_RADARS(var, value) (((var)[0] == (value)) && ((var)[1] == (value)))
#define ANY_RADAR(var, value) (((var)[0] == (value)) || ((var)[1] == (value)))

typedef int SpokeBearing;  // A value from 0 -- LINES_PER_ROTATION indicating a bearing (? = North,
                           // +ve = clockwise)

// Use the above to convert from 'raw' headings sent by the radar (0..4095) into classical degrees
// (0..359) and back
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double)DEGREES_PER_ROTATION / SPOKES)
#define SCALE_RAW_TO_DEGREES2048(raw) ((raw) * (double)DEGREES_PER_ROTATION / LINES_PER_ROTATION)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double)SPOKES / DEGREES_PER_ROTATION))
#define SCALE_DEGREES_TO_RAW2048(angle) ((int)((angle) * (double)LINES_PER_ROTATION / DEGREES_PER_ROTATION))
#define MOD_DEGREES(angle) (fmod(angle + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION))
#define MOD_ROTATION(raw) (((raw) + 2 * SPOKES) % SPOKES)
#define MOD_ROTATION2048(raw) (((raw) + 2 * LINES_PER_ROTATION) % LINES_PER_ROTATION)

#define WATCHDOG_TIMEOUT (10)  // After 10s assume GPS and heading data is invalid

#define TIMED_OUT(t, timeout) (t >= timeout)
#define NOT_TIMED_OUT(t, timeout) (!TIMED_OUT(t, timeout))

#ifndef M_SETTINGS
#define M_SETTINGS m_pi->m_settings
#endif
#define LOGLEVEL_INFO 0
#define LOGLEVEL_VERBOSE 1
#define LOGLEVEL_DIALOG 2
#define LOGLEVEL_TRANSMIT 4
#define LOGLEVEL_RECEIVE 8
#define LOGLEVEL_GUARD 16
#define IF_LOG_AT_LEVEL(x) if ((M_SETTINGS.verbose & x) != 0)
#define IF_LOG_AT(x, y)       \
  do {                        \
    IF_LOG_AT_LEVEL(x) { y; } \
  } while (0)
#define LOG_INFO wxLogMessage
#define LOG_VERBOSE IF_LOG_AT_LEVEL(LOGLEVEL_VERBOSE) wxLogMessage
#define LOG_DIALOG IF_LOG_AT_LEVEL(LOGLEVEL_DIALOG) wxLogMessage
#define LOG_TRANSMIT IF_LOG_AT_LEVEL(LOGLEVEL_TRANSMIT) wxLogMessage
#define LOG_RECEIVE IF_LOG_AT_LEVEL(LOGLEVEL_RECEIVE) wxLogMessage
#define LOG_GUARD IF_LOG_AT_LEVEL(LOGLEVEL_GUARD) wxLogMessage

enum { BM_ID_RED, BM_ID_RED_SLAVE, BM_ID_GREEN, BM_ID_GREEN_SLAVE, BM_ID_AMBER, BM_ID_AMBER_SLAVE, BM_ID_BLANK, BM_ID_BLANK_SLAVE };

// Arranged from low to high priority:
enum HeadingSource {
  HEADING_NONE,
  HEADING_FIX_COG,
  HEADING_FIX_HDM,
  HEADING_FIX_HDT,
  HEADING_NMEA_HDM,
  HEADING_NMEA_HDT,
  HEADING_RADAR_HDM,
  HEADING_RADAR_HDT
};

enum RadarState { RADAR_OFF, RADAR_STANDBY, RADAR_TRANSMIT, RADAR_WAKING_UP };

struct receive_statistics {
  int packets;
  int broken_packets;
  int spokes;
  int broken_spokes;
  int missing_spokes;
};

// WARNING
// WARNING If you add to ControlType, make sure to add strings to ControlTypeNames as well!
// WARNING
typedef enum ControlType {
  CT_RANGE,
  CT_GAIN,
  CT_SEA,
  CT_RAIN,
  CT_TRANSPARENCY,
  CT_INTERFERENCE_REJECTION,
  CT_TARGET_SEPARATION,
  CT_NOISE_REJECTION,
  CT_TARGET_BOOST,
  CT_TARGET_EXPANSION,
  CT_REFRESHRATE,
  CT_SCAN_SPEED,
  CT_SCAN_AGE,
  CT_TIMED_IDLE,
  CT_BEARING_ALIGNMENT,
  CT_SIDE_LOBE_SUPPRESSION,
  CT_ANTENNA_HEIGHT,
  CT_LOCAL_INTERFERENCE_REJECTION,
  CT_TARGET_TRAILS,
  CT_TRAILS_MOTION,
  CT_MAIN_BANG_SIZE,
  CT_MAX  // Keep this last, see below
} ControlType;

// The following are only for logging, so don't care about translations.
static string ControlTypeNames[CT_MAX] = {"Range",
                                          "Gain",
                                          "Sea",
                                          "Rain",
                                          "Transparency",
                                          "Interference rejection",
                                          "Target separation",
                                          "Noise rejection",
                                          "Target boost",
                                          "Target expansion",
                                          "Refresh rate",
                                          "Scan speed",
                                          "Scan age",
                                          "Timed idle",
                                          "Bearing alignment",
                                          "Side lobe suppression",
                                          "Antenna height",
                                          "Local interference rejection",
                                          "Target trails",
                                          "Target trails motion",
                                          "Main bang size"};

typedef enum GuardZoneType { GZ_ARC, GZ_CIRCLE } GuardZoneType;

typedef enum RadarType { RT_UNKNOWN, RT_BR24, RT_3G, RT_4G } RadarType;

enum BlobColour {
  BLOB_NONE,
  BLOB_HISTORY_0,
  BLOB_HISTORY_1,
  BLOB_HISTORY_2,
  BLOB_HISTORY_3,
  BLOB_HISTORY_4,
  BLOB_HISTORY_5,
  BLOB_HISTORY_6,
  BLOB_HISTORY_7,
  BLOB_HISTORY_8,
  BLOB_HISTORY_9,
  BLOB_HISTORY_10,
  BLOB_HISTORY_11,
  BLOB_HISTORY_12,
  BLOB_HISTORY_13,
  BLOB_HISTORY_14,
  BLOB_HISTORY_15,
  BLOB_HISTORY_16,
  BLOB_HISTORY_17,
  BLOB_HISTORY_18,
  BLOB_HISTORY_19,
  BLOB_HISTORY_20,
  BLOB_HISTORY_21,
  BLOB_HISTORY_22,
  BLOB_HISTORY_23,
  BLOB_HISTORY_24,
  BLOB_HISTORY_25,
  BLOB_HISTORY_26,
  BLOB_HISTORY_27,
  BLOB_HISTORY_28,
  BLOB_HISTORY_29,
  BLOB_HISTORY_30,
  BLOB_HISTORY_31,
  BLOB_WEAK,
  BLOB_INTERMEDIATE,
  BLOB_STRONG
};
#define BLOB_HISTORY_MAX BLOB_HISTORY_31
#define BLOB_HISTORY_COLOURS (BLOB_HISTORY_MAX - BLOB_NONE)
#define BLOB_COLOURS (BLOB_STRONG + 1)

extern const char *convertRadarToString(int range_meters, int units, int index);
extern double local_distance(double lat1, double lon1, double lat2, double lon2);
extern double local_bearing(double lat1, double lon1, double lat2, double lon2);

enum DisplayModeType { DM_CHART_OVERLAY, DM_CHART_NONE };
enum ToolbarIconColor { TB_NONE, TB_HIDDEN, TB_SEARCHING, TB_SEEN, TB_STANDBY, TB_ACTIVE };
enum VariationSource { VARIATION_SOURCE_NONE, VARIATION_SOURCE_NMEA, VARIATION_SOURCE_FIX, VARIATION_SOURCE_WMM };

static const int RangeUnitsToMeters[2] = {1852, 1000};

static const bool HasBitCount2[8] = {
    false,  // 000
    false,  // 001
    false,  // 010
    true,   // 011
    false,  // 100
    true,   // 101
    true,   // 110
    true,   // 111
};

#define DEFAULT_OVERLAY_TRANSPARENCY (5)
#define MIN_OVERLAY_TRANSPARENCY (0)
#define MAX_OVERLAY_TRANSPARENCY (10)
#define MIN_AGE (4)
#define MAX_AGE (12)

enum RangeUnits { RANGE_NAUTICAL, RANGE_METRIC };

/**
 * The data that is stored in the opencpn.ini file. Most of this is set in the OptionsDialog,
 * some of it is 'secret' and can only be set by manipulating the ini file directly.
 */
struct PersistentSettings {
  int overlay_transparency;
  int range_index;                  // index into range array, see RadarInfo.cpp
  int verbose;                      // Loglevel 0..4.
  int guard_zone_threshold;         // How many blobs must be sent by radar before we fire alarm
  int guard_zone_render_style;      // 0 = Shading, 1 = Outline, 2 = Shading + Outline
  int guard_zone_timeout;           // How long before we warn again when bogeys are found
  bool guard_zone_on_overlay;       // 0 = false, 1 = true
  bool trails_on_overlay;           // 0 = false, 1 = true
  int guard_zone_debug_inc;         // Value to add on every cycle to guard zone bearings, for testing.
  double skew_factor;               // Set to -1 or other value to correct skewing
  RangeUnits range_units;           // See enum
  int range_unit_meters;            // ... 1852 or 1000, depending on range_units
  int max_age;                      // Scans older than this in seconds will be removed
  int timed_idle;                   // 0 = off, 1 = 5 mins, etc. to 7 = 35 mins
  int idle_run_time;                // how long, in seconds, should a idle run be? Value < 30 is ignored set to 30.
  int refreshrate;                  // How quickly to refresh the display
  bool show;                        // whether to show any radar (overlay or window)
  bool show_radar[RADARS];          // whether to show radar window
  bool show_radar_control[RADARS];  // whether to show radar window
  bool transmit_radar[RADARS];      // whether radar should be transmitting (persistent)
  int chart_overlay;                // -1 = none, otherwise = radar number
  int menu_auto_hide;               // 0 = none, 1 = 10s, 2 = 30s
  bool pass_heading_to_opencpn;     // Pass heading coming from radar as NMEA data to OpenCPN
  bool enable_cog_heading;          // Allow COG as heading. Should be taken out back and shot.
  bool enable_dual_radar;           // Should the dual radar be enabled for 4G?
  bool emulator_on;                 // Emulator, useful when debugging without radar
  int drawing_method;               // VertexBuffer, Shader, etc.
  bool ignore_radar_heading;        // For testing purposes
  bool reverse_zoom;                // false = normal, true = reverse
  int threshold_red;                // Radar data has to be this strong to show as STRONG
  int threshold_green;              // Radar data has to be this strong to show as INTERMEDIATE
  int threshold_blue;               // Radar data has to be this strong to show as WEAK
  int threshold_multi_sweep;        // Radar data has to be this strong not to be ignored in multisweep
  int main_bang_size;               // Pixels at center to ignore
  int type_detection_method;        // 0 = default, 1 = ignore reports
  wxPoint control_pos[RADARS];      // Saved position of control menu windows
  wxPoint window_pos[RADARS];       // Saved position of radar windows, when floating and not docked
  wxPoint alarm_pos;                // Saved position of alarm window
  wxString alert_audio_file;        // Filepath of alarm audio file. Must be WAV.
  wxString mcast_address;           // Saved address of radar. Used to speed up next boot.
  wxColour trail_start_colour;      // Starting colour of a trail
  wxColour trail_end_colour;        // Ending colour of a trail
  wxColour strong_colour;           // Colour for STRONG returns
  wxColour intermediate_colour;     // Colour for INTERMEDIATE returns
  wxColour weak_colour;             // Colour for WEAK returns
};

struct scan_line {
  int range;                            // range of this scan line in decimeters
  wxLongLong age;                       // how old this scan line is. We keep old scans on-screen for a while
  UINT8 data[RETURNS_PER_LINE + 1];     // radar return strength, data[512] is an additional element,
                                        // accessed in drawing the spokes
  UINT8 history[RETURNS_PER_LINE + 1];  // contains per bit the history of previous scans.
  // Each scan this byte is left shifted one bit. If the strength (=level) of a return is above the
  // threshold
  // a 1 is added in the rightmost position, if below threshold, a 0.
};

//Check for AIS targets inside ARPA zone
#define SIZEAISAR (50)
struct AisArpa {
    long ais_mmsi;
    time_t ais_time_upd;
    double ais_lat;
    double ais_lon;
    wxString ais_name;
};

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define BR24RADAR_TOOL_POSITION -1  // Request default positioning of toolbar tool

#define PLUGIN_OPTIONS                                                                                                       \
  (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK | WANTS_OVERLAY_CALLBACK | WANTS_TOOLBAR_CALLBACK | \
   INSTALLS_TOOLBAR_TOOL | USES_AUI_MANAGER | WANTS_CONFIG | WANTS_NMEA_EVENTS | WANTS_NMEA_SENTENCES | WANTS_PREFERENCES |  \
   WANTS_PLUGIN_MESSAGING | WANTS_CURSOR_LATLON | WANTS_MOUSE_EVENTS)

class br24radar_pi : public opencpn_plugin_112 {
 public:
  br24radar_pi(void *ppimgr);
  ~br24radar_pi();
  void PrepareRadarImage(int angle);

  //    The required PlugIn Methods
  int Init(void);
  bool DeInit(void);

  int GetAPIVersionMajor();
  int GetAPIVersionMinor();
  int GetPlugInVersionMajor();
  int GetPlugInVersionMinor();

  wxBitmap *GetPlugInBitmap();
  wxString GetCommonName();
  wxString GetShortDescription();
  wxString GetLongDescription();

  //    The required override PlugIn Methods
  bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
  bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
  void SetPositionFix(PlugIn_Position_Fix &pfix);
  void SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix);
  void SetPluginMessage(wxString &message_id, wxString &message_body);
  void SetNMEASentence(wxString &sentence);
  void SetDefaults(void);
  int GetToolbarToolCount(void);
  void OnToolbarToolCallback(int id);
  void OnContextMenuItemCallback(int id);
  void ShowPreferencesDialog(wxWindow *parent);
  void SetCursorLatLon(double lat, double lon);
  bool MouseEventHook(wxMouseEvent &event);
  bool m_guard_bogey_confirmed;

  // The wxTimer overrides

  void Notify();

  // Other public methods

  void NotifyRadarWindowViz();

  void OnControlDialogClose(RadarInfo *ri);
  void SetDisplayMode(DisplayModeType mode);

  void ShowRadarControl(int radar, bool show = true, bool reparent = true);
  void ShowGuardZoneDialog(int radar, int zone);
  void OnGuardZoneDialogClose(RadarInfo *ri);
  void ConfirmGuardZoneBogeys();

  bool SetControlValue(int radar, ControlType controlType, int value);

  // Various state decisions
  bool IsRadarOnScreen(int radar) { return m_settings.show && (m_settings.show_radar[radar] || m_settings.chart_overlay == radar); }
  bool IsOverlayOnScreen(int radar) { return m_settings.show && m_settings.chart_overlay == radar; }

  bool LoadConfig();
  bool SaveConfig();

  long GetRangeMeters();
  long GetOptimalRangeMeters();

  wxString GetGuardZoneText(RadarInfo *ri);

  void SetMcastIPAddress(wxString &msg);

  void SetRadarHeading(double heading = nan(""), bool isTrue = false);

  wxFont m_font;      // The dialog font at a normal size
  wxFont m_fat_font;  // The dialog font at a bigger size, bold
  int m_display_width, m_display_height;

  PersistentSettings m_settings;
  RadarInfo *m_radar[RADARS];
  wxString m_perspective[RADARS];  // Temporary storage of window location when plugin is disabled

  br24MessageBox *m_pMessageBox;
  wxWindow *m_parent_window;
  wxGLContext *m_opencpn_gl_context;
  bool m_opencpn_gl_context_broken;

  double m_hdt;                    // this is the heading that the pi is using for all heading operations, in degrees.
                                   // m_hdt will come from the radar if available else from the NMEA stream.
  time_t m_hdt_timeout;            // When we consider heading is lost
  double m_hdm;                    // Last magnetic heading obtained
  time_t m_hdm_timeout;            // When we consider heading is lost
  double m_radar_heading;          // Last heading obtained from radar, or nan if none
  bool m_radar_heading_true;       // Was TRUE flag set on radar heading?
  time_t m_radar_heading_timeout;  // When last heading was obtained from radar, or 0 if not

  // Variation. Used to convert magnetic into true heading.
  // Can come from SetPositionFixEx, which may hail from the WMM plugin
  // and is thus to be preferred, or GPS or a NMEA sentence. The latter will probably
  // have an outdated variation model, so is less preferred. Besides, some devices
  // transmit invalid (zero) values. So we also let non-zero values prevail.
  double m_var;  // local magnetic variation, in degrees
  VariationSource m_var_source;
  time_t m_var_timeout;

  HeadingSource m_heading_source;
  bool m_opengl_mode;
  volatile bool m_opengl_mode_changed;
  bool m_bpos_set;
  time_t m_bpos_timestamp;

  // Cursor position. Used to show position in radar window
  double m_cursor_lat, m_cursor_lon;
  double m_ownship_lat, m_ownship_lon;

  bool m_initialized;      // True if Init() succeeded and DeInit() not called yet.
  bool m_first_init;       // True in first Init() call.
  wxLongLong m_boot_time;  // millis when started

  // Timed Transmit
  time_t m_idle_standby;   // When we will change to standby
  time_t m_idle_transmit;  // When we will change to transmit

  //Check for AIS targets inside ARPA zone
  wxString JsonAIS; //Temp for Json AIS message
  AisArpa ais_in_arpa[SIZEAISAR];
  int count_ais_in_arpa;
  bool FindAIS_at_arpaPos(const double &lat, const double &lon, const int &dist);

 private:
  void RadarSendState(void);
  void UpdateState(void);
  void DoTick(void);
  void Select_Clutter(int req_clutter_index);
  void Select_Rejection(int req_rejection_index);
  void CheckGuardZoneBogeys(void);
  void RenderRadarBuffer(wxDC *pdc, int width, int height);
  void PassHeadingToOpenCPN();
  void CacheSetToolbarToolBitmaps();
  void CheckTimedTransmit(RadarState state);
  void RequestStateAllRadars(RadarState state);
  void SetRadarWindowViz(bool reparent = false);

  wxCriticalSection m_exclusive;  // protects callbacks that come from multiple radars

  wxFileConfig *m_pconfig;
  int m_context_menu_control_id;
  int m_context_menu_show_id;
  int m_context_menu_hide_id;
  int m_context_menu_set_marpa_target;
  int m_context_menu_delete_marpa_target;
  int m_context_menu_delete_all_marpa_targets;

  int m_tool_id;
  wxBitmap *m_pdeficon;

  //    Controls added to Preferences panel
  wxCheckBox *m_pShowIcon;

  // Icons
  wxString m_shareLocn;
  // wxBitmap *m_ptemp_icon;

  NMEA0183 m_NMEA0183;

  ToolbarIconColor m_toolbar_button;
  ToolbarIconColor m_sent_toolbar_button;

  bool m_old_data_seen;
  bool m_notify_radar_window_viz;

#define HEADING_TIMEOUT (5)

  GuardZoneBogey *m_bogey_dialog;
  bool m_guard_bogey_seen;  // Saw guardzone bogeys on last check
  time_t m_alarm_sound_timeout;
  time_t m_guard_bogey_timeout;  // If we haven't seen bogeys for this long we reset confirm
#define CONFIRM_RESET_TIMEOUT (15)
};

PLUGIN_END_NAMESPACE

#include "br24OptionsDialog.h"
#include "br24ControlsDialog.h"
#include "br24MessageBox.h"
#include "br24Transmit.h"
#include "GuardZone.h"
#include "RadarInfo.h"

#endif /* _BR24RADAR_PI_H_ */
