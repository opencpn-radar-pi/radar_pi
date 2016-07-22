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
  int value;
  int button;
  bool mod;

  void Update(int v) {
    wxCriticalSectionLocker lock(m_exclusive);

    if (v != button) {
      mod = true;
      button = v;
    }
    value = v;
  };

  int GetButton() {
    wxCriticalSectionLocker lock(m_exclusive);

    mod = false;
    return button;
  }

  radar_control_item() {
    value = 0;
    button = 0;
    mod = false;
  }

 protected:
  wxCriticalSection m_exclusive;
};

class radar_range_control_item : public radar_control_item {
 public:
  const RadarRange *range;

  void Update(int v);

  radar_range_control_item() {
    value = 0;
    button = 0;
    mod = false;
    range = 0;
  }
};

struct DrawInfo {
  RadarDraw *draw;
  int drawing_method;
  bool color_option;
};

typedef UINT8 TrailRevolutionsAge;
#define SECONDS_TO_REVOLUTIONS(x) ((x)*2 / 5)
#define TRAIL_MAX_REVOLUTIONS SECONDS_TO_REVOLUTIONS(180)
#define TRAIL_CONTINUOUS SECONDS_TO_REVOLUTIONS(180 + 18)

class RadarInfo : public wxEvtHandler {
 public:
  wxString m_name;  // Either "Radar", "Radar A", "Radar B".
  br24radar_pi *m_pi;
  int m_radar;  // Which radar this is (0..., max 2 for now)

  /* User radar settings */

  radar_control_item m_state;        // RadarState (observed)
  radar_control_item m_orientation;  // 0 = Heading Up, 1 = North Up
#define ORIENTATION_HEAD_UP (0)
#define ORIENTATION_NORTH_UP (1)
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
  int m_overlay_refreshes_queued;
  int m_refreshes_queued;
  int m_refresh_millis;
  RadarState m_wantedState;

  GuardZone *m_guard_zone[GUARD_ZONES];
  double m_ebl[BEARING_LINES];
  double m_vrm[BEARING_LINES];
  receive_statistics m_statistics;

  bool m_multi_sweep_filter;
  UINT8 m_history[LINES_PER_ROTATION][RETURNS_PER_LINE];
#define HISTORY_FILTER_ALLOW(x) (HasBitCount2[(x)&7])

  TrailRevolutionsAge m_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];

  /* Methods */

  RadarInfo(br24radar_pi *pi, int radar);
  ~RadarInfo();

  bool Init(wxString name, int verbose);
  void StartReceive();
  void SetName(wxString name);
  void AdjustRange(int adjustment);
  void SetAutoRangeMeters(int meters);
  bool SetControlValue(ControlType controlType, int value);
  void ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters);
  void RefreshDisplay(wxTimerEvent &event);
  void RenderGuardZone();
  void ResetRadarImage();
  void RenderRadarImage(wxPoint center, double scale, double rotation, bool overlay);
  void ShowRadarWindow(bool show);
  void ShowControlDialog(bool show, bool reparent);
  void ShowBogeys(wxString text);
  void HideBogeys();


  bool IsPaneShown();

  void UpdateControlState(bool all);
  void ComputeColorMap();
  void ComputeTargetTrails();
  void FlipRadarState();
  wxString &GetRangeText();
  const char *GetDisplayRangeStr(size_t idx);
  int GetDisplayRange() { return m_range.value; };
  void SetNetworkCardAddress(struct sockaddr_in *address);
  void SetMouseLatLon(double lat, double lon);
  void SetMouseVrmEbl(double vrm, double ebl);
  void SetBearing(int bearing);
  void ClearTrails();
  bool IsDisplayNorthUp() { return m_orientation.value == ORIENTATION_NORTH_UP && m_pi->m_heading_source != HEADING_NONE; }

  wxString GetCanvasTextTopLeft();
  wxString GetCanvasTextBottomLeft();
  wxString GetCanvasTextCenter();

  double m_mouse_lat, m_mouse_lon, m_mouse_vrm, m_mouse_ebl;

  // Speedup lookup tables of color to r,g,b, set dependent on m_settings.display_option.
  GLubyte m_color_map_red[BLOB_RED + 1];
  GLubyte m_color_map_green[BLOB_RED + 1];
  GLubyte m_color_map_blue[BLOB_RED + 1];
  BlobColor m_color_map[UINT8_MAX + 1];

 private:
  void ResetSpokes();
  void RenderRadarImage(DrawInfo *di);
  wxString FormatDistance(double distance);
  wxString FormatAngle(double angle);

  int m_range_meters;  // what radar told us is the range in the last received spoke

  int m_previous_auto_range_meters;
  int m_auto_range_meters;

  wxCriticalSection m_exclusive;  // protects the following two
  DrawInfo m_draw_panel;          // Draw onto our own panel
  DrawInfo m_draw_overlay;        // Abstract painting method

  int m_verbose;
  wxTimer *m_timer;

  wxString m_range_text;

  BlobColor m_trail_color[TRAIL_MAX_REVOLUTIONS + 1];

  DECLARE_EVENT_TABLE()
};

PLUGIN_END_NAMESPACE

#endif
