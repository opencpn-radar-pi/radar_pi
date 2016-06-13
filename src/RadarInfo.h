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

class radar_control_item {
 public:
  int value;
  int button;
  bool mod;

  void Update(int v);
  int GetButton();

  radar_control_item() {
    value = 0;
    button = 0;
    mod = false;
  }

 private:
  wxMutex m_mutex;
};

struct DrawInfo {
  RadarDraw *draw;
  int drawing_method;
  bool color_option;
};

class RadarInfo : public wxEvtHandler {
 public:
  wxString name;  // Either "Radar", "Radar A", "Radar B".
  int radar;      // Which radar this is (0..., max 2 for now)

  /* User radar settings */

  radar_control_item state;     // RadarState (observed)
  radar_control_item rotation;  // 0 = Heading Up, 1 = North Up
  radar_control_item overlay;
  radar_control_item range;
  radar_control_item gain;
  radar_control_item interference_rejection;
  radar_control_item target_separation;
  radar_control_item noise_rejection;
  radar_control_item target_boost;
  radar_control_item target_expansion;
  radar_control_item sea;
  radar_control_item rain;
  radar_control_item scan_speed;
  radar_control_item bearing_alignment;
  radar_control_item antenna_height;
  radar_control_item local_interference_rejection;
  radar_control_item side_lobe_suppression;

  /* Per radar objects */

  br24Transmit *transmit;
  br24Receive *receive;
  br24ControlsDialog *control_dialog;
  RadarPanel *radar_panel;
  RadarCanvas *radar_canvas;

  /* Abstractions of our own. Some filled by br24Receive. */

  double viewpoint_rotation;

  time_t m_radar_timeout;      // When we consider the radar no longer valid
  time_t m_data_timeout;       // When we consider the data to be obsolete (radar no longer sending data)
  time_t m_stayalive_timeout;  // When we will send another stayalive ping
#define STAYALIVE_TIMEOUT (5)  // Send data every 5 seconds to ping radar
#define DATA_TIMEOUT (5)

  RadarType radar_type;
  bool auto_range_mode;
  int m_overlay_refreshes_queued;
  int m_refreshes_queued;
  int m_refresh_millis;
  RadarState wantedState;

  GuardZone *guard_zone[GUARD_ZONES];
  receive_statistics statistics;

  bool multi_sweep_filter;
  UINT8 history[LINES_PER_ROTATION][RETURNS_PER_LINE];
#define HISTORY_FILTER_ALLOW(x) (HasBitCount2[(x)&7])

  /* Methods */

  RadarInfo(br24radar_pi *pi, wxString name, int radar);
  ~RadarInfo();

  bool Init(int verbose);
  void StartReceive();
  void SetName(wxString name);
  void SetRangeIndex(int newValue);
  void SetRangeMeters(int range);
  void SetAutoRangeMeters(int meters);
  bool SetControlValue(ControlType controlType, int value);
  void ResetSpokes();
  void ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters);
  void RefreshDisplay(wxTimerEvent &event);
  void RenderGuardZone();
  void RenderRadarImage(wxPoint center, double scale, double rotation, bool overlay);
  void ShowRadarWindow(bool show);
  bool IsPaneShown();

  void UpdateControlState(bool all);
  void FlipRadarState();
  wxString &GetRangeText(int range_meters, int *index);
  const char *GetDisplayRangeStr(size_t idx);
  int GetDisplayRange(){return m_display_meters;};
  void SetNetworkCardAddress(struct sockaddr_in *address);
  void SetMouseLatLon(double lat, double lon);

  wxString GetCanvasTextTopLeft();
  wxString GetCanvasTextBottomLeft();
  wxString GetCanvasTextCenter();

  double m_mouse_lat;
  double m_mouse_lon;

 private:
  void RenderRadarImage(DrawInfo *di);
  int GetRangeMeters(int index);
  size_t convertRadarMetersToIndex(int *range_meters);

  int m_range_index;     // index into range array
  int m_range_meters;    // what radar told us is the range
  int m_display_meters;  // what the display size is (slightly less than m_range_meters)

  int m_previous_auto_range_meters;
  int m_auto_range_meters;

  wxMutex m_mutex;          // protects the following two
  DrawInfo m_draw_panel;    // Draw onto our own panel
  DrawInfo m_draw_overlay;  // Abstract painting method

  br24radar_pi *m_pi;
  int m_verbose;
  wxTimer *m_timer;

  wxString m_range_text;

  DECLARE_EVENT_TABLE()
};

PLUGIN_END_NAMESPACE

#endif
