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

#include "RadarInfo.h"
#include "drawutil.h"
#include "br24Receive.h"
#include "br24Transmit.h"

void radar_control_item::Update(int v)
{
    if (v != button) {
        mod = true;
        button = v;
    }
};

RadarInfo::RadarInfo( br24radar_pi *pi, wxString name, int radar )
{
    this->m_pi = pi;
    this->name = name;
    this->radar = radar;

    this->radar_type = RT_UNKNOWN;
    this->auto_range_mode = true;
    this->control_box_closed = false;
    this->control_box_opened = false;

    this->transmit = new br24Transmit(name, radar);
    this->receive = 0;
    this->draw = 0;

    for (size_t z = 0; z < GUARD_ZONES; z++) {
        guard_zone[z] = new GuardZone(pi);
    }

    m_quit = false;
}

RadarInfo::~RadarInfo( )
{
    m_quit = true;

    delete transmit;
    if (receive) {
        receive->Wait();
        delete receive;
        wxLogMessage(wxT("BR24radar_pi: %s thread stopped"), name);
    }
    if (radar_frame) {
        m_pi->m_dialogLocation[DL_RADARWINDOW + radar].pos = radar_frame->GetPosition();
        m_pi->m_dialogLocation[DL_RADARWINDOW + radar].size = radar_frame->GetSize();
        delete radar_frame;
    }
    if (draw) {
        delete draw;
    }
    for (size_t z = 0; z < GUARD_ZONES; z++) {
        delete guard_zone[z];
    }
}

bool RadarInfo::Init( int verbose )
{
    bool succeeded;

    m_verbose = verbose;

    succeeded = transmit->Init(verbose);
    // succeeded &= some other init
    // succeeded &= some other init

    return succeeded;
}

void RadarInfo::SetName( wxString name )
{
    this->name = name;
}

void RadarInfo::StartReceive( )
{
    if (!receive) {
        wxLogMessage(wxT("BR24radar_pi: Starting receive thread for %s"), name.c_str());
        receive = new br24Receive(m_pi, &m_quit, this);
        receive->Run();
    }
}

void RadarInfo::ResetSpokes()
{
    if (draw) {
        UINT8 zap[RETURNS_PER_LINE];
        memset(zap, 0, sizeof(zap));
        for (size_t r = 0; r < LINES_PER_ROTATION; r++) {
            draw->ProcessRadarSpoke(r, zap, sizeof(zap));
        }
    }

    for (size_t z = 0; z < GUARD_ZONES; z++) {
        // Zap them anyway just to be sure
        guard_zone[z]->ResetBogeys();
    }
}

/*
 * A spoke of data has been received by the receive thread and it calls this (in the context of
 * the receive thread, so no UI actions can be performed here.)
 *
 * @param angle                 Bearing at which the spoke is seen.
 * @param data                  A line of len bytes, each byte represents strength at that distance.
 * @param len                   Number of returns
 * @param range                 Range (in meters) of this data
 * @param nowMillis             Timestamp when this was received
 */
void RadarInfo::ProcessRadarSpoke( SpokeBearing angle, UINT8 * data, size_t len, int range_meters, wxLongLong nowMillis )
{
    UINT8 * hist_data = history[angle];
    bool calc_history = multi_sweep_filter;

    if (this->range_meters != range_meters) {
        // Wipe ALL spokes
        ResetSpokes();
        this->range_meters = range_meters;
    }
    //spoke[angle].age = nowMillis;

    if (!calc_history) {
        for (size_t z = 0; z < GUARD_ZONES; z++) {
            if (guard_zone[z]->type != GZ_OFF && guard_zone[z]->multi_sweep_filter) {
                calc_history = true;
            }
        }
    }

    if (calc_history) {
        for (size_t radius = 0; radius < len; radius++) {
            hist_data[radius] = hist_data[radius] << 1;     // shift left history byte 1 bit
            if (m_pi->m_color_map[data[radius]] != BLOB_NONE) {
                hist_data[radius] = hist_data[radius] | 1;    // and add 1 if above threshold
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
                data[radius] = 0; // Zero out data here, so draw doesn't need to know about filtering
            }
        }
    }

    if (draw) {
        draw->ProcessRadarSpoke(angle, data, len);
    }

}

void RadarInfo::ProcessRadarPacket( time_t now )
{
    if (radar_frame && radar_frame->IsShown()) {
        radar_frame->Refresh(false);
    }

    if (m_pi->m_settings.chart_overlay == this->radar) {
        int pos_age = difftime(now, m_pi->m_bpos_watchdog);   // the age of the postion, last call of SetPositionFixEx
        if (m_pi->m_refresh_busy_or_queued || pos_age >= 2 ) {
            // don't do additional refresh and reset the refresh conter
            m_refresh_countdown = m_pi->m_refresh_rate;  // rendering ongoing, reset the counter, don't refresh now
            // this will also balance performance, if too busy skip refresh
            // pos_age>=2 : OCPN too busy to pass position to pi, system overloaded
            // so skip next refresh
            if (m_verbose >= 2) {
                wxLogMessage(wxT("BR24radar_pi: busy encountered, pos_age = %d, refresh_busy_or_queued=%d"), pos_age, m_pi->m_refresh_busy_or_queued);
            }
        }
        else {
            m_refresh_countdown--;
            if (m_refresh_countdown <= 0) {       // display every "refreshrate time"
                if (m_pi->m_refresh_rate != 10) { // for 10 no refresh at all
                    m_pi->m_refresh_busy_or_queued = true;   // no further calls until br_refresh_busy_or_queued has been cleared by RenderGLOverlay
                    // Very important to pass "false" to refresh for high refresh rate
                    // radar so that opencpn doesn't invalidate the cached chart image
                    GetOCPNCanvasWindow()->Refresh(false);
                    if (m_verbose >= 4) {
                        wxLogMessage(wxT("BR24radar_pi: refresh issued"));
                    }
                }
                m_refresh_countdown = m_pi->m_refresh_rate;
            }
        }
    }
}

void RadarInfo::RenderGuardZone(wxPoint radar_center, double v_scale_ppm)
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
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
                start_bearing = guard_zone[z]->start_bearing;
                end_bearing = guard_zone[z]->end_bearing;
            }
            switch (m_pi->m_settings.guard_zone_render_style) {
                case 1:
                    glColor4ub((GLubyte)255, (GLubyte)0, (GLubyte)0, (GLubyte)255);
                    DrawOutlineArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing, end_bearing, true);
                    break;
                case 2:
                    glColor4ub(red, green, blue, alpha);
                    DrawOutlineArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing, end_bearing, false);
                    // fall thru
                default:
                    glColor4ub(red, green, blue, alpha);
                    DrawFilledArc(guard_zone[z]->outer_range * v_scale_ppm, guard_zone[z]->inner_range * v_scale_ppm, start_bearing, end_bearing);
            }
        }

        red = 0; green = 0; blue = 200;
    }

    glPopAttrib();
}

void RadarInfo::SetRangeMeters(int meters)
{
    if (radar_seen) {
        if (transmit->SetRange(meters)) {
            commanded_range_meters = meters;
        }
    }
}

bool RadarInfo::SetControlValue( ControlType controlType, int value )
{
    return transmit->SetControlValue(controlType, value);
}

void RadarInfo::ShowRadarWindow( )
{
    if (!radar_frame) {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        radar_frame = new wxFrame(0, -1,  wxT("RADAR"), m_pi->m_dialogLocation[DL_RADARWINDOW + radar].pos, m_pi->m_dialogLocation[DL_RADARWINDOW + radar].size);

        int args[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0};

        RadarWindow * radarWindow = new RadarWindow(this, radar_frame, args);
        sizer->Add(radarWindow, 1, wxEXPAND);
        radar_frame->SetSizer(sizer);
        radar_frame->SetAutoLayout(true);
    }
    radar_frame->Show();
}

// vim: sw=4:ts=8:
