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
#include "RadarMarpa.h"
#include "RadarPanel.h"
#include "br24ControlsDialog.h"
#include "br24Receive.h"
#include "br24Transmit.h"
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

bool g_first_render = true;

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

  wxCriticalSectionLocker lock(m_exclusive);

  size_t g;
  const RadarRange *newRange = 0;

  // Find out which nautical or metric range is the one represented by 'value'.
  // First we look up according to the desired setting (metric/nautical) and if
  // that doesn't work we look up nautical then metric.

  if (m_settings->range_units == RANGE_NAUTICAL) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
      if (g_ranges_nautic[g].meters == m_value) {
        newRange = &g_ranges_nautic[g];
        break;
      }
    }
  } else {
    for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
      if (g_ranges_metric[g].meters == m_value) {
        newRange = &g_ranges_metric[g];
        break;
      }
    }
  }
  if (!newRange) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_nautic); g++) {
      if (g_ranges_nautic[g].meters == m_value) {
        newRange = &g_ranges_nautic[g];
        break;
      }
    }
  }
  if (!newRange) {
    for (g = 0; g < ARRAY_SIZE(g_ranges_metric); g++) {
      if (g_ranges_metric[g].meters == m_value) {
        newRange = &g_ranges_metric[g];
        break;
      }
    }
  }

  m_range = newRange;
}

/**
 * Constructor.
 *
 * Called when the config is not yet known, so this should not start any
 * computations based on those yet.
 */
RadarInfo::RadarInfo(br24radar_pi *pi, int radar) {
  m_pi = pi;
  m_radar = radar;
  m_arpa = 0;
  m_radar_type = RT_UNKNOWN;
  m_auto_range_mode = true;
  m_course_index = 0;
  m_old_range = 0;
  m_dir_lat = 0;
  m_dir_lon = 0;
  m_range_meters = 0;
  m_auto_range_meters = 0;
  m_previous_auto_range_meters = 0;
  m_previous_orientation = ORIENTATION_HEAD_UP;
  m_stayalive_timeout = 0;
  m_radar_timeout = 0;
  m_data_timeout = 0;
  ClearTrails();
  CLEAR_STRUCT(m_statistics);
  CLEAR_STRUCT(m_course_log);

  m_mouse_lat = NAN;
  m_mouse_lon = NAN;
  for (int i = 0; i < ORIENTATION_NUMBER; i++) {
    m_mouse_ebl[i] = NAN;
    m_mouse_vrm = NAN;
    for (int b = 0; b < BEARING_LINES; b++) {
      m_ebl[i][b] = NAN;
      m_vrm[b] = NAN;
    }
  }
  m_transmit = 0;
  m_receive = 0;
  m_draw_panel.draw = 0;
  m_draw_overlay.draw = 0;
  m_radar_panel = 0;
  m_radar_canvas = 0;
  m_control_dialog = 0;
  m_state.Update(RADAR_OFF);
  m_range.m_settings = &m_pi->m_settings;
  m_refresh_millis = 50;

  m_arpa = new RadarArpa(m_pi, this);
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    m_guard_zone[z] = new GuardZone(m_pi, this, z);
  }
}

void RadarInfo::Shutdown() {
  if (m_receive) {
    m_receive->Shutdown();

    wxLongLong threadStartWait = wxGetUTCTimeMillis();
    m_receive->Wait();
    wxLongLong threadEndWait = wxGetUTCTimeMillis();
    wxLongLong threadExtraWait = 0;
    // See if Douwe is right and Wait() doesn't work properly -- attests it returns
    // before the thread is dead.
    while (!m_receive->m_is_shutdown) {
      wxYield();
      wxMilliSleep(10);
      threadExtraWait = wxGetUTCTimeMillis();
    }

    // Now log what we have done
    if (threadExtraWait != 0) {
      LOG_INFO(wxT("BR24radar_pi: %s receive thread wait did not work, had to wait for %lu ms extra"), m_name.c_str(),
               threadExtraWait - threadEndWait);
      threadEndWait = threadExtraWait;
    }
    if (m_receive->m_shutdown_time_requested != 0) {
      LOG_INFO(wxT("BR24radar_pi: %s receive thread stopped in %lu ms, had to wait for %lu ms"), m_name.c_str(),
               threadEndWait - m_receive->m_shutdown_time_requested, threadEndWait - threadStartWait);
    } else {
      LOG_INFO(wxT("BR24radar_pi: %s receive thread stopped in %lu ms, had to wait for %lu ms"), m_name.c_str(),
               threadEndWait - m_receive->m_shutdown_time_requested, threadEndWait - threadStartWait);
    }
    delete m_receive;
    m_receive = 0;
  }

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
  if (m_arpa) {
    delete m_arpa;
    m_arpa = 0;
  }
  for (size_t z = 0; z < GUARD_ZONES; z++) {
    delete m_guard_zone[z];
    m_guard_zone[z] = 0;
  }
}

/**
 * Initialize the on-screen and receive/transmit items.
 *
 * This is called after the config file has been loaded, so all state is known.
 */
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

  ComputeTargetTrails();

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
      LOG_VERBOSE(wxT("BR24radar_pi %s: Creating control dialog"), m_name.c_str());
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
  m_pi->NotifyControlDialog();
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
      if (m_receive) {
        delete m_receive;
      }
      m_receive = 0;
    }
  }
}

void RadarInfo::ComputeColourMap() {
  for (int i = 0; i <= UINT8_MAX; i++) {
    m_colour_map[i] = (i >= m_pi->m_settings.threshold_red) ? BLOB_STRONG
                                                            : (i >= m_pi->m_settings.threshold_green)
                                                                  ? BLOB_INTERMEDIATE
                                                                  : (i >= m_pi->m_settings.threshold_blue) ? BLOB_WEAK : BLOB_NONE;
  }

  for (int i = 0; i < BLOB_COLOURS; i++) {
    m_colour_map_rgb[i] = wxColour(0, 0, 0);
  }
  m_colour_map_rgb[BLOB_STRONG] = m_pi->m_settings.strong_colour;
  m_colour_map_rgb[BLOB_INTERMEDIATE] = m_pi->m_settings.intermediate_colour;
  m_colour_map_rgb[BLOB_WEAK] = m_pi->m_settings.weak_colour;

  if (m_trails_motion.GetValue() > 0) {
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

  LOG_VERBOSE(wxT("BR24radar_pi: reset spokes"));

  CLEAR_STRUCT(zap);
  CLEAR_STRUCT(m_history);

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
void RadarInfo::ProcessRadarSpoke(SpokeBearing angle, SpokeBearing bearing, UINT8 *data, size_t len, int range_meters,
                                  wxLongLong time_rec, double lat, double lon) {
  int orientation;

  // calculate course as the moving average of m_hdt over one revolution
  SampleCourse(angle);  // used for course_up mode

  for (int i = 0; i < m_pi->m_settings.main_bang_size; i++) {
    data[i] = 0;
  }

  if (m_pi->m_settings.show_extreme_range) {
    data[RETURNS_PER_LINE - 1] = 255;  //  range ring, do we want this? ActionL: make setting, switched on for testing
    data[1] = 255;                     // Main bang on purpose to show radar center
    data[0] = 255;                     // Main bang on purpose to show radar center
  }

  if (m_range_meters != range_meters) {
    ResetSpokes();
    if (m_arpa) {
      m_arpa->ClearContours();
    }
    LOG_VERBOSE(wxT("BR24radar_pi: %s detected spoke range change from %d to %d meters"), m_name.c_str(), m_range_meters,
                range_meters);
    m_range_meters = range_meters;
    if (!m_range.GetValue()) {
      m_range.Update(convertSpokeMetersToRangeMeters(range_meters));
    }
  }

  orientation = GetOrientation();
  if ((orientation == ORIENTATION_HEAD_UP || m_previous_orientation == ORIENTATION_HEAD_UP) &&
      (orientation != m_previous_orientation)) {
    ResetSpokes();
    m_previous_orientation = orientation;
  }

  // In NORTH or COURSE UP modes we store the radar data at the bearing received
  // in the spoke. In other words: at an absolute angle off north.
  // This way, when the boat rotates the data on the overlay doesn't rotate with it.
  // This is also called 'stabilized' mode, I guess.
  //
  // The history data used for the ARPA data is *always* in bearing mode, it is not usable
  // with relative data.
  //
  int stabilized_mode = orientation != ORIENTATION_HEAD_UP;
  uint8_t weakest_normal_blob = m_pi->m_settings.threshold_blue;

  UINT8 *hist_data = m_history[bearing].line;
  m_history[bearing].time = time_rec;
  m_history[bearing].lat = lat;
  m_history[bearing].lon = lon;
  for (size_t radius = 0; radius < len; radius++) {
    hist_data[radius] = 0;
    if (data[radius] >= weakest_normal_blob) {
      // and add 1 if above threshold and set the left 2 bits, used for ARPA
      hist_data[radius] = hist_data[radius] | 192;
    }
  }

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (m_guard_zone[z]->m_alarm_on) {
      m_guard_zone[z]->ProcessSpoke(angle, data, m_history[bearing].line, len, range_meters);
    }
  }

  bool draw_trails_on_overlay = (m_pi->m_settings.trails_on_overlay == 1);
  if (m_draw_overlay.draw && !draw_trails_on_overlay) {
    m_draw_overlay.draw->ProcessRadarSpoke(m_pi->m_settings.overlay_transparency, bearing, data, len);
  }

  UpdateTrailPosition();

  // True trails
  int motion = m_trails_motion.GetValue();
  PolarToCartesianLookupTable *polarLookup = GetPolarToCartesianLookupTable();
  for (size_t radius = 0; radius < len - 1; radius++) {  //  len - 1 : no trails on range circle
    int x = polarLookup->intx[bearing][radius] + TRAILS_SIZE / 2 + m_trails.offset.lat;
    int y = polarLookup->inty[bearing][radius] + TRAILS_SIZE / 2 + m_trails.offset.lon;

    if (x >= 0 && x < TRAILS_SIZE && y >= 0 && y < TRAILS_SIZE) {
      UINT8 *trail = &m_trails.true_trails[x][y];
      // when ship moves north, offset.lat > 0. Add to move trails image in opposite direction
      // when ship moves east, offset.lon > 0. Add to move trails image in opposite direction
      if (data[radius] >= weakest_normal_blob) {
        *trail = 1;
      } else {
        if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS) {
          (*trail)++;
        }
        if (motion == TARGET_MOTION_TRUE) {
          data[radius] = m_trail_colour[*trail];
        }
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
      if (motion == TARGET_MOTION_RELATIVE) {
        data[radius] = m_trail_colour[*trail];
      }
    }
    trail++;
  }

  if (m_draw_overlay.draw && draw_trails_on_overlay) {
    m_draw_overlay.draw->ProcessRadarSpoke(m_pi->m_settings.overlay_transparency, bearing, data, len);
  }

  if (m_draw_panel.draw) {
    m_draw_panel.draw->ProcessRadarSpoke(4, stabilized_mode ? bearing : angle, data, len);
  }
}

void RadarInfo::SampleCourse(int angle) {
  //  Calculates the moving average of m_hdt and returns this in m_course
  //  This is a bit more complicated then expected, average of 359 and 1 is 180 and that is not what we want
  if (m_pi->GetHeadingSource() != HEADING_NONE && ((angle & 127) == 0)) {  // sample m_hdt every 128 spokes
    if (m_course_log[m_course_index] > 720.) {                             // keep values within limits
      for (int i = 0; i < COURSE_SAMPLES; i++) {
        m_course_log[i] -= 720;
      }
    }
    if (m_course_log[m_course_index] < -720.) {
      for (int i = 0; i < COURSE_SAMPLES; i++) {
        m_course_log[i] += 720;
      }
    }
    double hdt = m_pi->GetHeadingTrue();
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
    m_course = fmod(sum / COURSE_SAMPLES + 720., 360);
  }
}

void RadarInfo::ZoomTrails(float zoom_factor) {
  // zoom_factor > 1 -> zoom in, enlarge image
  // zoom relative trails
  CLEAR_STRUCT(m_trails.copy_of_relative_trails);
  for (int i = 0; i < LINES_PER_ROTATION; i++) {
    for (int j = 0; j < RETURNS_PER_LINE; j++) {
      int index_j = int((float)j * zoom_factor);
      if (index_j >= RETURNS_PER_LINE) break;
      if (m_trails.relative_trails[i][j] != 0) {
        m_trails.copy_of_relative_trails[i][index_j] = m_trails.relative_trails[i][j];
      }
    }
  }
  memcpy(m_trails.relative_trails, m_trails.copy_of_relative_trails, sizeof(m_trails.copy_of_relative_trails));

  CLEAR_STRUCT(m_trails.copy_of_true_trails);
  // zoom true trails
  for (int i = wxMax(TRAILS_SIZE / 2 + m_trails.offset.lat - RETURNS_PER_LINE, 0);
       i < wxMin(TRAILS_SIZE / 2 + m_trails.offset.lat + RETURNS_PER_LINE, TRAILS_SIZE); i++) {
    int index_i = (int((float)(i - TRAILS_SIZE / 2 + m_trails.offset.lat) * zoom_factor)) + TRAILS_SIZE / 2 -
                  m_trails.offset.lat * zoom_factor;
    if (index_i >= TRAILS_SIZE - 1) break;  // allow adding an additional pixel later
    if (index_i < 0) continue;
    for (int j = wxMax(TRAILS_SIZE / 2 + m_trails.offset.lon - RETURNS_PER_LINE, 0);
         j < wxMin(TRAILS_SIZE / 2 + m_trails.offset.lon + RETURNS_PER_LINE, TRAILS_SIZE); j++) {
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
  wxCriticalSectionLocker lock(m_exclusive);
  time_t now = time(0);

  int state = m_state.GetValue();

  if (state == RADAR_TRANSMIT && TIMED_OUT(now, m_data_timeout)) {
    m_state.Update(RADAR_STANDBY);
    LOG_INFO(wxT("BR24radar_pi: %s data lost"), m_name.c_str());
  }
  if (state == RADAR_STANDBY && TIMED_OUT(now, m_radar_timeout)) {
    static wxString empty;

    m_state.Update(RADAR_OFF);
    m_pi->m_pMessageBox->SetRadarIPAddress(empty);
    LOG_INFO(wxT("BR24radar_pi: %s lost presence"), m_name.c_str());
    return;
  }

  if (!m_pi->IsRadarOnScreen(m_radar)) {
    return;
  }

  if (state == RADAR_TRANSMIT && TIMED_OUT(now, m_stayalive_timeout)) {
    m_transmit->RadarStayAlive();
    m_stayalive_timeout = now + STAYALIVE_TIMEOUT;
  }

  // If we find we have a radar and the boot flag is still set, turn radar on
  // Think about interaction with timed_transmit
  if (m_boot_state.GetValue() == RADAR_TRANSMIT && state == RADAR_STANDBY) {
    m_boot_state.Update(RADAR_OFF);
    RequestRadarState(RADAR_TRANSMIT);
  }
}

void RadarInfo::RequestRadarState(RadarState state) {
  int oldState = m_state.GetValue();

  if (m_pi->IsRadarOnScreen(m_radar) && oldState != RADAR_OFF) {                           // if radar is visible and detected
    if (oldState != state && !(oldState == RADAR_WAKING_UP && state == RADAR_TRANSMIT)) {  // and change is wanted
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
          break;

        case RADAR_STANDBY:
          if (m_pi->m_settings.emulator_on) {
            m_state.Update(RADAR_STANDBY);
          } else {
            m_transmit->RadarTxOff();
          }
          break;

        case RADAR_WAKING_UP:
        case RADAR_OFF:
          LOG_INFO(wxT("BR24radar_pi: %s unexpected status request %d"), m_name.c_str(), state);
      }
      m_stayalive_timeout = now + STAYALIVE_TIMEOUT;
    }
  }
}

void RadarInfo::UpdateTrailPosition() {
  double radar_lat;
  double radar_lon;
  int shift_lat;
  int shift_lon;

  // When position changes the trail image is not moved, only the pointer to the center
  // of the image (offset) is changed.
  // So we move the image around within the m_trails.true_trails buffer (by moving the pointer).
  // But when there is no room anymore (margin used) the whole trails image is shifted
  // and the offset is reset
  if (m_trails.offset.lon >= MARGIN || m_trails.offset.lon <= -MARGIN) {
    LOG_INFO(wxT("BR24radar_pi: offset lon too large %d"), m_trails.offset.lon);
    m_trails.offset.lon = 0;
  }
  if (m_trails.offset.lat >= MARGIN || m_trails.offset.lat <= -MARGIN) {
    LOG_INFO(wxT("BR24radar_pi: offset lat too large %d"), m_trails.offset.lat);
    m_trails.offset.lat = 0;
  }

  // zooming of trails required? First check conditions
  if (m_old_range == 0 || m_range_meters == 0) {
    ClearTrails();
    if (m_range_meters == 0) {
      return;
      if (m_old_range == 0) {
        m_old_range = m_range_meters;
        return;
      }
    }
  } else if (m_old_range != m_range_meters) {
    // zoom trails
    float zoom_factor = (float)m_old_range / (float)m_range_meters;
    m_old_range = m_range_meters;

    // center the image before zooming
    // otherwise the offset might get too large
    ShiftImageLatToCenter();
    ShiftImageLonToCenter();
    ZoomTrails(zoom_factor);  // this no longer modifies m_trails.offset, as the image is centered now
  }
  m_old_range = m_range_meters;

  if (!m_pi->GetRadarPosition(&radar_lat, &radar_lon) || m_pi->GetHeadingSource() == HEADING_NONE) {
    return;
  }

  // Did the ship move? No, return.
  if (m_trails.lat == radar_lat && m_trails.lon == radar_lon) {
    return;
  }

  // Check the movement of the ship
  double dif_lat = radar_lat - m_trails.lat;  // going north is positive
  double dif_lon = radar_lon - m_trails.lon;  // moving east is positive
  m_trails.lat = radar_lat;
  m_trails.lon = radar_lon;

  // get (floating point) shift of the ship in radar pixels
  double fshift_lat = dif_lat * 60. * 1852. / (double)m_range_meters * (double)(RETURNS_PER_LINE);
  double fshift_lon = dif_lon * 60. * 1852. / (double)m_range_meters * (double)(RETURNS_PER_LINE);
  fshift_lon *= cos(deg2rad(radar_lat));  // at higher latitudes a degree of longitude is fewer meters

  // Get the integer pixel shift, first add previous rounding error
  shift_lat = (int)(fshift_lat + m_trails.dif_lat);
  shift_lon = (int)(fshift_lon + m_trails.dif_lon);

  // Check for changes in the direction of movement, part of the image buffer has to be erased
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

  // save the rounding fraction and appy it next time
  m_trails.dif_lat = fshift_lat + m_trails.dif_lat - (double)shift_lat;
  m_trails.dif_lon = fshift_lon + m_trails.dif_lon - (double)shift_lon;

  if (shift_lat >= MARGIN || shift_lat <= -MARGIN || shift_lon >= MARGIN || shift_lon <= -MARGIN) {  // huge shift, reset trails
    ClearTrails();
    if (!m_pi->GetRadarPosition(&m_trails.lat, &m_trails.lon)) {
      m_trails.lat = 0.;
      m_trails.lon = 0.;
    }
    LOG_INFO(wxT("BR24radar_pi: %s Large movement trails reset"), m_name.c_str());
    return;
  }

  // offset lon too large: shift image
  if (abs(m_trails.offset.lon + shift_lon) >= MARGIN) {
    ShiftImageLonToCenter();
  }

  // offset lat too large: shift image in lat direction
  if (abs(m_trails.offset.lat + shift_lat) >= MARGIN) {
    ShiftImageLatToCenter();
  }
  // apply the shifts to the offset
  m_trails.offset.lat += shift_lat;
  m_trails.offset.lon += shift_lon;
}

// shifts the true trails image in lon direction to center
void RadarInfo::ShiftImageLonToCenter() {
  if (m_trails.offset.lon >= MARGIN || m_trails.offset.lon <= -MARGIN) {  // abs no good
    LOG_INFO(wxT("BR24radar_pi: offset lon too large %i"), m_trails.offset.lon);
    m_trails.offset.lon = 0;
    return;
  }
  if (m_trails.offset.lon > 0) {
    for (int i = 0; i < TRAILS_SIZE; i++) {
      memmove(&m_trails.true_trails[i][MARGIN], &m_trails.true_trails[i][MARGIN + m_trails.offset.lon], RETURNS_PER_LINE * 2);
      memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN], 0, MARGIN);
    }
  }
  if (m_trails.offset.lon < 0) {
    for (int i = 0; i < TRAILS_SIZE; i++) {
      memmove(&m_trails.true_trails[i][MARGIN], &m_trails.true_trails[i][MARGIN + m_trails.offset.lon], RETURNS_PER_LINE * 2);
      memset(&m_trails.true_trails[i][TRAILS_SIZE - MARGIN], 0, MARGIN);
      memset(&m_trails.true_trails[i][0], 0, MARGIN);
    }
  }
  m_trails.offset.lon = 0;
}

// shifts the true trails image in lat direction to center
void RadarInfo::ShiftImageLatToCenter() {
  if (m_trails.offset.lat >= MARGIN || m_trails.offset.lat <= -MARGIN) {  // abs not ok
    LOG_INFO(wxT("BR24radar_pi: offset lat too large %i"), m_trails.offset.lat);
    m_trails.offset.lat = 0;
  }

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
  m_trails.offset.lat = 0;
}

void RadarInfo::RenderGuardZone() {
  int start_bearing = 0, end_bearing = 0;
  GLubyte red = 0, green = 200, blue = 0, alpha = 50;

  for (size_t z = 0; z < GUARD_ZONES; z++) {
    if (m_guard_zone[z]->m_alarm_on || m_guard_zone[z]->m_arpa_on || m_guard_zone[z]->m_show_time + 5 > time(0)) {
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

  const RadarRange *range = m_range.GetRange();

  if (range) {
    if (range >= g_ranges_nautic && range < g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic)) {
      min = g_ranges_nautic;
      max = g_ranges_nautic + ARRAY_SIZE(g_ranges_nautic) - 1;
    } else if (range >= g_ranges_metric && range < g_ranges_metric + ARRAY_SIZE(g_ranges_metric)) {
      min = g_ranges_metric;
      max = g_ranges_metric + ARRAY_SIZE(g_ranges_metric) - 1;
    } else {
      return;
    }

    if (m_radar_type != RT_4G) {
      max--;  // only 4G has longest ranges
    }

    if (adjustment < 0 && range > min) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), range[0].meters, range[0].actual_meters,
                  range[-1].meters, range[-1].actual_meters);
      m_transmit->SetRange(range[-1].meters);
    } else if (adjustment > 0 && range < max) {
      LOG_VERBOSE(wxT("BR24radar_pi: Change radar range from %d/%d to %d/%d"), range[0].meters, range[0].actual_meters,
                  range[+1].meters, range[+1].actual_meters);
      m_transmit->SetRange(range[+1].meters);
    }
  }
}

void RadarInfo::SetAutoRangeMeters(int meters) {
  if (m_state.GetValue() == RADAR_TRANSMIT && m_auto_range_mode) {
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

bool RadarInfo::SetControlValue(ControlType controlType, int value, int autoValue) {
  return m_transmit->SetControlValue(controlType, value, autoValue);
}

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
    if (m_arpa) {
      m_arpa->ClearContours();
    }
    m_range_meters = 0;
  }
}

/**
 * plugin calls this to request a redraw of the PPI window.
 *
 * Called on GUI thread.
 */
void RadarInfo::RefreshDisplay() {
  if (IsPaneShown()) {
    m_radar_panel->Refresh(false);
  }
}

void RadarInfo::RenderRadarImage(DrawInfo *di) {
  wxCriticalSectionLocker lock(m_exclusive);
  int drawing_method = m_pi->m_settings.drawing_method;
  int state = m_state.GetValue();

  if (state != RADAR_TRANSMIT && state != RADAR_WAKING_UP) {
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
    wxLongLong startup_elapsed = wxGetUTCTimeMillis() - m_pi->GetBootMillis();
    LOG_INFO(wxT("BR24radar_pi: First radar image rendered after %llu ms\n"), startup_elapsed);
  }
}

int RadarInfo::GetOrientation() {
  int orientation;

  // check for no longer allowed value
  if (m_pi->GetHeadingSource() == HEADING_NONE) {
    orientation = ORIENTATION_HEAD_UP;
  } else {
    orientation = m_orientation.GetValue();
  }

  return orientation;
}

void RadarInfo::RenderRadarImage(wxPoint center, double scale, double overlay_rotate, bool overlay) {
  if (!m_range_meters) {
    return;
  }
  bool arpa_on = false;
  if (m_arpa) {
    for (int i = 0; i < GUARD_ZONES; i++) {
      if (m_guard_zone[i]->m_arpa_on) arpa_on = true;
    }
    if (m_arpa->GetTargetCount()) {
      arpa_on = true;
    }
  }

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);  // Save state
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  overlay_rotate += OPENGL_ROTATION;  // Difference between OpenGL and compass + radar
                                      // Note that for overlay=false this is purely OPENGL_ROTATION.

  double panel_rotate = overlay_rotate;
  double guard_rotate = overlay_rotate;
  double arpa_rotate;

  // So many combinations here

  int orientation = GetOrientation();
  int range = m_range.GetValue();

  if (!overlay) {
    arpa_rotate = 0.;
    switch (orientation) {
      case ORIENTATION_STABILIZED_UP:
        panel_rotate -= m_course;  // Panel only needs stabilized heading applied
        arpa_rotate -= m_course;
        guard_rotate += m_pi->GetHeadingTrue() - m_course;
        break;
      case ORIENTATION_COG_UP: {
        double cog = m_pi->GetCOG();
        panel_rotate -= cog;  // Panel only needs stabilized heading applied
        arpa_rotate -= cog;
        guard_rotate += m_pi->GetHeadingTrue() - cog;
      } break;
      case ORIENTATION_NORTH_UP:
        guard_rotate += m_pi->GetHeadingTrue();
        break;
      case ORIENTATION_HEAD_UP:
        arpa_rotate += -m_pi->GetHeadingTrue();  // Undo the actual heading calculation always done for ARPA
        break;
    }
  } else {
    guard_rotate += m_pi->GetHeadingTrue();
    arpa_rotate = overlay_rotate - OPENGL_ROTATION;
  }

  if (arpa_on) {
    m_arpa->RefreshArpaTargets();
  }

  if (overlay) {
    if (m_pi->m_settings.guard_zone_on_overlay) {
      glPushMatrix();
      glTranslated(center.x, center.y, 0);
      glRotated(guard_rotate, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);

      // LOG_DIALOG(wxT("BR24radar_pi: %s render guard zone on overlay"), m_name.c_str());
      RenderGuardZone();
      glPopMatrix();
    }

    double radar_pixels_per_meter = ((double)RETURNS_PER_LINE) / m_range_meters;
    double radar_scale = scale / radar_pixels_per_meter;
    glPushMatrix();
    glTranslated(center.x, center.y, 0);
    glRotated(panel_rotate, 0.0, 0.0, 1.0);
    glScaled(radar_scale, radar_scale, 1.);

    RenderRadarImage(&m_draw_overlay);
    glPopMatrix();

    if (arpa_on) {
      glPushMatrix();
      glTranslated(center.x, center.y, 0);
      LOG_VERBOSE(wxT("BR24radar_pi: %s render ARPA targets on overlay with rot=%f"), m_name.c_str(), arpa_rotate);

      glRotated(arpa_rotate, 0.0, 0.0, 1.0);
      glScaled(scale, scale, 1.);
      m_arpa->DrawArpaTargets();
      glPopMatrix();
    }

  } else if (range != 0) {
    wxStopWatch stopwatch;

    glPushMatrix();
    scale = 1.0 / range;
    glRotated(guard_rotate, 0.0, 0.0, 1.0);
    glScaled(scale, scale, 1.);
    RenderGuardZone();
    glPopMatrix();

    glPushMatrix();
    double overscan = (double)m_range_meters / (double)range;
    double radar_scale = overscan / RETURNS_PER_LINE;
    glScaled(radar_scale, radar_scale, 1.);
    glRotated(panel_rotate, 0.0, 0.0, 1.0);
    LOG_DIALOG(wxT("BR24radar_pi: %s render overscan=%g range=%d"), m_name.c_str(), overscan, range);
    RenderRadarImage(&m_draw_panel);
    glPopMatrix();

    if (arpa_on) {
      glPushMatrix();
      glScaled(scale, scale, 1.);
      glRotated(arpa_rotate, 0.0, 0.0, 1.0);
      m_arpa->DrawArpaTargets();
      glPopMatrix();
    }
    glFinish();
    m_draw_time_ms = stopwatch.Time();
  }

  glPopAttrib();
}

wxString RadarInfo::GetCanvasTextTopLeft() {
  wxString s;

  switch (GetOrientation()) {
    case ORIENTATION_HEAD_UP:
      s << _("Head Up");
      break;
    case ORIENTATION_STABILIZED_UP:
      s << _("Head Up") << wxT("\n") << _("Stabilized");
      break;
    case ORIENTATION_COG_UP:
      s << _("Course Up");
      break;
    case ORIENTATION_NORTH_UP:
      s << _("North Up");
      break;
    default:
      s << _("Unknown");
      break;
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

  int motion = m_trails_motion.GetValue();
  if (motion != TARGET_MOTION_OFF) {
    if (motion == TARGET_MOTION_TRUE) {
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
  if (GetOrientation() != ORIENTATION_HEAD_UP) {
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
  double radar_lat, radar_lon;
  wxString s = m_pi->GetGuardZoneText(this);

  if (m_state.GetValue() == RADAR_TRANSMIT) {
    double distance = 0.0, bearing = nan("");
    int orientation = GetOrientation();

    // Add VRM/EBLs

    for (int b = 0; b < BEARING_LINES; b++) {
      double bearing = m_ebl[orientation][b];
      if (!isnan(m_vrm[b]) && !isnan(bearing)) {
        if (orientation == ORIENTATION_STABILIZED_UP) {
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

    if (!isnan(m_mouse_vrm)) {
      distance = m_mouse_vrm;
      bearing = m_mouse_ebl[orientation];

      if (orientation == ORIENTATION_STABILIZED_UP) {
        bearing += m_course;
      } else if (orientation == ORIENTATION_COG_UP) {
        bearing += m_pi->GetCOG();
      }
      if (bearing >= 360) bearing -= 360;

    } else if (!isnan(m_mouse_lat) && !isnan(m_mouse_lon) && m_pi->GetRadarPosition(&radar_lat, &radar_lon)) {
      // Can't compute this upfront, ownship may move...
      distance = local_distance(radar_lat, radar_lon, m_mouse_lat, m_mouse_lon);
      bearing = local_bearing(radar_lat, radar_lon, m_mouse_lat, m_mouse_lon);
      if (GetOrientation() != ORIENTATION_NORTH_UP) {
        bearing -= m_pi->GetHeadingTrue();
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

  switch (m_state.GetValue()) {
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
  const RadarRange *r = m_range.GetRange();
  int meters = m_range.GetValue();

  if (!r) {
    m_range_text = wxT("");
    return m_range_text;
  }

  bool auto_range = m_auto_range_mode && (m_overlay.GetValue() > 0);

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
  const RadarRange *range = m_range.GetRange();

  if (range) {
    return (&range->name)[(idx + 1) % 4];
  }

  return 0;
}

void RadarInfo::SetMouseLatLon(double lat, double lon) {
  for (int i = 0; i < ORIENTATION_NUMBER; i++) {
    m_mouse_ebl[i] = NAN;
  }
  m_mouse_vrm = NAN;
  m_mouse_lat = lat;
  m_mouse_lon = lon;
  LOG_DIALOG(wxT("BR24radar_pi: SetMouseLatLon(%f, %f)"), lat, lon);
}

void RadarInfo::SetMouseVrmEbl(double vrm, double ebl) {
  double bearing;
  int orientation = GetOrientation();
  double cog = m_pi->GetCOG();

  m_mouse_vrm = vrm;
  switch (orientation) {
    case ORIENTATION_HEAD_UP:
    default:
      m_mouse_ebl[ORIENTATION_HEAD_UP] = ebl;
      bearing = ebl;
      break;
    case ORIENTATION_NORTH_UP:
      m_mouse_ebl[ORIENTATION_NORTH_UP] = ebl;
      m_mouse_ebl[ORIENTATION_STABILIZED_UP] = ebl - m_course;
      m_mouse_ebl[ORIENTATION_COG_UP] = ebl - cog;
      bearing = ebl;
      break;
    case ORIENTATION_STABILIZED_UP:
      m_mouse_ebl[ORIENTATION_NORTH_UP] = ebl + m_course;
      m_mouse_ebl[ORIENTATION_COG_UP] = ebl + m_course - cog;
      m_mouse_ebl[ORIENTATION_STABILIZED_UP] = ebl;
      bearing = ebl + m_pi->GetHeadingTrue();
      break;
    case ORIENTATION_COG_UP:
      m_mouse_ebl[ORIENTATION_NORTH_UP] = ebl + cog;
      m_mouse_ebl[ORIENTATION_STABILIZED_UP] = ebl + cog - m_course;
      m_mouse_ebl[ORIENTATION_COG_UP] = ebl;
      bearing = ebl + m_pi->GetHeadingTrue();
      break;
  }

  static double R = 6378.1e3 / 1852.;  // Radius of the Earth in nm
  double brng = deg2rad(bearing);
  double d = vrm;  // Distance in nm

  double lat1, lon1;
  if (m_pi->GetRadarPosition(&lat1, &lon1)) {
    lat1 = deg2rad(lat1);
    lon1 = deg2rad(lon1);

    double lat2 = asin(sin(lat1) * cos(d / R) + cos(lat1) * sin(d / R) * cos(brng));
    double lon2 = lon1 + atan2(sin(brng) * sin(d / R) * cos(lat1), cos(d / R) - sin(lat1) * sin(lat2));

    m_mouse_lat = rad2deg(lat2);
    m_mouse_lon = rad2deg(lon2);
    LOG_DIALOG(wxT("BR24radar_pi: SetMouseVrmEbl(%f, %f) = %f / %f"), vrm, ebl, m_mouse_lat, m_mouse_lon);
    if (m_control_dialog) {
      m_control_dialog->ShowCursorPane();
    }
  } else {
    m_mouse_lat = nan("");
    m_mouse_lon = nan("");
  }
}

void RadarInfo::SetBearing(int bearing) {
  int orientation = GetOrientation();
  double radar_lat, radar_lon;

  if (!isnan(m_vrm[bearing])) {
    m_vrm[bearing] = NAN;
    m_ebl[orientation][bearing] = NAN;
  } else if (!isnan(m_mouse_vrm)) {
    m_vrm[bearing] = m_mouse_vrm;
    for (int i = 0; i < ORIENTATION_NUMBER; i++) {
      m_ebl[i][bearing] = m_mouse_ebl[i];
    }
  } else if (!isnan(m_mouse_lat) && !isnan(m_mouse_lon) && m_pi->GetRadarPosition(&radar_lat, &radar_lon)) {
    m_vrm[bearing] = local_distance(radar_lat, radar_lon, m_mouse_lat, m_mouse_lon);
    m_ebl[orientation][bearing] = local_bearing(radar_lat, radar_lon, m_mouse_lat, m_mouse_lon);
  }
}

void RadarInfo::ClearTrails() {
  LOG_VERBOSE(wxT("BR24radar_pi: ClearTrails"));
  CLEAR_STRUCT(m_trails);
}

void RadarInfo::ComputeTargetTrails() {
  static TrailRevolutionsAge maxRevs[TRAIL_ARRAY_SIZE] = {
      SECONDS_TO_REVOLUTIONS(15),  SECONDS_TO_REVOLUTIONS(30),  SECONDS_TO_REVOLUTIONS(60), SECONDS_TO_REVOLUTIONS(180),
      SECONDS_TO_REVOLUTIONS(300), SECONDS_TO_REVOLUTIONS(600), TRAIL_MAX_REVOLUTIONS + 1};

  int target_trails = m_target_trails.GetValue();
  int trails_motion = m_trails_motion.GetValue();

  TrailRevolutionsAge maxRev = maxRevs[target_trails];
  if (trails_motion == 0) {
    maxRev = 0;
  }
  TrailRevolutionsAge revolution;
  double coloursPerRevolution = 0.;
  double colour = 0.;

  // Like plotter, continuous trails are all very white (non transparent)
  if ((trails_motion > 0) && (target_trails < TRAIL_CONTINUOUS)) {
    coloursPerRevolution = BLOB_HISTORY_COLOURS / (double)maxRev;
  }

  LOG_VERBOSE(wxT("BR24radar_pi: Target trail value %d = %d revolutions"), target_trails, maxRev);

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
