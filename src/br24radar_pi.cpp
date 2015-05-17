/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *
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

/*

 Additional contributors for release 1.2, spring 2015

 Douwe Fokkema for
 - Heading on radar. Use the heading that the radar puts in the line headers when it is getting a heading from the RI10 or
 RI11 interface box
 - When there is no heading on radar, the heading at the time of receiving the data will be used for the radar image.
 - Performance improvements of the display process, allowing for rapid refresh
 - Rapid refresh of the radar image. You can now see the beam rotating.
 - Changes of the range settings. Displayed ranges will now correspond better with the ranges that a HDS display shows.
 - Smoother switching on of the 4G radar
 - Various smaller corrections

 Håkan Svensson for
 - Timed transmit function
 - Many tests
 - Translation files

 */


#ifdef _WINDOWS
# include <WinSock2.h>
# include <ws2tcpip.h>
# pragma comment (lib, "Ws2_32.lib")
#endif

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif                          //precompiled headers

#include <wx/socket.h>
#include "wx/apptrait.h"
#include <wx/glcanvas.h>
#include "wx/sckaddr.h"
#include "wx/datetime.h"
#include <wx/fileconf.h>
#include <fstream>

using namespace std;

#ifdef __WXGTK__
# include <netinet/in.h>
# include <sys/ioctl.h>
#endif

#ifdef __WXOSX__
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
#endif

#ifdef __WXMSW__
# include "GL/glu.h"
#endif

#include "br24radar_pi.h"
//#include "ocpndc.h"


// A marker that uniquely identifies BR24 generation scanners, as opposed to 4G(eneration)
// Note that 3G scanners are BR24's with better power, so they are more BR23+ than 4G-.
// As far as we know they 3G's use exactly the same command set.

// If BR24MARK is found, we switch to BR24 mode, otherwise 4G.
static UINT8 BR24MARK[] = { 0x00, 0x44, 0x0d, 0x0e };

enum {
    // process ID's
    ID_OK,
    ID_RANGE_UNITS,
    ID_OVERLAYDISPLAYOPTION,
    ID_DISPLAYTYPE,
    ID_HEADINGSLIDER,
    ID_SELECT_SOUND,
    ID_TEST_SOUND
};

bool br_bpos_set = false;
double br_ownship_lat, br_ownship_lon;
double cur_lat, cur_lon;
double br_hdm;
double br_hdt;     // this is the heading that the pi is using for all heading operations, in degrees
                   // br_hdt will come from the radar if available else from the NMEA stream
int br_hdt_raw = 0;// if set by radar, the heading (in 0..4095)

// Variation. Used to convert magnetic into true heading.
// Can come from SetPositionFixEx, which may hail from the WMM plugin
// and is thus to be preferred, or GPS or a NMEA sentence. The latter will probably
// have an outdated variation model, so is less preferred. Besides, some devices
// transmit invalid (zero) values. So we also let non-zero values prevail.
double br_var = 0.0;     // local magnetic variation, in degrees
enum VariationSource { VARIATION_SOURCE_NONE, VARIATION_SOURCE_NMEA, VARIATION_SOURCE_FIX, VARIATION_SOURCE_WMM };
VariationSource br_var_source = VARIATION_SOURCE_NONE;

unsigned int i_display = 0;  // used in radar reive thread for display operation
int repeatOnDelay = 0;   // used to prevend additional TxOn commands from DoTick when radar heas just been switched on

bool br_heading_on_radar = false;
unsigned int downsample = 0;  // moved from display radar to here; also used in radar receive thread
unsigned int refreshRate = 1;  // refreshrate for radar used in process buffer
unsigned int refreshmapping[] = { 10, 9, 3, 1, 0}; // translation table for the refreshrate, interval between received frames
// user values 1 to 5 mapped to these values for refrehs interval
// user 1 - no additional refresh, 2 - interval between frames 9, so on.
bool RenderOverlay_busy = false;

double mark_rng = 0, mark_brg = 0;      // This is needed for context operation
long  br_range_meters = 0;      // current range for radar
int auto_range_meters = 0;      // What the range should be, at least, when AUTO mode is selected
int previous_auto_range_meters = 0;
bool setRangeIssued = false;
bool rangeChangeReceived = false;

int   br_last_idle_set = 0;     //Timed Transmit
int   br_idle_set_count = 0;
bool  onInit_Timed_Idle;
static time_t br_idle_watchdog;
int   IdleDialogTimeLeft = 0;

int   br_radar_state = 0;
int   br_scanner_state = 0;
bool  br_send_state;
RadarType br_radar_type = RT_UNKNOWN;

static bool  br_radar_seen = false;
static bool  br_data_seen = false;
static bool  br_opengl_mode = false;
static time_t      br_bpos_watchdog;
static time_t      br_hdt_watchdog;
static time_t      br_radar_watchdog;
static time_t      br_data_watchdog;
static time_t      br_var_watchdog;
#define     WATCHDOG_TIMEOUT (10)  // After 10s assume GPS and heading data is invalid
#define     TIMER_NOT_ELAPSED(watchdog) (now < watchdog + WATCHDOG_TIMEOUT)
#define     TIMER_ELAPSED(watchdog) (!TIMER_NOT_ELAPSED(watchdog))
time_t      br_dt_stayalive;
#define     STAYALIVE_TIMEOUT (5)  // Send data every 5 seconds to ping radar

int   radar_control_id = 0, guard_zone_id = 0;
bool  guard_context_mode;

bool        guard_bogey_confirmed = false;
time_t      alarm_sound_last;
#define     ALARM_TIMEOUT (10)

static sockaddr_in * br_mcast_addr = 0; // One of the threads finds out where the radar lives and writes our IP here
static sockaddr_in * br_radar_addr = 0; // One of the threads finds out where the radar lives and writes its IP here

// static wxCriticalSection br_scanLock;

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new br24radar_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}


/********************************************************************************************************/
//   Distance measurement for simple sphere
/********************************************************************************************************/
//static double twoPI = 2 * PI;

static double deg2rad(double deg)
{
    return (deg * PI / 180.0);
}

static double rad2deg(double rad)
{
    return (rad * 180.0 / PI);
}

static double local_distance (double lat1, double lon1, double lat2, double lon2) {
    // Spherical Law of Cosines
    double theta, dist;

    theta = lon2 - lon1;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);        // radians
    dist = rad2deg(dist);
    dist = fabs(dist) * 60    ;    // nautical miles/degree
    return (dist);
}

static double radar_distance(double lat1, double lon1, double lat2, double lon2, char unit)
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
    //    wxLogMessage(wxT("Radar Distance %f,%f,%f,%f,%f, %c"), lat1, lon1, lat2, lon2, dist, unit);
    return dist;
}

static double local_bearing (double lat1, double lon1, double lat2, double lon2) //FES
{
    double angle = atan2(deg2rad(lat2-lat1), (deg2rad(lon2-lon1) * cos(deg2rad(lat1))));

    angle = rad2deg(angle);
    angle = MOD_DEGREES(90.0 - rad2deg(angle));
    return angle;
}

/**
 * Draw an OpenGL blob at a particular angle in radians, of a particular arc_length, also in radians.
 *
 * The minimum distance from the center of the plane is radius, and it ends at radius + blob_heigth
 */
static void draw_blob_gl(double ca, double sa, double radius, double arc_width, double blob_heigth)
{
    const double blob_start = 0.0;
    const double blob_end = blob_heigth;

    double xm1 = (radius + blob_start) * ca;
    double ym1 = (radius + blob_start) * sa;
    double xm2 = (radius + blob_end) * ca;
    double ym2 = (radius + blob_end) * sa;

    double arc_width_start2 = (radius + blob_start) * arc_width;
    double arc_width_end2 =   (radius + blob_end) * arc_width;

    double xa = xm1 + arc_width_start2 * sa;
    double ya = ym1 - arc_width_start2 * ca;

    double xb = xm2 + arc_width_end2 * sa;
    double yb = ym2 - arc_width_end2 * ca;

    double xc = xm1 - arc_width_start2 * sa;
    double yc = ym1 + arc_width_start2 * ca;

    double xd = xm2 - arc_width_end2 * sa;
    double yd = ym2 + arc_width_end2 * ca;

    glBegin(GL_TRIANGLES);
    glVertex2d(xa, ya);
    glVertex2d(xb, yb);
    glVertex2d(xc, yc);

    glVertex2d(xb, yb);
    glVertex2d(xc, yc);
    glVertex2d(xd, yd);
    glEnd();
}

static void DrawArc(float cx, float cy, float r, float start_angle, float arc_angle, int num_segments)
{
    float theta = arc_angle / float(num_segments - 1); // - 1 comes from the fact that the arc is open

    float tangential_factor = tanf(theta);
    float radial_factor = cosf(theta);

    float x = r * cosf(start_angle);
    float y = r * sinf(start_angle);

    glBegin(GL_LINE_STRIP);
    for(int ii = 0; ii < num_segments; ii++) {
        glVertex2f(x + cx, y + cy);

        float tx = -y;
        float ty = x;

        x += tx * tangential_factor;
        y += ty * tangential_factor;

        x *= radial_factor;
        y *= radial_factor;
    }
    glEnd();
}

static void DrawOutlineArc(double r1, double r2, double a1, double a2, bool stippled)
{
    if (a1 > a2) {
        a2 += 360.0;
    }
    int  segments = (a2 - a1) * 4;
    bool circle = (a1 == 0.0 && a2 == 359.0);

    if (!circle) {
        a1 -= 0.5;
        a2 += 0.5;
    }
    a1 = deg2rad(a1);
    a2 = deg2rad(a2);

    if (stippled) {
        glEnable (GL_LINE_STIPPLE);
        glLineStipple (1, 0x0F0F);
        glLineWidth(2.0);
    } else {
        glLineWidth(3.0);
    }

    DrawArc(0.0, 0.0, r1, a1, a2 - a1, segments);
    DrawArc(0.0, 0.0, r2, a1, a2 - a1, segments);

    if (!circle) {
        glBegin(GL_LINES);
        glVertex2f(r1 * cosf(a1), r1 * sinf(a1));
        glVertex2f(r2 * cosf(a1), r2 * sinf(a1));
        glVertex2f(r1 * cosf(a2), r1 * sinf(a2));
        glVertex2f(r2 * cosf(a2), r2 * sinf(a2));
        glEnd();
    }
}

static void DrawFilledArc(double r1, double r2, double a1, double a2)
{
    if (a1 > a2) {
        a2 += 360.0;
    }

    for (double n = a1; n <= a2; ++n ) {
        double nr = deg2rad(n);
        draw_blob_gl(cos(nr), sin(nr), r2, deg2rad(0.5), r1 - r2);
    }
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

br24radar_pi::br24radar_pi(void *ppimgr)
: opencpn_plugin_110(ppimgr)
{
    // Create the PlugIn icons
    initialize_images();
    m_pdeficon = new wxBitmap(*_img_radar_blank);

}

int br24radar_pi::Init(void)
{
    int r;
#ifdef __WXMSW__
    WSADATA wsaData;

    // Initialize Winsock
    r = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (r != 0) {
        wxLogError(wxT("BR24radar_pi: Unable to initialise Windows Sockets, error %d"), r);
        // Might as well give up now
        return 0;
    }
#endif

    AddLocaleCatalog( _T("opencpn-br24radar_pi") );

    m_pControlDialog = NULL;

    br_radar_state = RADAR_OFF;
    br_scanner_state = RADAR_OFF;                 // Radar scanner is off

    br_dt_stayalive = time(0);
    alarm_sound_last = br_dt_stayalive;
    br_bpos_watchdog = 0;
    br_hdt_watchdog  = 0;
    br_var_watchdog = 0;
    br_radar_watchdog = 0;
    br_data_watchdog = 0;
    br_idle_watchdog = 0;

    m_ptemp_icon = NULL;
    m_sent_bm_id_normal = -1;
    m_sent_bm_id_rollover =  -1;

    m_heading_source = HEADING_NONE;

    m_statistics.broken_packets = 0;
    m_statistics.broken_spokes  = 0;
    m_statistics.missing_spokes = 0;
    m_statistics.packets        = 0;
    m_statistics.spokes         = 0;

    m_pOptionsDialog = 0;
    m_pControlDialog = 0;
    m_pGuardZoneDialog = 0;
    m_pGuardZoneBogey = 0;
    m_pIdleDialog = 0;

    memset(&guardZones, 0, sizeof(guardZones));

    settings.guard_zone = 0;   // this used to be active guard zone, now it means which guard zone window is active
    settings.display_mode = DM_CHART_OVERLAY;
    settings.master_mode = false;                 // we're not the master controller at startup
    settings.auto_range_mode = true;                    // starts with auto range change
    settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;
    settings.refreshrate = 1;
    settings.timed_idle = 0;

    //      Set default parameters for controls displays
    m_BR24Controls_dialog_x = 0;    // position
    m_BR24Controls_dialog_y = 0;
    m_BR24Controls_dialog_sx = 200;  // size
    m_BR24Controls_dialog_sy = 200;

    m_GuardZoneBogey_x = 200;
    m_GuardZoneBogey_y = 200;

    ::wxDisplaySize(&m_display_width, &m_display_height);

    //****************************************************************************************
    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
    if (LoadConfig()) {
        wxLogMessage(wxT("BR24radar_pi: Configuration file values initialised"));
        wxLogMessage(wxT("BR24radar_pi: Log verbosity = %d (to modify, set VerboseLog to 0..4)"), settings.verbose);
    }
    else {
        wxLogMessage(wxT("BR24radar_pi: configuration file values initialisation failed"));
        return 0; // give up
    }

    ComputeGuardZoneAngles();

    wxLongLong now = wxGetLocalTimeMillis();
    for (int i = 0; i < LINES_PER_ROTATION; i++) {
        m_scan_line[i].age = now - MAX_AGE * MILLISECONDS_PER_SECOND;
        m_scan_line[i].range = 0;
    }

    // Get a pointer to the opencpn display canvas, to use as a parent for the UI dialog
    m_parent_window = GetOCPNCanvasWindow();

    //    This PlugIn needs a toolbar icon

    m_tool_id  = InsertPlugInTool(wxT(""), _img_radar_red, _img_radar_red, wxITEM_NORMAL,
                                  wxT("BR24Radar"), wxT(""), NULL,
                                  BR24RADAR_TOOL_POSITION, 0, this);

    CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);

    //    Create the control socket for the Radar data receiver

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(0);
    int one = 1;
    m_radar_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_radar_socket == INVALID_SOCKET) {
        r = -1;
    }
    else {
        r = setsockopt(m_radar_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &one, sizeof(one));
    }

    if (!r) {
        r = bind(m_radar_socket, (struct sockaddr *) &adr, sizeof(adr));
    }

    if (r) {
        wxLogError(wxT("BR24radar_pi: Unable to create UDP sending socket"));
        // Might as well give up now
        return 0;
    }

    // Context Menu Items (Right click on chart screen)

    m_pmenu = new wxMenu();            // this is a dummy menu
    // required by Windows as parent to item created
    wxMenuItem *pmi = new wxMenuItem(m_pmenu, -1, _("Radar Control..."));
#ifdef __WXMSW__
    wxFont *qFont = OCPNGetFont(_("Menu"), 10);
    pmi->SetFont(*qFont);
#endif
    int miid = AddCanvasContextMenuItem(pmi, this);
    SetCanvasContextMenuItemViz(miid, true);


    wxMenuItem *pmi2 = new wxMenuItem(m_pmenu, -1, _("Set Guard Point"));
#ifdef __WXMSW__
    pmi->SetFont(*qFont);
#endif
    guard_zone_id = AddCanvasContextMenuItem(pmi2, this );
    SetCanvasContextMenuItemViz(guard_zone_id, false);
    guard_context_mode = false;

    //    Create the THREAD for Multicast radar data reception
    m_quit = false;
    m_dataReceiveThread = new RadarDataReceiveThread(this, &m_quit);
    m_dataReceiveThread->Run();
    m_commandReceiveThread = 0;
    if (settings.verbose >= 2) {
        m_commandReceiveThread = new RadarCommandReceiveThread(this, &m_quit);
        m_commandReceiveThread->Run();
    }
    m_reportReceiveThread = new RadarReportReceiveThread(this, &m_quit);
    m_reportReceiveThread->Run();

    return (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_OVERLAY_CALLBACK     |
            WANTS_CURSOR_LATLON        |
            WANTS_TOOLBAR_CALLBACK     |
            INSTALLS_TOOLBAR_TOOL      |
            INSTALLS_CONTEXTMENU_ITEMS |
            WANTS_CONFIG               |
            WANTS_NMEA_EVENTS          |
            WANTS_NMEA_SENTENCES       |
            WANTS_PREFERENCES          |
            WANTS_PLUGIN_MESSAGING
            );
}

bool br24radar_pi::DeInit(void)
{
    br_radar_state = RADAR_OFF;  // turn off, otherwise system may be too busy to quit
    SaveConfig();
    m_quit = true; // Signal quit to any of the threads. Takes up to 1s.

    if (m_dataReceiveThread) {
        m_dataReceiveThread->Wait();
        delete m_dataReceiveThread;
    }

    if (m_commandReceiveThread) {
        m_commandReceiveThread->Wait();
        delete m_commandReceiveThread;
    }

    if (m_reportReceiveThread) {
        m_reportReceiveThread->Wait();
        delete m_reportReceiveThread;
    }

    if (m_radar_socket != INVALID_SOCKET) {
        closesocket(m_radar_socket);
    }

    // I think we need to destroy any windows here

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

void br24radar_pi::SetDefaults(void)
{
    // This will be called upon enabling a PlugIn via the user Dialog.
    // We don't need to do anything special here.
}

void br24radar_pi::ShowPreferencesDialog(wxWindow* parent)
{
    m_pOptionsDialog = new BR24DisplayOptionsDialog;
    m_pOptionsDialog->Create(m_parent_window, this);
    m_pOptionsDialog->ShowModal();
}

void logBinaryData(const wxString& what, const UINT8 * data, int size)
{
    wxString explain;
    int i = 0;

    explain.Alloc(size * 3 + 50);
    explain += wxT("BR24radar_pi: ");
    explain += what;
    explain += wxString::Format(wxT(" %d bytes: "), size);
    for (i = 0; i < size; i++) {
        explain += wxString::Format(wxT(" %02X"), data[i]);
    }
    wxLogMessage(explain);
}

//*********************************************************************************
// Display Preferences Dialog
//*********************************************************************************
IMPLEMENT_CLASS(BR24DisplayOptionsDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24DisplayOptionsDialog, wxDialog)

EVT_CLOSE(BR24DisplayOptionsDialog::OnClose)
EVT_BUTTON(ID_OK, BR24DisplayOptionsDialog::OnIdOKClick)
EVT_RADIOBUTTON(ID_DISPLAYTYPE, BR24DisplayOptionsDialog::OnDisplayModeClick)
EVT_RADIOBUTTON(ID_OVERLAYDISPLAYOPTION, BR24DisplayOptionsDialog::OnDisplayOptionClick)

END_EVENT_TABLE()

BR24DisplayOptionsDialog::BR24DisplayOptionsDialog()
{
    Init();
}

BR24DisplayOptionsDialog::~BR24DisplayOptionsDialog()
{
}

void BR24DisplayOptionsDialog::Init()
{
}

bool BR24DisplayOptionsDialog::Create(wxWindow *parent, br24radar_pi *ppi)
{
    wxString m_temp;

    pParent = parent;
    pPlugIn = ppi;

    if (!wxDialog::Create(parent, wxID_ANY, _("BR24 Target Display Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)) {
        return false;
    }

    int font_size_y, font_descent, font_lead;
    GetTextExtent( _T("0"), NULL, &font_size_y, &font_descent, &font_lead );
    wxSize small_button_size( -1, (int) ( 1.4 * ( font_size_y + font_descent + font_lead ) ) );

    int border_size = 4;
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    wxFlexGridSizer * DisplayOptionsBox = new wxFlexGridSizer(2, 5, 5);
    topSizer->Add(DisplayOptionsBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

    //  BR24 toolbox icon checkbox
    //    wxStaticBox* DisplayOptionsCheckBox = new wxStaticBox(this, wxID_ANY, _T(""));
    //    wxStaticBoxSizer* DisplayOptionsCheckBoxSizer = new wxStaticBoxSizer(DisplayOptionsCheckBox, wxVERTICAL);
    //    DisplayOptionsBox->Add(DisplayOptionsCheckBoxSizer, 0, wxEXPAND | wxALL, border_size);

    //  Range Units options

    wxString RangeModeStrings[] = {
        _("Nautical Miles"),
        _("Kilometers"),
    };

    pRangeUnits = new wxRadioBox(this, ID_RANGE_UNITS, _("Range Units"),
                                 wxDefaultPosition, wxDefaultSize,
                                 2, RangeModeStrings, 1, wxRA_SPECIFY_COLS);
    DisplayOptionsBox->Add(pRangeUnits, 0, wxALL | wxEXPAND, 2);

    pRangeUnits->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                         wxCommandEventHandler(BR24DisplayOptionsDialog::OnRangeUnitsClick), NULL, this);

    pRangeUnits->SetSelection(pPlugIn->settings.range_units);

    /// Option settings
    wxString Overlay_Display_Options[] = {
        _("Monocolor-Red"),
        _("Multi-color"),
        _("Multi-color 2"),
    };

    pOverlayDisplayOptions = new wxRadioBox(this, ID_OVERLAYDISPLAYOPTION, _("Overlay Display Options"),
                                            wxDefaultPosition, wxDefaultSize,
                                            3, Overlay_Display_Options, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pOverlayDisplayOptions, 0, wxALL | wxEXPAND, 2);

    pOverlayDisplayOptions->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                                    wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayOptionClick), NULL, this);

    pOverlayDisplayOptions->SetSelection(pPlugIn->settings.display_option);

    //  Display Options
    //    wxStaticBox* itemStaticBoxSizerDisOptStatic = new wxStaticBox(this, wxID_ANY, _("Display Options"));
    //    wxStaticBoxSizer* itemStaticBoxSizerDisOpt = new wxStaticBoxSizer(itemStaticBoxSizerDisOptStatic, wxVERTICAL);
    //    DisplayOptionsBox->Add(itemStaticBoxSizerDisOpt, 0, wxEXPAND | wxALL, border_size);

    pDisplayMode = new wxRadioBox(this, ID_DISPLAYTYPE, _("Radar Display"),
                                  wxDefaultPosition, wxDefaultSize,
                                  ARRAY_SIZE(DisplayModeStrings), DisplayModeStrings, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pDisplayMode, 0, wxALL | wxEXPAND, 2);

    pDisplayMode->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                          wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayModeClick), NULL, this);

    pDisplayMode->SetSelection(pPlugIn->settings.display_mode);

    wxString GuardZoneStyleStrings[] = {
        _("Shading"),
        _("Outline"),
        _("Shading + Outline"),
    };
    pGuardZoneStyle = new wxRadioBox(this, ID_DISPLAYTYPE, _("Guard Zone Styling"),
                                     wxDefaultPosition, wxDefaultSize,
                                     3, GuardZoneStyleStrings, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsBox->Add(pGuardZoneStyle, 0, wxALL | wxEXPAND, 2);
    pGuardZoneStyle->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                             wxCommandEventHandler(BR24DisplayOptionsDialog::OnGuardZoneStyleClick), NULL, this);
    pGuardZoneStyle->SetSelection(pPlugIn->settings.guard_zone_render_style);


    //  Calibration
    wxStaticBox* itemStaticBoxCalibration = new wxStaticBox(this, wxID_ANY, _("Calibration"));
    wxStaticBoxSizer* itemStaticBoxSizerCalibration = new wxStaticBoxSizer(itemStaticBoxCalibration, wxVERTICAL);
    DisplayOptionsBox->Add(itemStaticBoxSizerCalibration, 0, wxEXPAND | wxALL, border_size);

    // Heading correction
    wxStaticText *pStatic_Heading_Correction = new wxStaticText(this, wxID_ANY, _("Heading correction\n(-180 to +180)"));
    itemStaticBoxSizerCalibration->Add(pStatic_Heading_Correction, 1, wxALIGN_LEFT | wxALL, 2);

    pText_Heading_Correction_Value = new wxTextCtrl(this, wxID_ANY);
    itemStaticBoxSizerCalibration->Add(pText_Heading_Correction_Value, 1, wxALIGN_LEFT | wxALL, border_size);
    m_temp.Printf(wxT("%2.1f"), pPlugIn->settings.heading_correction);
    pText_Heading_Correction_Value->SetValue(m_temp);
    pText_Heading_Correction_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                            wxCommandEventHandler(BR24DisplayOptionsDialog::OnHeading_Calibration_Value), NULL, this);

    // Guard Zone Alarm

    wxStaticBox* guardZoneBox = new wxStaticBox(this, wxID_ANY, _("Guard Zone Sound"));
    wxStaticBoxSizer* guardZoneSizer = new wxStaticBoxSizer(guardZoneBox, wxVERTICAL);
    DisplayOptionsBox->Add(guardZoneSizer, 0, wxEXPAND | wxALL, border_size);

    wxButton *pSelectSound = new wxButton(this, ID_SELECT_SOUND, _("Select Alert Sound"), wxDefaultPosition, small_button_size, 0);
    pSelectSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                          wxCommandEventHandler(BR24DisplayOptionsDialog::OnSelectSoundClick), NULL, this);
    guardZoneSizer->Add(pSelectSound, 0, wxALL, border_size);

    wxButton *pTestSound = new wxButton(this, ID_TEST_SOUND, _("Test Alert Sound"), wxDefaultPosition, small_button_size, 0);
    pTestSound->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                        wxCommandEventHandler(BR24DisplayOptionsDialog::OnTestSoundClick), NULL, this);
    guardZoneSizer->Add(pTestSound, 0, wxALL, border_size);

    // Accept/Reject button
    wxStdDialogButtonSizer* DialogButtonSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    topSizer->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, border_size);


    DimeWindow(this);

    Fit();
    SetMinSize(GetBestSize());

    return true;
}

void BR24DisplayOptionsDialog::OnRangeUnitsClick(wxCommandEvent &event)
{
    pPlugIn->settings.range_units = pRangeUnits->GetSelection();
}

void BR24DisplayOptionsDialog::OnDisplayOptionClick(wxCommandEvent &event)
{
    pPlugIn->settings.display_option = pOverlayDisplayOptions->GetSelection();
}

void BR24DisplayOptionsDialog::OnDisplayModeClick(wxCommandEvent &event)
{
    pPlugIn->SetDisplayMode((DisplayModeType) pDisplayMode->GetSelection());
}

void BR24DisplayOptionsDialog::OnGuardZoneStyleClick(wxCommandEvent &event)
{
    pPlugIn->settings.guard_zone_render_style = pGuardZoneStyle->GetSelection();
}

void BR24DisplayOptionsDialog::OnSelectSoundClick(wxCommandEvent &event)
{
    wxString *sharedData = GetpSharedDataLocation();
    wxString sound_dir;

    sound_dir.Append( *sharedData );
    sound_dir.Append( wxT("sounds") );

    wxFileDialog *openDialog = new wxFileDialog( NULL, _("Select Sound File"), sound_dir, wxT(""),
                                                _("WAV files (*.wav)|*.wav|All files (*.*)|*.*"), wxFD_OPEN );
    int response = openDialog->ShowModal();
    if (response == wxID_OK ) {
        pPlugIn->settings.alert_audio_file = openDialog->GetPath();
    }
}

void BR24DisplayOptionsDialog::OnTestSoundClick(wxCommandEvent &event)
{
    if (!pPlugIn->settings.alert_audio_file.IsEmpty()) {
        PlugInPlaySound(pPlugIn->settings.alert_audio_file);
    }
}

void BR24DisplayOptionsDialog::OnHeading_Calibration_Value(wxCommandEvent &event)
{
    wxString temp = pText_Heading_Correction_Value->GetValue();
    temp.ToDouble(&pPlugIn->settings.heading_correction);
}

void BR24DisplayOptionsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->SaveConfig();
    this->Hide();
}

void BR24DisplayOptionsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->SaveConfig();
    this->Hide();
}

//********************************************************************************
// Operation Dialogs - Control, Manual, and Options

void br24radar_pi::ShowRadarControl()
{
    if (!m_pControlDialog) {
        m_pControlDialog = new BR24ControlsDialog;
        m_pControlDialog->Create(m_parent_window, this);
        int range = (int) br_range_meters;    //  always use br_range_meters   this is the current value used in the pi
        //  will be updated in the receive thread if not correct
        m_pControlDialog->SetAutoRangeIndex(convertMetersToRadarAllowedValue(&range, settings.range_units, br_radar_type));
        // first time we display range from outside receive thread
    }
    m_pControlDialog->Show();
    m_pControlDialog->SetSize(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y,
                              m_BR24Controls_dialog_sx, m_BR24Controls_dialog_sy);
}

void br24radar_pi::OnContextMenuItemCallback(int id)
{
    if (!guard_context_mode) {
        ShowRadarControl();
    }

    if (guard_context_mode) {
        SetCanvasContextMenuItemViz(radar_control_id, false);
        mark_rng = local_distance(br_ownship_lat, br_ownship_lon, cur_lat, cur_lon);
        mark_brg = local_bearing(br_ownship_lat, br_ownship_lon, cur_lat, cur_lon);
        m_pGuardZoneDialog->OnContextMenuGuardCallback(mark_rng, mark_brg);
    }
}

void br24radar_pi::OnBR24ControlDialogClose()
{
    if (m_pControlDialog) {
        m_pControlDialog->GetPosition(&m_BR24Controls_dialog_x, &m_BR24Controls_dialog_y);
        m_pControlDialog->Hide();
        SetCanvasContextMenuItemViz(guard_zone_id, false);
    }

    SaveConfig();
}

void br24radar_pi::OnGuardZoneDialogClose()
{
    if (m_pGuardZoneDialog) {
        m_pGuardZoneDialog->GetPosition(&m_BR24Controls_dialog_x, &m_BR24Controls_dialog_y);
        m_pGuardZoneDialog->Hide();
        SetCanvasContextMenuItemViz(guard_zone_id, false);
        guard_context_mode = false;
        guard_bogey_confirmed = false;
        SaveConfig();
    }
    if (m_pControlDialog) {
        m_pControlDialog->UpdateGuardZoneState();
        m_pControlDialog->Show();
        m_pControlDialog->SetPosition(wxPoint(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y));
        SetCanvasContextMenuItemViz(radar_control_id, true);
    }

}

void br24radar_pi::OnGuardZoneBogeyConfirm()
{
    guard_bogey_confirmed = true; // This will stop the sound being repeated
}

void br24radar_pi::OnGuardZoneBogeyClose()
{
    guard_bogey_confirmed = true; // This will stop the sound being repeated
    if (m_pGuardZoneBogey) {
        m_pGuardZoneBogey->GetPosition(&m_GuardZoneBogey_x, &m_GuardZoneBogey_y);
        m_pGuardZoneBogey->Hide();
        // x        SaveConfig();
    }
}

void br24radar_pi::Select_Guard_Zones(int zone)
{
    settings.guard_zone = zone;

    if (!m_pGuardZoneDialog) {
        m_pGuardZoneDialog = new GuardZoneDialog;
        wxPoint pos = wxPoint(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y); // show at same loc as controls
        m_pGuardZoneDialog->Create(m_parent_window, this, wxID_ANY, _(" Guard Zone Control"), pos);
    }
    if (zone >= 0) {
        m_pControlDialog->GetPosition(&m_BR24Controls_dialog_x, &m_BR24Controls_dialog_y);
        m_pGuardZoneDialog->Show();
        m_pControlDialog->Hide();
        m_pGuardZoneDialog->SetPosition(wxPoint(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y));
        m_pGuardZoneDialog->OnGuardZoneDialogShow(zone);
        SetCanvasContextMenuItemViz(guard_zone_id, true);
        SetCanvasContextMenuItemViz(radar_control_id, false);
        guard_context_mode = true;
    }
    else {
        m_pGuardZoneDialog->Hide();
        SetCanvasContextMenuItemViz(guard_zone_id, false);
        SetCanvasContextMenuItemViz(radar_control_id, true);
        guard_context_mode = false;
    }
}

void br24radar_pi::SetDisplayMode(DisplayModeType mode)
{
    settings.display_mode = mode;
}

long br24radar_pi::GetRangeMeters()
{
    return (long) br_range_meters;
}

void br24radar_pi::UpdateDisplayParameters(void)
{
    RequestRefresh(GetOCPNCanvasWindow());
}

//*******************************************************************************
// ToolBar Actions

int br24radar_pi::GetToolbarToolCount(void)
{
    return 1;
}

void br24radar_pi::OnToolbarToolCallback(int id)
{
    if (br_radar_state == RADAR_OFF) {
        // turned off
        br_radar_state = RADAR_ON;
        settings.master_mode = true;
        if (settings.verbose) {
            wxLogMessage(wxT("BR24radar_pi: Master mode on"));
        }
        if (br_scanner_state != RADAR_ON) {
            // don't switch on if radar is on already, radar does not like that
            RadarTxOn();
            repeatOnDelay = 6;  // DoTick will not repeat the TxOn for 6 ticks
            // this is to prevent repeated "ON" commands
        }
        RadarStayAlive();   // moved to here, after TXOn
        RadarSendState();
        br_send_state = true; // Send state again as soon as we get any data
        if (id != 999999  && settings.timed_idle != 0) {
            m_pControlDialog->SetTimedIdleIndex(0) ; //Disable Timed Transmit if user click the icon while idle
        }
        ShowRadarControl();
    } else {
        br_radar_state = RADAR_OFF;
        if (settings.master_mode == true) {
            RadarTxOff();
            settings.master_mode = false;
            if (settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: Master mode off"));
            }
        }
        OnGuardZoneDialogClose();
        OnBR24ControlDialogClose();

        if (m_pGuardZoneBogey) {
            m_pGuardZoneBogey->Hide();
        }
    }

    UpdateState();
}

// DoTick
// ------
// Called on every RenderGLOverlay call, i.e. once a second.
//
// This checks if we need to ping the radar to keep it alive (or make it alive)
//*********************************************************************************
// Keeps Radar scanner on line if master and radar on -  run by RenderGLOverlay

void br24radar_pi::DoTick(void)
{
    time_t now = time(0);
    static time_t previousTicks = 0;

    if (now == previousTicks) {
        // Repeated call during scroll, do not do Tick processing
        return;
    }
    previousTicks = now;

    if (br_bpos_set && TIMER_ELAPSED(br_bpos_watchdog)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is continuously reset every time we receive a heading.
        br_bpos_set = false;
        wxLogMessage(wxT("BR24radar_pi: Lost Boat Position data"));
    }

    if (m_heading_source != HEADING_NONE && TIMER_ELAPSED(br_hdt_watchdog)) {
        // If the position data is 10s old reset our heading.
        // Note that the watchdog is continuously reset every time we receive a heading
        m_heading_source = HEADING_NONE;
        wxLogMessage(wxT("BR24radar_pi: Lost Heading data"));
        if (m_pControlDialog) {
            wxString info = wxT("");
            m_pControlDialog->SetHeadingInfo(info);
        }
    }

    if (br_var_source != VARIATION_SOURCE_NONE && TIMER_ELAPSED(br_var_watchdog)) {
        br_var_source = VARIATION_SOURCE_NONE;
        wxLogMessage(wxT("BR24radar_pi: Lost Variation source"));
        if (m_pControlDialog) {
            wxString info = wxT("");
            m_pControlDialog->SetVariationInfo(info);
        }
    }

    if (br_radar_seen && TIMER_ELAPSED(br_radar_watchdog)) {
        br_radar_seen = false;
        wxLogMessage(wxT("BR24radar_pi: Lost radar presence"));
    }
    if (m_statistics.spokes > m_statistics.broken_spokes) { // Something coming from radar unit?
        if (br_scanner_state != RADAR_ON) {
            wxLogMessage(wxT("BR24radar_pi: First radar data seen"));
        }
        br_scanner_state = RADAR_ON;
        if (settings.master_mode) {
            if (now - br_dt_stayalive >= STAYALIVE_TIMEOUT) {
                br_dt_stayalive = now;
                RadarStayAlive();
            }
            if (br_send_state) {
                RadarSendState();
                br_send_state = false;
            }
        }
        br_data_watchdog = now;
        br_data_seen = true;
    } else {
        br_scanner_state = RADAR_OFF;
        if (br_data_seen && TIMER_ELAPSED(br_data_watchdog)) {
            br_heading_on_radar = false;
            br_data_seen = false;
            wxLogMessage(wxT("BR24radar_pi: Lost radar data"));
        }
        if (br_radar_seen && !br_data_seen && settings.master_mode && br_scanner_state != RADAR_ON) {
            if (!repeatOnDelay) { // prevents sending repeated "ON" commands after turning on
                // Switch radar on if we want it to be on but it wasn' detected earlier
                RadarTxOn();
                RadarSendState();
                repeatOnDelay = 4;
            }
        }
    }
    if (repeatOnDelay) {
        repeatOnDelay--;   // count down the delay timer in every call of DoTick until 0
    }

    if (settings.PassHeadingToOCPN && br_heading_on_radar && br_radar_state == RADAR_ON) {
        wxString nmeastring;
        nmeastring.Printf(_T("$APHDT,%05.1f,M\r\n"), br_hdt );
        PushNMEABuffer(nmeastring);
    }

    wxString t;
    t.Printf(wxT("packets %d/%d\nspokes %d/%d/%d")
             , m_statistics.packets
             , m_statistics.broken_packets
             , m_statistics.spokes
             , m_statistics.broken_spokes
             , m_statistics.missing_spokes);

    if (m_pControlDialog) {
        m_pControlDialog->SetRadarInfo(t);
    }
    if (settings.verbose >= 2) {
        t.Replace(wxT("\n"), wxT(" "));
        wxLogMessage(wxT("BR24radar_pi: received %s, %d %d %d %d"), t.c_str(), br_bpos_set, m_heading_source, br_radar_seen, br_data_seen);
    }

    if (m_pControlDialog) {
        m_pControlDialog->UpdateMessage( br_opengl_mode
                                       , br_bpos_set
                                       , m_heading_source != HEADING_NONE
                                       , br_var_source != VARIATION_SOURCE_NONE
                                       , br_radar_seen
                                       , br_data_seen
                                       );
    }

    m_statistics.broken_packets = 0;
    m_statistics.broken_spokes  = 0;
    m_statistics.missing_spokes = 0;
    m_statistics.packets        = 0;
    m_statistics.spokes         = 0;

    /*******************************************
     Function Timed Transmit. Check if active
     ********************************************/
    if (settings.timed_idle != 0) {
        int factor = 5 * 60;
        if (br_last_idle_set == settings.timed_idle) {
            if (br_idle_watchdog > 0) {
                if (br_radar_state == RADAR_ON && (now > (br_idle_watchdog + (settings.idle_run_time * 60)) || onInit_Timed_Idle) ) {
                    onInit_Timed_Idle = false;
                    br_idle_watchdog = 0;
                    br24radar_pi::OnToolbarToolCallback(999999);    //Stop radar scanning
                }
                else if (br_radar_state == RADAR_OFF ) {
                    if (now > (br_idle_watchdog + (settings.timed_idle * factor))) {
                        br_idle_watchdog = 0;
                        if (m_pIdleDialog) m_pIdleDialog->Close();
                        br24radar_pi::OnToolbarToolCallback(999999);    //Start radar scanning
                    } else {
                        // Send minutes left to radar control
                        int time_left = ((br_idle_watchdog + (settings.timed_idle * factor)) - now)/60;
                        if (!m_pIdleDialog) {
                            m_pIdleDialog = new Idle_Dialog;
                            m_pIdleDialog->Create(m_parent_window, this);
                        }
                        if (IdleDialogTimeLeft != time_left) {
                            br24radar_pi::m_pIdleDialog->SetIdleTimes(settings.timed_idle * factor/60, time_left);
                            m_pIdleDialog->Show();
                            IdleDialogTimeLeft = time_left;
                        }
                    }
                }
            } else (br_idle_watchdog = now);
            return;
        }
        if (br_idle_set_count < 4) {    //Wait five turns, =5 sec, before action when the user is about to change it.
            br_idle_set_count ++;
            return;
        }
        br_idle_set_count = 0;
        if (br_last_idle_set == 0) onInit_Timed_Idle = true;    //Timed_Idle function init
        br_last_idle_set = settings.timed_idle;
    } else {
        br_idle_watchdog = 0;
        br_last_idle_set = 0;
    }
}

void br24radar_pi::UpdateState(void)   // -  run by RenderGLOverlay
{
    if (br_radar_state == RADAR_ON) {
        if (br_scanner_state  == RADAR_ON) {
            CacheSetToolbarToolBitmaps(BM_ID_GREEN, BM_ID_GREEN);     // ON
        } else {
            CacheSetToolbarToolBitmaps(BM_ID_AMBER, BM_ID_AMBER);     // Amber when scanner != radar
        }
    } else {
        if (br_scanner_state == RADAR_ON) {
            CacheSetToolbarToolBitmaps(BM_ID_AMBER, BM_ID_AMBER);
        } else {
            CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_RED);     // OFF
        }
    }
}

//***********************************************************************************************************
// Radar Image Graphic Display Processes
//***********************************************************************************************************

bool br24radar_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    br_opengl_mode = false;

    DoTick(); // update timers and watchdogs

    UpdateState(); // update the toolbar

    return true;
}

// Called by Plugin Manager on main system process cycle

bool br24radar_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    RenderOverlay_busy = true;   //  the receive thread should not queue another refresh (through refresh canvas) this when it is busy
    br_opengl_mode = true;
    // this is expected to be called at least once per second
    // but if we are scrolling or otherwise it can be MUCH more often!

    DoTick(); // update timers and watchdogs
    UpdateState(); // update the toolbar
    double max_distance = 0;
    wxPoint center_screen(vp->pix_width / 2, vp->pix_height / 2);
    wxPoint boat_center;

    if (br_bpos_set) {
        wxPoint pp;
        GetCanvasPixLL(vp, &pp, br_ownship_lat, br_ownship_lon);
        boat_center = pp;
    } else {
        boat_center = center_screen;
    }

    // now set a new value in the range control if an unsollicited range change has been received.
    // not for range change that the pi has initialized. For these the control was updated immediately

    if (rangeChangeReceived) {
        rangeChangeReceived = false;
        if (!setRangeIssued) {    // this range change was not initiated by the pi
            int current_range = (int) (br_range_meters * 0.65);
            if (m_pControlDialog) {
                m_pControlDialog->SetRangeIndex(convertMetersToRadarAllowedValue(&current_range, settings.range_units, br_radar_type));
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: range label changed from RenderOverlay to %d meters *.65 = %d"), br_range_meters, current_range);
                }
            }
        }
        else { //  set range issued, do not change the control value again
            setRangeIssued = false;
            if (settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: range label change skipped "));
            }
        }
    }


    // Calculate the "optimum" radar range setting in meters so Radar just fills Screen

    // We used to take the position of the boat into account, so that when you panned the zoom range would go up
    // This is not what the plotters do, so just to make it work the same way we're doing it the same way.
    // The radar range is set such that it covers the entire screen plus 50% so that a little panning is OK.
    // This is what the plotters do as well.

    if (settings.auto_range_mode) {
        max_distance = radar_distance(vp->lat_min, vp->lon_min, vp->lat_max, vp->lon_max, 'm');
        auto_range_meters =  max_distance / 2.0 * 0.7;        //* 1.5;  factor 1.5 removed. Covered by the radar as the actual range set is 1.4 to 1.7 times the ranges in the command
        // this is what the HDS display does as well. The range figure displayed will fit on the screen, but the real range set is larger
        // call convertMetersToRadarAllowedValue now to compute fitting allowed range
        // factor 0.9 added, range was too large, later changed to 0.7

        int displayedRange = auto_range_meters;  //  the value for use in the control
        size_t idx = convertMetersToRadarAllowedValue(&displayedRange, settings.range_units, br_radar_type);
        //    wxLogMessage(wxT("BR24radar_pi: screensize=%f autorange_meters=%d"), max_distance, auto_range_meters);
        if (auto_range_meters != previous_auto_range_meters)       {                        //   range change required
            if (settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: Automatic scale changed from %d to %d meters")
                             , previous_auto_range_meters, auto_range_meters);
            }
            previous_auto_range_meters = auto_range_meters;  // the value we send to the radar is NOT from the list of allowed values
            // the radar will accept any range, so we will be close to the range we want
            if (m_pControlDialog) {
                m_pControlDialog->SetAutoRangeIndex(idx);
                // Send command directly to radar
            }
            SetRangeMeters(auto_range_meters);
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

    switch (settings.display_mode) {
        case DM_CHART_OVERLAY:
        case DM_CHART_BLACKOUT:
        case DM_EMULATOR:
            RenderRadarOverlay(boat_center, v_scale_ppm, vp);
            break;
        case DM_SPECTRUM:
            if (br_radar_state == RADAR_ON) {
                RenderSpectrum(center_screen, v_scale_ppm, vp);
            }
            break;
    }
    RenderOverlay_busy = false;
    return true;
}

void br24radar_pi::RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
    if (settings.display_mode == DM_CHART_OVERLAY) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glPushMatrix();
    glTranslated(radar_center.x, radar_center.y, 0);

    double heading = MOD_DEGREES( rad2deg(vp->rotation)        // viewport rotation
                                - 90.0                         // difference between compass and OpenGL rotation
                                + settings.heading_correction  // fix any radome rotation fault
                                );
    glRotatef(heading, 0, 0, 1);

    // scaling...
    int meters = (int) br_range_meters;
    if (!meters) meters = auto_range_meters;
    if (!meters) meters = 1000;
    double radar_pixels_per_meter = ((double) RETURNS_PER_LINE) / meters;
    double scale_factor =  v_scale_ppm / radar_pixels_per_meter;  // screen pix/radar pix

    if ((br_bpos_set && m_heading_source != HEADING_NONE) || settings.display_mode == DM_EMULATOR) {
        glPushMatrix();
        glScaled(scale_factor, scale_factor, 1.);
        if ((int) br_range_meters && br_scanner_state == RADAR_ON) {
            DrawRadarImage(meters, radar_center);
        }
        glPopMatrix();
        // Guard Zone image
        if (br_radar_state == RADAR_ON) {
            if (guardZones[0].type != GZ_OFF || guardZones[1].type != GZ_OFF) {
                glRotatef(-settings.heading_correction + br_hdt, 0, 0, 1); //  Undo heading correction, and add heading to get relative zones
                RenderGuardZone(radar_center, v_scale_ppm, vp);
            }
        }
    }
    glPopMatrix();
    glPopAttrib();
}        // end of RenderRadarOverlay

/*
 * Precompute which angles returned from the radar are in which guard zones.
 * If there are many echoes from the radar we don't want to spend too much time
 * computing these.
 *
 * This needs to be called every time something changes in the GuardZone definitions
 * or heading correction.
 */
void br24radar_pi::ComputeGuardZoneAngles()
{
    int marks = 0;
    double angle_1, angle_2;
    for (size_t z = 0; z < GUARD_ZONES; z++) {
        switch (guardZones[z].type) {
            case GZ_CIRCLE:
                wxLogMessage(wxT("BR24radar_pi: GuardZone %d: circle at range %d to %d meters"), z + 1, guardZones[z].inner_range, guardZones[z].outer_range);
                angle_1 = 0.0;
                angle_2 = 360.0;
                break;
            case GZ_ARC:
                wxLogMessage(wxT("BR24radar_pi: GuardZone %d: bearing %f to %f range %d to %d meters"), z + 1
                             , guardZones[z].start_bearing
                             , guardZones[z].end_bearing
                             , guardZones[z].inner_range, guardZones[z].outer_range);
                angle_1 = guardZones[z].start_bearing + br_hdt;      // br_hdt added to provide guard zone relative to heading
                angle_2 = guardZones[z].end_bearing + br_hdt;        // br_hdt added to provide guard zone relative to heading
                break;
            default:
                wxLogMessage(wxT("BR24radar_pi: GuardZone %d: Off"), z + 1);
                angle_1 = 720.0; // Will never be reached, so no marks are set -> off...
                angle_2 = 720.0;
                break;
        }

        if (angle_1 > angle_2) {
            // fi. 270 to 90 means from left to right across boat.
            // Make this 270 to 450
            angle_2 += 360.0;
        }
        for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
            double angleDeg = MOD_DEGREES(SCALE_RAW_TO_DEGREES(i) + settings.heading_correction);

            bool mark = false;
            if (settings.verbose >= 4) {
                wxLogMessage(wxT("BR24radar_pi: GuardZone %d: angle %f < %f < %f"), z + 1, angle_1, angleDeg, angle_2);
            }
            if (angleDeg < angle_1) {
                angleDeg += 360.0;
            }
            if (angleDeg >= angle_1 && angleDeg <= angle_2) {
                mark = true;
                marks++;
            }
            guardZoneAngles[z][i] = mark;
        }
    }
    if (settings.verbose >= 3) {
        wxLogMessage(wxT("BR24radar_pi: ComputeGuardZoneAngles done, %d marks"), marks);
    }
}


void br24radar_pi::DrawRadarImage(int max_range, wxPoint radar_center)
{
    static unsigned int previousAngle = LINES_PER_ROTATION;
    static const double spokeWidthDeg = SCALE_RAW_TO_DEGREES(1);
    static const double spokeWidthRad = deg2rad(spokeWidthDeg); // How wide is one spoke?
    double angleDeg;
    double angleRad;

    wxLongLong now = wxGetLocalTimeMillis();
    UINT32 drawn_spokes = 0;
    UINT32 drawn_blobs  = 0;
    UINT32 skipped      = 0;
    wxLongLong max_age = 0; // Age in millis
    int bogey_count[GUARD_ZONES];

    downsample = (unsigned int) settings.downsample;
    refreshRate = refreshmapping [settings.refreshrate - 1];
    memset(&bogey_count, 0, sizeof(bogey_count));
    GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - settings.overlay_transparency) / MAX_OVERLAY_TRANSPARENCY;
    if (settings.verbose >= 4) {
        wxLogMessage(wxT("BR24radar_pi: ") wxTPRId64 wxT(" drawing start"), now);
    }
    if (downsample == 0) {
        wxLogMessage(wxT("BR24radar_pi: ASSERT FAILED: downsample not initialised"));
        // this should not happen, but prevent endless loop anyway
        downsample = 1;
    }
    // DRAWING PICTURE
    for (unsigned int angle = 0 ; angle <= LINES_PER_ROTATION - downsample; angle += downsample) {
        unsigned int scanAngle = angle, drawAngle = angle;
        scan_line * scan = 0;
        wxLongLong bestAge = settings.max_age * MILLISECONDS_PER_SECOND;
        // Find the newest scan in [angle, angle + downSample>
        for (unsigned int i = 0; i < downsample; i++) {
            scan_line * s = &m_scan_line[angle + i];
            wxLongLong diff = now - s->age;
            if (settings.verbose >= 4) {
                wxLogMessage(wxT("BR24radar_pi: ") wxT("    a=%d diff=%") wxTPRId64 wxT(" bestAge=%") wxTPRId64 wxT(" range=%d"), angle + i, diff, bestAge, s->range);
            }
            if (s->range && diff >= 0 && diff < bestAge) {
                scan = s;
                scanAngle = angle + i;
                while (scanAngle >= LINES_PER_ROTATION) scanAngle -= LINES_PER_ROTATION;
                bestAge = diff;
            }
        }
        if (!scan) {
            skipped++;
            continue;   // No or old data, don't show
        }

        if (bestAge > max_age) {
            max_age = bestAge;
        }
        unsigned int blobSpokesWide = downsample;
        if (settings.draw_algorithm == 1) {
            drawAngle = scanAngle;
            if (previousAngle < LINES_PER_ROTATION) {
                blobSpokesWide = (drawAngle - previousAngle + LINES_PER_ROTATION) % LINES_PER_ROTATION;
            }
            if (blobSpokesWide > LINES_PER_ROTATION / 16) {
                // Whoaaa, that would be much too wide. Fall back to normal width
                blobSpokesWide = downsample;
            }
            previousAngle = drawAngle;
        }

        // At this point we have:
        // scanAngle -- the angle in LINES_PER_ROTATION which has data
        // blobSpokesWide -- how many spokes wide this is going to be
        // Adjust the scanAngle accordingly

        double arc_width = spokeWidthRad * blobSpokesWide / 2.0;
        double arc_heigth = ((double) scan->range / (double) max_range);

        angleDeg = fmod((drawAngle - blobSpokesWide / 2.0 + 0.5) * spokeWidthDeg + 360.0, 360.0);
        angleRad = deg2rad(angleDeg);
        double angleCos = cos(angleRad);
        double angleSin = sin(angleRad);
        double r_begin = 0, r_end = 0;

        enum colors { BLOB_NONE, BLOB_BLUE, BLOB_GREEN, BLOB_RED };
        colors actual_color = BLOB_NONE, previous_color = BLOB_NONE;

        drawn_spokes++;

        scan->data[RETURNS_PER_LINE] = 0;  // make sure this element is initialized (just outside the range)
        for (int radius = 0; radius <= RETURNS_PER_LINE; ++radius) {   // loop 1 more time as only the previous one will be displayed
            GLubyte strength = (radius < RETURNS_PER_LINE) ? scan->data[radius] : 0;

            /**********************************************************************************************************/
            // Guard Section

            if (strength > 100) {
                for (size_t z = 0; z < GUARD_ZONES; z++) {
                    if (guardZoneAngles[z][scanAngle]) {
                        int inner_range = guardZones[z].inner_range; // now in meters
                        int outer_range = guardZones[z].outer_range; // now in meters
                        int bogey_range = radius * max_range / RETURNS_PER_LINE;
                        if (bogey_range > inner_range && bogey_range < outer_range) {
                            bogey_count[z]++;
                        }
                    }
                }
            }

            switch (settings.display_option) {
                    //  first find out the actual color
                case 0:
                    actual_color = BLOB_NONE;
                    if (strength > 50) {
                        actual_color = BLOB_RED;
                    }
                    break;

                case 1:
                    actual_color = BLOB_NONE;
                    if (strength > 200) {
                        actual_color = BLOB_RED;
                    } else if (strength > 100) {
                        actual_color = BLOB_GREEN;
                    } else if (strength > 50) {
                        actual_color = BLOB_BLUE;
                    }
                    break;

                case 2:
                    actual_color = BLOB_NONE;
                    if (strength > 250) {
                        actual_color = BLOB_RED;
                    } else if (strength > 100) {
                        actual_color = BLOB_GREEN;
                    } else if (strength > 20) {
                        actual_color = BLOB_BLUE;
                    }
                    break;
            }

            if (actual_color == BLOB_NONE && previous_color == BLOB_NONE) {
                // nothing to do, next radius
                continue;
            }

            if (actual_color == previous_color) {
                // continue with same color, just register it and continue with guard
                r_end += arc_heigth;
            }
            else if (previous_color == BLOB_NONE && actual_color != BLOB_NONE) {
                // blob starts, no display, just register
                r_begin = (double) radius * ((double) scan->range / (double) max_range);
                r_end = r_begin + arc_heigth;
                previous_color = actual_color;            // new color
            }
            else if (previous_color != BLOB_NONE && (previous_color != actual_color)) {
                // display time, first get the color in the glue byte
                GLubyte red = 0, green = 0, blue = 0;
                switch (previous_color) {
                    case BLOB_RED:
                        red = 255;
                        break;
                    case BLOB_GREEN:
                        green = 255;
                        break;
                    case BLOB_BLUE:
                        blue = 255;
                        break;
                    case BLOB_NONE:
                        break;   // just to prevent compile warnings
                }
                glColor4ub(red, green, blue, alpha);    // red, blue, green
                double heigth = r_end - r_begin;
                draw_blob_gl(angleCos, angleSin, r_begin, arc_width, heigth);
                drawn_blobs++;
                previous_color = actual_color;
                if (actual_color != BLOB_NONE) {            // change of color, start new blob
                    r_begin = (double) radius * ((double) scan->range / (double) max_range);
                    r_end = r_begin + arc_heigth;
                }
                else {            // actual_color == BLOB_NONE, blank pixel, next radius
                    continue;
                }
            }

        }   // end of loop over radius
    }
    if (settings.verbose >= 2) {
        now = wxGetLocalTimeMillis();
        wxLogMessage(wxT("BR24radar_pi: %") wxTPRId64 wxT(" drawn %u skipped %u spokes with %u blobs maxAge=%") wxTPRId64
                     wxT(" bogeys %d, %d")
                     , now, drawn_spokes, skipped, drawn_blobs, max_age, bogey_count[0], bogey_count[1]);
    }
    HandleBogeyCount(bogey_count);
}        // end of DrawRadarImage

void br24radar_pi::RenderSpectrum(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    int alpha = 0;
    long scan_distribution[255];    // intensity distribution

    memset(&scan_distribution[0], 0, 255);

    // wxCriticalSectionLocker locker(br_scanLock);

    for (int angle = 0 ; angle < LINES_PER_ROTATION ; angle++) {
        if (m_scan_line[angle].range != 0 ) {
            for (int radius = 1; radius < 510; ++radius) {
                alpha = m_scan_line[angle].data[radius];
                if (alpha > 0 && alpha < 255) {
                    scan_distribution[0] += 1;
                    scan_distribution[alpha] += 1;
                }
            }
        }
    }

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);
    glPushMatrix();

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glTranslated(10, vp->pix_height - 100, 0);
    glScaled(.5, -.5, 1);

    int x = 0, y = 0;
    for (x = 0; x < 254; ++x) {
        y = (int)(scan_distribution[x] * 100 / scan_distribution[0]);
        glColor4ub(x, 0, 255 - x, 255); // red, green, blue, alpha
        draw_histogram_column(x, y);
    }

    glPopMatrix();
    glPopAttrib();
}

void br24radar_pi::draw_histogram_column(int x, int y)  // x=0->255 => 0->1020, y=0->100 =>0->400
{
    int xa = 5 * x;
    int xb = xa + 5;
    y = 4 * y;

    glBegin(GL_TRIANGLES);        // draw blob in two triangles
    glVertex2i(xa, 0);
    glVertex2i(xb, 0);
    glVertex2i(xa, y);

    glVertex2i(xb, 0);
    glVertex2i(xb, y);
    glVertex2i(xa, y);

    glEnd();

}


//****************************************************************************
void br24radar_pi::RenderGuardZone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int start_bearing = 0, end_bearing = 0;
    GLubyte red = 0, green = 200, blue = 0, alpha = 50;

    for (size_t z = 0; z < GUARD_ZONES; z++) {

        if (guardZones[z].type != GZ_OFF) {
            if (guardZones[z].type == GZ_CIRCLE) {
                start_bearing = 0;
                end_bearing = 359;
            } else {
                start_bearing = guardZones[z].start_bearing;
                end_bearing = guardZones[z].end_bearing;
            }
            switch (settings.guard_zone_render_style) {
                case 1:
                    glColor4ub((GLubyte)255, (GLubyte)0, (GLubyte)0, (GLubyte)255);
                    DrawOutlineArc(guardZones[z].outer_range * v_scale_ppm, guardZones[z].inner_range * v_scale_ppm, start_bearing, end_bearing, true);
                    break;
                case 2:
                    glColor4ub(red, green, blue, alpha);
                    DrawOutlineArc(guardZones[z].outer_range * v_scale_ppm, guardZones[z].inner_range * v_scale_ppm, start_bearing, end_bearing, false);
                    // fall thru
                default:
                    glColor4ub(red, green, blue, alpha);
                    DrawFilledArc(guardZones[z].outer_range * v_scale_ppm, guardZones[z].inner_range * v_scale_ppm, start_bearing, end_bearing);
            }
        }

        red = 0; green = 0; blue = 200;
    }

    glPopAttrib();
}

void br24radar_pi::HandleBogeyCount(int *bogey_count)
{
    bool bogeysFound = false;

    for (int z = 0; z < GUARD_ZONES; z++) {
        if (bogey_count[z] > settings.guard_zone_threshold) {
            bogeysFound = true;
            break;
        }
    }

    if (bogeysFound) {
        // We have bogeys and there is no objection to showing the dialog

        if (settings.timed_idle != 0) m_pControlDialog->SetTimedIdleIndex(0) ; //Disable Timed Idle if set

        if (!m_pGuardZoneBogey) {
            // If this is the first time we have a bogey create & show the dialog immediately
            m_pGuardZoneBogey = new GuardZoneBogey;
            m_pGuardZoneBogey->Create(m_parent_window, this);
            m_pGuardZoneBogey->Show();
            m_pGuardZoneBogey->SetPosition(wxPoint(m_GuardZoneBogey_x, m_GuardZoneBogey_y));
        }
        else if (!guard_bogey_confirmed) {
            m_pGuardZoneBogey->Show();
        }
        time_t now = time(0);
        int delta_t = now - alarm_sound_last;
        if (!guard_bogey_confirmed && delta_t >= ALARM_TIMEOUT && bogeysFound) {
            // If the last time is 10 seconds ago we ping a sound, unless the user confirmed
            alarm_sound_last = now;

            if (!settings.alert_audio_file.IsEmpty()) {
                PlugInPlaySound(settings.alert_audio_file);
            }
            else {
                wxBell();
            }  // end of ping
            if (m_pGuardZoneBogey) {
                m_pGuardZoneBogey->Show();
            }
            delta_t = ALARM_TIMEOUT;
        }
        if (m_pGuardZoneBogey) {
            m_pGuardZoneBogey->SetBogeyCount(bogey_count, guard_bogey_confirmed ? -1 : ALARM_TIMEOUT - delta_t);
        }
    }

    if (!bogeysFound && m_pGuardZoneBogey) {
        m_pGuardZoneBogey->SetBogeyCount(bogey_count, -1);   // with -1 "next alarm in... "will not be displayed
        guard_bogey_confirmed = false; // Reset for next time we see bogeys
        // keep showing the bogey dialogue with 0 bogeys

    }

}



//****************************************************************************

bool br24radar_pi::LoadConfig(void)
{

    wxFileConfig *pConf = m_pconfig;

    if (pConf) {
        pConf->SetPath(wxT("/Plugins/BR24Radar"));
        pConf->Read(wxT("DisplayOption"), &settings.display_option, 0);
        pConf->Read(wxT("RangeUnits" ), &settings.range_units, 0 ); //0 = "Nautical miles"), 1 = "Kilometers"
        if (settings.range_units >= 2) {
            settings.range_units = 1;
        }
        settings.range_unit_meters = (settings.range_units == 1) ? 1000 : 1852;
        pConf->Read(wxT("DisplayMode"),  (int *) &settings.display_mode, 0);
        pConf->Read(wxT("VerboseLog"),  &settings.verbose, 0);
        pConf->Read(wxT("Transparency"),  &settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY);
        pConf->Read(wxT("Gain"),  &settings.gain, -1);
        pConf->Read(wxT("RainGain"),  &settings.rain_clutter_gain, 50);
        pConf->Read(wxT("ClutterGain"),  &settings.sea_clutter_gain, -1);
        pConf->Read(wxT("RangeCalibration"),  &settings.range_calibration, 1.0);
        pConf->Read(wxT("HeadingCorrection"),  &settings.heading_correction, 0);
        pConf->Read(wxT("BeamWidth"), &settings.beam_width, 2);
        pConf->Read(wxT("InterferenceRejection"), &settings.interference_rejection, 0);
        pConf->Read(wxT("TargetSeparation"), &settings.target_separation, 0);
        pConf->Read(wxT("NoiseRejection"), &settings.noise_rejection, 0);
        pConf->Read(wxT("TargetBoost"), &settings.target_boost, 0);
        pConf->Read(wxT("ScanMaxAge"), &settings.max_age, MIN_AGE);
        if (settings.max_age < MIN_AGE) {
            settings.max_age = MIN_AGE;
        } else if (settings.max_age > MAX_AGE) {
            settings.max_age = MAX_AGE;
        }
        pConf->Read(wxT("RunTimeOnIdle"), &settings.idle_run_time, 2);
        pConf->Read(wxT("DrawAlgorithm"), &settings.draw_algorithm, 1);
        pConf->Read(wxT("GuardZonesThreshold"), &settings.guard_zone_threshold, 5L);
        pConf->Read(wxT("GuardZonesRenderStyle"), &settings.guard_zone_render_style, 0);
        pConf->Read(wxT("ScanSpeed"), &settings.scan_speed, 0);
        pConf->Read(wxT("Downsample"), &settings.downsampleUser, 2);
        if (settings.downsampleUser < 1) {
            settings.downsampleUser = 1; // otherwise we get infinite loop
        }
        if (settings.downsampleUser > 8) {
            settings.downsampleUser = 1; // otherwise we get strange things
        }
        settings.downsample = 2 << (settings.downsampleUser - 1);
        pConf->Read(wxT("Refreshrate"), &settings.refreshrate, 1);
        if (settings.refreshrate < 1) {
            settings.refreshrate = 1; // not allowed
        }
        if (settings.refreshrate > 5) {
            settings.refreshrate = 5; // not allowed
        }
        refreshRate = refreshmapping[settings.refreshrate - 1];

        pConf->Read(wxT("PassHeadingToOCPN"), &settings.PassHeadingToOCPN, 0);

        pConf->Read(wxT("ControlsDialogSizeX"), &m_BR24Controls_dialog_sx, 300L);
        pConf->Read(wxT("ControlsDialogSizeY"), &m_BR24Controls_dialog_sy, 540L);
        pConf->Read(wxT("ControlsDialogPosX"), &m_BR24Controls_dialog_x, 20L);
        pConf->Read(wxT("ControlsDialogPosY"), &m_BR24Controls_dialog_y, 170L);

        pConf->Read(wxT("GuardZonePosX"), &m_GuardZoneBogey_x, 20L);
        pConf->Read(wxT("GuardZonePosY"), &m_GuardZoneBogey_y, 170L);

        pConf->Read(wxT("Zone1StBrng"), &guardZones[0].start_bearing, 0.0);
        pConf->Read(wxT("Zone1EndBrng"), &guardZones[0].end_bearing, 0.0);
        pConf->Read(wxT("Zone1OuterRng"), &guardZones[0].outer_range, 0);
        pConf->Read(wxT("Zone1InnerRng"), &guardZones[0].inner_range, 0);
        pConf->Read(wxT("Zone1ArcCirc"), &guardZones[0].type, 0);

        pConf->Read(wxT("Zone2StBrng"), &guardZones[1].start_bearing, 0.0);
        pConf->Read(wxT("Zone2EndBrng"), &guardZones[1].end_bearing, 0.0);
        pConf->Read(wxT("Zone2OuterRng"), &guardZones[1].outer_range, 0);
        pConf->Read(wxT("Zone2InnerRng"), &guardZones[1].inner_range, 0);
        pConf->Read(wxT("Zone2ArcCirc"), &guardZones[1].type, 0);

        pConf->Read(wxT("RadarAlertAudioFile"), &settings.alert_audio_file);

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
        pConf->Write(wxT("DisplayOption"), settings.display_option);
        pConf->Write(wxT("RangeUnits" ), settings.range_units);
        pConf->Write(wxT("DisplayMode"), (int)settings.display_mode);
        pConf->Write(wxT("VerboseLog"), settings.verbose);
        pConf->Write(wxT("Transparency"), settings.overlay_transparency);
        pConf->Write(wxT("Gain"), settings.gain);
        pConf->Write(wxT("RainGain"), settings.rain_clutter_gain);
        pConf->Write(wxT("ClutterGain"), settings.sea_clutter_gain);
        pConf->Write(wxT("RangeCalibration"),  settings.range_calibration);
        pConf->Write(wxT("HeadingCorrection"),  settings.heading_correction);
        pConf->Write(wxT("BeamWidth"),  settings.beam_width);
        pConf->Write(wxT("InterferenceRejection"), settings.interference_rejection);
        pConf->Write(wxT("TargetSeparation"), settings.target_separation);
        pConf->Write(wxT("NoiseRejection"), settings.noise_rejection);
        pConf->Write(wxT("TargetBoost"), settings.target_boost);
        pConf->Write(wxT("GuardZonesThreshold"), settings.guard_zone_threshold);
        pConf->Write(wxT("GuardZonesRenderStyle"), settings.guard_zone_render_style);
        pConf->Write(wxT("ScanMaxAge"), settings.max_age);
        pConf->Write(wxT("RunTimeOnIdle"), settings.idle_run_time);
        pConf->Write(wxT("DrawAlgorithm"), settings.draw_algorithm);
        pConf->Write(wxT("ScanSpeed"), settings.scan_speed);
        pConf->Write(wxT("Downsample"), settings.downsampleUser);
        pConf->Write(wxT("Refreshrate"), settings.refreshrate);
        pConf->Write(wxT("PassHeadingToOCPN"), settings.PassHeadingToOCPN);
        pConf->Write(wxT("RadarAlertAudioFile"), settings.alert_audio_file);

        pConf->Write(wxT("ControlsDialogSizeX"),  m_BR24Controls_dialog_sx);
        pConf->Write(wxT("ControlsDialogSizeY"),  m_BR24Controls_dialog_sy);
        pConf->Write(wxT("ControlsDialogPosX"),   m_BR24Controls_dialog_x);
        pConf->Write(wxT("ControlsDialogPosY"),   m_BR24Controls_dialog_y);

        pConf->Write(wxT("GuardZonePosX"),   m_GuardZoneBogey_x);
        pConf->Write(wxT("GuardZonePosY"),   m_GuardZoneBogey_y);

        pConf->Write(wxT("Zone1StBrng"), guardZones[0].start_bearing);
        pConf->Write(wxT("Zone1EndBrng"), guardZones[0].end_bearing);
        pConf->Write(wxT("Zone1OuterRng"), guardZones[0].outer_range);
        pConf->Write(wxT("Zone1InnerRng"), guardZones[0].inner_range);
        pConf->Write(wxT("Zone1ArcCirc"), guardZones[0].type);

        pConf->Write(wxT("Zone2StBrng"), guardZones[1].start_bearing);
        pConf->Write(wxT("Zone2EndBrng"), guardZones[1].end_bearing);
        pConf->Write(wxT("Zone2OuterRng"), guardZones[1].outer_range);
        pConf->Write(wxT("Zone2InnerRng"), guardZones[1].inner_range);
        pConf->Write(wxT("Zone2ArcCirc"), guardZones[1].type);

        pConf->Flush();
        //wxLogMessage(wxT("BR24radar_pi: saved config"));

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
    if (br_var_source <= VARIATION_SOURCE_FIX && !wxIsNaN(pfix.Var) && (fabs(pfix.Var) > 0.0 || br_var == 0.0)) {
        if (br_var_source < VARIATION_SOURCE_FIX || fabs(pfix.Var - br_var) > 0.0) {
            wxLogMessage(wxT("BR24radar_pi: Position fix provides new magnetic variation %f"), pfix.Var);
        }
        br_var = pfix.Var;
        br_var_source = VARIATION_SOURCE_FIX;
        br_var_watchdog = now;
        if (m_pControlDialog) {
            info = _("GPS");
            info << wxT(" ") << br_var;
            m_pControlDialog->SetVariationInfo(info);
        }
    }

    if (settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: SetPositionFixEx var=%f heading_on_radar=%d br_var_wd=%d br_radar_state=%d")
                    , pfix.Var
                    , br_heading_on_radar
                    , TIMER_NOT_ELAPSED(br_var_watchdog)
                    , br_radar_state
                    );
    }
    if (br_heading_on_radar && TIMER_NOT_ELAPSED(br_var_watchdog) && br_radar_state == RADAR_ON) {
        if (m_heading_source != HEADING_RADAR) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now Radar %f \n"), br_hdt);
            m_heading_source = HEADING_RADAR;
        }
        if (m_pControlDialog) {
            info = _("radar");
            info << wxT(" ") << br_hdt;
            m_pControlDialog->SetHeadingInfo(info);
        }
        br_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Hdm) && TIMER_NOT_ELAPSED(br_var_watchdog)) {
        br_hdt = pfix.Hdm + br_var;
        if (m_heading_source != HEADING_HDM) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now HDM %f"), br_hdt);
            m_heading_source = HEADING_HDM;
        }
        if (m_pControlDialog) {
            info = _("HDM");
            info << wxT(" ") << br_hdt;
            m_pControlDialog->SetHeadingInfo(info);
        }
        br_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Hdt)) {
        br_hdt = pfix.Hdt;
        if (m_heading_source != HEADING_HDT) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now HDT"));
            m_heading_source = HEADING_HDT;
        }
        if (m_pControlDialog) {
            info = _("HDT");
            info << wxT(" ") << br_hdt;
            m_pControlDialog->SetHeadingInfo(info);
        }
        br_hdt_watchdog = now;
    }
    else if (!wxIsNaN(pfix.Cog)) {
        br_hdt = pfix.Cog;
        if (m_heading_source != HEADING_COG) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now COG"));
            m_heading_source = HEADING_COG;
        }
        if (m_pControlDialog) {
            info = _("COG");
            info << wxT(" ") << br_hdt;
            m_pControlDialog->SetHeadingInfo(info);
        }
        br_hdt_watchdog = now;
    }

    if (pfix.FixTime && TIMER_NOT_ELAPSED(pfix.FixTime)) {
        br_ownship_lat = pfix.Lat;
        br_ownship_lon = pfix.Lon;
        if (!br_bpos_set) {
            wxLogMessage(wxT("BR24radar_pi: GPS position is now known"));
        }
        br_bpos_set = true;
        br_bpos_watchdog = now;
    }
}

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
                if (br_var_source != VARIATION_SOURCE_WMM) {
                    wxLogMessage(wxT("BR24radar_pi: WMM plugin provides new magnetic variation %f"), variation);
                }
                br_var = variation;
                br_var_source = VARIATION_SOURCE_WMM;
                br_var_watchdog = time(0);
                if (m_pControlDialog) {
                    wxString info = _("WMM");
                    info << wxT(" ") << br_var;
                    m_pControlDialog->SetVariationInfo(info);
                }
            }
        }
    }
}

