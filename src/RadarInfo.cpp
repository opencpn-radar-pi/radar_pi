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
#include "RadarCanvas.h"
#include "RadarDraw.h"
#include "RadarPanel.h"
#include "br24ControlsDialog.h"
#include "br24Receive.h"
#include "br24Transmit.h"
#include "drawutil.h"
#include "RadarMarpa.h"

PLUGIN_BEGIN_NAMESPACE

bool g_first_render = true;

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

static size_t convertMetersToRadarAllowedValue(int *range_meters, int units, RadarType radarType) {
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

static int convertSpokeMetersToRangeMeters(int value) {
  int g;

  for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
    if (g_ranges_nautic[g].actual_meters == value) {
      return g_ranges_nautic[g].meters;
    }
  }
  for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
    if (g_ranges_metric[g].actual_meters == value) {
      return g_ranges_metric[g].meters;
    }
  }
  return 0;
}

void radar_range_control_item::Update(int v) {
  radar_control_item::Update(v);

  size_t g;

  // Find out which nautical or metric range is the one represented by 'value'.
  // First we look up according to the desired setting (metric/nautical) and if
  // that doesn't work we look up nautical then metric.

  range = 0;
  if (m_settings->range_units == RANGE_NAUTICAL) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
      if (g_ranges_nautic[g].meters == value) {
        range = &g_ranges_nautic[g];
        break;
      }
    }
  } else {
    for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
      if (g_ranges_metric[g].meters == value) {
        range = &g_ranges_metric[g];
        break;
      }
    }
  }
  if (!range) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
      if (g_ranges_nautic[g].meters == value) {
        range = &g_ranges_nautic[g];
        break;
      }
    }
  }
  if (!range) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
      if (g_ranges_metric[g].meters == value) {
        range = &g_ranges_metric[g];
        break;
      }
    }
  }
}

RadarInfo::RadarInfo(br24radar_pi *pi, int radar) {
  m_pi = pi;
  m_radar = radar;
  m_marpa = 0;
  m_radar_type = RT_UNKNOWN;
  m_auto_range_mode = true;
  m_course_index = 0;
  m_old_range = 0;
  m_dir_lat = 0;
  m_dir_lon = 0;
  m_range_meters = 0;
  m_auto_range_meters = 0;
  m_previous_auto_range_meters = 0;
  m_stayalive_timeout = 0;
  m_radar_timeout = 0;
  m_data_timeout = 0;
  m_multi_sweep_filter = false;

  memset(&m_statistics, 0, sizeof(m_statistics));

  m_mouse_lat = 0.0;
  m_mouse_lon = 0.0;
  for (int i = 0; i < ORIENTATION_NUMBER; i++) {
    m_mouse_ebl[i] = nanl("");
    m_mouse_vrm[i] = 0.0;
    for (int b = 0; b < BEARING_LINES; b++) {
      m_ebl[i][b] = nanl("");
      m_vrm[b] = 0.0;
    }
  }
  m_transmit = 0;
  m_receive = 0;
  m_draw_panel.draw = 0;
  m_draw_overlay.draw = 0;
  m_radar_panel = 0;
  m_radar_canvas = 0;
  m_control_dialog = 0;
  m_state.value = 0;
  m_state.mod = false;
  m_state.button = 0;
  m_range.m_settings = &m_pi->m_settings;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    m_guard_zone[z] = new GuardZone(pi, radar, z);
  }

  ComputeTargetTrails();

  m_timer = new wxTimer(this, TIMER_ID);
  m_overlay_refreshes_queued = 0;
  m_refreshes_queued = 0;
  m_refresh_millis = 50;
}

void RadarInfo::DeleteDialogs() {
  if (m_control_dialog) {
    delete m_control_dialog;
    m_control_dialog = 0;
  }
  if (m_radar_panel) {
    delete m_radar_panel;
    m_radar_panel = 0;
  }
}

RadarInfo::~RadarInfo() {
  m_timer->Stop();
  if (m_receive) {
    LOG_VERBOSE(wxT("BR24radar_pi: %s receive thread request stop"), m_name.c_str());
    m_receive->Shutdown();
    LOG_VERBOSE(wxT("BR24radar_pi: %s receive thread stopped"), m_name.c_str());
    m_receive->Wait();
    LOG_VERBOSE(wxT("BR24radar_pi: %s receive thread delete"), m_name.c_str());
    delete m_receive;
    LOG_VERBOSE(wxT("BR24radar_pi: %s receive thread deleted"), m_name.c_str());
    m_receive = 0;
  }
  DeleteDialogs();
  if (m_draw_panel.draw) {
    delete m_draw_panel.draw;
    m_draw_panel.draw = 0;
  }
  if (m_draw_overlay.draw) {
    delete m_draw_overlay.draw;
    m_draw_overlay.draw = 0;
  }
  if (m_transmit) {
    delete m_transmit;
    m_transmit = 0;
  }
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    delete m_guard_zone[z];
    m_guard_zone[z] = 0;
  }
}

bool RadarInfo::Init(wxString name, int verbose) {
  m_verbose = verbose;

  m_name = name;

  ComputeColourMap();

  m_transmit = new br24Transmit(m_pi, name, m_radar);

  m_radar_panel = new RadarPanel(m_pi, this, GetOCPNCanvasWindow());
  if (!m_radar_panel || !m_radar_panel->Create()) {
    wxLogError(wxT("BR24radar_pi %s: Unable to create RadarPanel"), name.c_str());
    return false;
  }

  m_timer->Start(m_refresh_millis);
  return true;
}

void RadarInfo::ShowControlDialog(bool show, bool reparent) {
  if (show) {
    wxPoint panel_pos = wxDefaultPosition;
    bool manually_positioned = false;

    if (m_control_dialog && reparent) {
      panel_pos = m_control_dialog->m_panel_position;
      manually_positioned = m_control_dialog->m_manually_positioned;
      delete m_control_dialog;
      m_control_dialog = 0;
      LOG_VERBOSE(wxT("BR24radar_pi %s: Reparenting control dialog"), m_name.c_str());
    }
    if (!m_control_dialog) {
      m_control_dialog = new br24ControlsDialog;
      m_control_dialog->m_panel_position = panel_pos;
      m_control_dialog->m_manually_positioned = manually_positioned;
      wxWindow *parent = (wxWindow *)m_radar_panel;
      if (!m_pi->m_settings.show_radar[m_radar]) {
        parent = GetOCPNCanvasWindow();
      }
      m_control_dialog->Create(parent, m_pi, this, wxID_ANY, m_name, m_pi->m_settings.control_pos[m_radar]);
    }
    m_control_dialog->ShowDialog();
    UpdateControlState(true);
  } else {
    if (m_control_dialog) {
      m_control_dialog->HideDialog();
    }
  }
}

void RadarInfo::SetNetworkCardAddress(struct sockaddr_in *address) {
  if (!m_transmit->Init(address)) {
    wxLogError(wxT("BR24radar_pi %s: Unable to create transmit socket"), m_name.c_str());
  }
  m_stayalive_timeout = 0;  // Allow immediate restart of any TxOn or TxOff command
  m_pi->NotifyRadarWindowViz();
}

void RadarInfo::SetName(wxString name) {
  if (name != m_name) {
    LOG_DIALOG(wxT("BR24radar_pi: Changing name of radar #%d from '%s' to '%s'"), m_radar, m_name.c_str(), name.c_str());
    m_name = name;
    m_radar_panel->SetCaption(name);
    if (m_control_dialog) {
      m_control_dialog->SetTitle(name);
    }
  }
}

void RadarInfo::StartReceive() {
  if (!m_receive) {
    LOG_RECEIVE(wxT("BR24radar_pi: %s starting receive thread"), m_name.c_str());
    m_receive = new br24Receive(m_pi, this);
    if (!m_receive || (m_receive->Run() != wxTHREAD_NO_ERROR)) {
      LOG_INFO(wxT("BR24radar_pi: %s unable to start receive thread."), m_name.c_str());
      m_receive = 0;
    }
  }
}

void RadarInfo::ComputeColourMap() {
  for (int i = 0; i <= UINT8_MAX; i++) {
    m_colour_map[i] =
        (i >= m_pi->m_settings.threshold_red) ? BLOB_STRONG : (i >= m_pi->m_settings.threshold_green)
                                                                  ? BLOB_INTERMEDIATE
                                                                  : (i >= m_pi->m_settings.threshold_blue) ? BLOB_WEAK : BLOB_NONE;
  }

  for (int i = 0; i < BLOB_COLOURS; i++) {
    m_colour_map_rgb[i] = wxColour(0, 0, 0);
  }
  m_colour_map_rgb[BLOB_STRONG] = m_pi->m_settings.strong_colour;
  m_colour_map_rgb[BLOB_INTERMEDIATE] = m_pi->m_settings.intermediate_colour;
  m_colour_map_rgb[BLOB_WEAK] = m_pi->m_settings.weak_colour;

  if (m_trails_motion.value > 0) {
    float r1 = m_pi->m_settings.trail_start_colour.Red();
    float g1 = m_pi->m_settings.trail_start_colour.Green();
    float b1 = m_pi->m_settings.trail_start_colour.Blue();
    float r2 = m_pi->m_settings.trail_end_colour.Red();
    float g2 = m_pi->m_settings.trail_end_colour.Green();
    float b2 = m_pi->m_settings.trail_end_colour.Blue();
    float delta_r = (r2 - r1) / BLOB_HISTORY_COLOURS;
    float delta_g = (g2 - g1) / BLOB_HISTORY_COLOURS;
    float delta_b = (b2 - b1) / BLOB_HISTORY_COLOURS;

    for (BlobColour history = BLOB_HISTORY_0; history <= BLOB_HISTORY_MAX; history = (BlobColour)(history + 1)) {
      m_colour_map[history] = history;

      m_colour_map_rgb[history] = wxColour(r1, g1, b1);
      r1 += delta_r;
      g1 += delta_g;
      b1 += delta_b;
    }
  }
}

void RadarInfo::ResetSpokes() {
  UINT8 zap[RETURNS_PER_LINE];

  LOG_VERBOSE(wxT("BR24radar_pi: reset spokes, history and trails"));

  memset(zap, 0, sizeof(zap));
  memset(m_history, 0, sizeof(m_history));

  if (m_draw_panel.draw) {
    for (size_t r = 0; r < LINES_PER_ROTATION; r++) {
      m_draw_panel.draw->ProcessRadarSpoke(0, r, zap, sizeof(zap));
    }
  }
  if (m_draw_overlay.draw) {
    for (size_t r = 0; r < LINES_PER_ROTATION; r++) {
      m_draw_overlay.draw->ProcessRadarSpoke(0, r, zap, sizeof(zap));
    }
  }
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    // Zap them anyway just to be sure
    m_guard_zone[z]->ResetBogeys();
  }
}

/*
 * A spoke of data has been received by the receive thread and it calls this (in
 * the context of the receive thread, so no UI actions can be performed here.)
 *
 * @param angle                 Bearing (relative to Boat)  at which the spoke is seen.
 * @param bearing               Bearing (relative to North) at which the spoke is seen.
 * @param data                  A line of len bytes, each byte represents strength at that distance.
 * @param len                   Number of returns
 * @param range                 Range (in meters) of this data
 */
