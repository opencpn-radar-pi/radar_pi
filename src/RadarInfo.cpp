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

static const RadarRange g_ranges_metric[] = {
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

static const RadarRange g_ranges_nautic[] = {{50, 98, "50 m", 0, 0, 0},
                                             {75, 146, "75 m", 0, 0, 0},
                                             {100, 195, "100 m", "25", "50", "75"},
                                             {1852 / 8, 451, "1/8 NM", 0, "1/16", 0},
                                             {1852 / 4, 673, "1/4 NM", "1/32", "1/8", 0},
                                             {1852 / 2, 1389, "1/2 NM", "1/8", "1/4", "3/8"},
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
                                             {1852 * 24, 72704, "24 NM", "6", "12", "18"},
                                             {1852 * 36, 72704, "36 NM", "9", "18", "27"}};

static const int METRIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_metric);
static const int NAUTIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_nautic);

static const int g_range_maxValue[2] = {NAUTIC_RANGE_COUNT - 1, METRIC_RANGE_COUNT - 1};

extern size_t convertMetersToRadarAllowedValue(int *range_meters, int units, RadarType radarType) {
  const RadarRange *ranges;
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
  m_range_meters = 0;
  m_current_range = 0;
  m_auto_range_meters = 0;
  m_previous_auto_range_meters = 1;
  m_stayalive_timeout = 0;

  memset(&statistics, sizeof(statistics), 0);

  m_mouse_lat = 0.0;
  m_mouse_lon = 0.0;
  m_mouse_vrm = 0.0;
  m_mouse_ebl = 0.0;
  for (int b = 0; b < BEARING_LINES; b++) {
    m_ebl[b] = 0.0;
    m_vrm[b] = 0.0;
  }

  transmit = new br24Transmit(pi, name, radar);
  receive = 0;
  m_draw_panel.draw = 0;
  m_draw_overlay.draw = 0;
  radar_panel = 0;
  radar_canvas = 0;
  control_dialog = 0;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    guard_zone[z] = new GuardZone(pi);
  }

  ComputeTargetTrails();

  m_timer = new wxTimer(this, TIMER_ID);
  m_overlay_refreshes_queued = 0;
  m_refreshes_queued = 0;
  m_refresh_millis = 50;
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
  if (!radar_panel || !radar_panel->Create()) {
    wxLogError(wxT("BR24radar_pi %s: Unable to create RadarPanel"), name.c_str());
    return false;
  }

  return true;
}

void RadarInfo::SetNetworkCardAddress(struct sockaddr_in *address) {
  if (!transmit->Init(address)) {
    wxLogError(wxT("BR24radar_pi %s: Unable to create transmit socket"), name.c_str());
  }
}

void RadarInfo::SetName(wxString name) {
  if (name != this->name) {
    LOG_DIALOG(wxT("BR24radar_pi: Changing name of radar #%d from '%s' to '%s'"), radar, this->name.c_str(), name.c_str());
    this->name = name;
    radar_panel->SetCaption(name);
    if (control_dialog) {
      control_dialog->SetTitle(name);
    }
  }
}

void RadarInfo::StartReceive() {
  if (!receive) {
    LOG_RECEIVE(wxT("BR24radar_pi: %s starting receive thread"), name.c_str());
    receive = new br24Receive(m_pi, this);
    receive->Run();
  }
}