//**************** Cursor position events **********************
void br24radar_pi::SetCursorLatLon(double lat, double lon)
{
    cur_lat = lat;
    cur_lon = lon;
}

//************************************************************************
// Plugin Command Data Streams
//************************************************************************
void br24radar_pi::TransmitCmd(UINT8 * msg, int size)
{
    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl((236 << 24) | (6 << 16) | (7 << 8) | 10); // 236.6.7.10
    adr.sin_port=htons(6680);

    if (m_radar_socket == INVALID_SOCKET || sendto(m_radar_socket, (char *) msg, size, 0, (struct sockaddr *) &adr, sizeof(adr)) < size) {
        wxLogError(wxT("BR24radar_pi: Unable to transmit command to radar"));
        return;
    } else if (settings.verbose) {
        logBinaryData(wxT("command"), msg, size);
    }
};

void br24radar_pi::RadarTxOff(void)
{
    //    wxLogMessage(wxT("BR24radar_pi: radar turned Off manually."));

    UINT8 pck[3] = {0x00, 0xc1, 0x00};
    TransmitCmd(pck, sizeof(pck));

    pck[0] = 0x01;
    TransmitCmd(pck, sizeof(pck));
}

void br24radar_pi::RadarTxOn(void)
{
    wxLogMessage(wxT("BR24radar_pi: Send TRANSMIT request"));

    UINT8 pck[3] = {0x00, 0xc1, 0x01};               // ON
    TransmitCmd(pck, sizeof(pck));

    pck[0] = 0x01;
    TransmitCmd(pck, sizeof(pck));
}

void br24radar_pi::RadarStayAlive(void)
{
    if (settings.master_mode && (br_radar_state == RADAR_ON)) {
        UINT8 pck[] = {0xA0, 0xc1};
        TransmitCmd(pck, sizeof(pck));
    }
}

void br24radar_pi::RadarSendState(void)
{
    SetControlValue(CT_GAIN, settings.gain);
    SetControlValue(CT_RAIN, settings.rain_clutter_gain);
    SetControlValue(CT_SEA, settings.sea_clutter_gain);
    SetControlValue(CT_INTERFERENCE_REJECTION, settings.interference_rejection);
    SetControlValue(CT_TARGET_SEPARATION, settings.target_separation);
    SetControlValue(CT_NOISE_REJECTION, settings.noise_rejection);
    SetControlValue(CT_TARGET_BOOST, settings.target_boost);
    SetControlValue(CT_SCAN_SPEED, settings.scan_speed);
}

void br24radar_pi::SetRangeMeters(long meters)
{
    if (settings.master_mode) {
        if (meters >= 50 && meters <= 64000) {
            //         long decimeters = meters * 10L/1.762;   //  why divide by 1.762? HDS display sends range*10
            long decimeters = meters * 10L;            // modification by Douwe Fokkema
            UINT8 pck[] = { 0x03
                , 0xc1
                , (UINT8) ((decimeters >>  0) & 0XFFL)
                , (UINT8) ((decimeters >>  8) & 0XFFL)
                , (UINT8) ((decimeters >> 16) & 0XFFL)
                , (UINT8) ((decimeters >> 24) & 0XFFL)
            };
            if (settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: SetRangeMeters: %ld meters\n"), meters);
            }
            TransmitCmd(pck, sizeof(pck));
            //  do not update radar control value here, is only done from receive thread
            setRangeIssued = true;
        }
    }
}

void br24radar_pi::SetControlValue(ControlType controlType, int value)
{
    wxString msg;

    if (settings.master_mode || controlType == CT_TRANSPARENCY || controlType == CT_SCAN_AGE) {
        switch (controlType) {
            case CT_GAIN: {
                settings.gain = value;
                if (value < 0) {                // AUTO gain
                    UINT8 cmd[] = {
                        0x06,
                        0xc1,
                        0, 0, 0, 0, 0x01,
                        0, 0, 0, 0xa1
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("BR24radar_pi: Gain: Auto"));
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                } else {                        // Manual Gain
                    int v = value * 255 / 100;
                    if (v > 255) {
                        v = 255;
                    }
                    UINT8 cmd[] = {
                        0x06,
                        0xc1,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        (UINT8) v
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("BR24radar_pi: Gain: %d"), value);
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                }
                break;
            }
            case CT_RAIN: {                       // Rain Clutter - Manual. Range is 0x01 to 0x50
                settings.rain_clutter_gain = value;
                int v = value * 0x50 / 100;
                if (v > 255) {
                    v = 255;
                }

                UINT8 cmd[] = {
                    0x06,
                    0xc1,
                    0x04,
                    0, 0, 0, 0, 0, 0, 0,
                    (UINT8) v
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Rain: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_SEA: {
                settings.sea_clutter_gain = value;
                if (value < 0) {                 // Sea Clutter - Auto
                    UINT8 cmd[11] = {
                        0x06,
                        0xc1,
                        0x02,
                        0, 0, 0, 0x01,
                        0, 0, 0, 0xd3
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("BR24radar_pi: Sea: Auto"));
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                } else {                       // Sea Clutter
                    int v = value * 255 / 100;
                    if (v > 255) {
                        v = 255;
                    }
                    UINT8 cmd[] = {
                        0x06,
                        0xc1,
                        0x02,
                        0, 0, 0, 0, 0, 0, 0,
                        (UINT8) v
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("BR24radar_pi: Sea: %d"), value);
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                }
                break;
            }
            case CT_INTERFERENCE_REJECTION: {
                settings.interference_rejection = value;
                UINT8 cmd[] = {
                    0x08,
                    0xc1,
                    (UINT8) settings.interference_rejection
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Rejection: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_TARGET_SEPARATION: {
                settings.target_separation = value;
                UINT8 cmd[] = {
                    0x22,
                    0xc1,
                    (UINT8) value
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Target separation: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_NOISE_REJECTION: {
                settings.noise_rejection = value;
                UINT8 cmd[] = {
                    0x21,
                    0xc1,
                    (UINT8) settings.noise_rejection
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Noise rejection: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_TARGET_BOOST: {
                settings.target_boost = value;
                UINT8 cmd[] = {
                    0x0a,
                    0xc1,
                    (UINT8) value
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Target boost: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_SCAN_SPEED: {
                settings.scan_speed = value;
                UINT8 cmd[] = {
                    0x0f,
                    0xc1,
                    (UINT8) value
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: Scan speed: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_TRANSPARENCY: {
                settings.overlay_transparency = value;
                break;
            }
            case CT_SCAN_AGE: {
                settings.max_age = value;
                break;
            }
            case CT_TIMED_IDLE: {
                settings.timed_idle = value;
                break;
            }
            case CT_DOWNSAMPLE: {
                settings.downsampleUser = value;
                settings.downsample = 2 << (settings.downsampleUser - 1);
                break;
            }

            case CT_REFRESHRATE: {
                settings.refreshrate = value;
                break;
            }

            case CT_PASSHEADING: {
                settings.PassHeadingToOCPN = value;
                break;
            }

            default: {
                wxLogMessage(wxT("BR24radar_pi: Unhandled control setting for control %d"), controlType);
            }
        }
    }
    else {
        wxLogMessage(wxT("BR24radar_pi: Not master, so ignore SetControlValue: %d %d"), controlType, value);
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
            if (settings.verbose >= 2) {
                wxLogMessage(wxT("BR24radar_pi: received HDG variation=%f var_source=%d br_var=%f"), m_NMEA0183.Hdg.MagneticVariationDegrees, br_var_source, br_var);
            }
            if (!wxIsNaN(m_NMEA0183.Hdg.MagneticVariationDegrees) &&
                (br_var_source <= VARIATION_SOURCE_NMEA || (br_var == 0.0 && m_NMEA0183.Hdg.MagneticVariationDegrees > 0.0))) {
                double newVar;
                if (m_NMEA0183.Hdg.MagneticVariationDirection == East) {
                    newVar = +m_NMEA0183.Hdg.MagneticVariationDegrees;
                }
                else {
                    newVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
                }
                if (fabs(newVar - br_var) >= 0.1) {
                    wxLogMessage(wxT("BR24radar_pi: NMEA provides new magnetic variation %f"), newVar);
                }
                br_var = newVar;
                br_var_source = VARIATION_SOURCE_NMEA;
                br_var_watchdog = now;
                if (m_pControlDialog) {
                    wxString info = _("NMEA");
                    info << wxT(" ") << br_var;
                    m_pControlDialog->SetVariationInfo(info);
                }
            }
            if (m_heading_source == HEADING_HDM && !wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees)) {
                br_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + br_var;
                br_hdt_watchdog = now;
            }
        }
        else if (m_heading_source == HEADING_HDM
              && m_NMEA0183.LastSentenceIDReceived == _T("HDM")
              && m_NMEA0183.Parse()
              && !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic)) {
            br_hdt = m_NMEA0183.Hdm.DegreesMagnetic + br_var;
            br_hdt_watchdog = now;
        }
        else if (m_heading_source == HEADING_HDT
              && m_NMEA0183.LastSentenceIDReceived == _T("HDT")
              && m_NMEA0183.Parse()
              && !wxIsNaN(m_NMEA0183.Hdt.DegreesTrue)) {
            br_hdt = m_NMEA0183.Hdt.DegreesTrue;
            br_hdt_watchdog = now;
        }
    }
}


// Ethernet packet stuff *************************************************************


RadarDataReceiveThread::~RadarDataReceiveThread()
{
}

void RadarDataReceiveThread::OnExit()
{
    //  wxLogMessage(wxT("BR24radar_pi: radar thread is stopping."));
}

static int my_inet_aton(const char *cp, struct in_addr *addr)
{
    register u_long val;
    register int base, n;
    register char c;
    u_int parts[4];
    register u_int *pp = parts;

    c = *cp;
    for (;;) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, isdigit=decimal.
         */
        if (!isdigit(c)) {
            return (0);
        }
        val = 0;
        base = 10;
        if (c == '0') {
            c = *++cp;
            if (c == 'x' || c == 'X') {
                base = 16, c = *++cp;
            } else {
                base = 8;
            }
        }
        for (;;) {
            if (isascii(c) && isdigit(c)) {
                val = (val * base) + (c - '0');
                c = *++cp;
            }
            else if (base == 16 && isascii(c) && isxdigit(c)) {
                val = (val << 4) |
                (c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            } else {
                break;
            }
        }
        if (c == '.') {
            /*
             * Internet format:
             *    a.b.c.d
             *    a.b.c    (with c treated as 16 bits)
             *    a.b    (with b treated as 24 bits)
             */
            if (pp >= parts + 3) {
                return 0;
            }
            *pp++ = val;
            c = *++cp;
        } else {
            break;
        }
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && (!isascii(c) || !isspace(c))) {
        return 0;
    }
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = pp - parts + 1;
    switch (n) {

        case 0:
            return 0;        /* initial nondigit */

        case 1:                /* a -- 32 bits */
            break;

        case 2:                /* a.b -- 8.24 bits */
            if (val > 0xffffff) {
                return 0;
            }
            val |= parts[0] << 24;
            break;

        case 3:                /* a.b.c -- 8.8.16 bits */
            if (val > 0xffff) {
                return 0;
            }
            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

        case 4:                /* a.b.c.d -- 8.8.8.8 bits */
            if (val > 0xff) {
                return 0;
            }
            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
    }
    if (addr) {
        addr->s_addr = htonl(val);
    }
    return 1;
}

static bool socketReady( SOCKET sockfd, int timeout )
{
    int r = 0;
    fd_set fdin;
    struct timeval tv = { (long) timeout / MILLISECONDS_PER_SECOND, (long) (timeout % MILLISECONDS_PER_SECOND) * MILLISECONDS_PER_SECOND };

    FD_ZERO(&fdin);
    if (sockfd != INVALID_SOCKET) {
        FD_SET(sockfd, &fdin);
        r = select(sockfd + 1, &fdin, 0, &fdin, &tv);
    } else {
#ifndef __WXMSW__
        // Common UNIX style sleep, unlike 'sleep' this causes no alarms
        // and has fewer threading issues.
        select(1, 0, 0, 0, &tv);
#else
        Sleep(timeout);
#endif
        r = 0;
    }

    return r > 0;
}

static SOCKET startUDPMulticastReceiveSocket( br24radar_pi *pPlugIn, struct sockaddr_in * addr, UINT16 port, const char * mcastAddr)
{
    SOCKET rx_socket;
    struct sockaddr_in adr;
    int one = 1;
    wxString errorMsg;

    if (!addr) {
        return INVALID_SOCKET;
    }

    UINT8 * a = (UINT8 *) &addr->sin_addr; // sin_addr is in network layout
    wxString address;
    address.Printf(wxT(" %u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);

    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = htonl(INADDR_ANY); // I know, htonl is unnecessary here
    adr.sin_port = htons(port);
    rx_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rx_socket == INVALID_SOCKET) {
        errorMsg << _("Cannot create UDP socket");
        goto fail;
    }
    if (setsockopt(rx_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &one, sizeof(one))) {
        errorMsg << _("Cannot set reuse address option on socket");
        goto fail;
    }

    if (bind(rx_socket, (struct sockaddr *) &adr, sizeof(adr))) {
        errorMsg << _("Cannot bind UDP socket to port ") << port;
        goto fail;
    }

    // Subscribe rx_socket to a multicast group
    struct ip_mreq mreq;
    mreq.imr_interface = addr->sin_addr;

    if (!my_inet_aton(mcastAddr, &mreq.imr_multiaddr)) {
        errorMsg << _("Invalid multicast address") << wxT(" ") << wxString::FromUTF8(mcastAddr);
        goto fail;
    }

    if (setsockopt(rx_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *) &mreq, sizeof(mreq))) {
        errorMsg << _("Invalid IP address for UDP multicast");
        goto fail;
    }

    // Hurrah! Success!
    return rx_socket;

fail:
    errorMsg << wxT(" ") << address;
    wxLogError(wxT("BR2radar_pi: %s"), errorMsg.c_str());
    if (pPlugIn && pPlugIn->m_pControlDialog) {
        pPlugIn->m_pControlDialog->SetErrorMessage(errorMsg);
    }
    if (rx_socket != INVALID_SOCKET) {
        closesocket(rx_socket);
    }
    return INVALID_SOCKET;
}

void *RadarDataReceiveThread::Entry(void)
{
    SOCKET rx_socket = INVALID_SOCKET;
    int r = 0;

    sockaddr_storage rx_addr;
    socklen_t        rx_len;

    //    Loop until we quit
    while (!*m_quit) {
        if (pPlugIn->settings.display_mode == DM_EMULATOR) {
            socketReady(INVALID_SOCKET, 1000); // sleep for 1s
            emulate_fake_buffer();
            if (pPlugIn->m_pControlDialog) {
                wxString ip;
                ip << _("emulator");
                pPlugIn->m_pControlDialog->SetRadarIPAddress(ip);
            }
        }
        else {
            if (rx_socket == INVALID_SOCKET) {
                rx_socket = startUDPMulticastReceiveSocket(pPlugIn, br_mcast_addr, 6678, "236.6.7.8");
                // If it is still INVALID_SOCKET now we just sleep for 1s in socketReady
                if (rx_socket != INVALID_SOCKET) {
                    wxString addr;
                    UINT8 * a = (UINT8 *) &br_mcast_addr->sin_addr; // sin_addr is in network layout
                    addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);
                    wxLogMessage(wxT("BR24radar_pi: Listening for radar data on %s"), addr.c_str());
                }
            }

            if (socketReady(rx_socket, 1000)) {
                radar_frame_pkt packet;
                rx_len = sizeof(rx_addr);
                r = recvfrom(rx_socket, (char *) &packet, sizeof(packet), 0, (struct sockaddr *) &rx_addr, &rx_len);
                if (r > 0) {
                    process_buffer(&packet, r);
                }
                if (r < 0 || !br_mcast_addr || !br_data_seen || !br_radar_seen) {
                    closesocket(rx_socket);
                    rx_socket = INVALID_SOCKET;
                }
            }

            if (!br_radar_seen || !br_mcast_addr) {
                if (rx_socket != INVALID_SOCKET) {
                    wxLogMessage(wxT("BR24radar_pi: Stopped listening for radar data"));
                    closesocket(rx_socket);
                    rx_socket = INVALID_SOCKET;
                }
            }
        }
    }

    if (rx_socket != INVALID_SOCKET) {
        closesocket(rx_socket);
    }
    return 0;
}

// process_buffer
// --------------
// Process one radar frame packet, which can contain up to 32 'spokes' or lines extending outwards
// from the radar up to the range indicated in the packet.
//
// We only get Data packets of fixed length from PORT (6678), see structure in .h file
//
void RadarDataReceiveThread::process_buffer(radar_frame_pkt * packet, int len)
{
    wxLongLong nowMillis = wxGetLocalTimeMillis();
    time_t now = time(0);
    br_radar_seen = true;
    br_radar_watchdog = now;

    // wxCriticalSectionLocker locker(br_scanLock);

    static int next_scan_number = -1;
    int scan_number = 0;
    pPlugIn->m_statistics.packets++;

    if (len < (int) sizeof(packet->frame_hdr)) {
        pPlugIn->m_statistics.broken_packets++;
        return;
    }
    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(radar_line);

    if (scanlines_in_packet != 32) {
        pPlugIn->m_statistics.broken_packets++;
    }

    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        radar_line * line = &packet->line[scanline];

        // Validate the spoke
        scan_number = line->br24.scan_number[0] | (line->br24.scan_number[1] << 8);
        pPlugIn->m_statistics.spokes++;
        if (line->br24.headerLen != 0x18) {
            if (pPlugIn->settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: strange header length %d"), line->br24.headerLen);
            }
            // Do not draw something with this...
            pPlugIn->m_statistics.missing_spokes++;
            next_scan_number = (scan_number + 1) % LINES_PER_ROTATION;
            continue;
        }
        if (line->br24.status != 0x02 && line->br24.status != 0x12) {
            if (pPlugIn->settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: strange status %02x"), line->br24.status);
            }
            pPlugIn->m_statistics.broken_spokes++;
        }
        if (next_scan_number >= 0 && scan_number != next_scan_number) {
            if (scan_number > next_scan_number) {
                pPlugIn->m_statistics.missing_spokes += scan_number - next_scan_number;
            } else {
                pPlugIn->m_statistics.missing_spokes += LINES_PER_ROTATION + scan_number - next_scan_number;
            }
        }
        next_scan_number = (scan_number + 1) % LINES_PER_ROTATION;

        int range_raw = 0;
        int angle_raw = 0;

        short int hdm_raw = 0;

        short int large_range = 0;
        short int small_range = 0;
        int range_meters = 0;


        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0) {
            // BR24 and 3G mode
            range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
            range_meters = (int) ((double)range_raw * 10.0 / sqrt(2.0));

            br_radar_type = RT_BR24;
        } else {
            // 4G mode
            large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
            small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];

            if (large_range == 0x80) {
                if (small_range == -1) {
                    range_raw = 0; // Invalid range received
                } else {
                    range_raw = small_range;
                }
            } else {
                range_raw = large_range * 256;
            }
            range_meters = range_raw / 4;
            br_radar_type = RT_4G;
        }

        // Range change received from radar?

        if (range_meters != (int) br_range_meters) {

            if (pPlugIn->settings.verbose >= 1) {
                if (range_meters == 0) {
                    wxLogMessage(wxT("BR24radar_pi:  Invalid range received, keeping %d meters"), br_range_meters);
                }
                else {
                    wxLogMessage(wxT("BR24radar_pi:  Range Change: %d --> %d meters (raw value: %d"), br_range_meters, range_meters, range_raw);
                }
            }

            br_range_meters = (long) range_meters;
            rangeChangeReceived = true;  // to signal render overlay to change control value

            // Set the control's value to the real range that we received, not a table idea
            // will be handled in RenderOverlay, not safe to to screen IO from receive thread
        }

        hdm_raw = (line->br4g.heading[1] << 8) | line->br4g.heading[0];
        if (hdm_raw != INT16_MIN && TIMER_NOT_ELAPSED(br_var_watchdog) && br_radar_type == RT_4G) {
            br_heading_on_radar = true;                            // heading on radar
            br_hdt_raw = MOD_ROTATION(hdm_raw + SCALE_DEGREES_TO_RAW(br_var));
            br_hdt = MOD_DEGREES(SCALE_RAW_TO_DEGREES(br_hdt_raw));
            angle_raw += br_hdt_raw;
        }
        else {                                // no heading on radar
            br_heading_on_radar = false;
            br_hdt_raw = SCALE_DEGREES_TO_RAW(br_hdt);
            angle_raw += br_hdt_raw;             // map spoke on true direction
        }
        angle_raw = MOD_ROTATION(angle_raw);

        UINT8 *dest_data1 = pPlugIn->m_scan_line[angle_raw].data;
        memcpy(dest_data1, line->data, RETURNS_PER_LINE);

        // The following line is a quick hack to confirm on-screen where the range ends, by putting a 'ring' of
        // returned radar energy at the max range line.
        // TODO: create nice actual range circles.
        dest_data1[RETURNS_PER_LINE - 1] = 0xff;

        pPlugIn->m_scan_line[angle_raw].range = range_meters;
        pPlugIn->m_scan_line[angle_raw].age = nowMillis;
    }
    //  all scanlines ready now, refresh section follows
    int pos_age = difftime (time(0), br_bpos_watchdog);   // the age of the postion, last call of SetPositionFixEx
    if (RenderOverlay_busy || pos_age >= 2) { // don't do additional refresh and reset the refresh conter
        i_display = 0;            // RenderOverlay_busy: rendering ongoing, reset the counter, don't refresh now
        // this will also balance performance, if too busy skip refresh
        // pos_age>=2 : OCPN too busy to pass position to pi, system overloaded
        // so skip next refresh
        if (pPlugIn->settings.verbose >= 2) {
            if (pos_age >= 2) wxLogMessage(wxT("BR24radar_pi:  busy encountered, br_bpos_watchdog = %i"), pos_age);
            if (RenderOverlay_busy) wxLogMessage(wxT("BR24radar_pi:  busy encountered"));
        }
    }
    else {
        if (br_radar_state == RADAR_ON) {
            if (i_display >=  refreshRate ) {   //    display every "refreshrate time"
                if (refreshRate != 10) { // for 10 no refresh at all
                    GetOCPNCanvasWindow()->Refresh(true);
                    if (pPlugIn->settings.verbose >= 4) {
                        wxLogMessage(wxT("BR24radar_pi:  refresh issued"));
                    }
                }
                i_display = 0;
                RenderOverlay_busy = true;   // no further calls until RenderOverlay_busy has been cleared by RenderGLOverlay
            }
            i_display ++;
        }
    }
}

/*
 * Called once a second. Emulate a radar return that is
 * at the current desired auto_range.
 * Speed is 24 images per minute, e.g. 1/2.5 of a full
 * image.
 */
void RadarDataReceiveThread::emulate_fake_buffer(void)
{
    wxLongLong nowMillis = wxGetLocalTimeMillis();
    time_t now = time(0);

    static int next_scan_number = 0;
    pPlugIn->m_statistics.packets++;
    br_data_seen = true;
    br_radar_seen = true;
    br_radar_watchdog = now;
    br_data_watchdog = br_radar_watchdog;

    int scanlines_in_packet = 4096 * 24 / 60;
    int range_meters = auto_range_meters;
    int spots = 0;
    br_radar_type = RT_BR24;
    if (range_meters != br_range_meters) {
        br_range_meters = range_meters;
        // Set the control's value to the real range that we received, not a table idea
        if (pPlugIn->m_pControlDialog) {
            pPlugIn->m_pControlDialog->SetRangeIndex(convertMetersToRadarAllowedValue(&range_meters, pPlugIn->settings.range_units, br_radar_type));
        }
    }

    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        int angle_raw = next_scan_number;
        next_scan_number = (next_scan_number + 2) % LINES_PER_ROTATION;
        pPlugIn->m_statistics.spokes++;

        // Invent a pattern. Outermost ring, then a square pattern
        UINT8 *dest_data1 = pPlugIn->m_scan_line[angle_raw].data;
        for (int range = 0; range < RETURNS_PER_LINE; range++) {
            int bit = range >> 5;

            // use bit 'bit' of angle_raw
            UINT8 color = ((angle_raw >> 3) & (2 << bit)) > 0 ? 200 : 0;
            dest_data1[range] = color;
            if (color > 0) {
                spots++;
            }
        }

        // The following line is a quick hack to confirm on-screen where the range ends, by putting a 'ring' of
        // returned radar energy at the max range line.
        // TODO: create nice actual range circles.
        dest_data1[RETURNS_PER_LINE - 1] = 0xff;

        pPlugIn->m_scan_line[angle_raw].range = range_meters;
        pPlugIn->m_scan_line[angle_raw].age = nowMillis;
    }
    if (pPlugIn->settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: %") wxTPRId64 wxT(" emulating %d spokes at range %d with %d spots"), nowMillis, scanlines_in_packet, range_meters, spots);
    }
}

RadarCommandReceiveThread::~RadarCommandReceiveThread()
{
}

void RadarCommandReceiveThread::OnExit()
{
}

void *RadarCommandReceiveThread::Entry(void)
{
    SOCKET rx_socket = INVALID_SOCKET;
    int r = 0;

    union {
        sockaddr_storage addr;
        sockaddr_in      ipv4;
    } rx_addr;
    socklen_t        rx_len;

    //    Loop until we quit
    while (!*m_quit) {
        if (rx_socket == INVALID_SOCKET && pPlugIn->settings.display_mode != DM_EMULATOR) {
            rx_socket = startUDPMulticastReceiveSocket(pPlugIn, br_mcast_addr, 6680, "236.6.7.10");
            // If it is still INVALID_SOCKET now we just sleep for 1s in socketReady
            if (rx_socket != INVALID_SOCKET) {
                wxLogMessage(wxT("Listening for commands"));
            }
        }

        if (socketReady(rx_socket, 1000)) {
            UINT8 command[1500];
            rx_len = sizeof(rx_addr);
            r = recvfrom(rx_socket, (char * ) command, sizeof(command), 0, (struct sockaddr *) &rx_addr, &rx_len);
            if (r > 0) {
                wxString s;

                if (rx_addr.addr.ss_family == AF_INET) {
                    UINT8 * a = (UINT8 *) &rx_addr.ipv4.sin_addr; // sin_addr is in network layout

                    s.Printf(wxT("%u.%u.%u.%u sent command"), a[0] , a[1] , a[2] , a[3]);
                } else {
                    s = wxT("non-IPV4 sent command");
                }
                logBinaryData(s, command, r);
            }
            if (r < 0 || !br_radar_seen) {
                closesocket(rx_socket);
                rx_socket = INVALID_SOCKET;
            }
        } else if (!br_radar_seen || !br_mcast_addr) {
            if (rx_socket != INVALID_SOCKET) {
                closesocket(rx_socket);
                rx_socket = INVALID_SOCKET;
            }
        }
    }

    if (rx_socket != INVALID_SOCKET) {
        closesocket(rx_socket);
    }
    return 0;
}


RadarReportReceiveThread::~RadarReportReceiveThread()
{
}

void RadarReportReceiveThread::OnExit()
{
}

#ifndef __WXMSW__

// Mac and Linux have ifaddrs.
# include <ifaddrs.h>
# include <net/if.h>

#else

// Emulate (just enough of) ifaddrs on Windows
// Thanks to https://code.google.com/p/openpgm/source/browse/trunk/openpgm/pgm/getifaddrs.c?r=496&spec=svn496
// Although that file has interesting new APIs the old ioctl works fine with XP and W7, and does enough
// for what we want to do.

struct ifaddrs
{
    struct ifaddrs  * ifa_next;
    struct sockaddr * ifa_addr;
    ULONG             ifa_flags;
};

struct ifaddrs_storage
{
    struct ifaddrs           ifa;
    struct sockaddr_storage  addr;
};

static int getifaddrs( struct ifaddrs ** ifap )
{
    char buf[2048];
    DWORD bytesReturned;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        wxLogError(wxT("BR24radar_pi: Cannot get socket"));
        return -1;
    }

    if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, buf, sizeof(buf), &bytesReturned, 0, 0) < 0) {
        wxLogError(wxT("BR24radar_pi: Cannot get interface list"));
        closesocket(sock);
        return -1;
    }

    /* guess return structure from size */
    unsigned iilen;
    INTERFACE_INFO *ii;
    INTERFACE_INFO_EX *iix;

    if (0 == bytesReturned % sizeof(INTERFACE_INFO)) {
        iilen = bytesReturned / sizeof(INTERFACE_INFO);
        ii    = (INTERFACE_INFO*) buf;
        iix   = NULL;
    } else {
        iilen  = bytesReturned / sizeof(INTERFACE_INFO_EX);
        ii     = NULL;
        iix    = (INTERFACE_INFO_EX*)buf;
    }

    /* alloc a contiguous block for entire list */
    unsigned n = iilen, k =0;
    struct ifaddrs_storage * ifa = (struct ifaddrs_storage *) malloc(n * sizeof(struct ifaddrs_storage));
    memset(ifa, 0, n * sizeof(struct ifaddrs_storage));

    /* foreach interface */
    struct ifaddrs_storage * ift = ifa;

    for (unsigned i = 0; i < iilen; i++) {
        ift->ifa.ifa_addr = (sockaddr *) &ift->addr;
        if (ii) {
            memcpy (ift->ifa.ifa_addr, &ii[i].iiAddress.AddressIn, sizeof(struct sockaddr_in));
            ift->ifa.ifa_flags = ii[i].iiFlags;
        } else {
            memcpy (ift->ifa.ifa_addr, iix[i].iiAddress.lpSockaddr, iix[i].iiAddress.iSockaddrLength);
            ift->ifa.ifa_flags = iix[i].iiFlags;
        }

        k++;
        if (k < n) {
            ift->ifa.ifa_next = (struct ifaddrs *)(ift + 1);
            ift = (struct ifaddrs_storage *)(ift->ifa.ifa_next);
        }
    }

    *ifap = (struct ifaddrs*) ifa;
    closesocket(sock);
    return 0;
}

void freeifaddrs( struct ifaddrs *ifa)
{
    free(ifa);
}

#endif

#define VALID_IPV4_ADDRESS(i) \
(  i \
&& i->ifa_addr \
&& i->ifa_addr->sa_family == AF_INET \
&& (i->ifa_flags & IFF_UP) > 0 \
&& (i->ifa_flags & IFF_LOOPBACK) == 0 \
&& (i->ifa_flags & IFF_MULTICAST) > 0 )

void *RadarReportReceiveThread::Entry(void)
{
    SOCKET rx_socket = INVALID_SOCKET;
    int r = 0;
    int count = 0;

    // This thread is special as it is the only one that loops round over the interfaces
    // to find the radar

    struct ifaddrs * ifAddrStruct;
    struct ifaddrs * ifa;
    static struct sockaddr_in mcastFoundAddr;
    static struct sockaddr_in radarFoundAddr;

    sockaddr_storage rx_addr;
    socklen_t        rx_len;

    ifAddrStruct = 0;
    ifa = 0;

    if (pPlugIn->settings.verbose) {
        wxLogMessage(wxT("BR24radar_pi: Listening for reports"));
    }
    //    Loop until we quit

    while (!*m_quit) {
        if (rx_socket == INVALID_SOCKET && pPlugIn->settings.display_mode != DM_EMULATOR) {
            // Pick the next ethernet card

            // If set, we used this one last time. Go to the next card.
            if (ifa) {
                ifa = ifa->ifa_next;
            }
            // Loop until card with a valid IPv4 address
            while (ifa && !VALID_IPV4_ADDRESS(ifa)) {
                ifa = ifa->ifa_next;
            }
            if (!ifa) {
                if (ifAddrStruct) {
                    freeifaddrs(ifAddrStruct);
                    ifAddrStruct = 0;
                }
                if (!getifaddrs(&ifAddrStruct)) {
                    ifa = ifAddrStruct;
                }
                // Loop until card with a valid IPv4 address
                while (ifa && !VALID_IPV4_ADDRESS(ifa)) {
                    ifa = ifa->ifa_next;
                }
            }
            if (VALID_IPV4_ADDRESS(ifa)) {
                rx_socket = startUDPMulticastReceiveSocket(pPlugIn, (struct sockaddr_in *)ifa->ifa_addr, 6679, "236.6.7.9");
                if (rx_socket != INVALID_SOCKET) {
                    wxString addr;
                    UINT8 * a = (UINT8 *) &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr; // sin_addr is in network layout
                    addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);
                    if (pPlugIn->settings.verbose >= 1) {
                        wxLogMessage(wxT("BR24radar_pi: Listening for radar reports on %s"), addr.c_str());
                    }
                    if (pPlugIn->m_pControlDialog) {
                        pPlugIn->m_pControlDialog->SetMcastIPAddress(addr);
                    }
                    count = 0;
                }
            }
            // If it is still INVALID_SOCKET now we just sleep for 1s in socketReady
        }

        if (socketReady(rx_socket, 1000)) {
            UINT8 report[1500];
            rx_len = sizeof(rx_addr);
            r = recvfrom(rx_socket, (char * ) report, sizeof(report), 0, (struct sockaddr *) &rx_addr, &rx_len);
            if (r > 0) {
                if (ProcessIncomingReport(report, r)) {
                    memcpy(&mcastFoundAddr, ifa->ifa_addr, sizeof(mcastFoundAddr));
                    br_mcast_addr = &mcastFoundAddr;
                    memcpy(&radarFoundAddr, &rx_addr, sizeof(radarFoundAddr));
                    br_radar_addr = &radarFoundAddr;

                    wxString addr;
                    UINT8 * a = (UINT8 *) &br_radar_addr->sin_addr; // sin_addr is in network layout
                    addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);

                    if (pPlugIn->m_pControlDialog) {
                        pPlugIn->m_pControlDialog->SetRadarIPAddress(addr);
                    }
                    if (!br_radar_seen) {
                        wxLogMessage(wxT("BR24radar_pi: detected radar at %s"), addr.c_str());
                    }
                    br_radar_seen = true;
                    br_radar_watchdog = time(0);
                }
            }
            if (r < 0 || !br_radar_seen) { // on error, or if we haven't received anything we start looping again
                closesocket(rx_socket);
                rx_socket = INVALID_SOCKET;
                br_mcast_addr = 0;
                br_radar_addr = 0;
            }
        } else if (count >= 2 && !br_radar_seen && rx_socket != INVALID_SOCKET) {
            closesocket(rx_socket);
            rx_socket = INVALID_SOCKET;
            br_mcast_addr = 0;
            br_radar_addr = 0;
        } else {
            count++;
        }
    }

    if (rx_socket != INVALID_SOCKET) {
        closesocket(rx_socket);
    }
    if (ifAddrStruct) {
        freeifaddrs(ifAddrStruct);
    }
    return 0;
}

//
// The following is the received radar state. It sends this regularly
// but especially after something sends it a state change.
//
#pragma pack(push,1)
struct radar_state {
    UINT8  what;    // 0x02
    UINT8  command; // 0xC4
    UINT16 field1;  // 0x06 0x09
    UINT32 field2;  // 0
    UINT32 field3;  // 1
    UINT8  field4a;
    UINT32 field4b;
    UINT32 sea;     // sea state
    UINT32 field6a;
    UINT32 field6b;
    UINT32 field6c;
    UINT8  field6d;
    UINT32 rejection;
    UINT32 field7;
    UINT32 target_boost;
    UINT32 field8;
    UINT32 field9;
    UINT32 field10;
    UINT32 field11;
    UINT32 field12;
    UINT32 field13;
    UINT32 field14;
};
#pragma pack(pop)

bool RadarReportReceiveThread::ProcessIncomingReport( UINT8 * command, int len )
{
    static char prevStatus = 0;

    if (command[1] == 0xC4) {
        // Looks like a radar report. Is it a known one?
        switch ((len << 8) + command[0]) {
            case (18 << 8) + 0x01:
                // Radar status in byte 2
                if (command[2] != prevStatus) {
                    if (pPlugIn->settings.verbose > 0) {
                        wxLogMessage(wxT("BR24radar_pi: radar status = %u"), command[2]);
                    }
                    prevStatus = command[2];
                }
                break;

            case (99 << 8) + 0x02:
                if (pPlugIn->settings.verbose > 0) {
                    radar_state * s = (radar_state *) command;

                    wxLogMessage(wxT("BR24radar_pi: radar state f1=%u f2=%u f3=%u f4a=%u f4b=%u sea=%u f6a=%u f6b=%u f6c=%u f6d=%u rejection=%u f7=%u target_boost=%u f8=%u f9=%u f10=%u f11=%u f12=%u f13=%u f14=%u")
                                 , s->field1
                                 , s->field2
                                 , s->field3
                                 , s->field4a
                                 , s->field4b
                                 , s->sea
                                 , s->field6a
                                 , s->field6b
                                 , s->field6c
                                 , s->field6d
                                 , s->rejection
                                 , s->field7
                                 , s->target_boost
                                 , s->field8
                                 , s->field9
                                 , s->field10
                                 , s->field11
                                 , s->field12
                                 , s->field13
                                 , s->field14
                                 );
                    logBinaryData(wxT("state"), command, len);
                }
                break;

            case (564 << 8) + 0x05:
                // Content unknown, but we know that BR24 radomes send this
                if (pPlugIn->settings.verbose >= 4) {
                    logBinaryData(wxT("received familiar report"), command, len);
                }
                break;

            default:
                if (pPlugIn->settings.verbose >= 2) {
                    logBinaryData(wxT("received unknown report"), command, len);
                }
                break;

        }
        return true;
    }
    if (command[1] == 0xF5) {
        // Looks like a radar report. Is it a known one?
        switch ((len << 8) + command[0]) {
            case ( 16 << 8) + 0x0f:
            case (  8 << 8) + 0x10:
            case ( 10 << 8) + 0x12:
            case ( 46 << 8) + 0x13:
                // Content unknown, but we know that BR24 radomes send this
                if (pPlugIn->settings.verbose >= 4) {
                    logBinaryData(wxT("received familiar report"), command, len);
                }
                break;

            default:
                if (pPlugIn->settings.verbose >= 2) {
                    logBinaryData(wxT("received unknown report"), command, len);
                }
                break;

        }
        return true;
    }
    if (pPlugIn->settings.verbose >= 2) {
        logBinaryData(wxT("received unknown message"), command, len);
    }
    return false;
}


// vim: sw=4:ts=8:
