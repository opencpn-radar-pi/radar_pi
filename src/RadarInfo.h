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

#ifndef _RADAR_INFO_H_
#define _RADAR_INFO_H_

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

class RadarDraw;
class RadarCanvas;
class RadarPanel;
class GuardZoneBogey;

struct RadarRange {
  int meters;
  int actual_meters;
  const char *name;
  const char *range1;
  const char *range2;
  const char *range3;
};

class radar_control_item {
 public:
  void Update(int v) {
    wxCriticalSectionLocker lock(m_exclusive);

    if (v != m_button) {
      m_mod = true;
      m_button = v;
    }
    m_value = v;
  };

  bool GetButton(int *value) {
    wxCriticalSectionLocker lock(m_exclusive);
    bool changed = m_mod;
    if (value) {
      *value = this->m_value;
    }

    m_mod = false;
    return changed;
  }

  int GetButton() {
    wxCriticalSectionLocker lock(m_exclusive);

    m_mod = false;
    return m_button;
  }

  int GetValue() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_value;
  }

  bool IsModified() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_mod;
  }

  radar_control_item() {
    m_value = 0;
    m_button = 0;
    m_mod = false;
  }

 protected:
  wxCriticalSection m_exclusive;
  int m_value;
  int m_button;
  bool m_mod;
};

class radar_range_control_item : public radar_control_item {
 public:
  PersistentSettings *m_settings;

  void Update(int v);
  const RadarRange *GetRange() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_range;
  }

  radar_range_control_item() {
    m_value = 0;
    m_button = 0;
    m_mod = false;
    m_range = 0;
    m_settings = 0;
  }

 private:
  const RadarRange *m_range;
};

struct DrawInfo {
  RadarDraw *draw;
  int drawing_method;
  bool color_option;
};

typedef UINT8 TrailRevolutionsAge;
#define SECONDS_TO_REVOLUTIONS(x) ((x)*2 / 5)
#define TRAIL_MAX_REVOLUTIONS SECONDS_TO_REVOLUTIONS(600) + 1
enum { TRAIL_15SEC, TRAIL_30SEC, TRAIL_1MIN, TRAIL_3MIN, TRAIL_5MIN, TRAIL_10MIN, TRAIL_CONTINUOUS, TRAIL_ARRAY_SIZE };

class RadarInfo {
 public:
  wxString m_name;  // Either "Radar", "Radar A", "Radar B".
  br24radar_pi *m_pi;
  int m_radar;  // Which radar this is (0..., max 2 for now)
#define COURSE_SAMPLES (16)
  double m_course;  // m_course is the moving everage of m_hdt used for course_up
  double m_course_log[COURSE_SAMPLES];
  int m_course_index;
  RadarArpa *m_arpa;
  wxCriticalSection m_exclusive;  // protects the following two

  /* User radar settings */

  radar_control_item m_state;       // RadarState (observed)
  radar_control_item m_boot_state;  // Can contain RADAR_TRANSMIT until radar is seen at boot

  radar_control_item m_orientation;  // See below for allowed values.
// Orientation HEAD_UP is available if there is no heading or dev mode is switched on
// Other orientations are available if there is a heading
#define ORIENTATION_HEAD_UP \
  (0)  // Unstabilized heading (as if without compass)                                     // Available if no compass or in dev mode
#define ORIENTATION_STABILIZED_UP (1)  // Stabilized heading (averaged over a few seconds)
#define ORIENTATION_NORTH_UP (2)       // North up
#define ORIENTATION_COG_UP (3)         // Averaged GPS COG up (same way as OpenCPN)
#define ORIENTATION_NUMBER (4)

  int m_min_contour_length;  // minimum contour length of an ARPA or MARPA target

  radar_control_item m_overlay;
  radar_range_control_item m_range;  // value in meters
  radar_control_item m_gain;
  radar_control_item m_interference_rejection;
  radar_control_item m_target_separation;
  radar_control_item m_noise_rejection;
  radar_control_item m_target_boost;
  radar_control_item m_target_expansion;
  radar_control_item m_sea;
  radar_control_item m_rain;
  radar_control_item m_scan_speed;
  radar_control_item m_bearing_alignment;
  radar_control_item m_antenna_height;
  radar_control_item m_local_interference_rejection;
  radar_control_item m_side_lobe_suppression;
  radar_control_item m_target_trails;
  radar_control_item m_trails_motion;
#define TARGET_MOTION_OFF (0)
#define TARGET_MOTION_RELATIVE (1)
#define TARGET_MOTION_TRUE (2)

  /* Per radar objects */

  br24Transmit *m_transmit;
  br24Receive *m_receive;
  br24ControlsDialog *m_control_dialog;
  RadarPanel *m_radar_panel;
  RadarCanvas *m_radar_canvas;

  /* Abstractions of our own. Some filled by br24Receive. */

  double m_viewpoint_rotation;