void RadarInfo::ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters, wxLongLong time_rec, double lat, double lon) {
  wxCriticalSectionLocker lock(m_exclusive);

  for (int i = 0; i < m_pi->m_settings.main_bang_size; i++) {
    data[i] = 0;
  }
  // calculate course as the moving average of m_hdt over one revolution
  SampleCourse(angle);  // used for course_up mode

  //static int rev = 0;   //   test cases only
  //if (angle == 0) rev++;
 // for (int i = 0; i < RETURNS_PER_LINE; i++){  
   //   data[i] = 0;
  //    if (i <= 300 && i >= 290 && bearing > 10 && bearing < 64) data[i] = 200;  
  //    if (i <= 300 && i >= 290 && bearing >= 64 && bearing < 96 && rev%2 == 0) data[i] = 200;
      //if ((i == 289 /*|| i == 288*/) && (bearing == 20 || bearing == 21))data[i] = 200;  
      //if ((i == 287 /*|| i == 286*/) && bearing == 20) data[i] = 200;  
      //if (i == 288 && bearing == 20) data[i] = 200;
      //if (i == 286 && bearing == 20) data[i] = 200;  
      //if (i == 285 && bearing == 20) data[i] = 200;  
      //if (i == 285 && bearing == 21) data[i] = 200;  
      //if (i == 286 && bearing == 21) data[i] = 200;

      //if (i == 295 && bearing == 10) data[i] = 200;
      //if (i == 295 && bearing == 9) data[i] = 200;
      //if (i == 295 && bearing == 8) data[i] = 200;
      //if (i == 295 && bearing == 7) data[i] = 200;
      //if (i == 295 && bearing == 6) data[i] = 200;
      //if (i == 295 && bearing == 5) data[i] = 200;

    /*  if (i == 296 && bearing == 6) data[i] = 200;
      if (i == 296 && bearing == 5) data[i] = 200;*/
//  }

  // Douwe likes this, and I think it has some value in testing, but I think it distracts as well.
  // Why don't we make this an option?
  data[RETURNS_PER_LINE - 1] = 200;  //  range ring, do we want this? ActionL: make setting


  if (m_range_meters != range_meters) {
    ResetSpokes();
    LOG_VERBOSE(wxT("BR24radar_pi: %s detected spoke range change from %d to %d meters"), m_name.c_str(), m_range_meters,
                range_meters);
    m_range_meters = range_meters;
    if (!m_range.value) {
      m_range.Update(convertSpokeMetersToRangeMeters(range_meters));
    }

  } else if (m_orientation.mod) {
    ResetSpokes();
    LOG_VERBOSE(wxT("BR24radar_pi: %s HeadUp/NorthUp/CourseUp change"));
  }
  int north_or_course_up = m_orientation.GetButton() != ORIENTATION_HEAD_UP;  // true for north up or course up
  uint8_t weakest_normal_blob = m_pi->m_settings.threshold_blue;

    UINT8 *hist_data = m_history[bearing].line;
    m_history[bearing].time = time_rec;
    m_history[bearing].lat = lat;
    m_history[bearing].lon = lon;
    for (size_t radius = 0; radius < len; radius++) {
      hist_data[radius] = hist_data[radius] << 1;  // shift left history byte 1 bit
      if (data[radius] >= weakest_normal_blob) {
        hist_data[radius] = hist_data[radius] | 1;  // and add 1 if above threshold
      }
    }

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (m_guard_zone[z]->m_type != GZ_OFF) {
      m_guard_zone[z]->ProcessSpoke(bearing, data, m_history[bearing].line, len, range_meters);
    }
  }

  if (m_multi_sweep_filter) {
    for (size_t radius = 0; radius < len; radius++) {
      if (!HISTORY_FILTER_ALLOW(m_history[bearing].line[radius])) { 
        data[radius] = 0;
      }
    }
  }

  bool draw_trails_on_overlay = (m_pi->m_settings.trails_on_overlay == 1);
  if (m_draw_overlay.draw && !draw_trails_on_overlay) {
    m_draw_overlay.draw->ProcessRadarSpoke(m_pi->m_settings.overlay_transparency, bearing, data, len);
  }

  PolarToCartesianLookupTable *polarLookup;
  polarLookup = GetPolarToCartesianLookupTable();
  if (m_old_range != m_range_meters && m_old_range != 0 && m_range_meters != 0) {
    // zoom trails
    float zoom_factor = (float)m_old_range / (float)m_range_meters;
    ZoomTrails(zoom_factor);
  }
  if (m_old_range == 0 || m_range_meters == 0) {
    ClearTrails();
  }
  m_old_range = m_range_meters;

  UpdateTrailPosition();  // for true trails

  // True trails
  for (size_t radius = 0; radius < len - 1; radius++) {  //  len - 1 : no trails on range circle
    UINT8 *trail = &m_trails.true_trails[polarLookup->intx[bearing][radius] + TRAILS_SIZE / 2 + m_trails.offset.lat]
                                        // when ship moves north, offset.lat > 0. Add to move trails image in opposite direction
                                        [polarLookup->inty[bearing][radius] + TRAILS_SIZE / 2 + m_trails.offset.lon];
    // when ship moves east, offset.lon > 0. Add to move trails image in opposite direction
    if (data[radius] >= weakest_normal_blob) {
      *trail = 1;
    } else {
      if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
        (*trail)++;
      }
      if (m_trails_motion.value == TARGET_MOTION_TRUE) {
        data[radius] = m_trail_colour[*trail];
      }
    }
  }

  // Relative trails
  UINT8 *trail = m_trails.relative_trails[angle];
  for (size_t radius = 0; radius < len - 1; radius++) {  // len - 1 : no trails on range circle
    if (data[radius] >= weakest_normal_blob) {
      *trail = 1;
    } else {
      if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
        (*trail)++;
      }
      if (m_trails_motion.value == TARGET_MOTION_RELATIVE) {
        data[radius] = m_trail_colour[*trail];
      }
    }
    trail++;
  }

  if (m_draw_overlay.draw && draw_trails_on_overlay) {
    m_draw_overlay.draw->ProcessRadarSpoke(m_pi->m_settings.overlay_transparency, bearing, data, len);
  }

  if (m_draw_panel.draw) {
    m_draw_panel.draw->ProcessRadarSpoke(3, north_or_course_up ? bearing : angle, data, len);
  }
}

void RadarInfo::SampleCourse(int angle) {
  //  Calculates the moving average of m_hdt and returns this in m_course
  //  This is a bit more complicated then expected, average of 359 and 1 is 180 and that is not what we want
  if (m_pi->m_heading_source != HEADING_NONE && ((angle & 127) == 0)) {  // sample m_hdt every 128 spokes
    if (m_course_log[m_course_index] > 720.) {                           // keep values within limits
      for (int i = 0; i < COURSE_SAMPLES; i++) {
        m_course_log[i] -= 720;
      }
    }
    if (m_course_log[m_course_index] < -720.) {
      for (int i = 0; i < COURSE_SAMPLES; i++) {
        m_course_log[i] += 720;
      }
    }
    double hdt = m_pi->m_hdt;
    while (m_course_log[m_course_index] - hdt > 180.) {  // compare with previous value
      hdt += 360.;
    }
    while (m_course_log[m_course_index] - hdt < -180.) {
      hdt -= 360.;
    }
    m_course_index++;
    if (m_course_index >= COURSE_SAMPLES) m_course_index = 0;
    m_course_log[m_course_index] = hdt;
    double sum = 0;
    for (int i = 0; i < COURSE_SAMPLES; i++) {
      sum += m_course_log[i];
    }
    m_course = fmod(sum / 16 + 720., 360);
  }
}