void RadarInfo::ResetSpokes() {
  UINT8 zap[RETURNS_PER_LINE];

  LOG_VERBOSE(wxT("BR24radar_pi: reset spokes, history and trails"));

  memset(zap, 0, sizeof(zap));
  memset(history, 0, sizeof(history));
  memset(trails, 0, sizeof(trails));

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
 */
void RadarInfo::ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters) {
  bool calc_history = false;

  wxMutexLocker lock(m_mutex);

  if (m_range_meters != range_meters) {
    ResetSpokes();
    LOG_VERBOSE(wxT("BR24radar_pi: %s detected spoke range change from %d to %d meters"), name.c_str(), m_range_meters,
                range_meters);
    m_range_meters = range_meters;

#if 0
    // m_display_meters is now range.value and is set by radar report...
    m_display_meters = range_meters;
    m_range_index = convertRadarMetersToIndex(&m_display_meters);
    range.Update(range_meters);
    LOG_VERBOSE(wxT("BR24radar_pi: %s detected range change to range #%d, display_meters=%d"), name.c_str(), m_range_index,
                m_display_meters);
#endif
  } else if (orientation.mod) {
    ResetSpokes();
    LOG_VERBOSE(wxT("BR24radar_pi: %s HeadUp/NorthUp change"));
  }
  int north_up = orientation.GetButton() == ORIENTATION_NORTH_UP;

  if (!calc_history) {
    for (size_t z = 0; z < GUARD_ZONES; z++) {
      if (guard_zone[z]->type != GZ_OFF && guard_zone[z]->multi_sweep_filter) {
        calc_history = true;
      }
    }
  }

  if (calc_history) {
    UINT8 *hist_data = history[angle];
    for (size_t radius = 0; radius < len; radius++) {
      hist_data[radius] = hist_data[radius] << 1;  // shift left history byte 1 bit
      if (m_pi->m_color_map[data[radius]] >= BLOB_BLUE) {
        hist_data[radius] = hist_data[radius] | 1;  // and add 1 if above threshold
      }
    }
    for (size_t z = 0; z < GUARD_ZONES; z++) {
      if (guard_zone[z]->type != GZ_OFF) {
        guard_zone[z]->ProcessSpoke(angle, data, hist_data, len, range_meters);
      }
    }
  }

  if (m_draw_overlay.draw) {
    m_draw_overlay.draw->ProcessRadarSpoke(bearing, data, len);
  }

  if (target_trails.value != 0) {
    for (size_t radius = 0; radius < len; radius++) {
      BlobColor pixel = m_pi->m_color_map[data[radius]];
      UINT8 *trail = &trails[angle][radius];

      if (pixel >= BLOB_BLUE) {
        *trail = 1;
      } else {
        if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
          (*trail)++;
        }
        data[radius] = m_trail_color[*trail];
      }
    }
  }

  if (m_draw_panel.draw) {
    m_draw_panel.draw->ProcessRadarSpoke(north_up ? bearing : angle, data, len);
  }
}

void RadarInfo::RefreshDisplay(wxTimerEvent &event) {
  if (m_overlay_refreshes_queued > 0) {
    // don't do additional refresh when too busy
    LOG_DIALOG(wxT("BR24radar_pi: %s busy encountered, overlay_refreshes_queued=%d"), name.c_str(), m_overlay_refreshes_queued);
  } else if (m_pi->IsOverlayOnScreen(radar)) {
    m_overlay_refreshes_queued++;
    GetOCPNCanvasWindow()->Refresh(false);
  }

  if (m_refreshes_queued > 0) {
    // don't do additional refresh and reset the refresh conter
    // this will also balance performance, if too busy skip refresh
    LOG_DIALOG(wxT("BR24radar_pi: %s busy encountered, refreshes_queued=%d"), name.c_str(), m_refreshes_queued);
  } else if (IsPaneShown()) {
    m_refreshes_queued++;
    radar_panel->Refresh(false);
  }

  // Calculate refresh speed
  if (m_pi->m_settings.refreshrate) {
    int millis = 1000 / (1 + ((m_pi->m_settings.refreshrate) - 1) * 5);

    if (millis != m_refresh_millis) {
      m_refresh_millis = millis;
      LOG_VERBOSE(wxT("BR24radar_pi: %s changed timer interval to %d milliseconds"), name.c_str(), m_refresh_millis);
      m_timer->Start(m_refresh_millis);
    }
  }
}

void RadarInfo::RenderGuardZone() {
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
          DrawOutlineArc(guard_zone[z]->outer_range, guard_zone[z]->inner_range, start_bearing, end_bearing, true);
          break;
        case 2:
          glColor4ub(red, green, blue, alpha);
          DrawOutlineArc(guard_zone[z]->outer_range, guard_zone[z]->inner_range, start_bearing, end_bearing, false);
        // fall thru
        default:
          glColor4ub(red, green, blue, alpha);
          DrawFilledArc(guard_zone[z]->outer_range, guard_zone[z]->inner_range, start_bearing, end_bearing);
      }
    }

    red = 0;
    green = 0;
    blue = 200;
  }
}

void RadarInfo::AdjustRange(int adjustment) {
  const RadarRange *min, *max;

  // Note that we don't actually use m_settings.units here, so that if we are metric and
  // the plotter in NM, and it chose the last range, we start using nautic miles as well.

  if (m_current_range) {
    if (m_current_range > g_ranges_nautic & m_current_range < g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic)) {
      min = g_ranges_nautic;
      max = g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic) - 1;
    } else if (m_current_range > g_ranges_metric & m_current_range < g_ranges_metric + ARRAY_SIZE(g_ranges_metric)) {
      min = g_ranges_metric;
      max = g_ranges_metric + ARRAY_SIZE(g_ranges_metric) - 1;
    }

    if (radar_type != RT_4G) {
      max--;  // only 4G has longest ranges
    }

    if (adjustment < 0 && m_current_range > min) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), m_current_range[0].meters,
                  m_current_range[0].actual_meters, m_current_range[-1].meters, m_current_range[-1].actual_meters);
      transmit->SetRange(m_current_range[-1].meters);
    } else if (adjustment > 0 && m_current_range < max) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), m_current_range[0].meters,
                  m_current_range[0].actual_meters, m_current_range[+1].meters, m_current_range[+1].actual_meters);
      transmit->SetRange(m_current_range[+1].meters);
    }
  }
}

void RadarInfo::SetAutoRangeMeters(int meters) {
  if (state.value == RADAR_TRANSMIT && auto_range_mode) {
    m_auto_range_meters = meters;
    // Don't adjust auto range meters continuously when it is oscillating a little bit (< 5%)
    int test = 100 * m_previous_auto_range_meters / m_auto_range_meters;
    if (test < 95 || test > 105) {  //   range change required
      // Compute a 'standard' distance. This will be slightly smaller.
      convertMetersToRadarAllowedValue(&meters, m_pi->m_settings.range_units, radar_type);
      if (meters != m_range_meters) {
        if (m_pi->m_settings.verbose) {
          LOG_VERBOSE(wxT("BR24radar_pi: Automatic range changed from %d to %d meters"), m_previous_auto_range_meters,
                      m_auto_range_meters);
        }
        transmit->SetRange(meters);
        m_previous_auto_range_meters = m_auto_range_meters;
      }
    }
  }
}

bool RadarInfo::SetControlValue(ControlType controlType, int value) { return transmit->SetControlValue(controlType, value); }

void RadarInfo::ShowRadarWindow(bool show) { radar_panel->ShowFrame(show); }

bool RadarInfo::IsPaneShown() { return radar_panel->IsPaneShown(); }

void RadarInfo::UpdateControlState(bool all) {
  wxMutexLocker lock(m_mutex);

  overlay.Update(m_pi->m_settings.chart_overlay == radar);

#ifdef OPENCPN_NO_LONGER_MIXES_GL_CONTEXT
  //
  // Once OpenCPN doesn't mess up with OpenGL context anymore we can do this
  //
  if (overlay.value == 0 && m_draw_overlay.draw) {
    LOG_DIALOG(wxT("BR24radar_pi: Removing draw method as radar overlay is not shown"));
    delete m_draw_overlay.draw;
    m_draw_overlay.draw = 0;
  }
  if (!IsShown() && m_draw_panel.draw) {
    LOG_DIALOG(wxT("BR24radar_pi: Removing draw method as radar window is not shown"));
    delete m_draw_panel.draw;
    m_draw_panel.draw = 0;
  }
#endif

  if (control_dialog) {
    control_dialog->UpdateControlValues(all);
    control_dialog->UpdateDialogShown();
  }

/* Don't do this, it interferes with a 2nd device */
#if 0
  if (wantedState != state.value && state.value != RADAR_OFF) {
    FlipRadarState();
  }
#endif

  if (IsPaneShown()) {
    radar_panel->Refresh(false);
  }
}

void RadarInfo::RenderRadarImage(DrawInfo *di) {
  wxMutexLocker lock(m_mutex);
  int drawing_method = m_pi->m_settings.drawing_method;
  bool colorOption = m_pi->m_settings.display_option > 0;

  if (state.value != RADAR_TRANSMIT) {
    if (m_range_meters) {
      ResetSpokes();
      m_range_meters = 0;
    }
    return;
  }

  // Determine if a new draw method is required
  if (!di->draw || (drawing_method != di->drawing_method) || (colorOption != di->color_option)) {
    RadarDraw *newDraw = RadarDraw::make_Draw(m_pi, drawing_method);
    if (!newDraw) {
      wxLogError(wxT("BR24radar_pi: out of memory"));
      return;
    } else if (newDraw->Init(colorOption)) {
      wxArrayString methods;
      RadarDraw::GetDrawingMethods(methods);
      if (di == &m_draw_overlay) {
        LOG_VERBOSE(wxT("BR24radar_pi: %s new drawing method %s for overlay"), name.c_str(), methods[drawing_method].c_str());
      } else {
        LOG_VERBOSE(wxT("BR24radar_pi: %s new drawing method %s for panel"), name.c_str(), methods[drawing_method].c_str());
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

  di->draw->DrawRadarImage();
}

void RadarInfo::RenderRadarImage(wxPoint center, double scale, double rotate, bool overlay) {
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);  // Save state
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (overlay) {
    if (m_pi->m_settings.guard_zone_on_overlay) {
      glPushMatrix();
      glTranslated(center.x, center.y, 0);
      glRotated(rotate + m_pi->m_hdt, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);

      LOG_DIALOG(wxT("BR24radar_pi: %s render guard zone on overlay"), name.c_str());

      RenderGuardZone();
      glPopMatrix();
    }
    double radar_pixels_per_meter = ((double)RETURNS_PER_LINE) / m_range_meters;
    scale = scale / radar_pixels_per_meter;
    glPushMatrix();
    glTranslated(center.x, center.y, 0);
    if (rotate != 0.0) {
      glRotated(rotate, 0.0, 0.0, 1.0);
    }
    glScaled(scale, scale, 1.);

    RenderRadarImage(&m_draw_overlay);
    if (m_overlay_refreshes_queued > 0) {
      m_overlay_refreshes_queued--;
    }
  } else {
    glPushMatrix();
    scale = 1.0 / range.value;
    glScaled(scale, scale, 1.);
    if (orientation.value) {
      glRotated(m_pi->m_hdt, 0.0, 0.0, 1.0);
    }
    RenderGuardZone();
    glPopMatrix();

    glPushMatrix();
    double overscan = (double)m_range_meters / (double)range.value;
    scale = overscan / RETURNS_PER_LINE;
    glScaled(scale, scale, 1.);
    LOG_DIALOG(wxT("BR24radar_pi: %s render overscan=%g range=%d"), name.c_str(), overscan, range.value);
    RenderRadarImage(&m_draw_panel);
    if (m_refreshes_queued > 0) {
      m_refreshes_queued--;
    }
  }

  glPopMatrix();
  glPopAttrib();
}

void RadarInfo::FlipRadarState() {
  if (m_pi->IsRadarOnScreen(radar)) {
    if (state.button == RADAR_STANDBY) {
      transmit->RadarTxOn();
      state.Update(RADAR_TRANSMIT);
      wantedState = RADAR_TRANSMIT;
    } else {
      transmit->RadarTxOff();
      m_data_timeout = 0;
      state.Update(RADAR_STANDBY);
      wantedState = RADAR_STANDBY;
    }
  }
}

wxString RadarInfo::GetCanvasTextTopLeft() {
  wxString s;

  if (orientation.value == ORIENTATION_NORTH_UP && m_pi->m_heading_source != HEADING_NONE) {
    s << _("North Up");
  } else {
    s << _("Head Up");
  }
  if (m_pi->m_settings.emulator_on) {
    s << wxT("\n") << _("Emulator");
  }
  if (m_range_meters) {
    s << wxT("\n") << GetRangeText();
  }

  return s;
}

wxString RadarInfo::FormatDistance(double distance) {
  wxString s;

  if (m_pi->m_settings.range_units > 0) {
    distance *= 1.852;

    if (distance < 1.000) {
      int meters = distance * 1000.0;
      s << meters;
      s << "m";
    } else {
      s << wxString::Format(wxT("%.2fkm"), distance);
    }
  } else {
    if (distance < 0.25 * 1.852) {
      int meters = distance * 1852.0;
      s << meters;
      s << "m";
    } else {
      s << wxString::Format(wxT("%.2fnm"), distance);
    }
  }

  return s;
}

wxString RadarInfo::GetCanvasTextBottomLeft() {
  wxString s = m_pi->GetGuardZoneText(this, false);

  if (state.value == RADAR_TRANSMIT) {
    double distance = 0.0, bearing;

    // Add VRM/EBLs

    for (int b = 0; b < BEARING_LINES; b++) {
      if (m_vrm[b] != 0.0) {
        if (s.length()) {
          s << wxT("\n");
        }
        s << wxString::Format(wxT("VRM%d=%s EBL%d=%.1f\u00B0T"), b + 1, FormatDistance(m_vrm[b]), b + 1, m_ebl[b]);
      }
    }

    // Add in mouse cursor location

    if (m_mouse_vrm != 0.0) {
      distance = m_mouse_vrm;
      bearing = m_mouse_ebl;
    } else if ((m_mouse_lat != 0.0 || m_mouse_lon != 0.0) && m_pi->m_bpos_set) {
      // Can't compute this upfront, ownship may move...
      distance = local_distance(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
      bearing = local_bearing(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
    }

    if (distance != 0.0) {
      if (s.length()) {
        s << wxT("\n");
      }
      s << FormatDistance(distance) << wxString::Format(wxT(", %.1f\u00B0T"), bearing);
    }
  }
  return s;
}

wxString RadarInfo::GetCanvasTextCenter() {
  wxString s;

  if (state.value == RADAR_OFF) {
    s << _("No radar");
  } else if (state.value == RADAR_STANDBY) {
    s << _("Radar is in Standby");
    switch (radar_type) {
      case RT_BR24:
        s << wxT("\nBR24");
        break;
      case RT_4G:
        s << wxT("\n4G");
        break;
      case RT_UNKNOWN:
      default:
        break;
    }
  } else if (!m_draw_panel.draw) {
    s << _("No valid drawing method");
  }

  return s;
}

wxString &RadarInfo::GetRangeText() {
  int meters = range.value;

  if (meters == 0) {
    m_current_range = 0;
    m_range_text = wxT("");
    return m_range_text;
  }

  const RadarRange *r = 0;
  int g;

  for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
    if (g_ranges_nautic[g].meters == meters) {
      r = &g_ranges_nautic[g];
      break;
    }
  }
  if (!r) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
      if (g_ranges_metric[g].meters == meters) {
        r = &g_ranges_metric[g];
        break;
      }
    }
  }

  bool auto_range = auto_range_mode && (overlay.button > 0);

  m_range_text = wxT("");
  if (auto_range) {
    m_range_text = _("Auto");
    m_range_text << wxT(" (");
  }
  if (r) {
    m_range_text << wxString::FromUTF8(r->name);
  } else {
    m_range_text << wxString::Format(wxT("/%d m/"), meters);
  }
  m_current_range = r;

  if (auto_range) {
    m_range_text << wxT(")");
  }
  LOG_DIALOG(wxT("br24radar_pi: range label '%s' for spokerange=%d range=%d auto=%d"), m_range_text.c_str(), m_range_meters, meters,
             auto_range_mode);
  return m_range_text;
}

const char *RadarInfo::GetDisplayRangeStr(size_t idx) {
  if (m_current_range) {
    return (&m_current_range->name)[(idx + 1) % 4];
  }

  return 0;
}

void RadarInfo::SetMouseLatLon(double lat, double lon) {
  m_mouse_lat = lat;
  m_mouse_lon = lon;
  m_mouse_ebl = 0.0;
  m_mouse_vrm = 0.0;
  LOG_DIALOG(wxT("BR24radar_pi: SetMouseLatLon(%f, %f)"), lat, lon);
}

void RadarInfo::SetMouseVrmEbl(double vrm, double ebl) {
  m_mouse_vrm = vrm;
  m_mouse_ebl = ebl;
  m_mouse_lat = 0.0;
  m_mouse_lon = 0.0;
  LOG_DIALOG(wxT("BR24radar_pi: SetMouseVrmEbl(%f, %f)"), vrm, ebl);
}

void RadarInfo::SetBearing(int bearing) {
  if (m_vrm[bearing] != 0.0) {
    m_vrm[bearing] = 0.0;
    m_ebl[bearing] = 0.0;
  } else if (m_mouse_vrm != 0.0) {
    m_vrm[bearing] = m_mouse_vrm;
    m_ebl[bearing] = m_mouse_ebl;
  } else if (m_mouse_lat != 0.0 || m_mouse_lon != 0.0) {
    m_vrm[bearing] = local_distance(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
    m_ebl[bearing] = local_bearing(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
  }
}

void RadarInfo::ClearTrails() {
  memset(trails, 0, sizeof(trails));
}

void RadarInfo::ComputeTargetTrails() {
  static TrailRevolutionsAge maxRevs[6] = {SECONDS_TO_REVOLUTIONS(0),  SECONDS_TO_REVOLUTIONS(15),  SECONDS_TO_REVOLUTIONS(30),
                                           SECONDS_TO_REVOLUTIONS(60), SECONDS_TO_REVOLUTIONS(180), SECONDS_TO_REVOLUTIONS(1800)};

  TrailRevolutionsAge maxRev = maxRevs[target_trails.value];
  TrailRevolutionsAge revolution;
  double colorsPerRevolution = ((int) BLOB_HISTORY_9 - BLOB_HISTORY_0 + 1 )/ (double)maxRev;
  double color = 0.;

  LOG_VERBOSE(wxT("BR24radar_pi: Target trail value %d = %d revolutions"), target_trails.value, maxRev);

  // Disperse the ten BLOB_HISTORY values over 0..maxrev
  // with maxrev
  for (revolution = 0; revolution <= TRAIL_MAX_REVOLUTIONS; revolution++) {
    if (revolution >= 1 && revolution <= maxRev) {
      m_trail_color[revolution] = (BlobColor)(BLOB_HISTORY_0 + (int)color);
      color += colorsPerRevolution;
    } else {
      m_trail_color[revolution] = BLOB_NONE;
    }
  }

  ClearTrails();
}

PLUGIN_END_NAMESPACE
