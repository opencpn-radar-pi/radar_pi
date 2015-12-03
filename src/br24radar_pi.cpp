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

#include "br24radar_pi.h"

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin * create_pi( void *ppimgr )
{
    return new br24radar_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi( opencpn_plugin* p )
{
    delete p;
}


/********************************************************************************************************/
//   Distance measurement for simple sphere
/********************************************************************************************************/

static double local_distance( double lat1, double lon1, double lat2, double lon2 )
{
    // Spherical Law of Cosines
    double theta, dist;

    theta = lon2 - lon1;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);        // radians
    dist = rad2deg(dist);
    dist = fabs(dist) * 60    ;    // nautical miles/degree
    return (dist);
}

static double radar_distance( double lat1, double lon1, double lat2, double lon2, char unit )
{
    double dist = local_distance (lat1, lon1, lat2, lon2);

    switch (unit) {
        case 'M':              // statute miles
            dist = dist * 1.1515;
            break;
        case 'K':              // kilometers
            dist = dist * 1.852;
            break;
        case 'm':              // meters
            dist = dist * 1852.0;
            break;
        case 'N':              // nautical miles
            break;
    }
    return dist;
}

//---------------------------------------------------------------------------------------------------------
//
//    BR24Radar PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"
//#include "default_pi.xpm"
#include "icons.cpp"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

br24radar_pi::br24radar_pi( void *ppimgr ) : opencpn_plugin_110( ppimgr )
{
    // Create the PlugIn icons
    initialize_images();
    m_pdeficon = new wxBitmap(*_img_radar_blank);
}

int br24radar_pi::Init( void )
{
#ifdef __WXMSW__
    WSADATA wsaData;

    // Initialize Winsock
    DWORD r = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (r != 0) {
        wxLogError(wxT("BR24radar_pi: Unable to initialise Windows Sockets, error %d"), r);
        // Might as well give up now
        return 0;
    }
#endif

    AddLocaleCatalog( _T("opencpn-br24radar_pi") );

    m_font = *OCPNGetFont(_("Dialog"), 12);
    m_fat_font = m_font;
    m_fat_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_fat_font.SetPointSize(m_font.GetPointSize() + 1);

    m_pMessageBox = NULL;

    m_refresh_rate = 1;

    m_scanner_state = RADAR_OFF;                 // Radar scanner is off
    m_var = 0.0;
    m_var_source = VARIATION_SOURCE_NONE;
    m_heading_on_radar = false;
    m_refresh_busy_or_queued = false;
    m_bpos_set = false;
    m_auto_range_meters = 0;
    m_previous_auto_range_meters = 0;
    m_update_address_control = false;
    m_update_error_control = false;
    m_idle_dialog_time_left = 999; // Secret value, I hate secret values!
    m_TimedTransmit_IdleBoxMode = 0;
    m_idle_time_left = 0;
    m_guard_bogey_confirmed = false;
    m_want_message_box = false;

    m_dt_stayalive = time(0);
    m_alarm_sound_last = m_dt_stayalive;
    m_bpos_watchdog = 0;
    m_hdt_watchdog  = 0;
    m_var_watchdog = 0;
    m_idle_watchdog = 0;
    memset(m_bogey_count, 0, sizeof(m_bogey_count));   // set bogey count 0
    memset(m_dialogLocation, 0, sizeof(m_dialogLocation));

    m_radar[0] = new RadarInfo(this, _("Radar"), 0);
    m_radar[1] = new RadarInfo(this, _("Radar B"), 1);

    m_ptemp_icon = NULL;
    m_sent_bm_id_normal = -1;
    m_sent_bm_id_rollover =  -1;

    m_heading_source = HEADING_NONE;
    m_pOptionsDialog = 0;
    m_pMessageBox = 0;
    m_pGuardZoneDialog = 0;
    m_pGuardZoneBogey = 0;
    m_pIdleDialog = 0;

    m_settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;
    m_settings.refreshrate = 1;
    m_settings.timed_idle = 0;

    ::wxDisplaySize(&m_display_width, &m_display_height);

    //****************************************************************************************
    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
    if (LoadConfig()) {
        wxLogMessage(wxT("BR24radar_pi: Configuration file values initialised"));
        wxLogMessage(wxT("BR24radar_pi: Log verbosity = %d (to modify, set VerboseLog to 0..4)"), m_settings.verbose);
    }
    else {
        wxLogMessage(wxT("BR24radar_pi: configuration file values initialisation failed"));
        return 0; // give up
    }
    ComputeColorMap();

    for (size_t r = 0; r < RADARS; r++) {
        if (!m_radar[r]->Init(m_settings.verbose)) {
            wxLogMessage(wxT("BR24radar_pi: initialisation failed"));
            return 0;
        }
    }

    // Get a pointer to the opencpn display canvas, to use as a parent for the UI dialog
    m_parent_window = GetOCPNCanvasWindow();

    //    This PlugIn needs a toolbar icon

    m_tool_id  = InsertPlugInTool(wxT(""), _img_radar_red, _img_radar_red, wxITEM_NORMAL,
                                  wxT("BR24Radar"), wxT(""), NULL,
                                  BR24RADAR_TOOL_POSITION, 0, this);

    CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);

    // Context Menu Items (Right click on chart screen)

    m_pmenu = new wxMenu();            // this is a dummy menu
    // required by Windows as parent to item created
    wxMenuItem *pmi = new wxMenuItem(m_pmenu, -1, _("Radar Control..."));
#ifdef __WXMSW__
    wxFont *qFont = OCPNGetFont(_("Menu"), 10);
    pmi->SetFont(*qFont);
#endif
    int radar_control_id = AddCanvasContextMenuItem(pmi, this);
    SetCanvasContextMenuItemViz(radar_control_id, true);

    ShowRadarControl(0, m_settings.show_radar == RADAR_ON);
    ShowRadarControl(1, m_settings.show_radar == RADAR_ON);

    return (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_OVERLAY_CALLBACK     |
            WANTS_TOOLBAR_CALLBACK     |
            INSTALLS_TOOLBAR_TOOL      |
            INSTALLS_CONTEXTMENU_ITEMS |
            USES_AUI_MANAGER           |
            WANTS_CONFIG               |
            WANTS_NMEA_EVENTS          |
            WANTS_NMEA_SENTENCES       |
            WANTS_PREFERENCES          |
            WANTS_PLUGIN_MESSAGING
            );

}

bool br24radar_pi::DeInit( void )
{
    OnControlDialogClose(m_radar[0]);
    OnControlDialogClose(m_radar[1]);
    OnMessageBoxClose();

    if (m_pGuardZoneDialog) {
        m_pGuardZoneDialog->Close();
    }
    if (m_pGuardZoneBogey) {
        delete m_pGuardZoneBogey;
    }

    ShowRadarControl(0, false);
    ShowRadarControl(1, false);
    m_radar[0]->ShowRadarWindow(false);
    m_radar[1]->ShowRadarWindow(false);

    SaveConfig();

    for (int r = 0; r < RADARS; r++) {
        if (m_radar[r]) {
            delete m_radar[r];
        }
    }

    return true;
}

int br24radar_pi::GetAPIVersionMajor()
{
    return MY_API_VERSION_MAJOR;
}

int br24radar_pi::GetAPIVersionMinor()
{
    return MY_API_VERSION_MINOR;
}

int br24radar_pi::GetPlugInVersionMajor()
{
    return PLUGIN_VERSION_MAJOR;
}

int br24radar_pi::GetPlugInVersionMinor()
{
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *br24radar_pi::GetPlugInBitmap()
{
    return m_pdeficon;
}

wxString br24radar_pi::GetCommonName()
{
    return wxT("BR24Radar");
}


wxString br24radar_pi::GetShortDescription()
{
    return _("Navico Radar PlugIn for OpenCPN");
}


wxString br24radar_pi::GetLongDescription()
{
    return _("Navico Broadband BR24/3G/4G Radar PlugIn for OpenCPN\n");
}

void br24radar_pi::SetDefaults( void )
{
    // This will be called upon enabling a PlugIn via the user Dialog.
    // We don't need to do anything special here.
}

void br24radar_pi::ShowPreferencesDialog( wxWindow* parent )
{
    m_pOptionsDialog = new br24OptionsDialog;
    m_pOptionsDialog->Create(m_parent_window, this);
    m_pOptionsDialog->ShowModal();
}

void br24radar_pi::UpdateAuiStatus( void )
{
    //    This method is called after the PlugIn is initialized
    //    and the frame has done its initial layout, possibly from a saved wxAuiManager "Perspective"
    //    It is a chance for the PlugIn to syncronize itself internally with the state of any Panes that
    //    were added to the frame in the PlugIn ctor.

    // Walk all wxAuiPane
#if 0
    for (size_t i = 0; i < RADARS; i++ ) {
        DashboardWindowContainer *cont = m_ArrayOfDashboardWindow.Item( i );
        wxAuiPaneInfo &pane = m_pauimgr->GetPane( cont->m_pDashboardWindow );
        // Initialize visible state as perspective is loaded now
        cont->m_bIsVisible = ( pane.IsOk() && pane.IsShown() );
    }
#endif

    // Maybe update toolbar button?

}

//********************************************************************************
// Operation Dialogs - Control, Manual, and Options

void br24radar_pi::ShowRadarControl( int radar, bool show )
{
    if (m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: ShowRadarControl(%d, %d)"), radar, (int) show);
    }

    m_radar[radar]->control_box_opened = show;
    if (show) {
        m_radar[radar]->control_box_closed = false;
        if (!m_pMessageBox) {
            m_pMessageBox = new br24MessageBox;
            m_pMessageBox->Create(m_parent_window, this);
            m_pMessageBox->SetPosition(m_dialogLocation[DL_MESSAGE].pos);
            m_pMessageBox->Fit();
            m_pMessageBox->Hide();
        }
        if (!m_radar[radar]->control_dialog) {
            m_radar[radar]->control_dialog = new br24ControlsDialog;
            m_radar[radar]->control_dialog->Create(m_parent_window, this, m_radar[radar]);
            m_radar[radar]->control_dialog->SetPosition(m_dialogLocation[DL_CONTROL + radar].pos);
            m_radar[radar]->control_dialog->Fit();
            m_radar[radar]->control_dialog->Hide();
            int range = m_radar[radar]->range_meters;
            int idx = convertMetersToRadarAllowedValue(&range, m_settings.range_units, m_radar[radar]->radar_type);
            m_radar[radar]->control_dialog->SetRangeIndex(idx);
            m_radar[radar]->range.Update(idx);
            m_radar[radar]->control_dialog->Show();
        }
    }

    m_radar[radar]->UpdateControlState(true);
    m_pMessageBox->UpdateMessage(m_opengl_mode
        , m_bpos_set
        , m_heading_source != HEADING_NONE
        , m_var_source != VARIATION_SOURCE_NONE
        , m_radar[radar]->radar_seen
        , m_radar[radar]->data_seen
        );

    if (!show && m_radar[radar]->control_dialog) {
        m_radar[radar]->control_dialog->Hide();
    }
}

void br24radar_pi::OnContextMenuItemCallback( int id )
{
    ShowRadarControl(0, true);
    /* TODO show radar B? */
}

void br24radar_pi::OnControlDialogClose( RadarInfo * ri )
{
    if (ri->control_dialog) {
        m_dialogLocation[DL_CONTROL + ri->radar].pos = ri->control_dialog->GetPosition();
        ri->control_dialog->Hide();
    }
    ri->control_box_closed = true;
    ri->control_box_opened = false;
}

void br24radar_pi::OnMessageBoxClose()
{
    if (m_pMessageBox) {
        m_dialogLocation[DL_MESSAGE].pos = m_pMessageBox->GetPosition();
        m_pMessageBox->Hide();
    }
}

void br24radar_pi::OnGuardZoneDialogClose( RadarInfo * ri )
{
    if (m_pGuardZoneDialog) {
        m_pGuardZoneDialog->Hide();
        m_guard_bogey_confirmed = false;
        SaveConfig(); // Extra so that if O crashes we don't lose the config
    }
    if (ri->control_dialog) {
        ri->control_dialog->UpdateGuardZoneState();
        if (!ri->control_box_closed) {
            ri->control_dialog->Show();
        }
    }
}

void br24radar_pi::OnGuardZoneBogeyConfirm()
{
    m_guard_bogey_confirmed = true; // This will stop the sound being repeated
}

void br24radar_pi::OnGuardZoneBogeyClose()
{
    m_guard_bogey_confirmed = true; // This will stop the sound being repeated
    if (m_pGuardZoneBogey) {
        m_dialogLocation[DL_BOGEY].pos = m_pGuardZoneBogey->GetPosition();
        m_pGuardZoneBogey->Hide();
    }
}

void br24radar_pi::ShowGuardZoneDialog( int radar, int zone )
{
    if (!m_pGuardZoneDialog) {
        m_pGuardZoneDialog = new GuardZoneDialog;
        m_pGuardZoneDialog->Create(m_parent_window, this, wxID_ANY, _(" Guard Zone Control"), m_dialogLocation[DL_CONTROL].pos);
    }
    if (zone >= 0) {
        m_dialogLocation[DL_CONTROL + radar].pos = m_radar[radar]->control_dialog->GetPosition();
        m_pGuardZoneDialog->Show();
        m_radar[radar]->control_dialog->Hide();
        m_pGuardZoneDialog->SetPosition(m_dialogLocation[DL_CONTROL + radar].pos);
        m_pGuardZoneDialog->OnGuardZoneDialogShow(m_radar[radar], zone);
    }
    else {
        m_pGuardZoneDialog->Hide();
    }
}

void br24radar_pi::ComputeColorMap( )
{
    switch (m_settings.display_option)
    {
    case 0:
        for (int i = 0; i <= UINT8_MAX; i++) {
            m_color_map[i] = (i >= m_settings.threshold_blue) ? BLOB_RED : BLOB_NONE;
        }
        break;
    case 1:
        for (int i = 0; i <= UINT8_MAX; i++) {
            m_color_map[i] = (i >= m_settings.threshold_red) ? BLOB_RED :
                             (i >= m_settings.threshold_green) ? BLOB_GREEN :
                             (i >= m_settings.threshold_blue) ? BLOB_BLUE : BLOB_NONE;
        }
        break;
    }

    memset(m_color_map_red, 0, sizeof(m_color_map_red));
    memset(m_color_map_green, 0, sizeof(m_color_map_green));
    memset(m_color_map_blue, 0, sizeof(m_color_map_blue));
    m_color_map_red[BLOB_RED] = 255;
    m_color_map_green[BLOB_GREEN] = 255;
    m_color_map_blue[BLOB_BLUE] = 255;
}

void br24radar_pi::UpdateDisplayParameters( void )
{
    m_parent_window->Refresh(false);
}


//*******************************************************************************
// ToolBar Actions

int br24radar_pi::GetToolbarToolCount( void )
{
    return 1;
}

/*
 * The radar icon is clicked. In previous versions all sorts of behavior was linked to clicking
 * on the button, which wasn't very 'discoverable'. So now it just shows/hides all radar
 * windows.
 */
void br24radar_pi::OnToolbarToolCallback( int id )
{
    if (m_settings.show_radar == RADAR_ON) {
        m_settings.show_radar = RADAR_OFF;
        for (int r = 0; r < RADARS; r++) {
            ShowRadarControl(r, false);
            m_radar[r]->ShowRadarWindow(false);
        }
    } else {
        m_settings.show_radar = RADAR_ON;
        ShowRadarControl(0, true);
        m_radar[0]->ShowRadarWindow(true);
        if (m_settings.enable_dual_radar) {
            ShowRadarControl(1, true);
            m_radar[1]->ShowRadarWindow(true);
        }
    }

/*
    if (m_pMessageBox) {
        m_pMessageBox->UpdateMessage(m_opengl_mode
            , m_bpos_set
            , m_heading_source != HEADING_NONE
            , m_var_source != VARIATION_SOURCE_NONE
            , m_radar[0]->radar_seen || m_radar[1]->radar_seen
            , m_radar[0]->data_seen || m_radar[1]->data_seen
            );
    }
    if (m_pGuardZoneBogey) {
        m_pGuardZoneBogey->Hide();
    }
*/
    UpdateState();
}

// DoTick
// ------
// Called on every RenderGLOverlay call, i.e. once a second.
//
// This checks if we need to ping the radar to keep it alive (or make it alive)
//*********************************************************************************
// Keeps Radar scanner on line if master and radar on -  run by RenderGLOverlay

void br24radar_pi::DoTick( void )
{
    time_t now = time(0);
    static time_t previousTicks = 0;

    if (m_radar[0]->radar_type == RT_BR24) {
        m_settings.enable_dual_radar = 0;
    }

    if (m_update_error_control) {
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                m_pMessageBox->SetErrorMessage(m_error_msg);
                m_update_error_control = false;
            }
        }
    }
    if (m_update_address_control) {
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                m_pMessageBox->SetMcastIPAddress(m_ip_address);
                m_update_address_control = false;
            }
        }
    }

    if (m_settings.verbose >= 2) {
        static time_t refresh_indicator = 0;
        static int performance_counter = 0;
        performance_counter++;
        if (now - refresh_indicator >= 1) {
            refresh_indicator = now;
            wxLogMessage(wxT("BR24radar_pi: number of refreshes last second = %d"), performance_counter);
            performance_counter = 0;
        }
    }
    if (now == previousTicks) {
        // Repeated call during scroll, do not do Tick processing
        return;
    }
    previousTicks = now;

    if (m_settings.show_radar == RADAR_ON) {
        if (!m_radar[0]->receive) {
            m_radar[0]->StartReceive();
        }
        if (!m_radar[1]->receive && m_settings.enable_dual_radar && m_radar[0]->radar_type == RT_4G) {
            m_radar[1]->StartReceive();
            m_radar[0]->SetName(_("Radar A"));
        }
    }

    m_refresh_rate = REFRESHMAPPING[m_settings.refreshrate - 1];  // set actual refresh rate

    if (m_bpos_set && TIMER_ELAPSED(now, m_bpos_watchdog)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is continuously reset every time we receive a heading.
        m_bpos_set = false;
        wxLogMessage(wxT("BR24radar_pi: Lost Boat Position data"));
    }

    if (m_heading_source != HEADING_NONE && TIMER_ELAPSED(now, m_hdt_watchdog)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is continuously reset every time we receive a heading
        m_heading_source = HEADING_NONE;
        wxLogMessage(wxT("BR24radar_pi: Lost Heading data"));
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                wxString info = wxT("");
                m_pMessageBox->SetHeadingInfo(info);
            }
        }
    }

    if (m_var_source != VARIATION_SOURCE_NONE && TIMER_ELAPSED(now, m_var_watchdog)) {
        m_var_source = VARIATION_SOURCE_NONE;
        wxLogMessage(wxT("BR24radar_pi: Lost Variation source"));
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                wxString info = wxT("");
                m_pMessageBox->SetVariationInfo(info);
            }
        }
    }

#if TODO
    // This area is stil a mess, not sure what was intended here
    for (size_t r = 0; r < RADARS; r++) {
        if (m_radar[r]->radar_seen) {
            if (TIMER_ELAPSED(now, m_radar[r].radar_watchdog)) {
                m_radar[r]->radar_seen = false;
                m_radar[r]->state.Update(RADAR_OFF);
                wxLogMessage(wxT("BR24radar_pi: Lost %s presence"), m_radar[r]->name);
            }
            else (m_radar[0]->radar_seen) {
                if (m_radar[0]->transmit.RadarStayAlive()) {      // send stay alive to obtain control blocks from radar
                    m_previous_radar_seen = true;
                }
            }
#endif

    bool any_data_seen = false;
    for (size_t r = 0; r < RADARS; r++) {
        any_data_seen |= m_radar[r]->data_seen;
    }
    if (any_data_seen) { // Something coming from radar unit?
        m_scanner_state = RADAR_ON;
        if (m_settings.show_radar == RADAR_ON) {   // if not, radar will time out and go standby
            if (TIMER_ELAPSED(now, m_dt_stayalive)) {
                m_dt_stayalive = now + STAYALIVE_TIMEOUT;
                m_radar[0]->transmit->RadarStayAlive();
                if (m_settings.enable_dual_radar) {
                    m_radar[1]->transmit->RadarStayAlive();
                }
            }
        }
    }
    else {
        m_scanner_state = RADAR_OFF;
        m_heading_on_radar = false;
    }

    if (m_settings.pass_heading_to_opencpn && m_heading_on_radar) {
        wxString nmeastring;
        nmeastring.Printf(_T("$APHDT,%05.1f,M\r\n"), m_hdt);
        PushNMEABuffer(nmeastring);
    }

    if ((m_pMessageBox && m_pMessageBox->IsShown()) || (m_settings.verbose >= 1)) {
        wxString t;
        for (size_t r = 0; r < RADARS; r++) {
            if (m_radar[r]->data_seen) {
                t << wxString::Format(wxT("%s\npackets %d/%d\nspokes %d/%d/%d\n")
                                     , m_radar[r]->name
                                     , m_radar[r]->statistics.packets
                                     , m_radar[r]->statistics.broken_packets
                                     , m_radar[r]->statistics.spokes
                                     , m_radar[r]->statistics.broken_spokes
                                     , m_radar[r]->statistics.missing_spokes);
            }
        }
        if (m_pMessageBox && m_pMessageBox->IsShown()) {
            m_pMessageBox->SetRadarInfo(t);
        }
        if (m_settings.verbose >= 1) {
            t.Replace(wxT("\n"), wxT(" "));
            wxLogMessage(wxT("BR24radar_pi: %s"), t.c_str());
        }
    }

    for (size_t r = 0; r < RADARS; r++) {
        m_radar[r]->UpdateControlState(false);

        if (m_radar[r]->data_seen) {
            m_radar[r]->ShowRadarWindow();
        }
    }

    if (m_pMessageBox) {
        m_pMessageBox->UpdateMessage(m_opengl_mode
            , m_bpos_set
            , m_heading_source != HEADING_NONE
            , m_var_source != VARIATION_SOURCE_NONE
            , m_radar[0]->radar_seen || m_radar[1]->radar_seen
            , m_radar[0]->data_seen || m_radar[1]->data_seen
            );
    }

    for (int r = 0; r < RADARS; r++) {
        m_radar[r]->statistics.broken_packets = 0;
        m_radar[r]->statistics.broken_spokes = 0;
        m_radar[r]->statistics.missing_spokes = 0;
        m_radar[r]->statistics.packets = 0;
        m_radar[r]->statistics.spokes = 0;
    }

#ifdef TODO
    /*******************************************
     Function Timed Transmit. Check if active
     ********************************************/
    if (m_settings.timed_idle != 0 && m_toolbar_button) { //Reset function if radar connection is lost RED==0
        time_t TT_now = time(0);
        int factor = 5 * 60;
        if (m_init_timed_transmit) {   //Await user finalizing idle time option set.
            if (m_toolbar_button == TB_GREEN) {
                TimedTransmit_IdleBoxMode = 2;
                if (TT_now > (m_idle_watchdog + (m_settings.idle_run_time * 60)) || m_idle_dialog_time_left == 999) {
                    RadarTxOff();                 //Stop radar scanning
                    m_settings.show_radar = RADAR_OFF;
                    m_idle_watchdog = TT_now;
                }
            }
            else if (m_toolbar_button == TB_AMBER) {
                TimedTransmit_IdleBoxMode = 1;
                if (TT_now > (m_idle_watchdog + (m_settings.timed_idle * factor)) || m_idle_dialog_time_left == 999) {
                    br24radar_pi::OnToolbarToolCallback(999999);    //start radar scanning
                    m_idle_watchdog = TT_now;
                }
            }
            // Send minutes left to Idle dialog box
            if (!m_pIdleDialog) {
                m_pIdleDialog = new Idle_Dialog;
                m_pIdleDialog->Create(m_parent_window, this);
                m_pIdleDialog->SetPosition(m_dialogLocation[DL_TIMEDTRANSMIT].pos);
            }
            if (TimedTransmit_IdleBoxMode == 1) {   //Idle
                time_left = ((m_idle_watchdog + (m_settings.timed_idle * factor)) - TT_now) / 60;
                if (m_idle_dialog_time_left != time_left) {
                    br24radar_pi::m_pIdleDialog->SetIdleTimes(TimedTransmit_IdleBoxMode, m_settings.timed_idle * factor / 60, time_left);
                    m_pIdleDialog->Show();
                    m_idle_dialog_time_left = time_left;
                }
            }
            if (TimedTransmit_IdleBoxMode == 2) {   //Transmit
                time_left = ((m_idle_watchdog + (m_settings.idle_run_time * 60)) - TT_now) / 60;
                if (m_idle_dialog_time_left != time_left) {
                       br24radar_pi::m_pIdleDialog->SetIdleTimes(TimedTransmit_IdleBoxMode, m_settings.idle_run_time, time_left);
                    m_pIdleDialog->Show();
                    m_idle_dialog_time_left = time_left;
                }
            }
        }
        else {
            if (m_radar[radar]->control_dialog) {
                if (m_radar[radar]->control_dialog->topSizer->IsShown(m_radar[radar]->control_dialog->controlBox)) {
                    m_init_timed_transmit = true;  //First time init: Await user to leave Timed transmit setting menu.
                    m_idle_watchdog = TT_now;
                }
            }
        }
    }
    else {
        if (m_init_timed_transmit) {
            m_idle_dialog_time_left = 999;
            if (m_pIdleDialog) m_pIdleDialog->Close();
            m_settings.timed_idle = 0;
            m_init_timed_transmit = false;
        }
    }   //End of Timed Transmit
#endif

    UpdateState();
}

// UpdateState
// run by RenderGLOverlay  updates the color of the toolbar button
void br24radar_pi::UpdateState( void )
{
    bool radar_seen = false;
    bool data_seen = false;

    for (int r = 0; r < RADARS; r++) {
        radar_seen |= m_radar[r]->radar_seen;
        data_seen |= m_radar[r]->data_seen;
    }

    if (!radar_seen || !m_opengl_mode) {
        m_toolbar_button = TB_RED;
        CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_RED);
    }
    else if (data_seen && m_settings.show_radar == RADAR_ON) {
        m_toolbar_button = TB_GREEN;
        CacheSetToolbarToolBitmaps(BM_ID_GREEN, BM_ID_GREEN);
    }
    else {
        m_toolbar_button = TB_AMBER;
        CacheSetToolbarToolBitmaps(BM_ID_AMBER, BM_ID_AMBER);
    }
}