void RadarInfo::ZoomTrails(float zoom_factor) {
  // zoom_factor > 1 -> zoom in, enlarge image

  // zoom relative trails
  memset(&m_trails.copy_of_relative_trails, 0, sizeof(m_trails.copy_of_relative_trails));
  for (int i = 0; i < LINES_PER_ROTATION; i++) {
    for (int j = 0; j < RETURNS_PER_LINE; j++) {
      int index_j = (int((float)j * zoom_factor));
      if (index_j >= RETURNS_PER_LINE) break;
      if (m_trails.relative_trails[i][j] != 0) {
        m_trails.copy_of_relative_trails[i][index_j] = m_trails.relative_trails[i][j];
      }
    }
  }
  memcpy(&m_trails.relative_trails[0][0], &m_trails.copy_of_relative_trails[0][0], sizeof(m_trails.copy_of_relative_trails));

  // zoom true trails
  memset(&m_trails.copy_of_true_trails, 0, sizeof(m_trails.copy_of_true_trails));
  for (int i = TRAILS_SIZE / 2 + m_trails.offset.lat - RETURNS_PER_LINE;
       i < TRAILS_SIZE / 2 + m_trails.offset.lat + RETURNS_PER_LINE; i++) {
    int index_i = (int((float)(i - TRAILS_SIZE / 2 + m_trails.offset.lat) * zoom_factor)) + TRAILS_SIZE / 2 -
                  m_trails.offset.lat * zoom_factor;
    if (index_i >= TRAILS_SIZE - 1) break;  // allow adding an additional pixel later
    if (index_i < 0) continue;
    for (int j = TRAILS_SIZE / 2 + m_trails.offset.lon - RETURNS_PER_LINE;
         j < TRAILS_SIZE / 2 + m_trails.offset.lon + RETURNS_PER_LINE; j++) {
      int index_j = (int((float)(j - TRAILS_SIZE / 2 + m_trails.offset.lon) * zoom_factor)) + TRAILS_SIZE / 2 -
                    m_trails.offset.lon * zoom_factor;
      if (index_j >= TRAILS_SIZE - 1) break;
      if (index_j < 0) continue;
      if (m_trails.true_trails[i][j] != 0) {  // many to one mapping, prevent overwriting trails with 0
        m_trails.copy_of_true_trails[index_i][index_j] = m_trails.true_trails[i][j];
        if (zoom_factor > 1.2) {
          // add an extra pixel in the y direction
          m_trails.copy_of_true_trails[index_i][index_j + 1] = m_trails.true_trails[i][j];
          if (zoom_factor > 1.6) {
            // also add  pixel in the x direction
            m_trails.copy_of_true_trails[index_i + 1][index_j] = m_trails.true_trails[i][j];
            m_trails.copy_of_true_trails[index_i + 1][index_j + 1] = m_trails.true_trails[i][j];
          }
        }
      }
    }
  }
  memcpy(m_trails.true_trails, m_trails.copy_of_true_trails, sizeof(m_trails.copy_of_true_trails));
  m_trails.offset.lon *= zoom_factor;
  m_trails.offset.lat *= zoom_factor;
}

void RadarInfo::UpdateTransmitState() {
  time_t now = time(0);

  if (m_state.value == RADAR_TRANSMIT && TIMED_OUT(now, m_data_timeout)) {
    m_state.Update(RADAR_STANDBY);
    LOG_INFO(wxT("BR24radar_pi: %s data lost"), m_name.c_str());
  }
  if (m_state.value == RADAR_STANDBY && TIMED_OUT(now, m_radar_timeout)) {
    static wxString empty;

    m_state.Update(RADAR_OFF);
    m_pi->m_pMessageBox->SetRadarIPAddress(empty);
    LOG_INFO(wxT("BR24radar_pi: %s lost presence"), m_name.c_str());
    return;
  }

  if (!m_pi->IsRadarOnScreen(m_radar)) {
    return;
  }

  if (m_state.value == RADAR_TRANSMIT && TIMED_OUT(now, m_stayalive_timeout)) {
    m_transmit->RadarStayAlive();
    m_stayalive_timeout = now + STAYALIVE_TIMEOUT;
  }

  // If we find we have a radar and the boot flag is still set, turn radar on
  // Think about interaction with timed_transmit
  if (m_boot_state.value == RADAR_TRANSMIT && m_state.value == RADAR_STANDBY) {
    m_boot_state.Update(RADAR_OFF);
    RequestRadarState(RADAR_TRANSMIT);
  }
}

void RadarInfo::RequestRadarState(RadarState state) {
  if (m_pi->IsRadarOnScreen(m_radar) && m_state.value != RADAR_OFF) {  // if radar is visible and detected
    if (m_state.value != state && !(m_state.value == RADAR_WAKING_UP && state == RADAR_TRANSMIT)) {  // and change is wanted
      time_t now = time(0);

      switch (state) {
        case RADAR_TRANSMIT:
          if (m_pi->m_settings.emulator_on) {
            m_state.Update(RADAR_TRANSMIT);
          } else {
            m_transmit->RadarTxOn();
          }
          // Refresh radar immediately so that we generate draw mechanisms
          if (m_pi->m_settings.chart_overlay == m_radar) {
            GetOCPNCanvasWindow()->Refresh(false);
          }
          if (m_radar_panel) {
            m_radar_panel->Refresh();
          }
          m_pi->m_idle_standby = now + wxMax(m_pi->m_settings.idle_run_time, SECONDS_PER_TRANSMIT_BURST);
          break;

        case RADAR_STANDBY:
          if (m_pi->m_settings.emulator_on) {
            m_state.Update(RADAR_STANDBY);
          } else {
            m_transmit->RadarTxOff();
          }
          m_pi->m_idle_transmit = now + m_pi->m_settings.timed_idle * SECONDS_PER_TIMED_IDLE_SETTING;
          break;

        case RADAR_WAKING_UP:
        case RADAR_OFF:
          LOG_INFO(wxT("BR24radar_pi: %s unexpected status request %d"), m_name.c_str(), state);
      }
      m_stayalive_timeout = time(0) + STAYALIVE_TIMEOUT;
    }
  }
}

void RadarInfo::UpdateTrailPosition() {
  // When position changes the trail image is not moved, only the pointer to the center
  // of the image (offset) is changed.
  // So we move the image around within the m_trails.true_trails buffer (by moving the pointer).
  // But when there is no room anymore (margin used) the whole trails image is shifted
  // and the is offset reset
  if (!m_pi->m_bpos_set || m_pi->m_heading_source == HEADING_NONE) {
    return;
  }
  if (m_trails.lat == m_pi->m_ownship_lat && m_trails.lon == m_pi->m_ownship_lon) {  // don't do anything until position changes
    return;
  }

  double dif_lat = m_pi->m_ownship_lat - m_trails.lat;  // going north is positive
  double dif_lon = m_pi->m_ownship_lon - m_trails.lon;  // moving east is positive
  m_trails.lat = m_pi->m_ownship_lat;
  m_trails.lon = m_pi->m_ownship_lon;
  double fshift_lat = dif_lat * 60. * 1852. / (double)m_range_meters * (double)(RETURNS_PER_LINE);
  double fshift_lon = dif_lon * 60. * 1852. / (double)m_range_meters * (double)(RETURNS_PER_LINE);
  fshift_lon *= cos(deg2rad(m_pi->m_ownship_lat));  // at higher latitudes a degree of longitude is fewer meters
  int shift_lat = (int)(fshift_lat + m_trails.dif_lat);

  if (shift_lat > 0 && m_dir_lat <= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction (this area might not be empty)
    memset(&m_trails.true_trails[TRAILS_SIZE - MARGIN + m_trails.offset.lat][0], 0, TRAILS_SIZE * (MARGIN - m_trails.offset.lat));
    m_dir_lat = 1;
  }

  if (shift_lat < 0 && m_dir_lat >= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    memset(&m_trails.true_trails[0][0], 0, TRAILS_SIZE * (MARGIN + m_trails.offset.lat));
    m_dir_lat = -1;
  }

  int shift_lon = (int)(fshift_lon + m_trails.dif_lon);
  if (shift_lon > 0 && m_dir_lon <= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    for (int i = 0; i < TRAILS_SIZE; i++) {
      memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN + m_trails.offset.lon], 0, MARGIN - m_trails.offset.lon);
    }
    m_dir_lon = 1;
  }

  if (shift_lon < 0 && m_dir_lon >= 0) {
    // change of direction of movement
    // clear space in true_trails outside image in that direction
    for (int i = 0; i < TRAILS_SIZE; i++) {
      memset(&m_trails.true_trails[i][0], 0, MARGIN + m_trails.offset.lon);
    }
    m_dir_lon = -1;
  }

  m_trails.dif_lat = fshift_lat + m_trails.dif_lat - (double)shift_lat;  // save the rounding fraction and appy it next time
  m_trails.dif_lon = fshift_lon + m_trails.dif_lon - (double)shift_lon;

  if (abs(shift_lat) >= MARGIN || abs(shift_lon) >= MARGIN) {  // huge shift, reset trails
    ClearTrails();
    m_trails.lat = m_pi->m_ownship_lat;
    m_trails.lon = m_pi->m_ownship_lon;
    m_trails.dif_lat = 0.;
    m_trails.dif_lon = 0.;
    LOG_INFO(wxT("BR24radar_pi: %s Large movement trails reset"), m_name.c_str());
    return;
  }

  // don't shift the image yet, only shift the center
  m_trails.offset.lat += shift_lat;
  m_trails.offset.lon += shift_lon;  //  index as follows: array[lat][lon]

  if (abs(m_trails.offset.lon) >= MARGIN) {  // offset too large: shift image
                                             // shift in the opposite direction of the offset
    m_trails.offset.lon -= shift_lon;        // subtract again, image should be indide limits
    if (m_trails.offset.lon > 0) {
      for (int i = 0; i < TRAILS_SIZE; i++) {
        memmove(&m_trails.true_trails[i][MARGIN], &m_trails.true_trails[i][MARGIN + m_trails.offset.lon], RETURNS_PER_LINE * 2);
        memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN], 0, MARGIN);
      }
    }
    if (m_trails.offset.lon < 0) {
      for (int i = 0; i < TRAILS_SIZE; i++) {
        memmove(&m_trails.true_trails[i][MARGIN], &m_trails.true_trails[i][MARGIN + m_trails.offset.lon], RETURNS_PER_LINE * 2);
        //     memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN], 0, MARGIN);
        memset(&m_trails.true_trails[i][0], 0, MARGIN);
      }
    }
    m_trails.offset.lon = shift_lon;
  }

  if (abs(m_trails.offset.lat) >= MARGIN) {  // offset too large: shift image
    m_trails.offset.lat -= shift_lat;        // image inside array

    if (m_trails.offset.lat > 0) {
      memmove(&m_trails.true_trails[MARGIN][0], &m_trails.true_trails[MARGIN + m_trails.offset.lat][0],
              (RETURNS_PER_LINE * 2) * TRAILS_SIZE);
      memset(&m_trails.true_trails[TRAILS_SIZE - MARGIN][0], 0, TRAILS_SIZE * MARGIN);
    }

    if (m_trails.offset.lat < 0) {
      memmove(&m_trails.true_trails[MARGIN][0], &m_trails.true_trails[MARGIN + m_trails.offset.lat][0],
              RETURNS_PER_LINE * 2 * TRAILS_SIZE);

      memset(&m_trails.true_trails[0][0], 0, TRAILS_SIZE * MARGIN);
    }
    m_trails.offset.lat = shift_lat;
  }
}

void RadarInfo::RefreshDisplay(wxTimerEvent &event) {
  if (m_radar == 0) {
    time_t now = time(0);
    if (TIMED_OUT(now, m_main_timer_timeout)) {
      m_pi->Notify();
      m_main_timer_timeout = now + 1;
    }
  }

  if (m_overlay_refreshes_queued > 0) {
    // don't do additional refresh when too busy
    LOG_DIALOG(wxT("BR24radar_pi: %s busy encountered, overlay_refreshes_queued=%d"), m_name.c_str(), m_overlay_refreshes_queued);
  } else if (m_pi->IsOverlayOnScreen(m_radar)) {
    m_overlay_refreshes_queued++;
    GetOCPNCanvasWindow()->Refresh(false);
  }

  if (m_refreshes_queued > 0) {
    // don't do additional refresh and reset the refresh conter
    // this will also balance performance, if too busy skip refresh
    LOG_DIALOG(wxT("BR24radar_pi: %s busy encountered, refreshes_queued=%d"), m_name.c_str(), m_refreshes_queued);
  } else if (IsPaneShown()) {
    m_refreshes_queued++;
    m_radar_panel->Refresh(false);
  }

  // Calculate refresh speed
  if (m_pi->m_settings.refreshrate) {
    int millis = 1000 / (1 + ((m_pi->m_settings.refreshrate) - 1) * 5);

    if (millis != m_refresh_millis) {
      m_refresh_millis = millis;
      LOG_VERBOSE(wxT("BR24radar_pi: %s changed timer interval to %d milliseconds"), m_name.c_str(), m_refresh_millis);
      m_timer->Start(m_refresh_millis);
    }
  }
}

void RadarInfo::RenderGuardZone() {
  int start_bearing = 0, end_bearing = 0;
  GLubyte red = 0, green = 200, blue = 0, alpha = 50;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (m_guard_zone[z]->m_type != GZ_OFF) {
      if (m_guard_zone[z]->m_type == GZ_CIRCLE) {
        start_bearing = 0;
        end_bearing = 359;
      } else {
        start_bearing = SCALE_RAW_TO_DEGREES2048(m_guard_zone[z]->m_start_bearing);
        end_bearing = SCALE_RAW_TO_DEGREES2048(m_guard_zone[z]->m_end_bearing);
      }
      switch (m_pi->m_settings.guard_zone_render_style) {
        case 1:
          glColor4ub((GLubyte)255, (GLubyte)0, (GLubyte)0, (GLubyte)255);
          DrawOutlineArc(m_guard_zone[z]->m_outer_range, m_guard_zone[z]->m_inner_range, start_bearing, end_bearing, true);
          break;
        case 2:
          glColor4ub(red, green, blue, alpha);
          DrawOutlineArc(m_guard_zone[z]->m_outer_range, m_guard_zone[z]->m_inner_range, start_bearing, end_bearing, false);
        // fall thru
        default:
          glColor4ub(red, green, blue, alpha);
          DrawFilledArc(m_guard_zone[z]->m_outer_range, m_guard_zone[z]->m_inner_range, start_bearing, end_bearing);
      }
    }

    red = 0;
    green = 0;
    blue = 200;
  }
}

void RadarInfo::AdjustRange(int adjustment) {
  const RadarRange *min, *max;

  m_auto_range_mode = false;
  m_previous_auto_range_meters = 0;

  // Note that we don't actually use m_settings.units here, so that if we are metric and
  // the plotter in NM, and it chose the last range, we start using nautic miles as well.

  if (m_range.range) {
    if (m_range.range >= g_ranges_nautic && m_range.range < g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic)) {
      min = g_ranges_nautic;
      max = g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic) - 1;
    } else if (m_range.range >= g_ranges_metric && m_range.range < g_ranges_metric + ARRAY_SIZE(g_ranges_metric)) {
      min = g_ranges_metric;
      max = g_ranges_metric + ARRAY_SIZE(g_ranges_metric) - 1;
    } else {
      return;
    }

    if (m_radar_type != RT_4G) {
      max--;  // only 4G has longest ranges
    }

    if (adjustment > 0 && m_range.range > min) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), m_range.range[0].meters,
                  m_range.range[0].actual_meters, m_range.range[-1].meters, m_range.range[-1].actual_meters);
      m_transmit->SetRange(m_range.range[-1].meters);
    } else if (adjustment < 0 && m_range.range < max) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), m_range.range[0].meters,
                  m_range.range[0].actual_meters, m_range.range[+1].meters, m_range.range[+1].actual_meters);
      m_transmit->SetRange(m_range.range[+1].meters);
    }
  }
}

void RadarInfo::SetAutoRangeMeters(int meters) {
  if (m_state.value == RADAR_TRANSMIT && m_auto_range_mode) {
    m_auto_range_meters = meters;
    // Don't adjust auto range meters continuously when it is oscillating a little bit (< 5%)
    int test = 100 * m_previous_auto_range_meters / m_auto_range_meters;
    if (test < 95 || test > 105) {  //   range change required
      // Compute a 'standard' distance. This will be slightly smaller.
      convertMetersToRadarAllowedValue(&meters, m_pi->m_settings.range_units, m_radar_type);
      if (meters != m_range_meters) {
        if (m_pi->m_settings.verbose) {
          LOG_VERBOSE(wxT("BR24radar_pi: Automatic range changed from %d to %d meters"), m_previous_auto_range_meters,
                      m_auto_range_meters);
        }
        m_transmit->SetRange(meters);
        m_previous_auto_range_meters = m_auto_range_meters;
      }
    }
  } else {
    m_previous_auto_range_meters = 0;
  }
}

bool RadarInfo::SetControlValue(ControlType controlType, int value) { return m_transmit->SetControlValue(controlType, value); }

void RadarInfo::ShowRadarWindow(bool show) { m_radar_panel->ShowFrame(show); }

bool RadarInfo::IsPaneShown() { return m_radar_panel->IsPaneShown(); }

void RadarInfo::UpdateControlState(bool all) {
  wxCriticalSectionLocker lock(m_exclusive);

  m_overlay.Update(m_pi->m_settings.chart_overlay == m_radar);

#ifdef OPENCPN_NO_LONGER_MIXES_GL_CONTEXT
  //
  // Once OpenCPN doesn't mess up with OpenGL context anymore we can do this
  //
  if (m_overlay.value == 0 && m_draw_overlay.draw) {
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

  if (m_control_dialog) {
    m_control_dialog->UpdateControlValues(all);
    m_control_dialog->UpdateDialogShown();
  }

  if (IsPaneShown()) {
    m_radar_panel->Refresh(false);
  }
}

void RadarInfo::ResetRadarImage() {
  if (m_range_meters) {
    ResetSpokes();
    ClearTrails();
    m_range_meters = 0;
  }
}

void RadarInfo::RenderRadarImage(DrawInfo *di) {
  wxCriticalSectionLocker lock(m_exclusive);
  int drawing_method = m_pi->m_settings.drawing_method;

  if (m_state.value != RADAR_TRANSMIT && m_state.value != RADAR_WAKING_UP) {
    ResetRadarImage();
    return;
  }

  // Determine if a new draw method is required
  if (!di->draw || (drawing_method != di->drawing_method)) {
    RadarDraw *newDraw = RadarDraw::make_Draw(this, drawing_method);
    if (!newDraw) {
      wxLogError(wxT("BR24radar_pi: out of memory"));
      return;
    } else if (newDraw->Init()) {
      wxArrayString methods;
      RadarDraw::GetDrawingMethods(methods);
      if (di == &m_draw_overlay) {
        LOG_VERBOSE(wxT("BR24radar_pi: %s new drawing method %s for overlay"), m_name.c_str(), methods[drawing_method].c_str());
      } else {
        LOG_VERBOSE(wxT("BR24radar_pi: %s new drawing method %s for panel"), m_name.c_str(), methods[drawing_method].c_str());
      }
      if (di->draw) {
        delete di->draw;
      }
      di->draw = newDraw;
      di->drawing_method = drawing_method;
    } else {
      m_pi->m_settings.drawing_method = 0;
      delete newDraw;
    }
    if (!di->draw) {
      return;
    }
  }

  di->draw->DrawRadarImage();
  if (g_first_render) {
    g_first_render = false;
    wxLongLong startup_elapsed = wxGetUTCTimeMillis() - m_pi->m_boot_time;
    LOG_INFO(wxT("BR24radar_pi: First radar image rendered after %llu ms\n"), startup_elapsed);
    
  }
}

void RadarInfo::RenderRadarImage(wxPoint center, double scale, double overlay_rotate, bool overlay) {
  if (!m_range_meters) {
    return;
  }
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);  // Save state
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  overlay_rotate += OPENGL_ROTATION;  // Difference between OpenGL and compass + radar
  double panel_rotate = overlay_rotate;
  if (m_orientation.value == ORIENTATION_COURSE_UP) {
    panel_rotate -= m_course;
  }
  double guard_rotate = overlay_rotate;
  if (overlay || m_orientation.value == ORIENTATION_NORTH_UP || m_orientation.value == ORIENTATION_COURSE_UP) {
    guard_rotate += m_pi->m_hdt;
  }
  if (!overlay && m_orientation.value == ORIENTATION_COURSE_UP) {
    guard_rotate -= m_course;
  }

  if (overlay) {
      if (m_marpa){
          m_marpa->RefreshArpaTargets();
          glPushMatrix();
          glTranslated(center.x, center.y, 0);
          glScaled(scale, scale, 1.);
          m_marpa->DrawArpaTargets();
          glPopMatrix();
      }

    if (m_pi->m_settings.guard_zone_on_overlay) {
      glPushMatrix();
      glTranslated(center.x, center.y, 0);
      glRotated(guard_rotate, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);

      // LOG_DIALOG(wxT("BR24radar_pi: %s render guard zone on overlay"), name.c_str());
      RenderGuardZone();
      glPopMatrix();
    }
    double radar_pixels_per_meter = ((double)RETURNS_PER_LINE) / m_range_meters;
    scale = scale / radar_pixels_per_meter;
    glPushMatrix();
    glTranslated(center.x, center.y, 0);
    if (overlay_rotate != 0.0) {
      glRotated(overlay_rotate, 0.0, 0.0, 1.0);
    }
    glScaled(scale, scale, 1.);

    RenderRadarImage(&m_draw_overlay);
    if (m_overlay_refreshes_queued > 0) {
      m_overlay_refreshes_queued--;
    }
  } else if (m_range.value) {
    glPushMatrix();
    scale = 1.0 / m_range.value;
    glScaled(scale, scale, 1.);
    glRotated(guard_rotate, 0.0, 0.0, 1.0);
    RenderGuardZone();
    glPopMatrix();

    glPushMatrix();
    double overscan = (double)m_range_meters / (double)m_range.value;
    scale = overscan / RETURNS_PER_LINE;
    glScaled(scale, scale, 1.);
    glRotated(panel_rotate, 0.0, 0.0, 1.0);
    LOG_DIALOG(wxT("BR24radar_pi: %s render overscan=%g range=%d"), m_name.c_str(), overscan, m_range.value);
    RenderRadarImage(&m_draw_panel);
    if (m_refreshes_queued > 0) {
      m_refreshes_queued--;
    }
  }

  glPopMatrix();
  glPopAttrib();
}

wxString RadarInfo::GetCanvasTextTopLeft() {
  wxString s;

  if (IsDisplayNorthUp()) {
    s << _("North Up");
  } else if (m_orientation.value == ORIENTATION_COURSE_UP && m_pi->m_heading_source != HEADING_NONE) {
    s << _("Course Up");
  } else {
    s << _("Head Up");
  }
  if (m_pi->m_settings.emulator_on) {
    s << wxT("\n") << _("Emulator");
  }
  if (m_range_meters) {
    s << wxT("\n") << GetRangeText();
  }
  if (s.Right(1) != wxT("\n")) {
    s << wxT("\n");
  }
  if (m_trails_motion.value > 0) {
    if (m_trails_motion.value == TARGET_MOTION_TRUE) {
      s << wxT("RM(T)");
    } else {
      s << wxT("RM(R)");
    }
  } else {
    s << wxT("RM");
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

wxString RadarInfo::FormatAngle(double angle) {
  wxString s;

  wxString relative;
  if (angle > 360) angle -= 360;
  if (IsDisplayNorthUp() || (m_orientation.value == ORIENTATION_COURSE_UP && m_pi->m_heading_source != HEADING_NONE)) {
    relative = wxT("T");
  } else {
    if (angle > 180.0) {
      angle = -(360.0 - angle);
    }
    relative = wxT("R");
  }
  s << wxString::Format(wxT("%.1f\u00B0%s"), angle, relative);

  return s;
}

wxString RadarInfo::GetCanvasTextBottomLeft() {
  wxString s = m_pi->GetGuardZoneText(this);

  if (m_state.value == RADAR_TRANSMIT) {
    double distance = 0.0, bearing = nanl("");

    // Add VRM/EBLs

    for (int b = 0; b < BEARING_LINES; b++) {
      double bearing = m_ebl[m_orientation.value][b];
      if (m_vrm[b] != 0.0 && bearing != 0.) {
        if (m_orientation.value == ORIENTATION_COURSE_UP) {
          bearing += m_course;
          if (bearing >= 360) bearing -= 360;
        }

        if (s.length()) {
          s << wxT("\n");
        }
        s << wxString::Format(wxT("VRM%d=%s EBL%d=%s"), b + 1, FormatDistance(m_vrm[b]), b + 1, FormatAngle(bearing));
      }
    }
    // Add in mouse cursor location

    if (m_mouse_vrm[m_orientation.value] != 0.0) {
      distance = m_mouse_vrm[m_orientation.value];
      bearing = m_mouse_ebl[m_orientation.value];
      if (m_orientation.value == ORIENTATION_COURSE_UP) {
        bearing += m_course;
        if (bearing >= 360) bearing -= 360;
      }

    } else if ((m_mouse_lat != 0.0 || m_mouse_lon != 0.0) && m_pi->m_bpos_set) {
      // Can't compute this upfront, ownship may move...
      distance = local_distance(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
      bearing = local_bearing(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
      if (!IsDisplayNorthUp()) {
        bearing -= m_pi->m_hdt;
      }
    }

    if (distance != 0.0) {
      if (s.length()) {
        s << wxT("\n");
      }
      s << FormatDistance(distance) << wxT(", ") << FormatAngle(bearing);
    }
  }
  return s;
}

wxString RadarInfo::GetCanvasTextCenter() {
  wxString s;

  switch (m_state.value) {
    case RADAR_OFF:
      s << _("No radar");
      break;
    case RADAR_STANDBY:
      s << _("Radar is in Standby");
      break;
    case RADAR_WAKING_UP:
      s << _("Radar is waking up");
      break;
    case RADAR_TRANSMIT:
      if (m_draw_panel.draw) {
        return s;
      }
      s << _("Radar not transmitting");
      break;
  }

  switch (m_radar_type) {
    case RT_BR24:
      s << wxT("\nBR24");
      break;
    case RT_3G:
      s << wxT("\n3G");
      break;
    case RT_4G:
      s << wxT("\n4G");
      break;
    case RT_UNKNOWN:
    default:
      break;
  }

  return s;
}

wxString &RadarInfo::GetRangeText() {
  const RadarRange *r = m_range.range;
  int meters = m_range.value;

  if (!r) {
    m_range_text = wxT("");
    return m_range_text;
  }

  bool auto_range = m_auto_range_mode && (m_overlay.button > 0);

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

  if (auto_range) {
    m_range_text << wxT(")");
  }
  LOG_DIALOG(wxT("br24radar_pi: range label '%s' for spokerange=%d range=%d auto=%d"), m_range_text.c_str(), m_range_meters, meters,
             m_auto_range_mode);
  return m_range_text;
}

const char *RadarInfo::GetDisplayRangeStr(size_t idx) {
  if (m_range.range) {
    return (&m_range.range->name)[(idx + 1) % 4];
  }

  return 0;
}

void RadarInfo::SetMouseLatLon(double lat, double lon) {
  for (int i = 0; i < ORIENTATION_NUMBER; i++) {
    m_mouse_ebl[i] = nanl("");
    m_mouse_vrm[i] = 0.0;
  }
  m_mouse_lat = lat;
  m_mouse_lon = lon;
  LOG_DIALOG(wxT("BR24radar_pi: SetMouseLatLon(%f, %f)"), lat, lon);
}

void RadarInfo::SetMouseVrmEbl(double vrm, double ebl) {
  if (m_orientation.value == ORIENTATION_HEAD_UP) {
    m_mouse_vrm[ORIENTATION_HEAD_UP] = vrm;
    m_mouse_ebl[ORIENTATION_HEAD_UP] = ebl;
  }
  if (m_orientation.value == ORIENTATION_NORTH_UP) {
    m_mouse_ebl[ORIENTATION_NORTH_UP] = ebl;
    m_mouse_ebl[ORIENTATION_COURSE_UP] = ebl - m_course;
    m_mouse_vrm[ORIENTATION_NORTH_UP] = vrm;
    m_mouse_vrm[ORIENTATION_COURSE_UP] = vrm;
  }
  if (m_orientation.value == ORIENTATION_COURSE_UP) {
    m_mouse_ebl[ORIENTATION_NORTH_UP] = ebl + m_course;
    m_mouse_ebl[ORIENTATION_COURSE_UP] = ebl;
    m_mouse_vrm[ORIENTATION_NORTH_UP] = vrm;
    m_mouse_vrm[ORIENTATION_COURSE_UP] = vrm;
  }

  m_mouse_lat = 0.0;
  m_mouse_lon = 0.0;
  LOG_DIALOG(wxT("BR24radar_pi: SetMouseVrmEbl(%f, %f)"), vrm, ebl);
}

void RadarInfo::SetBearing(int bearing) {
  if (m_vrm[bearing] != 0.0) {
    m_vrm[bearing] = 0.0;
    m_ebl[m_orientation.value][bearing] = nanl("");
  } else if (m_mouse_vrm[m_orientation.value] != 0.0) {
    m_vrm[bearing] = m_mouse_vrm[m_orientation.value];
    if (m_orientation.value == ORIENTATION_HEAD_UP) {
      m_ebl[ORIENTATION_HEAD_UP][bearing] = m_mouse_ebl[ORIENTATION_HEAD_UP];
    } else {
      m_ebl[ORIENTATION_NORTH_UP][bearing] = m_mouse_ebl[ORIENTATION_NORTH_UP];
      m_ebl[ORIENTATION_COURSE_UP][bearing] = m_mouse_ebl[ORIENTATION_COURSE_UP];
    }
  } else if (m_mouse_lat != 0.0 || m_mouse_lon != 0.0) {
    m_vrm[bearing] = local_distance(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
    m_ebl[m_orientation.value][bearing] = local_bearing(m_pi->m_ownship_lat, m_pi->m_ownship_lon, m_mouse_lat, m_mouse_lon);
  }
}

void RadarInfo::ClearTrails() { memset(&m_trails, 0, sizeof(m_trails)); }

void RadarInfo::ComputeTargetTrails() {
  static TrailRevolutionsAge maxRevs[TRAIL_ARRAY_SIZE] = {
      SECONDS_TO_REVOLUTIONS(15),  SECONDS_TO_REVOLUTIONS(30),  SECONDS_TO_REVOLUTIONS(60), SECONDS_TO_REVOLUTIONS(180),
      SECONDS_TO_REVOLUTIONS(300), SECONDS_TO_REVOLUTIONS(600), TRAIL_MAX_REVOLUTIONS + 1};

  TrailRevolutionsAge maxRev = maxRevs[m_target_trails.value];
  if (m_trails_motion.value == 0) {
    maxRev = 0;
  }
  TrailRevolutionsAge revolution;
  double coloursPerRevolution = 0.;
  double colour = 0.;

  // Like plotter, continuous trails are all very white (non transparent)
  if ((m_trails_motion.value > 0) && (m_target_trails.value < TRAIL_CONTINUOUS)) {
    coloursPerRevolution = BLOB_HISTORY_COLOURS / (double)maxRev;
  }

  LOG_VERBOSE(wxT("BR24radar_pi: Target trail value %d = %d revolutions"), m_target_trails.value, maxRev);

  // Disperse the BLOB_HISTORY values over 0..maxrev
  for (revolution = 0; revolution <= TRAIL_MAX_REVOLUTIONS; revolution++) {
    if (revolution >= 1 && revolution < maxRev) {
      m_trail_colour[revolution] = (BlobColour)(BLOB_HISTORY_0 + (int)colour);
      colour += coloursPerRevolution;
    } else {
      m_trail_colour[revolution] = BLOB_NONE;
    }
    // LOG_VERBOSE(wxT("BR24radar_pi: ComputeTargetTrails rev=%u color=%d"), revolution, m_trail_colour[revolution]);
  }
}

PLUGIN_END_NAMESPACE
