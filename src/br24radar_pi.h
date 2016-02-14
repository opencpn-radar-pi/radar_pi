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

#ifndef _BR24RADARPI_H_
#define _BR24RADARPI_H_

#define MY_API_VERSION_MAJOR 1
#define MY_API_VERSION_MINOR 10

#include "pi_common.h"
#include "jsonreader.h"
#include "version.h"
#include "nmea0183/nmea0183.h"

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class GuardZone;
class IdleDialog;
class RadarInfo;

class br24ControlsDialog;
class br24MessageBox;
class br24OptionsDialog;
class br24Receive;
class br24Transmit;
class br24radar_pi;

#define SPOKES (4096)               // BR radars can generate up to 4096 spokes per rotation,
#define LINES_PER_ROTATION (2048)   // but use only half that in practice
#define RETURNS_PER_LINE (512)      // BR radars generate 512 separate values per range, at 8 bits each
#define DEGREES_PER_ROTATION (360)  // Classical math
#define RADARS (2)                  // Number of radars supported by this PI. 2 since 4G supports 2. More work
                                    // needed if you intend to add multiple radomes to network!
#define GUARD_ZONES (2)             // Could be increased if wanted

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
#define TIMER_NOT_ELAPSED(t, watchdog) (t < watchdog + WATCHDOG_TIMEOUT)
#define TIMER_ELAPSED(t, watchdog) (!TIMER_NOT_ELAPSED(t, watchdog))

enum { BM_ID_RED, BM_ID_RED_SLAVE, BM_ID_GREEN, BM_ID_GREEN_SLAVE, BM_ID_AMBER, BM_ID_AMBER_SLAVE, BM_ID_BLANK, BM_ID_BLANK_SLAVE };

enum HeadingSource { HEADING_NONE, HEADING_HDM, HEADING_HDT, HEADING_COG, HEADING_RADAR };

enum RadarState { RADAR_OFF, RADAR_STANDBY, RADAR_TRANSMIT };

struct receive_statistics {
  int packets;
  int broken_packets;
  int spokes;
  int broken_spokes;
  int missing_spokes;
};

static wxSize g_buttonSize;

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
  CT_REFRESHRATE,
  CT_PASSHEADING,
  CT_SCAN_SPEED,
  CT_SCAN_AGE,
  CT_TIMED_IDLE,
  CT_BEARING_ALIGNMENT,
  CT_SIDE_LOBE_SUPPRESSION,
  CT_ANTENNA_HEIGHT,
  CT_LOCAL_INTERFERENCE_REJECTION,
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
                                          "Refresh rate",
                                          "Pass heading",
                                          "Scan speed",
                                          "Scan age",
                                          "Timed idle",
                                          "Bearing alignment",
                                          "Side lobe suppression",
                                          "Antenna height",
                                          "Local interference rejection"};

typedef enum GuardZoneType { GZ_OFF, GZ_ARC, GZ_CIRCLE } GuardZoneType;

typedef enum RadarType {
  RT_UNKNOWN,
  RT_BR24,  // 3G is just a fancy BR24
  RT_4G
} RadarType;

enum BlobColor { BLOB_NONE, BLOB_BLUE, BLOB_GREEN, BLOB_RED };

extern size_t convertRadarMetersToIndex(int *range_meters, int units, RadarType radar_type);
extern size_t convertMetersToRadarAllowedValue(int *range_meters, int units, RadarType radar_type);

enum DisplayModeType { DM_CHART_OVERLAY, DM_CHART_NONE };
enum ToolbarIconColor { TB_RED, TB_AMBER, TB_GREEN };
enum VariationSource { VARIATION_SOURCE_NONE, VARIATION_SOURCE_NMEA, VARIATION_SOURCE_FIX, VARIATION_SOURCE_WMM };

enum DialogLocationID { DL_MESSAGE, DL_BOGEY, DL_TIMEDTRANSMIT, DL_RADARWINDOW, DL_CONTROL };
#define DIALOG_MAX (DL_CONTROL + RADARS)

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

struct WindowLocation {  // Position and size of a particular window, possibly only pos is set, or
                         // nothing.
  wxPoint pos;
  wxSize size;
};

/**
 * The data that is stored in the opencpn.ini file. Most of this is set in the OptionsDialog,
 * some of it is 'secret' and can only be set by manipulating the ini file directly.
 */
struct PersistentSettings {
  int overlay_transparency;
  int range_index;
  int verbose;  // Loglevel 0..4.
  int display_option;
  int chart_overlay;         // -1 = none, otherwise = radar number
  int guard_zone_threshold;  // How many blobs must be sent by radar before we fire alarm
  int guard_zone_render_style;
  double range_calibration;
  double heading_correction;
  double skew_factor;
  int range_units;        // 0 = Nautical miles, 1 = Kilometers
  int range_unit_meters;  // ... 1852 or 1000, depending on range_units
  int max_age;
  int timed_idle;
  int idle_run_time;
  int draw_algorithm;
  int refreshrate;
  int show_radar;
  bool pass_heading_to_opencpn;
  bool enable_dual_radar;  // Should the dual radar be enabled for 4G?
  bool emulator_on;
  int drawing_method;  // VertexBuffer, Shader, etc.
  int threshold_red;
  int threshold_green;
  int threshold_blue;
  int threshold_multi_sweep;
  wxString alert_audio_file;
  int automatic_dialog_location;
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

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define BR24RADAR_TOOL_POSITION -1  // Request default positioning of toolbar tool

#define PLUGIN_OPTIONS                                                                                                       \
  (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK | WANTS_OVERLAY_CALLBACK | WANTS_TOOLBAR_CALLBACK | \
   INSTALLS_TOOLBAR_TOOL | INSTALLS_CONTEXTMENU_ITEMS | USES_AUI_MANAGER | WANTS_CONFIG | WANTS_NMEA_EVENTS |                \
   WANTS_NMEA_SENTENCES | WANTS_PREFERENCES | WANTS_PLUGIN_MESSAGING)

class br24radar_pi : public opencpn_plugin_110 {
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
  void OnContextMenuItemCallback(int id);
  void SetNMEASentence(wxString &sentence);

  void SetDefaults(void);
  int GetToolbarToolCount(void);
  void OnToolbarToolCallback(int id);
  bool SetRadarWindowViz(bool show);
  void ShowPreferencesDialog(wxWindow *parent);

  // Other public methods

  void OnControlDialogClose(RadarInfo *ri);
  void OnMessageBoxClose();
  void SetDisplayMode(DisplayModeType mode);
  void UpdateDisplayParameters(void);

  void ShowRadarControl(int radar, bool show = true);
  void ShowGuardZoneDialog(int radar, int zone);
  void OnGuardZoneDialogClose(RadarInfo *ri);
  void ConfirmGuardZoneBogeys();

  void SetControlValue(int radar, ControlType controlType, int value);
  void ComputeColorMap();

  bool LoadConfig(void);
  bool SaveConfig(void);

  long GetRangeMeters();
  long GetOptimalRangeMeters();

  void RenderGuardZone(wxPoint radar_center, double v_scale_ppm, int AB);
  wxString GetGuardZoneText(RadarInfo *ri, bool withTimeout);

  wxFont m_font;      // The dialog font at a normal size
  wxFont m_fat_font;  // The dialog font at a bigger size, bold

  PersistentSettings m_settings;
  RadarInfo *m_radar[RADARS];
  wxString m_perspective[RADARS];  // Temporary storage of window location when plugin is disabled

  int m_OldUseShader;
  int m_OldDisplayOption;

  br24OptionsDialog *m_pOptionsDialog;
  br24MessageBox *m_pMessageBox;
  IdleDialog *m_pIdleDialog;

  wxGLContext *m_opencpn_gl_context;
  bool m_opencpn_gl_context_broken;

  bool m_want_message_box;
  bool m_heading_on_radar;
  double m_hdt;  // this is the heading that the pi is using for all heading operations, in degrees.
                 // m_hdt will come from the radar if available else from the NMEA stream.

  // Variation. Used to convert magnetic into true heading.
  // Can come from SetPositionFixEx, which may hail from the WMM plugin
  // and is thus to be preferred, or GPS or a NMEA sentence. The latter will probably
  // have an outdated variation model, so is less preferred. Besides, some devices
  // transmit invalid (zero) values. So we also let non-zero values prevail.
  double m_var;  // local magnetic variation, in degrees
  VariationSource m_var_source;
  time_t m_var_watchdog;

  HeadingSource m_heading_source;
  bool m_opengl_mode;
  bool m_bpos_set;
  time_t m_bpos_watchdog;

  bool m_initialized;  // True if Init() succeeded and DeInit() not called yet.
  bool m_first_init;   // True in first Init() call.

  WindowLocation m_dialogLocation[DIALOG_MAX];

  // Speedup lookup tables of color to r,g,b, set dependent on m_settings.display_option.
  GLubyte m_color_map_red[BLOB_RED + 1];
  GLubyte m_color_map_green[BLOB_RED + 1];
  GLubyte m_color_map_blue[BLOB_RED + 1];
  BlobColor m_color_map[UINT8_MAX + 1];

 private:
  void RadarSendState(void);
  void UpdateState(void);
  void DoTick(void);
  void Select_Clutter(int req_clutter_index);
  void Select_Rejection(int req_rejection_index);
  void CheckGuardZoneBogeys(void);
  void RenderRadarBuffer(wxDC *pdc, int width, int height);
  void RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, double rotation);
  void PassHeadingToOpenCPN();
  void CacheSetToolbarToolBitmaps(int bm_id_normal, int bm_id_rollover);
  void CheckTimedTransmit(RadarState state);
  void SetDesiredStateAllRadars(RadarState desiredState);

  wxFileConfig *m_pconfig;
  wxWindow *m_parent_window;
  wxMenu *m_pmenu;

  int m_display_width, m_display_height;
  int m_tool_id;
  wxBitmap *m_pdeficon;

  //    Controls added to Preferences panel
  wxCheckBox *m_pShowIcon;

  wxBitmap *m_ptemp_icon;
  int m_sent_bm_id_normal;
  int m_sent_bm_id_rollover;

  // Control id's of Context Menu Items
  int m_context_menu_control_id;
  int m_context_menu_show_window_id;
  int m_context_menu_hide_window_id;

  NMEA0183 m_NMEA0183;

  double llat, llon, ulat, ulon, dist_y, pix_y, v_scale_ppm;

  ToolbarIconColor m_toolbar_button;
  double m_ownship_lat, m_ownship_lon;

  double m_hdm;

  bool m_old_data_seen;

  int m_auto_range_meters;  // What the range should be, at least, when AUTO mode is selected
  int m_previous_auto_range_meters;
  bool m_update_error_control;
  wxString m_error_msg;

  // Timed Transmit
  bool m_init_timed_transmit;
  int m_idle_dialog_time_left;
  int m_TimedTransmit_IdleBoxMode;
  int m_idle_time_left;

  time_t m_idle_watchdog;
  time_t m_hdt_watchdog;

  bool m_guard_bogey_confirmed;
  time_t m_alarm_sound_last;
#define ALARM_TIMEOUT (5)
};

PLUGIN_END_NAMESPACE

#include "br24OptionsDialog.h"
#include "br24ControlsDialog.h"
#include "br24MessageBox.h"
#include "br24Transmit.h"
#include "GuardZone.h"
#include "IdleDialog.h"
#include "RadarInfo.h"

#endif /* _BRRADAR_PI_H_ */