  time_t m_radar_timeout;      // When we consider the radar no longer valid
  time_t m_data_timeout;       // When we consider the data to be obsolete (radar no longer sending data)
  time_t m_stayalive_timeout;  // When we will send another stayalive ping
#define STAYALIVE_TIMEOUT (5)  // Send data every 5 seconds to ping radar
#define DATA_TIMEOUT (5)

  RadarType m_radar_type;
  bool m_auto_range_mode;

  int m_refresh_millis;

  GuardZone *m_guard_zone[GUARD_ZONES];
  double m_ebl[ORIENTATION_NUMBER][BEARING_LINES];
  double m_vrm[BEARING_LINES];
  receive_statistics m_statistics;

  struct line_history {
    UINT8 line[RETURNS_PER_LINE];
    wxLongLong time;
    double lat;
    double lon;
  };

  line_history m_history[LINES_PER_ROTATION];

#define MARGIN (100)
#define TRAILS_SIZE (RETURNS_PER_LINE * 2 + MARGIN * 2)
  //#define TRAILS_MIDDLE (TRAILS_SIZE / 2)

  struct IntVector {
    int lat;
    int lon;
  };
  struct TrailBuffer {
    TrailRevolutionsAge true_trails[TRAILS_SIZE][TRAILS_SIZE];
    TrailRevolutionsAge relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];
    union {
      TrailRevolutionsAge copy_of_true_trails[TRAILS_SIZE][TRAILS_SIZE];
      TrailRevolutionsAge copy_of_relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];
    };
    double lat;
    double lon;
    double dif_lat;  // Fraction of a pixel expressed in lat/lon for True Motion Target Trails
    double dif_lon;
    IntVector offset;
  };
  int m_old_range;
  int m_dir_lat;
  int m_dir_lon;
  TrailBuffer m_trails;

  /* Methods */

  RadarInfo(br24radar_pi *pi, int radar);
  ~RadarInfo();

  bool Init(wxString name, int verbose);
  void StartReceive();
  void SetName(wxString name);
  void AdjustRange(int adjustment);
  void SetAutoRangeMeters(int meters);
  bool SetControlValue(ControlType controlType, int value, int autoValue);
  void ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters, wxLongLong time,
                         double lat, double lon);
  void RefreshDisplay();
  void UpdateTrailPosition();
  void RenderGuardZone();
  void ResetRadarImage();
  void ShiftImageLonToCenter();
  void ShiftImageLatToCenter();
  void RenderRadarImage(wxPoint center, double scale, double rotation, bool overlay);
  void ShowRadarWindow(bool show);
  void ShowControlDialog(bool show, bool reparent);
  void Shutdown();
  // void DeleteReceive();
  void UpdateTransmitState();
  void RequestRadarState(RadarState state);
  int GetDrawTime() {
    wxCriticalSectionLocker lock(m_exclusive);
    return IsPaneShown() ? m_draw_time_ms : 0;
  };
  bool IsPaneShown();

  void UpdateControlState(bool all);
  void ComputeColourMap();
  void ComputeTargetTrails();
  wxString &GetRangeText();
  const char *GetDisplayRangeStr(size_t idx);
  int GetDisplayRange() { return m_range.GetValue(); };
  void SetNetworkCardAddress(struct sockaddr_in *address);
  void SetMouseLatLon(double lat, double lon);
  void SetMouseVrmEbl(double vrm, double ebl);
  void SetBearing(int bearing);
  void ClearTrails();
  void ZoomTrails(float zoom_factor);
  void SampleCourse(int angle);
  int GetOrientation();

  wxString GetCanvasTextTopLeft();
  wxString GetCanvasTextBottomLeft();
  wxString GetCanvasTextCenter();

  double m_mouse_lat, m_mouse_lon;
  double m_mouse_ebl[ORIENTATION_NUMBER];
  double m_mouse_vrm;
  int m_range_meters;  // what radar told us is the range in the last received spoke

  // Speedup lookup tables of color to r,g,b, set dependent on m_settings.display_option.
  wxColour m_colour_map_rgb[BLOB_COLOURS];
  BlobColour m_colour_map[UINT8_MAX + 1];

 private:
  void ResetSpokes();
  void RenderRadarImage(DrawInfo *di);
  wxString FormatDistance(double distance);
  wxString FormatAngle(double angle);

  int m_previous_auto_range_meters;
  int m_auto_range_meters;

  //  wxCriticalSection m_exclusive;  // protects the following two
  DrawInfo m_draw_panel;    // Draw onto our own panel
  DrawInfo m_draw_overlay;  // Abstract painting method

  int m_verbose;
  int m_draw_time_ms;  // Number of millis spent drawing

  wxString m_range_text;

  BlobColour m_trail_colour[TRAIL_MAX_REVOLUTIONS + 1];

  int m_previous_orientation;
};

PLUGIN_END_NAMESPACE

#endif