//***********************************************************************************************************
// Radar Image Graphic Display Processes
//***********************************************************************************************************

bool br24radar_pi::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
    m_opengl_mode = false;

    DoTick(); // update timers and watchdogs

    UpdateState(); // update the toolbar

    return true;
}

// Called by Plugin Manager on main system process cycle

bool br24radar_pi::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    m_refresh_busy_or_queued = true;   //  the receive thread should not queue another refresh (through refresh canvas) this when it is busy
    m_opengl_mode = true;

    // this is expected to be called at least once per second
    // but if we are scrolling or otherwise it can be MUCH more often!

    // Always compute m_auto_range_meters, possibly needed by SendState() called from DoTick().
    double max_distance = radar_distance(vp->lat_min, vp->lon_min, vp->lat_max, vp->lon_max, 'm');
    // max_distance is the length of the diagonal of the viewport. If the boat were centered, the
    // max length to the edge of the screen is exactly half that.
    double edge_distance = max_distance / 2.0;
    m_auto_range_meters = (int) edge_distance;
    if (m_auto_range_meters < 50)
    {
      m_auto_range_meters = 50;
    }
    DoTick(); // update timers and watchdogs

    if (m_settings.chart_overlay < 0 || m_settings.show_radar == RADAR_OFF) {  // No overlay desired
        return true;
    }
    if (!m_bpos_set) {                   // No overlay possible (yet)
        return true;
    }

    wxPoint boat_center;
    GetCanvasPixLL(vp, &boat_center, m_ownship_lat, m_ownship_lon);

    // now set a new value in the range control if an unsollicited range change has been received.
    // not for range change that the pi has initialized. For these the control was updated immediately
#if TODO
    if (m_update_range_control[m_settings.selectRadarB]) {
        m_update_range_control[m_settings.selectRadarB] = false;
        int radar_range = m_range_meters[m_settings.selectRadarB];
        int idx = convertRadarMetersToIndex(&radar_range, m_settings.range_units, m_radar_type);
        m_radar[m_settings.selectRadarB]->range.Update(idx);
        // above also updates radar_range to be a display value (lower, rounded number)
        if (m_radar[radar]->control_dialog) {
            if (radar_range != m_radar[radar]->commanded_range_meters) { // this range change was not initiated by the pi
                m_radar[radar]->control_dialog->SetRemoteRangeIndex(idx);
                if (m_settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: remote range change to %d meters = %d (plugin commanded %d meters)"), m_range_meters[m_settings.selectRadarB], radar_range, m_radar[radar]->commanded_range_meters);
                }
            }
            else {
                m_radar[radar]->control_dialog->SetRangeIndex(idx);
                if (m_settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: final range change to %d meters = %d"), m_range_meters[m_settings.selectRadarB], radar_range);
                }
            }
        }
    }
#endif

    // Calculate the "optimum" radar range setting in meters so the radar image just fills the screen

    if (m_radar[m_settings.chart_overlay]->auto_range_mode) {
        // Don't adjust auto range meters continuously when it is oscillating a little bit (< 5%)
        // This also prevents the radar from issuing a range command after a remote range change
        int test = 100 * m_previous_auto_range_meters / m_auto_range_meters;
        if (test < 95 || test > 105) { //   range change required
            if (m_settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: Automatic range changed from %d to %d meters")
                             , m_previous_auto_range_meters, m_auto_range_meters);
            }
            m_previous_auto_range_meters = m_auto_range_meters;
            // Compute a 'standard' distance. This will be slightly smaller.
            int displayedRange = m_auto_range_meters;
            size_t idx = convertMetersToRadarAllowedValue(&displayedRange, m_settings.range_units, m_radar[m_settings.chart_overlay]->radar_type);
            if (displayedRange != m_radar[m_settings.chart_overlay]->commanded_range_meters) {
                if (m_radar[m_settings.chart_overlay]->control_dialog) {
                    m_radar[m_settings.chart_overlay]->control_dialog->SetRangeIndex(idx);
                }
                m_radar[m_settings.chart_overlay]->SetRangeMeters(displayedRange);
            }
        }
    }

    //    Calculate image scale factor

    GetCanvasLLPix(vp, wxPoint(0, vp->pix_height-1), &ulat, &ulon);  // is pix_height a mapable coordinate?
    GetCanvasLLPix(vp, wxPoint(0, 0), &llat, &llon);
    dist_y = radar_distance(llat, llon, ulat, ulon, 'm'); // Distance of height of display - meters
    pix_y = vp->pix_height;
    v_scale_ppm = 1.0;
    if (dist_y > 0.) {
        // v_scale_ppm = vertical pixels per meter
        v_scale_ppm = vp->pix_height / dist_y ;    // pixel height of screen div by equivalent meters
    }

    if (m_settings.verbose >= 4) {
        wxLogMessage(wxT("BR24radar_pi: RenderRadarOverlay lat=%g lon=%g v_scale_ppm=%g rotation=%g skew=%g scale=%f")
                    , vp->clat
                    , vp->clon
                    , vp->view_scale_ppm
                    , vp->rotation
                    , vp->skew
                    , vp->chart_scale
                    );
    }

    RenderRadarOverlay(boat_center, v_scale_ppm, rad2deg(vp->rotation + vp->skew * m_settings.skew_factor));

    m_refresh_busy_or_queued = false;
    return true;
}

void br24radar_pi::RenderRadarOverlay( wxPoint radar_center, double v_scale_ppm, double rotation )
{
    RadarInfo * ri = m_radar[m_settings.chart_overlay];

    rotation = MOD_DEGREES( rotation                       // viewport rotation + skew (in degrees now)
                          + m_settings.heading_correction  // fix any radome rotation fault
                          );

    // scaling...
    int meters = ri->range_meters;

    if (meters) {
        double radar_pixels_per_meter = ((double) RETURNS_PER_LINE) / meters;
        double scale_factor =  v_scale_ppm / radar_pixels_per_meter;  // screen pix/radar pix

        ri->RenderRadarImage(radar_center, scale_factor, rotation, true);
    }
}


#ifdef TODO
// scan image for bogeys
void br24radar_pi::ScanGuardZones( int max_range, int AB )
{
    int begin_arc, end_arc = 0;
    for (size_t z = 0; z < GUARD_ZONES; z++) {
        switch (m_radar[AB]->guard_zone[z]->type) {
            case GZ_CIRCLE:
                begin_arc = 0;
                end_arc = LINES_PER_ROTATION;
                break;
            case GZ_ARC:
                begin_arc = m_radar[AB]->guard_zone[z]->start_bearing;
                end_arc = m_radar[AB]->guard_zone[z]->end_bearing;
                if (true /*!blackout[AB]*/) {
                    begin_arc += m_hdt;   // arc still in degrees!
                    end_arc += m_hdt;
                }
                begin_arc = SCALE_DEGREES_TO_RAW2048 (begin_arc);      // m_hdt added to provide guard zone relative to heading
                end_arc = SCALE_DEGREES_TO_RAW2048(end_arc);    // now arc in lines

                begin_arc = MOD_ROTATION2048(begin_arc);
                end_arc = MOD_ROTATION2048(end_arc);
                break;
            case GZ_OFF:
            default:
                begin_arc = 0;
                end_arc = 0;
                break;
        }

        if (begin_arc > end_arc) {
            end_arc += LINES_PER_ROTATION;  // now end_arc may be larger than LINES_PER_ROTATION!
        }
        for (int angle = begin_arc ; angle < end_arc ; angle++) {
            unsigned int angle1 = MOD_ROTATION2048 (angle);
            scan_line *scan = &m_scan_line[AB][angle1];
            if (!scan) return;   // No or old data
            for (int radius = 0; radius <= RETURNS_PER_LINE - 2; ++radius) {
                // - 2 added, -1 contains the range circle, should not raise alarm
                //           if (guard_zone[z][angle]) {
                int inner_range = m_radar[AB]->guard_zone[z]->inner_range; // now in meters
                int outer_range = m_radar[AB]->guard_zone[z]->outer_range; // now in meters
                int bogey_range = radius * max_range / RETURNS_PER_LINE;
                if (bogey_range > inner_range && bogey_range < outer_range) {   // within range, now check requirement for alarm
                    if ((m_settings.multi_sweep_filter[AB][z]) != 0) {  // multi sweep filter on for this z
                        GLubyte hist = scan->history[radius] & 7; // check only last 3 bits
                        if (!(hist == 3 || hist >= 5)) {  // corresponds to the patterns 011, 101, 110, 111
                            continue;                      // multi sweep filter on, no valid bogeys
                        }                   // so go to next radius
                    }
                    else {   // multi sweep filter off
                        GLubyte strength = scan->data[radius];
                        if (strength <= displaysetting_threshold[m_settings.display_option]) continue;
                    }
                    int index = z + 2 * AB;
                    m_bogey_count[index]++;
                }
            }
        }
    }
}
#endif

#ifdef TODO

void br24radar_pi::HandleBogeyCount(int *bogey_count)
{
    bool bogeysFound;

    for (int z = 0; z < 2 * GUARD_ZONES; z++) {
        if (bogey_count[z] > m_settings.guard_zone_threshold) {
            bogeysFound = true;
            break;
        }
    }
    if (m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: handle bogeycount bogeysFound %d"), bogeysFound);
    }

    if (bogeysFound) {
        // We have bogeys and there is no objection to showing the dialog
        if (m_settings.timed_idle != 0) m_radar[radar]->control_dialog->SetTimedIdleIndex(0); //Disable Timed Idle if set

        if (!m_pGuardZoneBogey && m_settings.show_radar == RADAR_ON) {
            // If this is the first time we have a bogey create & show the dialog immediately
            m_pGuardZoneBogey = new GuardZoneBogey;
            m_pGuardZoneBogey->Create(m_parent_window, this);
            m_pGuardZoneBogey->Show();
            m_pGuardZoneBogey->SetPosition(m_dialogLocation[DL_BOGEY].pos);
        }
        else if (!m_guard_bogey_confirmed && (m_settings.show_radar == RADAR_ON)) {
            m_pGuardZoneBogey->Show();
        }
        time_t now = time(0);
        int delta_t = now - m_alarm_sound_last;
        if (!m_guard_bogey_confirmed && delta_t >= ALARM_TIMEOUT && bogeysFound) {
            // If the last time is 10 seconds ago we ping a sound, unless the user confirmed
            m_alarm_sound_last = now;

            if (!m_settings.alert_audio_file.IsEmpty()) {
                PlugInPlaySound(m_settings.alert_audio_file);
            }
            else {
                wxBell();
            }
            if (m_pGuardZoneBogey && m_settings.show_radar == RADAR_ON) {
                m_pGuardZoneBogey->Show();
            }
            delta_t = ALARM_TIMEOUT;
        }
        if (m_pGuardZoneBogey) {
            m_pGuardZoneBogey->SetBogeyCount(bogey_count, m_guard_bogey_confirmed ? -1 : ALARM_TIMEOUT - delta_t);
        }
    }

    if (!bogeysFound && m_pGuardZoneBogey) {
        m_pGuardZoneBogey->SetBogeyCount(bogey_count, -1);   // with -1 "next alarm in... "will not be displayed
        m_guard_bogey_confirmed = false; // Reset for next time we see bogeys
        // keep showing the bogey dialogue with 0 bogeys
    }
}
#endif




//****************************************************************************

bool br24radar_pi::LoadConfig(void)
{
    wxFileConfig *pConf = m_pconfig;
    int intValue;

    if (pConf) {

        pConf->SetPath(wxT("/Plugins/BR24Radar"));
        pConf->Read(wxT("DisplayOption"), &m_settings.display_option, 0);
        pConf->Read(wxT("RangeUnits" ), &m_settings.range_units, 0 ); //0 = "Nautical miles"), 1 = "Kilometers"
        if (m_settings.range_units >= 2) {
            m_settings.range_units = 1;
        }
        m_settings.range_unit_meters = (m_settings.range_units == 1) ? 1000 : 1852;
        pConf->Read(wxT("ChartOverlay"),  &m_settings.chart_overlay, -1);
        pConf->Read(wxT("EmulatorOn"), &intValue, 0);
        m_settings.emulator_on = intValue != 0;
        pConf->Read(wxT("VerboseLog"),  &m_settings.verbose, 0);
        pConf->Read(wxT("Transparency"),  &m_settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY);
        pConf->Read(wxT("RangeCalibration"),  &m_settings.range_calibration, 1.0);
        pConf->Read(wxT("HeadingCorrection"),  &m_settings.heading_correction, 0);
        pConf->Read(wxT("ScanMaxAge"), &m_settings.max_age, 6);   // default 6
        if (m_settings.max_age < MIN_AGE) {
            m_settings.max_age = MIN_AGE;
        } else if (m_settings.max_age > MAX_AGE) {
            m_settings.max_age = MAX_AGE;
        }
        pConf->Read(wxT("RunTimeOnIdle"), &m_settings.idle_run_time, 2);

        pConf->Read(wxT("DrawAlgorithm"), &m_settings.draw_algorithm, 1);
        pConf->Read(wxT("GuardZonesThreshold"), &m_settings.guard_zone_threshold, 5L);
        pConf->Read(wxT("GuardZonesRenderStyle"), &m_settings.guard_zone_render_style, 0);
        pConf->Read(wxT("Refreshrate"), &m_settings.refreshrate, 1);
        if (m_settings.refreshrate < 1) {
            m_settings.refreshrate = 1; // not allowed
        }
        if (m_settings.refreshrate > 5) {
            m_settings.refreshrate = 5; // not allowed
        }
        m_refresh_rate = REFRESHMAPPING[m_settings.refreshrate - 1];

        pConf->Read(wxT("PassHeadingToOCPN"), &intValue, 0);
        m_settings.pass_heading_to_opencpn = intValue != 0;
        pConf->Read(wxT("DrawingMethod"), &m_settings.drawing_method, 0);

        for (size_t i = 0; i < ARRAY_SIZE(m_dialogLocation); i++) {
           int x, y, sx, sy;

           pConf->Read(wxString::Format(wxT("Control%dPosX"), i), &x, 300);
           pConf->Read(wxString::Format(wxT("Control%dPosY"), i), &y, 300);
           pConf->Read(wxString::Format(wxT("Control%dSizeX"), i), &sx, 0);
           pConf->Read(wxString::Format(wxT("Control%dSizeY"), i), &sy, 0);
           m_dialogLocation[i].pos = wxPoint(x, y);
           m_dialogLocation[i].size = wxSize(x, y);
        }

        for (int r = 0; r < RADARS; r++) {
            pConf->Read(wxString::Format(wxT("Radar%dRotation"), r), &m_radar[r]->rotation.value, 0);
            for (int i = 0; i < GUARD_ZONES; i++) {
                int v;
                pConf->Read(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), &m_radar[r]->guard_zone[i]->start_bearing, 0.0);
                pConf->Read(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), &m_radar[r]->guard_zone[i]->end_bearing, 0.0);
                pConf->Read(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), &m_radar[r]->guard_zone[i]->outer_range, 0);
                pConf->Read(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), &m_radar[r]->guard_zone[i]->inner_range, 0);
                pConf->Read(wxString::Format(wxT("Radar%dZone%dType"), r, i), &v, 0);
                m_radar[r]->guard_zone[i]->type = (GuardZoneType) v;
                pConf->Read(wxString::Format(wxT("Radar%dZone%dFilter"), r, i), &m_radar[r]->guard_zone[i]->multi_sweep_filter, 0);
            }
        }

        pConf->Read(wxT("RadarAlertAudioFile"), &m_settings.alert_audio_file);
        pConf->Read(wxT("EnableDualRadar"), &m_settings.enable_dual_radar, 0);

        pConf->Read(wxT("SkewFactor"), &m_settings.skew_factor, 1);

        pConf->Read(wxT("ThresholdRed"), &m_settings.threshold_red, 200);
        pConf->Read(wxT("ThresholdGreen"), &m_settings.threshold_green, 100);
        pConf->Read(wxT("ThresholdBlue"), &m_settings.threshold_blue, 50);
        pConf->Read(wxT("ThresholdMultiSweep"), &m_settings.threshold_multi_sweep, 20);


        pConf->Read(wxT("ShowRadar"), &intValue, 0);
        m_settings.show_radar = intValue ? RADAR_ON : RADAR_OFF;

        SaveConfig();
        return true;
    }
    return false;
}

bool br24radar_pi::SaveConfig(void)
{

    wxFileConfig *pConf = m_pconfig;

    if (pConf) {
        pConf->SetPath(wxT("/Plugins/BR24Radar"));
        pConf->Write(wxT("DisplayOption"), m_settings.display_option);
        pConf->Write(wxT("RangeUnits" ), m_settings.range_units);
        pConf->Write(wxT("ChartOverlay"), m_settings.chart_overlay);
        pConf->Write(wxT("VerboseLog"), m_settings.verbose);
        pConf->Write(wxT("Transparency"), m_settings.overlay_transparency);
        pConf->Write(wxT("RangeCalibration"),  m_settings.range_calibration);
        pConf->Write(wxT("HeadingCorrection"),  m_settings.heading_correction);
        pConf->Write(wxT("GuardZonesThreshold"), m_settings.guard_zone_threshold);
        pConf->Write(wxT("GuardZonesRenderStyle"), m_settings.guard_zone_render_style);
        pConf->Write(wxT("ScanMaxAge"), m_settings.max_age);
        pConf->Write(wxT("RunTimeOnIdle"), m_settings.idle_run_time);
        pConf->Write(wxT("DrawAlgorithm"), m_settings.draw_algorithm);
        pConf->Write(wxT("Refreshrate"), m_settings.refreshrate);
        pConf->Write(wxT("PassHeadingToOCPN"), m_settings.pass_heading_to_opencpn);
        pConf->Write(wxT("DrawingMethod"), m_settings.drawing_method);
        pConf->Write(wxT("EmulatorOn"), (int) m_settings.emulator_on);
        pConf->Write(wxT("ShowRadar"), (int) m_settings.show_radar);
        pConf->Write(wxT("RadarAlertAudioFile"), m_settings.alert_audio_file);
        pConf->Write(wxT("EnableDualRadar"), m_settings.enable_dual_radar);

        for (size_t i = 0; i < ARRAY_SIZE(m_dialogLocation); i++) {
            if (m_dialogLocation[i].pos.x) {
                pConf->Write(wxString::Format(wxT("Control%dPosX"), i), m_dialogLocation[i].pos.x);
            }
            if (m_dialogLocation[i].pos.y) {
                pConf->Write(wxString::Format(wxT("Control%dPosY"), i), m_dialogLocation[i].pos.y);
            }
            if (m_dialogLocation[i].size.x) {
                pConf->Write(wxString::Format(wxT("Control%dSizeX"), i), m_dialogLocation[i].size.x);
            }
            if (m_dialogLocation[i].size.y) {
                pConf->Write(wxString::Format(wxT("Control%dSizeY"), i), m_dialogLocation[i].size.y);
            }
        }

        for (size_t r = 0; r < RADARS; r++) {
            pConf->Write(wxString::Format(wxT("Radar%dRotation"), r), m_radar[r]->rotation.value);
            for (size_t i = 0; i < GUARD_ZONES; i++) {
                pConf->Write(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), m_radar[r]->guard_zone[i]->start_bearing);
                pConf->Write(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), m_radar[r]->guard_zone[i]->end_bearing);
                pConf->Write(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), m_radar[r]->guard_zone[i]->outer_range);
                pConf->Write(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), m_radar[r]->guard_zone[i]->inner_range);
                pConf->Write(wxString::Format(wxT("Radar%dZone%dType"), r, i), (int) m_radar[r]->guard_zone[i]->type);
                pConf->Write(wxString::Format(wxT("Radar%dZone%dFilter"), r, i), m_radar[r]->guard_zone[i]->multi_sweep_filter);
            }
        }

        pConf->Write(wxT("SkewFactor"), m_settings.skew_factor);

        pConf->Write(wxT("ThresholdRed"), m_settings.threshold_red);
        pConf->Write(wxT("ThresholdGreen"), m_settings.threshold_green);
        pConf->Write(wxT("ThresholdBlue"), m_settings.threshold_blue);
        pConf->Write(wxT("ThresholdMultiSweep"), m_settings.threshold_multi_sweep);

        pConf->Flush();
        wxLogMessage(wxT("BR24radar_pi: Saved settings"));
        return true;
    }

    return false;
}


// Positional Data passed from NMEA to plugin
void br24radar_pi::SetPositionFix(PlugIn_Position_Fix &pfix)
{
}

void br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix)
{
    time_t now = time(0);
    wxString info;

    // PushNMEABuffer (_("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,,230394,003.1,W"));  // only for test, position without heading
    // PushNMEABuffer (_("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A")); //with heading for test
    if (m_var_source <= VARIATION_SOURCE_FIX && !wxIsNaN(pfix.Var) && (fabs(pfix.Var) > 0.0 || m_var == 0.0)) {
        if (m_var_source < VARIATION_SOURCE_FIX || fabs(pfix.Var - m_var) > 0.05) {
            wxLogMessage(wxT("BR24radar_pi: Position fix provides new magnetic variation %f"), pfix.Var);
            if (m_pMessageBox) {
                if (m_pMessageBox->IsShown()) {
                    info = _("GPS");
                    info << wxT(" ") << m_var;
                    m_pMessageBox->SetVariationInfo(info);
                }
            }
        }
        m_var = pfix.Var;
        m_var_source = VARIATION_SOURCE_FIX;
        m_var_watchdog = now;
    }

    if (m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: SetPositionFixEx var=%f heading_on_radar=%d var_wd=%d show_radar=%d")
                    , pfix.Var
                    , m_heading_on_radar
                    , TIMER_NOT_ELAPSED(now, m_var_watchdog)
                    , m_settings.show_radar
                    );
    }
    if (m_heading_on_radar && TIMER_NOT_ELAPSED(now, m_var_watchdog) && m_settings.show_radar) {
        if (m_heading_source != HEADING_RADAR) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now Radar %f"), m_hdt);
            m_heading_source = HEADING_RADAR;
        }
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                info = _("radar");
                info << wxT(" ") << m_hdt;
                m_pMessageBox->SetHeadingInfo(info);
            }
        }
        m_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Hdm) && TIMER_NOT_ELAPSED(now, m_var_watchdog)) {
        m_hdt = pfix.Hdm + m_var;
        if (m_heading_source != HEADING_HDM) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now HDM %f"), m_hdt);
            m_heading_source = HEADING_HDM;
        }
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                info = _("HDM");
                info << wxT(" ") << m_hdt;
                m_pMessageBox->SetHeadingInfo(info);
            }
        }
        m_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Hdt)) {
        m_hdt = pfix.Hdt;
        if (m_heading_source != HEADING_HDT) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now HDT"));
            m_heading_source = HEADING_HDT;
        }
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                info = _("HDT");
                info << wxT(" ") << m_hdt;
                m_pMessageBox->SetHeadingInfo(info);
            }
        }
        m_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Cog)) {
        m_hdt = pfix.Cog;
        if (m_heading_source != HEADING_COG) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now COG"));
            m_heading_source = HEADING_COG;
        }
        if (m_pMessageBox) {
            if (m_pMessageBox->IsShown()) {
                info = _("COG");
                info << wxT(" ") << m_hdt;
                m_pMessageBox->SetHeadingInfo(info);
            }
        }
        m_hdt_watchdog = now;
    }

    if (pfix.FixTime && TIMER_NOT_ELAPSED(now, pfix.FixTime)) {
        m_ownship_lat = pfix.Lat;
        m_ownship_lon = pfix.Lon;
        if (!m_bpos_set) {
            wxLogMessage(wxT("BR24radar_pi: GPS position is now known"));
        }
        m_bpos_set = true;
        m_bpos_watchdog = now;
    }
}    // end of br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix)

void br24radar_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
    static const wxString WMM_VARIATION_BOAT = wxString(_T("WMM_VARIATION_BOAT"));
    if (message_id.Cmp(WMM_VARIATION_BOAT) == 0) {
        wxJSONReader reader;
        wxJSONValue message;
        if (!reader.Parse(message_body, &message)) {
            wxJSONValue defaultValue(360);
            double variation = message.Get(_T("Decl"), defaultValue).AsDouble();

            if (variation != 360.0) {
                if (m_var_source != VARIATION_SOURCE_WMM) {
                    wxLogMessage(wxT("BR24radar_pi: WMM plugin provides new magnetic variation %f"), variation);
                }
                m_var = variation;
                m_var_source = VARIATION_SOURCE_WMM;
                m_var_watchdog = time(0);
                if (m_pMessageBox) {
                    if (m_pMessageBox->IsShown()) {
                        wxString info = _("WMM");
                        info << wxT(" ") << m_var;
                        m_pMessageBox->SetVariationInfo(info);
                    }
                }
            }
        }
    }
}

void br24radar_pi::SetControlValue(int radar, ControlType controlType, int value)
{                                                   // sends the command to the radar
    switch (controlType) {
        case CT_TRANSPARENCY: {
            m_settings.overlay_transparency = value;
            break;
        }
        case CT_SCAN_AGE: {
            m_settings.max_age = value;
            break;
        }
        case CT_TIMED_IDLE: {
            m_settings.timed_idle = value;
            break;
        }
        case CT_REFRESHRATE: {
            m_settings.refreshrate = value;
            break;
        }

        default: {
            if (m_radar[radar]->radar_seen) {
                if (!m_radar[radar]->SetControlValue(controlType, value)) {
                    wxLogMessage(wxT("BR24radar_pi: %s unhandled control setting for control %d"), m_radar[radar]->name, controlType);
                }
            }
        }
    }
}

//*****************************************************************************************************
void br24radar_pi::CacheSetToolbarToolBitmaps(int bm_id_normal, int bm_id_rollover)
{
    if ((bm_id_normal == m_sent_bm_id_normal) && (bm_id_rollover == m_sent_bm_id_rollover)) {
        return;    // no change needed
    }

    if ((bm_id_normal == -1) || (bm_id_rollover == -1)) {         // don't do anything, caller's responsibility
        m_sent_bm_id_normal = bm_id_normal;
        m_sent_bm_id_rollover = bm_id_rollover;
        return;
    }

    m_sent_bm_id_normal = bm_id_normal;
    m_sent_bm_id_rollover = bm_id_rollover;

    wxBitmap *pnormal = NULL;
    wxBitmap *prollover = NULL;

    switch (bm_id_normal) {
        case BM_ID_RED:
            pnormal = _img_radar_red;
            break;
        case BM_ID_RED_SLAVE:
            pnormal = _img_radar_red_slave;
            break;
        case BM_ID_GREEN:
            pnormal = _img_radar_green;
            break;
        case BM_ID_GREEN_SLAVE:
            pnormal = _img_radar_green_slave;
            break;
        case BM_ID_AMBER:
            pnormal = _img_radar_amber;
            break;
        case BM_ID_AMBER_SLAVE:
            pnormal = _img_radar_amber_slave;
            break;
        case BM_ID_BLANK:
            pnormal = _img_radar_blank;
            break;
        case BM_ID_BLANK_SLAVE:
            pnormal = _img_radar_blank_slave;
            break;
        default:
            break;
    }

    switch (bm_id_rollover) {
        case BM_ID_RED:
            prollover = _img_radar_red;
            break;
        case BM_ID_RED_SLAVE:
            prollover = _img_radar_red_slave;
            break;
        case BM_ID_GREEN:
            prollover = _img_radar_green;
            break;
        case BM_ID_GREEN_SLAVE:
            prollover = _img_radar_green_slave;
            break;
        case BM_ID_AMBER:
            prollover = _img_radar_amber;
            break;
        case BM_ID_AMBER_SLAVE:
            prollover = _img_radar_amber_slave;
            break;
        case BM_ID_BLANK:
            prollover = _img_radar_blank;
            break;
        case BM_ID_BLANK_SLAVE:
            prollover = _img_radar_blank_slave;
            break;
        default:
            break;
    }

    if ((pnormal) && (prollover)) {
        SetToolbarToolBitmaps(m_tool_id, pnormal, prollover);
    }
}


/*
   SetNMEASentence is used to speed up rotation and variation
   detection if SetPositionEx() is not called very often. This will
   be the case if you have a high speed heading sensor (for instance, 2 to 20 Hz)
   but only a 1 Hz GPS update.

   Note that the type of heading source is only updated in SetPositionEx().
*/

void br24radar_pi::SetNMEASentence( wxString &sentence )
{
    m_NMEA0183 << sentence;
    time_t now = time(0);

    if (m_NMEA0183.PreParse()) {
        if (m_NMEA0183.LastSentenceIDReceived == _T("HDG") && m_NMEA0183.Parse()) {
            if (m_settings.verbose >= 2) {
                wxLogMessage(wxT("BR24radar_pi: received HDG variation=%f var_source=%d m_var=%f"), m_NMEA0183.Hdg.MagneticVariationDegrees, m_var_source, m_var);
            }
            if (!wxIsNaN(m_NMEA0183.Hdg.MagneticVariationDegrees) &&
                (m_var_source <= VARIATION_SOURCE_NMEA || (m_var == 0.0 && m_NMEA0183.Hdg.MagneticVariationDegrees > 0.0))) {
                double newVar;
                if (m_NMEA0183.Hdg.MagneticVariationDirection == East) {
                    newVar = +m_NMEA0183.Hdg.MagneticVariationDegrees;
                }
                else {
                    newVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
                }
                if (m_settings.verbose && fabs(newVar - m_var) >= 0.1) {
                    wxLogMessage(wxT("BR24radar_pi: NMEA provides new magnetic variation %f"), newVar);
                }
                m_var = newVar;
                m_var_source = VARIATION_SOURCE_NMEA;
                m_var_watchdog = now;
                if (m_pMessageBox) {
                    if (m_pMessageBox->IsShown()) {
                        wxString info = _("NMEA");
                        info << wxT(" ") << m_var;
                        m_pMessageBox->SetVariationInfo(info);
                    }
                }
            }
            if (m_heading_source == HEADING_HDM && !wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees)) {
                m_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + m_var;
                m_hdt_watchdog = now;
            }
        }
        else if (m_heading_source == HEADING_HDM
              && m_NMEA0183.LastSentenceIDReceived == _T("HDM")
              && m_NMEA0183.Parse()
              && !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic)) {
            m_hdt = m_NMEA0183.Hdm.DegreesMagnetic + m_var;
            m_hdt_watchdog = now;
        }
        else if (m_heading_source == HEADING_HDT
              && m_NMEA0183.LastSentenceIDReceived == _T("HDT")
              && m_NMEA0183.Parse()
              && !wxIsNaN(m_NMEA0183.Hdt.DegreesTrue)) {
            m_hdt = m_NMEA0183.Hdt.DegreesTrue;
            m_hdt_watchdog = now;
        }
    }
}


#if TODO
    /* TODO - GuardZone handling */
    if ((m_settings.show_radar == RADAR_ON && m_bpos_set && m_heading_source != HEADING_NONE && m_data_seen) ||
        (m_settings.emulator_on && m_settings.show_radar == RADAR_ON)) {
        if (m_range_meters[m_settings.selectRadarB] > 0 && m_scanner_state == RADAR_ON) {
            // Guard Section
            for (int i = 0; i < 4; i++) {
                m_bogey_count[i] = 0;
            }
            static int metersA, metersB;
            if (m_settings.selectRadarB == 0) metersA = meters;
            if (m_settings.selectRadarB == 1) metersB = meters;
            if (m_settings.show_radar == RADAR_ON && metersA != 0) {
                ScanGuardZones(metersA, 0);
            }
            if (m_settings.show_radar == RADAR_ON && metersB != 0) {
                ScanGuardZones(metersB, 1);
            }
            m_draw->RenderRadarImage(radar_center, scale_factor, 0.0, true);
        }
        HandleBogeyCount(m_bogey_count);
#endif

#ifdef TODO
        /* TODO -- INTEGRATE GUARD ZONE RENDERING INTO PrepareRadarImage */
        // Guard Zone image and heading line for radar only
        if (m_settings.show_radar == RADAR_ON) {
            if (m_radar[m_settings.selectRadarB]->guard_zone[0]->type != GZ_OFF || m_radar[m_settings.selectRadarB]->guard_zone[1]->type != GZ_OFF) {
                RenderGuardZone(radar_center, v_scale_ppm, m_settings.selectRadarB);
            }
        }
#endif

// vim: sw=4:ts=8:
