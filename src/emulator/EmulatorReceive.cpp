/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Hakan Svensson
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

#include "EmulatorReceive.h"

PLUGIN_BEGIN_NAMESPACE

/*
 * This file not only contains the radar receive threads, it is also
 * the only unit that understands what the radar returned data looks like.
 * The rest of the plugin uses a (slightly) abstract definition of the radar.
 */

#define MILLIS_PER_SELECT 250
#define SECONDS_SELECT(x) ((x)*MILLISECONDS_PER_SECOND / MILLIS_PER_SELECT)

/*
 * Called once a second. Emulate a radar return that is
 * at the current desired auto_range.
 * Speed is 24 images per minute, e.g. 1/2.5 of a full
 * image.
 */

void EmulatorReceive::EmulateFakeBuffer(void) {
  time_t now = time(0);
  UINT8 data[RETURNS_PER_LINE];

  m_ri->m_radar_timeout = now + WATCHDOG_TIMEOUT;

  int state = m_ri->m_state.GetValue();

  if (state != RADAR_TRANSMIT) {
    if (state == RADAR_OFF) {
      m_ri->m_state.Update(RADAR_STANDBY);
    }
    return;
  }

  m_ri->m_statistics.packets++;
  m_ri->m_data_timeout = now + WATCHDOG_TIMEOUT;

  m_next_rotation = (m_next_rotation + 1) % SPOKES;

  int scanlines_in_packet = SPOKES * 24 / 60 * MILLIS_PER_SELECT / MILLISECONDS_PER_SECOND;
  int range_meters = 2308;
  int display_range_meters = 3000;
  int spots = 0;
#ifdef TODO
  m_ri->m_radar_type = RT_EMULATOR;  // Fake for emulator
  m_pi->m_pMessageBox->SetRadarType(RT_EMULATOR);
#endif
  m_ri->m_range.Update(display_range_meters);

  for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
    int angle_raw = m_next_spoke;
    m_next_spoke = (m_next_spoke + 1) % SPOKES;
    m_ri->m_statistics.spokes++;

    // Invent a pattern. Outermost ring, then a square pattern
    for (size_t range = 0; range < sizeof(data); range++) {
      size_t bit = range >> 7;
      // use bit 'bit' of angle_raw
      UINT8 colour = (((angle_raw + m_next_rotation) >> 5) & (2 << bit)) > 0 ? (range / 2) : 0;
      if (range > sizeof(data) - 10) {
        colour = ((angle_raw + m_next_rotation) % SPOKES) <= 8 ? 255 : 0;
      }
      data[range] = colour;
      if (colour > 0) {
        spots++;
      }
    }

    int hdt_raw = SCALE_DEGREES_TO_RAW(m_pi->GetHeadingTrue());
    int bearing_raw = angle_raw + hdt_raw;
    bearing_raw += SCALE_DEGREES_TO_RAW(270);  // Compensate openGL rotation compared to North UP

    SpokeBearing a = MOD_ROTATION2048(angle_raw / 2);    // divide by 2 to map on 2048 scanlines
    SpokeBearing b = MOD_ROTATION2048(bearing_raw / 2);  // divide by 2 to map on 2048 scanlines
    wxLongLong time_rec;
    double lat = 0.;
    double lon = 0.;
    m_ri->ProcessRadarSpoke(a, b, data, sizeof(data), range_meters, time_rec, lat, lon);
  }

  LOG_VERBOSE(wxT("radar_pi: emulating %d spokes at range %d with %d spots"), scanlines_in_packet, range_meters, spots);
}

/*
 * Entry
 *
 * Called by wxThread when the new thread is running.
 * It should remain running until Shutdown is called.
 */
void *EmulatorReceive::Entry(void) {
  int r = 0;

  LOG_VERBOSE(wxT("radar_pi: EmulatorReceive thread %s starting"), m_ri->m_name.c_str());

  while (!m_shutdown) {
    struct timeval tv = {(long)0, (long)(MILLIS_PER_SELECT * 1000)};

    fd_set fdin;
    FD_ZERO(&fdin);

    r = select(0, &fdin, 0, 0, &tv);

    EmulateFakeBuffer();

  }  // endless loop until thread destroy

  LOG_VERBOSE(wxT("radar_pi: %s receive thread stopping"), m_ri->m_name.c_str());
  return 0;
}

// Called from the main thread to stop this thread.
// We send a simple one byte message to the thread so that it awakens from the select() call with
// this message ready for it to be read on 'm_receive_socket'. See the constructor in EmulatorReceive.h
// for the setup of these two sockets.

void EmulatorReceive::Shutdown() {
  LOG_INFO(wxT("radar_pi: %s receive thread will take long time to stop"), m_ri->m_name.c_str());
  m_shutdown = true;
}

PLUGIN_END_NAMESPACE
