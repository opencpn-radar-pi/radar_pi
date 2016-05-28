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

#include "RadarInfo.h"
#include "drawutil.h"
#include "br24Receive.h"
#include "br24Transmit.h"
#include "RadarDraw.h"
#include "RadarCanvas.h"
#include "RadarPanel.h"

PLUGIN_BEGIN_NAMESPACE

enum { TIMER_ID = 1 };

BEGIN_EVENT_TABLE(RadarInfo, wxEvtHandler)
EVT_TIMER(TIMER_ID, RadarInfo::RefreshDisplay)
END_EVENT_TABLE()

void radar_control_item::Update(int v) {
  wxMutexLocker lock(m_mutex);

  if (v != button) {
    mod = true;
    button = v;
  }
  value = v;
};

int radar_control_item::GetButton() {
  wxMutexLocker lock(m_mutex);

  mod = false;
  return button;
}

struct RadarRanges {
  int meters;
  int actual_meters;
  const char *name;
  const char *range1;
  const char *range2;
  const char *range3;
};

static const RadarRanges g_ranges_metric[] = {
    /* Nautical (mixed) first */
    {50, 98, "50 m", 0, 0, 0},
    {75, 146, "75 m", 0, 0, 0},
    {100, 195, "100 m", "25", "50", "75"},
    {250, 488, "250 m", 0, "125", 0},
    {500, 808, "500 m", "125", "250", "375"},
    {750, 1154, "750 m", 0, "375", 0},
    {1000, 1616, "1 km", "250", "500", "750"},
    {1500, 2308, "1.5 km", 0, "750", 0},
    {2000, 3366, "2 km", "500", "1000", "1500"},
    {3000, 4713, "3 km", 0, "1500", 0},
    {4000, 5655, "4 km", "1", "2", "3"},
    {6000, 9408, "6 km", "1.5", "3.0", "4.5"},
    {8000, 12096, "8 km", "2", "4", "6"},
    {12000, 18176, "12 km", "3", "6", "9"},
    {16000, 26240, "16 km", "4", "8", "12"},
    {24000, 36352, "24 km", "6", "12", "18"},
    {36000, 52480, "36 km", "9", "18", "27"},
    {48000, 72704, "48 km", "12", "24", "36"}};

static const RadarRanges g_ranges_nautic[] = {{50, 98, "50 m", 0, 0, 0},
                                              {75, 146, "75 m", 0, 0, 0},
                                              {100, 195, "100 m", "25", "50", "75"},
                                              {1852 / 8, 451, "1/8 NM", 0, "1/16", 0},
                                              {1852 / 4, 673, "1/4 NM", "1/32", "1/8", 0},
                                              {1852 / 2, 1346, "1/2 NM", "1/8", "1/4", "3/8"},
                                              {1852 * 3 / 4, 2020, "3/4 NM", 0, "3/8", 0},
                                              {1852 * 1, 2693, "1 NM", "1/4", "1/2", "3/4"},
                                              {1852 * 3 / 2, 4039, "1.5 NM", "3/8", "3/4", 0},
                                              {1852 * 2, 5655, "2 NM", "0.5", "1.0", "1.5"},
                                              {1852 * 3, 8079, "3 NM", "0.75", "1.5", "2.25"},
                                              {1852 * 4, 10752, "4 NM", "1", "2", "3"},
                                              {1852 * 6, 16128, "6 NM", "1.5", "3", "4.5"},
                                              {1852 * 8, 22208, "8 NM", "2", "4", "6"},
                                              {1852 * 12, 36352, "12 NM", "3", "6", "9"},
                                              {1852 * 16, 44416, "16 NM", "4", "8", "12"},
                                              {1852 * 24, 72704, "24 NM", "6", "12", "18"}};

static const int METRIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_metric);
static const int NAUTIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_nautic);

static const int g_range_maxValue[2] = {NAUTIC_RANGE_COUNT - 1, METRIC_RANGE_COUNT - 1};

extern const char *convertRadarToString(int range_meters, int units, int index) {
  const RadarRanges *ranges = units ? g_ranges_metric : g_ranges_nautic;

  size_t n;

  n = g_range_maxValue[units];
  for (; n > 0; n--) {
    if (ranges[n].actual_meters <= range_meters) {  // step down until past the right range value
      break;
    }
  }
  return (&ranges[n].name)[(index + 1) % 4];
}

size_t RadarInfo::convertRadarMetersToIndex(int *range_meters) {
  const RadarRanges *ranges;
  int units = m_pi->m_settings.range_units;
  int myrange = *range_meters;
  size_t n;

  n = g_range_maxValue[units];
  ranges = units ? g_ranges_metric : g_ranges_nautic;

  if (radar_type != RT_4G) {
    n--;  // only 4G has longest ranges
  }
  for (; n > 0; n--) {
    if (ranges[n].actual_meters <= myrange) {  // step down until past the right range value
      break;
    }
  }
  *range_meters = ranges[n].meters;

  return n;
}

extern size_t convertMetersToRadarAllowedValue(int *range_meters, int units, RadarType radarType) {
  const RadarRanges *ranges;
  int myrange = *range_meters;
  size_t n;

  n = g_range_maxValue[units];
  ranges = units ? g_ranges_metric : g_ranges_nautic;

  if (radarType != RT_4G) {
    n--;  // only 4G has longest ranges
  }
  for (; n > 0; n--) {
    if (ranges[n].meters <= myrange) {  // step down until past the right range value
      break;
    }
  }
  *range_meters = ranges[n].meters;

  return n;
}

RadarInfo::RadarInfo(br24radar_pi *pi, wxString name, int radar) {
  m_pi = pi;
  this->name = name;
  this->radar = radar;

  radar_type = RT_UNKNOWN;
  auto_range_mode = true;
  range_meters = 0;

  transmit = new br24Transmit(name, radar);
  receive = 0;
  m_draw_panel.draw = 0;
  m_draw_overlay.draw = 0;
  radar_panel = 0;
  control_dialog = 0;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    guard_zone[z] = new GuardZone(pi);
  }

  m_timer = new wxTimer(this, TIMER_ID);
  m_overlay_refreshes_queued = 0;
  m_refreshes_queued = 0;
  m_refresh_millis = 1000;
  m_timer->Start(m_refresh_millis);
}

RadarInfo::~RadarInfo() {
  m_timer->Stop();

  if (receive) {
    receive->Delete();
    delete receive;
  }
  if (transmit) {
    delete transmit;
  }
  if (radar_panel) {
    delete radar_panel;
  }
  if (m_draw_panel.draw) {
    delete m_draw_panel.draw;
    m_draw_panel.draw = 0;
  }
  if (m_draw_overlay.draw) {
    delete m_draw_overlay.draw;
    m_draw_overlay.draw = 0;
  }
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    delete guard_zone[z];
  }
}

bool RadarInfo::Init(int verbose) {
  m_verbose = verbose;

  radar_panel = new RadarPanel(m_pi, this, GetOCPNCanvasWindow());
  if (!radar_panel) {
    wxLogMessage(wxT("BR24radar_pi %s: Unable to create RadarPanel"), name.c_str());
    return false;
  }
  if (!radar_panel->Create()) {
    wxLogMessage(wxT("BR24radar_pi %s: Unable to create RadarCanvas"), name.c_str());
    return false;
  }

  return true;
}

void RadarInfo::SetNetworkCardAddress(struct sockaddr_in *address) {
  if (!transmit->Init(m_verbose, address)) {
    wxLogMessage(wxT("BR24radar_pi %s: Unable to create transmit socket"), name.c_str());
  }
}

void RadarInfo::SetName(wxString name) {
  if (name != this->name) {
    wxLogMessage(wxT("BR24radar_pi: Changing name of radar #%d from '%s' to '%s'"), radar, this->name.c_str(), name.c_str());
    this->name = name;
    radar_panel->SetCaption(name);
  }
}

void RadarInfo::StartReceive() {
  if (!receive) {
    wxLogMessage(wxT("BR24radar_pi: %s starting receive thread"), name.c_str());
    receive = new br24Receive(m_pi, this);
    receive->Run();
  }
}

void RadarInfo::ResetSpokes() {
  UINT8 zap[RETURNS_PER_LINE];

  memset(zap, 0, sizeof(zap));

  if (m_draw_panel.draw) {
    for (size_t r = 0; r < LINES_PER_ROTATION; r++) {
      m_draw_panel.draw->ProcessRadarSpoke(r, zap, sizeof(zap));
    }
  }
  if (m_draw_overlay.draw) {
    for (size_t r = 0; r < LINES_PER_ROTATION; r++) {
      m_draw_overlay.draw->ProcessRadarSpoke(r, zap, sizeof(zap));
    }
  }
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    // Zap them anyway just to be sure
    guard_zone[z]->ResetBogeys();
  }
}

/*
 * A spoke of data has been received by the receive thread and it calls this (in
 * the context of
 * the receive thread, so no UI actions can be performed here.)
 *
 * @param angle                 Bearing (relative to Boat)  at which the spoke
 * is seen.
 * @param bearing               Bearing (relative to North) at which the spoke
 * is seen.
 * @param data                  A line of len bytes, each byte represents
 * strength at that distance.
 * @param len                   Number of returns
 * @param range                 Range (in meters) of this data
 * @param nowMillis             Timestamp when this was received
 */
void RadarInfo::ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters,
                                  wxLongLong nowMillis) {
  UINT8 *hist_data = history[angle];
  bool calc_history = multi_sweep_filter;

  wxMutexLocker lock(m_mutex);

  if (this->range_meters != range_meters) {
    // Wipe ALL spokes
    ResetSpokes();
    this->range_meters = range_meters;
    this->range.Update(range_meters);
    if (m_pi->m_settings.verbose) {
      wxLogMessage(wxT("BR24radar_pi: %s detected range %d"), name.c_str(), range_meters);
    }
  } else if (rotation.mod) {
    ResetSpokes();
  }
  int north_up = rotation.GetButton();

  // spoke[angle].age = nowMillis;

  if (!calc_history) {
    for (size_t z = 0; z < GUARD_ZONES; z++) {
      if (guard_zone[z]->type != GZ_OFF && guard_zone[z]->multi_sweep_filter) {
        calc_history = true;
      }
    }
  }

  if (calc_history) {
    for (size_t radius = 0; radius < len; radius++) {
      hist_data[radius] = hist_data[radius] << 1;  // shift left history byte 1 bit
      if (m_pi->m_color_map[data[radius]] != BLOB_NONE) {
        hist_data[radius] = hist_data[radius] | 1;  // and add 1 if above threshold
      }
    }
  }

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (guard_zone[z]->type != GZ_OFF) {
      guard_zone[z]->ProcessSpoke(angle, data, hist_data, len, range_meters);
    }
  }

  if (multi_sweep_filter) {
    for (size_t radius = 0; radius < len; radius++) {
      if (data[radius] && !HISTORY_FILTER_ALLOW(hist_data[radius])) {
        data[radius] = 0;  // Zero out data here, so draw doesn't need to know
                           // about filtering
      }
    }
  }

  if (m_draw_panel.draw) {
    m_draw_panel.draw->ProcessRadarSpoke(north_up ? bearing : angle, data, len);
  }
  if (m_draw_overlay.draw) {
    m_draw_overlay.draw->ProcessRadarSpoke(bearing, data, len);
  }
}

void RadarInfo::RefreshDisplay(wxTimerEvent &event) {
  if (m_overlay_refreshes_queued > 0) {
    // don't do additional refresh when too busy
    if (m_verbose >= 1) {
      wxLogMessage(wxT("BR24radar_pi: %s busy encountered, overlay_refreshes_queued=%d"), name.c_str(), m_overlay_refreshes_queued);
    }
  } else if (m_pi->m_settings.chart_overlay == this->radar) {
    m_overlay_refreshes_queued++;
    GetOCPNCanvasWindow()->Refresh(false);
  }

  if (m_refreshes_queued > 0) {
    // don't do additional refresh and reset the refresh conter
    // this will also balance performance, if too busy skip refresh
    if (m_verbose >= 1) {
      wxLogMessage(wxT("BR24radar_pi: %s busy encountered, refreshes_queued=%d"), name.c_str(), m_refreshes_queued);
    }
  } else if (radar_panel->IsShown()) {
    m_refreshes_queued++;
    radar_panel->Refresh(false);
  }

  // Calculate refresh speed
  if (m_pi->m_settings.refreshrate) {
    int millis = 1000 / (1 + ((m_pi->m_settings.refreshrate) - 1) * 5);

    if (millis != m_refresh_millis) {
      m_refresh_millis = millis;
      wxLogMessage(wxT("BR24radar_pi: %s changed timer interval to %d milliseconds"), name.c_str(), m_refresh_millis);
      m_timer->Start(m_refresh_millis);
    }
  }
}

void RadarInfo::RenderGuardZone(wxPoint radar_center, double v_scale_ppm) {
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);  // Save state
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  int start_bearing = 0, end_bearing = 0;
  GLubyte red = 0, green = 200, blue = 0, alpha = 50;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (guard_zone[z]->type != GZ_OFF) {
      if (guard_zone[z]->type == GZ_CIRCLE) {
        start_bearing = 0;
        end_bearing = 359;
      } else {
        start_bearing = SCALE_RAW_TO_DEGREES2048(guard_zone[z]->start_bearing) - 90;
        end_bearing = SCALE_RAW_TO_DEGREES2048(guard_zone[z]->end_bearing) - 90;
      }
      switch (m_pi->m_settings.guard_zone_render_style) {
        case 1:
          glColor4ub((GLubyte)255, (GLubyte)0, (GLubyte)0, (GLubyte)255);
          DrawOutlineArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing,
                         end_bearing, true);
          break;
        case 2:
          glColor4ub(red, green, blue, alpha);
          DrawOutlineArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing,
                         end_bearing, false);
        // fall thru
        default:
          glColor4ub(red, green, blue, alpha);
          DrawFilledArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing,
                        end_bearing);
      }
    }

    red = 0;
    green = 0;
    blue = 200;
  }

  glPopAttrib();
}

void RadarInfo::SetRangeMeters(int meters) {
  if (state.value == RADAR_TRANSMIT) {
    convertMetersToRadarAllowedValue(&meters, m_pi->m_settings.range_units, radar_type);
    transmit->SetRange(meters);
  }
}

void RadarInfo::SetRangeIndex(int newValue) {
  if (state.value == RADAR_TRANSMIT) {
    // newValue is the index of the new range
    // sends the command for the new range to the radar
    // but depends on radar feedback to update the actual value
    auto_range_mode = false;
    int meters = GetRangeMeters(newValue);
    wxLogMessage(wxT("br24radar_pi: range change request meters=%d new=%d"), meters, newValue);
    transmit->SetRange(meters);
  }
}

bool RadarInfo::SetControlValue(ControlType controlType, int value) { return transmit->SetControlValue(controlType, value); }

void RadarInfo::ShowRadarWindow(bool show) { radar_panel->ShowFrame(show); }

void RadarInfo::UpdateControlState(bool all) {
  wxMutexLocker lock(m_mutex);

  overlay.Update(m_pi->m_settings.chart_overlay == radar);

#ifdef OPENCPN_NO_LONGER_MIXES_GL_CONTEXT
  //
  // Once OpenCPN doesn't mess up with OpenGL context anymore we can do this
  //
  if (overlay.value == 0 && m_draw_overlay.draw) {
    wxLogMessage(wxT("BR24radar_pi: Removing draw method as radar overlay is not shown"));
    delete m_draw_overlay.draw;
    m_draw_overlay.draw = 0;
  }
  if (!radar_panel->IsShown() && m_draw_panel.draw) {
    wxLogMessage(wxT("BR24radar_pi: Removing draw method as radar window is not shown"));
    delete m_draw_panel.draw;
    m_draw_panel.draw = 0;
  }
#endif

  if (control_dialog) {
    control_dialog->UpdateControlValues(all);
    control_dialog->UpdateDialogShown();
  }

  if (radar_panel->IsShown()) {
    radar_panel->Refresh(false);
  }
}

void RadarInfo::RenderRadarImage(wxPoint center, double scale, double rotation, DrawInfo *di) {
  wxMutexLocker lock(m_mutex);
  int drawing_method = m_pi->m_settings.drawing_method;
  bool colorOption = m_pi->m_settings.display_option > 0;

  if (state.value != RADAR_TRANSMIT) {
    if (range_meters) {
      ResetSpokes();
      range_meters = 0;
    }
    return;
  }

  // Determine if a new draw method is required
  if (!di->draw || (drawing_method != di->drawing_method) || (colorOption != di->color_option)) {
    RadarDraw *newDraw = RadarDraw::make_Draw(m_pi, drawing_method);
    if (!newDraw) {
      wxLogMessage(wxT("BR24radar_pi: out of memory"));
      return;
    } else if (newDraw->Init(colorOption)) {
      wxArrayString methods;
      RadarDraw::GetDrawingMethods(methods);
      if (di == &m_draw_overlay) {
        wxLogMessage(wxT("BR24radar_pi: %s new drawing method %s for overlay"), name.c_str(), methods[drawing_method].c_str());
      } else {
        wxLogMessage(wxT("BR24radar_pi: %s new drawing method %s for panel"), name.c_str(), methods[drawing_method].c_str());
      }
      if (di->draw) {
        delete di->draw;
      }
      di->draw = newDraw;
      di->drawing_method = drawing_method;
      di->color_option = colorOption;
    } else {
      m_pi->m_settings.drawing_method = 0;
      delete newDraw;
    }
    if (!di->draw) {
      return;
    }
  }

  di->draw->DrawRadarImage(center, scale, rotation);
}

void RadarInfo::RenderRadarImage(wxPoint center, double scale, double rotation, bool overlay) {
  if (overlay) {
    RenderRadarImage(center, scale, rotation, &m_draw_overlay);
    if (m_overlay_refreshes_queued > 0) {
      m_overlay_refreshes_queued--;
    }
  } else {
    RenderGuardZone(center, scale);
    RenderRadarImage(center, scale, rotation, &m_draw_panel);
    if (m_refreshes_queued > 0) {
      m_refreshes_queued--;
    }
  }
}

void RadarInfo::FlipRadarState() {
  if (state.button == RADAR_STANDBY) {
    transmit->RadarTxOn();
    state.Update(RADAR_TRANSMIT);
  } else {
    transmit->RadarTxOff();
    m_data_timeout = 0;
    state.Update(RADAR_STANDBY);
  }
}

wxString RadarInfo::GetCanvasTextTopLeft() {
  wxString s;
  int index;

  if (rotation.value > 0) {
    s << _("North Up");
  } else {
    s << _("Head Up");
  }
  if (m_pi->m_settings.emulator_on) {
    s << wxT(" (");
    s << _("Emulator");
    s << wxT(")");
  } else if (range_meters) {
    s << wxT("\n") << GetRangeText(range_meters, &index);
  }

  return s;
}

wxString RadarInfo::GetCanvasTextBottomLeft() { return m_pi->GetGuardZoneText(this, false); }

wxString RadarInfo::GetCanvasTextCenter() {
  wxString s;

  if (state.value == RADAR_OFF) {
    s << _("No radar");
  } else if (state.value == RADAR_STANDBY) {
    s << _("Standby");
    if (this->radar_type == RT_4G) {
      s << wxT(" 4G");
    }
  } else if (!m_draw_panel.draw) {
    s << _("No valid drawing method");
  }

  return s;
}

int RadarInfo::GetRangeMeters(int index) {
  // returns the range in meters for a particular range index
  int units = m_pi->m_settings.range_units;
  int maxValue = g_range_maxValue[units];
  static const int minValue = 0;
  if (index > maxValue) {
    index = maxValue;
  } else if (index < minValue) {
    index = minValue;
  }

  int meters = units ? g_ranges_metric[index].meters : g_ranges_nautic[index].meters;

  return meters;
}

wxString &RadarInfo::GetRangeText(int range_meters, int *index) {
  int meters = range_meters;
  int units = m_pi->m_settings.range_units;
  int value = convertRadarMetersToIndex(&meters);
  bool auto_range = auto_range_mode && (overlay.button > 0);

  int maxValue = g_range_maxValue[units];
  static const int minValue = 0;

  if (value < minValue) {
    value = minValue;
  } else if (value > maxValue) {
    value = maxValue;
  }

  m_range_text = wxT("");
  if (auto_range) {
    m_range_text = _("Auto");
    m_range_text << wxT(" (");
  }
  if (value < 0) {
    m_range_text << wxT("?");
  } else {
    m_range_text << wxString::FromUTF8(units ? g_ranges_metric[value].name : g_ranges_nautic[value].name);
  }
  if (auto_range) {
    m_range_text << wxT(")");
  }
  if (m_pi->m_settings.verbose > 3) {
    wxLogMessage(wxT("br24radar_pi: range label '%s' for meters=%d range=%d auto=%d unit=%d max=%d idx=%d"), m_range_text.c_str(),
                 range_meters, meters, auto_range_mode, units, maxValue, value);
  }
  *index = value;
  return m_range_text;
}

PLUGIN_END_NAMESPACE
