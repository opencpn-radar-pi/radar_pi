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

#ifndef __WXMSW__
typedef char byte;
#endif

#include "br24radar_pi.h"
#include "ocpndc.h"

#include "OCPN_Sound.h" //Devil

// A marker that uniquely identifies BR24 generation scanners, as opposed to 4G(eneration)
// Note that 3G scanners are BR24's with better power, so they are more BR23+ than 4G-.
// As far as we know they 3G's use exactly the same command set.

// If BR24MARK is found, we switch to BR24 mode, otherwise 4G.
static unsigned char BR24MARK[] = { 0x00, 0x44, 0x0d, 0x0e };

enum {
    // process ID's
    ID_OK,
    ID_RANGE_UNITS,
    ID_OVERLAYDISPLAYOPTION,
    ID_DISPLAYTYPE,
    ID_INTERVALSLIDER,
    ID_HEADINGSLIDER,
};

union packet_buf {
    radar_frame_pkt packet;
    unsigned char   buf[20000];     // 32 lines of 512 = 16,384, DO NOT ALTER from 20000!
};

bool br_bpos_set = true;
bool bpos_warn_msg = false;
double br_ownship_lat, br_ownship_lon;
double cur_lat, cur_lon;
double br_hdm;
double br_hdt;

double mark_rng, mark_brg;      // This is needed for context operation
long  br_range_meters = 0;      // current range for radar
int auto_range_meters = 0;      // What the range should be, at least, when AUTO mode is selected
int previous_auto_range_meters = 0;

int   br_radar_state;
int   br_scanner_state;
RadarType br_radar_type = RT_BR24;

long  br_display_interval(0);

wxDateTime  watchdog;
wxDateTime  br_dt_stayalive;
int   br_scan_packets_per_tick;

bool  br_bshown_dc_message;
wxTextCtrl        *plogtc;

int   radar_control_id, alarm_zone_id;
bool  alarm_context_mode;


wxSound     RadarAlarm;     //This is the Devil
wxString    RadarAlertAudioFile;
bool        alarm_bogey_confirmed = false;
wxDateTime  alarm_sound_last;

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
/*
static double mod(double numb1, double numb2)
{
    double result = numb1 - (numb2 * int(numb1/numb2));
    return result;
}

static double modcrs(double numb1, double numb2)
{
    if (numb1 > twoPI)
        numb1 = mod(numb1, twoPI);
    double result = twoPI - mod(twoPI-numb1, numb2);
    return result;
}
*/
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
    double angle = atan2 ( deg2rad(lat2-lat1), (deg2rad(lon2-lon1) * cos(deg2rad(lat1))));

    angle = rad2deg(angle) ;
    angle = 90.0 - angle;
    if (angle < 0) angle = 360 + angle;
    return (angle);
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
    : opencpn_plugin_18(ppimgr)
{
    // Create the PlugIn icons
    initialize_images();
    m_pdeficon = new wxBitmap(*_img_radar_blank);

}

int br24radar_pi::Init(void)
{
    AddLocaleCatalog( _T("opencpn-br24radar_pi") );

    m_pControlDialog = NULL;

    br_radar_state = RADAR_OFF;
    br_scanner_state = RADAR_OFF;                 // Radar scanner is off

    watchdog = wxDateTime::Now();
    br_dt_stayalive = wxDateTime::Now();
    alarm_sound_last = wxDateTime::Now();

    m_ptemp_icon = NULL;
    m_sent_bm_id_normal = -1;
    m_sent_bm_id_rollover =  -1;

    m_hdt_source = 0;
    m_hdt_prev_source = 0;

    for (int i = 0; i < LINES_PER_ROTATION; i++) {
        m_scan_range[i][0] = 0;
        m_scan_range[i][1] = 0;
        m_scan_range[i][2] = 0;
    }

    memset(&guardZones, 0, sizeof(guardZones));

    settings.alarm_zone = 0;   // this used to be active alarm zone, now it means which guard zone window is active
    settings.display_mode = 0;
    settings.master_mode = false;                 // we're not the master controller at startup
    settings.auto_range_mode = true;                    // starts with auto range change
    settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;

//      Set default parameters for controls displays
    m_BR24Controls_dialog_x = 0;
    m_BR24Controls_dialog_y = 0;
    m_BR24Controls_dialog_sx = 200;
    m_BR24Controls_dialog_sy = 200;


    ::wxDisplaySize(&m_display_width, &m_display_height);

//****************************************************************************************
    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
    LoadConfig();

    // Get a pointer to the opencpn display canvas, to use as a parent for the UI dialog
    m_parent_window = GetOCPNCanvasWindow();

    //    This PlugIn needs a toolbar icon
    {
        m_tool_id  = InsertPlugInTool(wxT(""), _img_radar_red, _img_radar_red, wxITEM_NORMAL,
                                      wxT("BR24Radar"), wxT(""), NULL,
                                      BR24RADAR_TOOL_POSITION, 0, this);

        CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);
    }

    //    Create the control socket for the Radar data receiver

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(6680);
    int one = 1;
    int r = 0;
    m_radar_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_radar_socket == INVALID_SOCKET) {
        r = -1;
    }
    else {
        r = setsockopt(m_radar_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &one, sizeof(one));
    }

    if (!r)
    {
        r = bind(m_radar_socket, (struct sockaddr *) &adr, sizeof(adr));
    }

    if (r)
    {
        wxLogError(wxT("BR24radar_pi: Unable to create UDP sending socket"));
        // Might as well give up now
        return 0;
    }

    // Context Menu Items (Right click on chart screen)

    m_pmenu = new wxMenu();            // this is a dummy menu
    // required by Windows as parent to item created
    wxMenuItem *pmi = new wxMenuItem(m_pmenu, -1, _("Radar Control..."));
    int miid = AddCanvasContextMenuItem(pmi, this);
    SetCanvasContextMenuItemViz(miid, true);

    wxMenuItem *pmi2 = new wxMenuItem(m_pmenu, -1, _("Set Alarm Point"));
    alarm_zone_id = AddCanvasContextMenuItem(pmi2, this );
    SetCanvasContextMenuItemViz(alarm_zone_id, false);
    alarm_context_mode = false;

    //    Create the THREAD for Multicast radar data reception
    m_quit = false;
    m_receiveThread = new MulticastRXThread(this, &m_quit);
    m_receiveThread->Run();

    return (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_OVERLAY_CALLBACK     |
            WANTS_CURSOR_LATLON        |
            WANTS_TOOLBAR_CALLBACK     |
            INSTALLS_TOOLBAR_TOOL      |
            INSTALLS_CONTEXTMENU_ITEMS |
            WANTS_CONFIG               |
            WANTS_NMEA_EVENTS          |
            WANTS_PREFERENCES
           );
}

bool br24radar_pi::DeInit(void)
{
    SaveConfig();

    if (m_receiveThread) {
        m_quit = true;
        m_receiveThread->Wait();
        delete m_receiveThread;
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
    // If the config somehow says NOT to show the icon, override it so the user gets good feedback
    if (!m_bShowIcon) {
        m_bShowIcon = true;

        m_tool_id  = InsertPlugInTool(wxT(""), _img_radar_red, _img_radar_red, wxITEM_CHECK,
                                      wxT("BR24Radar"), wxT(""), NULL,
                                      BR24RADAR_TOOL_POSITION, 0, this);
    }

}

void br24radar_pi::ShowPreferencesDialog(wxWindow* parent)
{
    m_pOptionsDialog = new BR24DisplayOptionsDialog;
    m_pOptionsDialog->Create(m_parent_window, this);
    m_pOptionsDialog->Show();
}

void logBinaryData(const wxString& what, const char * data, int size)
{
    wxString explain;
    int i;

    explain.Alloc(size * 3 + 50);
    explain += wxT("BR24radar_pi: ");
    explain += what;
    explain += wxString::Format(wxT(" %d bytes: "), size);
    for (i = 0; i < size; i++)
    {
      explain += wxString::Format(wxT(" %02X"), (unsigned char) data[i]);
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
    pParent = parent;
    pPlugIn = ppi;

    if (!wxDialog::Create(parent, wxID_ANY, _("BR24 Target Display Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)) {
        return false;
    }

    int border_size = 4;
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    wxBoxSizer* DisplayOptionsBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(DisplayOptionsBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND, 2);

    //  BR24 toolbox icon checkbox
    wxStaticBox* DisplayOptionsCheckBox = new wxStaticBox(this, wxID_ANY, _T(""));
    wxStaticBoxSizer* DisplayOptionsCheckBoxSizer = new wxStaticBoxSizer(DisplayOptionsCheckBox, wxVERTICAL);
    DisplayOptionsBox->Add(DisplayOptionsCheckBoxSizer, 0, wxEXPAND | wxALL, border_size);

     //  Range Units options
    wxStaticBox* BoxRangeUnits = new wxStaticBox(this, wxID_ANY, _("Range Units"));
    wxStaticBoxSizer* BoxSizerOperation = new wxStaticBoxSizer(BoxRangeUnits, wxVERTICAL);
    DisplayOptionsCheckBoxSizer->Add(BoxSizerOperation, 0, wxEXPAND | wxALL, border_size);

    wxString RangeModeStrings[] = {
        _("Nautical Miles"),
        _("Kilometers"),
    };

    pRangeUnits = new wxRadioBox(this, ID_RANGE_UNITS, _("Range Units"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, RangeModeStrings, 1, wxRA_SPECIFY_COLS);

    BoxSizerOperation->Add(pRangeUnits, 0, wxALL | wxEXPAND, 2);

    pRangeUnits->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                            wxCommandEventHandler(BR24DisplayOptionsDialog::OnRangeUnitsClick), NULL, this);

    pRangeUnits->SetSelection(pPlugIn->settings.range_units);

    /// Option settings
    wxString Overlay_Display_Options[] = {
        _("Monocolor-Red"),
        _("Multi-color"),
        _("GL3+"),
    };

    pOverlayDisplayOptions = new wxRadioBox(this, ID_OVERLAYDISPLAYOPTION, _("Overlay Display Options"),
                                            wxDefaultPosition, wxDefaultSize,
                                            3, Overlay_Display_Options, 1, wxRA_SPECIFY_COLS);

    DisplayOptionsCheckBoxSizer->Add(pOverlayDisplayOptions, 0, wxALL | wxEXPAND, 2);

    pOverlayDisplayOptions->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                                    wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayOptionClick), NULL, this);

    pOverlayDisplayOptions->SetSelection(pPlugIn->settings.display_option);

//  Display Options
    wxStaticBox* itemStaticBoxSizerDisOptStatic = new wxStaticBox(this, wxID_ANY, _("Display Options"));
    wxStaticBoxSizer* itemStaticBoxSizerDisOpt = new wxStaticBoxSizer(itemStaticBoxSizerDisOptStatic, wxVERTICAL);
    DisplayOptionsCheckBoxSizer->Add(itemStaticBoxSizerDisOpt, 0, wxEXPAND | wxALL, border_size);

    wxString DisplayUpdateStrings[] = {
        _("Radar Chart Overlay"),
        _("Radar Standalone"),
        _("Spectrum"),
    };

    pDisplayMode = new wxRadioBox(this, ID_DISPLAYTYPE, _("Radar Display"),
                                  wxDefaultPosition, wxDefaultSize,
                                  3, DisplayUpdateStrings, 1, wxRA_SPECIFY_COLS);

    itemStaticBoxSizerDisOpt->Add(pDisplayMode, 0, wxALL | wxEXPAND, 2);

    pDisplayMode->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED,
                          wxCommandEventHandler(BR24DisplayOptionsDialog::OnDisplayModeClick), NULL, this);

    pDisplayMode->SetSelection(pPlugIn->settings.display_mode);

//  Calibration
    wxStaticBox* itemStaticBoxCalibration = new wxStaticBox(this, wxID_ANY, _("Calibration"));
    wxStaticBoxSizer* itemStaticBoxSizerCalibration = new wxStaticBoxSizer(itemStaticBoxCalibration, wxVERTICAL);
    DisplayOptionsCheckBoxSizer->Add(itemStaticBoxSizerCalibration, 0, wxEXPAND | wxALL, border_size);

    // Range Factor
    wxStaticText *pStatic_Range_Calibration = new wxStaticText(this, wxID_ANY, _("Range factor"));
    itemStaticBoxSizerCalibration->Add(pStatic_Range_Calibration, 1, wxALIGN_LEFT | wxALL, 2);

    pText_Range_Calibration_Value = new wxTextCtrl(this, wxID_ANY);
    itemStaticBoxSizerCalibration->Add(pText_Range_Calibration_Value, 1, wxALIGN_LEFT | wxALL, 5);
    wxString m_temp;
    m_temp.Printf(wxT("%2.5f"), pPlugIn->settings.range_calibration);
    pText_Range_Calibration_Value->SetValue(m_temp);
    pText_Range_Calibration_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(BR24DisplayOptionsDialog::OnRange_Calibration_Value), NULL, this);

    // Heading correction
    wxStaticText *pStatic_Heading_Correction = new wxStaticText(this, wxID_ANY, _("Heading factor (+ or -, 0 ->180)"));
    itemStaticBoxSizerCalibration->Add(pStatic_Heading_Correction, 1, wxALIGN_LEFT | wxALL, 2);

    pText_Heading_Correction_Value = new wxTextCtrl(this, wxID_ANY);
    itemStaticBoxSizerCalibration->Add(pText_Heading_Correction_Value, 1, wxALIGN_LEFT | wxALL, 5);
    m_temp.Printf(wxT("%2.5f"), pPlugIn->settings.heading_correction);
    pText_Heading_Correction_Value->SetValue(m_temp);
    pText_Heading_Correction_Value->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                                           wxCommandEventHandler(BR24DisplayOptionsDialog::OnHeading_Calibration_Value), NULL, this);



 /*   pHeadingSlider = new wxSlider(this, ID_HEADINGSLIDER, 0 , -180, +180, wxDefaultPosition,  wxDefaultSize,
                               wxSL_HORIZONTAL | wxSL_LABELS,  wxDefaultValidator, _("slider"));

    headingBoxSizer->Add(pHeadingSlider, 0, wxALL | wxEXPAND, 2);

    pHeadingSlider->Connect(wxEVT_SCROLL_CHANGED, wxCommandEventHandler(BR24DisplayOptionsDialog::OnHeadingSlider), NULL, this);
    pHeadingSlider->SetValue(pPlugIn->settings.heading_correction);
*/
// Accept/Reject button
    wxStdDialogButtonSizer* DialogButtonSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    DisplayOptionsBox->Add(DialogButtonSizer, 0, wxALIGN_RIGHT | wxALL, 5);


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
    pPlugIn->SetDisplayMode(pDisplayMode->GetSelection());
}

void BR24DisplayOptionsDialog::OnRange_Calibration_Value(wxCommandEvent &event)
{
    wxString temp = pText_Range_Calibration_Value->GetValue();
    temp.ToDouble(&pPlugIn->settings.range_calibration);
}

void BR24DisplayOptionsDialog::OnHeading_Calibration_Value(wxCommandEvent &event)
{
    wxString temp = pText_Heading_Correction_Value->GetValue();
    temp.ToDouble(&pPlugIn->settings.heading_correction);
}

void BR24DisplayOptionsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->SaveConfig();
}

void BR24DisplayOptionsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->SaveConfig();
}

//********************************************************************************
// Operation Dialogs - Control, Manual, and Options

void br24radar_pi::OnContextMenuItemCallback(int id)
{
   if (!alarm_context_mode) {
        if (!m_pControlDialog) {
            m_pControlDialog = new BR24ControlsDialog;
            m_pControlDialog->Create(m_parent_window, this);
            
            int range = auto_range_meters;
            m_pControlDialog->SetAutoRangeIndex(convertMetersToRadarAllowedValue(&range, settings.range_units, br_radar_type));
            if (br_range_meters) {
                range = br_range_meters;
                m_pControlDialog->SetRangeIndex(convertMetersToRadarAllowedValue(&range, settings.range_units, br_radar_type));
            }
        }
        m_pControlDialog->Show();
        m_pControlDialog->SetSize(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y,
                                  m_BR24Controls_dialog_sx, m_BR24Controls_dialog_sy);
    }

   if (alarm_context_mode) {
       SetCanvasContextMenuItemViz(radar_control_id, false);
       mark_rng = local_distance(br_ownship_lat, br_ownship_lon, cur_lat, cur_lon);
       mark_brg = local_bearing(br_ownship_lat, br_ownship_lon, cur_lat, cur_lon);
       m_pAlarmZoneDialog->OnContextMenuAlarmCallback(mark_rng, mark_brg);
   }
}

void br24radar_pi::OnBR24ControlDialogClose()
{
    if (m_pControlDialog) {
        m_pControlDialog->Hide();
        SetCanvasContextMenuItemViz(alarm_zone_id, false);
    }

    SaveConfig();
}

void br24radar_pi::OnAlarmZoneDialogClose()
{
    if (m_pAlarmZoneDialog) {
        m_pAlarmZoneDialog->Hide();
        SetCanvasContextMenuItemViz(alarm_zone_id, false);
        alarm_context_mode = false;
        alarm_bogey_confirmed = false;
        SaveConfig();
    }
    if (m_pControlDialog) {
        m_pControlDialog->UpdateGuardZoneState();
        m_pControlDialog->Show();
        SetCanvasContextMenuItemViz(radar_control_id, true);
    }

}

void br24radar_pi::OnAlarmZoneBogeyConfirm()
{
    alarm_bogey_confirmed = true; // This will stop the sound being repeated
}

void br24radar_pi::OnAlarmZoneBogeyClose()
{
    alarm_bogey_confirmed = true; // This will stop the sound being repeated
    if (m_pAlarmZoneBogey) {
        m_pAlarmZoneBogey->Hide();
    }
}

void br24radar_pi::Select_Alarm_Zones(int zone)
{
    settings.alarm_zone = zone;

    if (!m_pAlarmZoneDialog) {
        m_pAlarmZoneDialog = new AlarmZoneDialog;
        wxPoint pos = wxPoint(m_BR24Controls_dialog_x, m_BR24Controls_dialog_y); // show at same loc as controls
        m_pAlarmZoneDialog->Create(m_parent_window, this, wxID_ANY, _(" Alarm Zone Control"), pos);
    }
    if (zone >= 0) {
        m_pAlarmZoneDialog->Show();
        m_pControlDialog->Hide();
        m_pAlarmZoneDialog->OnAlarmZoneDialogShow(zone);
        SetCanvasContextMenuItemViz(alarm_zone_id, true);
        SetCanvasContextMenuItemViz(radar_control_id, false);
        alarm_context_mode = true;
    }
    else {
        m_pAlarmZoneDialog->Hide();
        SetCanvasContextMenuItemViz(alarm_zone_id, false);
        SetCanvasContextMenuItemViz(radar_control_id, true);
        alarm_context_mode = false;
    }
}

void br24radar_pi::SetDisplayMode(int mode)
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
    if (br_radar_state == RADAR_OFF) {  // turned off
        br_radar_state = RADAR_ON;
        if (br_scanner_state == RADAR_OFF) {
            settings.master_mode = true;
//            wxLogMessage(wxT("BR24radar_pi: Master mode on"));
            RadarStayAlive();
            RadarTxOn();
        }
    } else {
        br_radar_state = RADAR_OFF;
        if (settings.master_mode == true) {
            RadarTxOff();
            settings.master_mode = false;
//            wxLogMessage(wxT("BR24radar_pi: Master mode off"));
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
    wxDateTime now = wxDateTime::Now();

    long delta_t = now.Subtract(watchdog).GetSeconds().ToLong();

    if (delta_t > 60) {
        // If the position data is over one minute old reset our heading and sound an alarm.
        // Note that the watchdog is continuously reset every time we receive a heading.
        br_bpos_set = false;
        m_hdt_source = 0;
        watchdog = now;
    }

#ifdef NEVER
    if ((br_bpos_set == bpos_warn_msg)) {       // needs synchronization
        wxString message(_("The Radar Overlay has lost GPS position and heading data"));
        wxMessageDialog dlg(GetOCPNCanvasWindow(),  message, wxT("System Message"), wxOK);
        dlg.ShowModal();
        bpos_warn_msg = true;
    }
#endif
    
    if (br_scan_packets_per_tick > 0) { // Something coming from radar unit?
        br_scanner_state = RADAR_ON ;
        if (settings.master_mode) {
            delta_t = now.Subtract(br_dt_stayalive).GetSeconds().ToLong();
            if (delta_t > 5) {
                br_dt_stayalive = now;
                RadarStayAlive();
            }
        }
    } else {
        br_scanner_state = RADAR_OFF;
    }
    br_scan_packets_per_tick = 0;
}

void br24radar_pi::UpdateState(void)   // -  run by RenderGLOverlay
{
    /*
        wxString msg;
        msg.Printf(wxT("UpdateState:  Master State: %d  Radar state: %d   Scanner state:  %d "), settings.master_mode, br_radar_state, br_scanner_state);
        wxLogMessage(msg);
    */
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
    if (0 == br_bshown_dc_message) {
        br_bshown_dc_message = 1;
        wxString message(_("The Radar Overlay PlugIn requires the Accelerated Graphics (OpenGL) mode to be activated in Options->Display->Chart Display Options"));
        wxMessageDialog dlg(GetOCPNCanvasWindow(),  message, wxT("br24radar message"), wxOK);
        dlg.ShowModal();
    }

    return false;
}

// Called by Plugin Manager on main system process cycle

bool br24radar_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    br_bshown_dc_message = 0;             // show message box if RenderOverlay() is called again
    wxString msg;

    // this is expected to be called at least once per second

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
    
    // Calculate the "optimum" radar range setting in meters so Radar just fills Screen
    
    // We used to take the position of the boat into account, so that when you panned the zoom range would go up
    // This is not what the plotters do, so just to make it work the same way we're doing it the same way.
    // The radar range is set such that it covers the entire screen plus 50% so that a little panning is OK.
    // This is what the plotters do as well.
    
    max_distance = radar_distance(vp->lat_min, vp->lon_min, vp->lat_max, vp->lon_max, 'm');
    
    auto_range_meters =  max_distance / 2.0 * 1.5;
    size_t idx = convertMetersToRadarAllowedValue(&auto_range_meters, settings.range_units, br_radar_type);
    if (auto_range_meters != previous_auto_range_meters) {
        if (settings.verbose) {
            wxLogMessage(wxT("Automatic scale changed from %d to %d meters")
                         , previous_auto_range_meters, auto_range_meters);
        }
        previous_auto_range_meters = auto_range_meters;
        if (m_pControlDialog) {
            m_pControlDialog->SetAutoRangeIndex(idx);
        }
        if (settings.auto_range_mode) {
            // Send command directly to radar
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
        case 0:                                    // direct real time sweep render mode
            if (m_hdt_source > 0) {
                RenderRadarOverlay(boat_center, v_scale_ppm, vp);
            }
            break;
        case 1:                                     // plain scope look
            if (br_radar_state == RADAR_ON) {
                RenderRadarStandalone(boat_center, v_scale_ppm, vp);
            }
            break;
        case 2:
            if (br_radar_state == RADAR_ON) {
                RenderSpectrum(center_screen, v_scale_ppm, vp);
            }
            break;
    }
    return true;
}

void br24radar_pi::RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    glPushMatrix();

    glTranslated(radar_center.x, radar_center.y, 0);

    double heading = fmod(br_hdt + settings.heading_correction + rad2deg(vp->rotation) + 360.0, 360.0);
//    wxLogMessage(wxT("Rotating image for HDT=%f Correction=%d Rotation=%f Result=%f"), br_hdt, settings.heading_correction,
//    rad2deg(vp->rotation), heading);
    glRotatef(heading - 90.0, 0, 0, 1);        //correction for boat heading -90 for base north

    // scaling...
    int meters = br_range_meters;
    if (!meters) meters = auto_range_meters;
    if (!meters) meters = 1000;
    double radar_pixels_per_meter = 512. / meters;
    double scale_factor =  v_scale_ppm / radar_pixels_per_meter;  // screen pix/radar pix
    glScaled(scale_factor, scale_factor, 1.);
    if (br_range_meters && br_scanner_state == RADAR_ON) { // only draw radar if something received
        DrawRadarImage(br_range_meters, radar_center);
    }
    
    // Alarm Zone image
    if (br_radar_state == RADAR_ON) {
        if (guardZones[0].type != GZ_OFF || guardZones[1].type != GZ_OFF) {
            RenderAlarmZone(radar_center, v_scale_ppm, vp);
        }
    }

    glPopMatrix();
    glPopAttrib();

}

 /******************************************************************************************************/

void br24radar_pi::RenderRadarStandalone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    wxString msg;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

 //   glEnable(GL_BLEND);
 //   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

    glTranslated(radar_center.x, radar_center.y, 0);
    glRotatef(-90.0, 0, 0, 1);

    // scaling...
    int meters = br_range_meters;
    if (!meters) meters = auto_range_meters;
    if (!meters) meters = 1000;
    double radar_pixels_per_meter = 512. / meters;
    double scale_factor =  v_scale_ppm / radar_pixels_per_meter;  // screen pix/radar pix
    glScaled(scale_factor, scale_factor, 1.);
    if (br_range_meters && br_scanner_state == RADAR_ON) { // only draw radar if something received
        DrawRadarImage(br_range_meters, radar_center);
    }

    // Alarm Zone image
    if (guardZones[0].type != GZ_OFF || guardZones[1].type != GZ_OFF) {
            RenderAlarmZone(radar_center, v_scale_ppm, vp);
    }
    glPopMatrix();
    glPopAttrib();
}

void br24radar_pi::DrawRadarImage(int max_range, wxPoint radar_center)
{
    int bogey_count[GUARD_ZONES];
    
    memset(&bogey_count, 0, sizeof(bogey_count));

    // DRAWING PICTURE
    GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - settings.overlay_transparency) / MAX_OVERLAY_TRANSPARENCY;
    const double spoke_width = deg2rad(360) / LINES_PER_ROTATION; // How wide is one spoke?

    for (int angle = 0 ; angle < LINES_PER_ROTATION; ++angle) {

        if (!m_scan_range[angle][0] && !m_scan_range[angle][1] && !m_scan_range[angle][2]) {
            continue;   // test for entries in each prior scan, if all three empty - stop the process
        }
        for (int radius = 0; radius < 512; ++radius) {

            GLubyte red = 0, green = 0, blue = 0, strength = m_scan_buf[angle][radius];

            if (strength > 50) { // Only draw when there is color, saves lots of CPU
                switch (settings.display_option) {
                    case 0:
                        red = 255;
                        break;
                    case 1:
                        if (strength > 200) {
                            red = 255;
                        } else if (strength > 100) {
                            green = 255;
                        } else if (strength > 50) {
                            blue = 255;
                        }
                        break;
                    case 2:
                        if (strength > 175) {
                            red = 255;
                        } else if (strength > 100) {
                            green = 255;
                        } else if (strength > 50) {
                            blue = 255;
                        }
                        break;
                }

                glColor4ub(red, green, blue, alpha);    // red, blue, green
                double arc_width = spoke_width;
                double arc_heigth = 1;
                double angleRad = angle * spoke_width;

                draw_blob_gl(angleRad, radius, arc_width, arc_heigth);

/**********************************************************************************************************/
// Alarm Section

                if (br_radar_state == RADAR_ON) {
                    for (size_t z = 0; z < GUARD_ZONES; z++) {
                        if (guardZones[z].type != GZ_OFF) {
                            double angle_deg = (angle * 360.0) / LINES_PER_ROTATION;
                            double inner_range = guardZones[z].inner_range;
                            double outer_range = guardZones[z].outer_range;
                            double bogey_range = (radius / 512.0) * (max_range / 1852.0);
                            double angle_1 = guardZones[z].start_bearing;
                            double angle_2 = guardZones[z].end_bearing;
                            
                            if (angle_1 > angle_2) {
                                angle_2 += 360.0;
                            }
                            if (angle_1 > angle_deg) {
                                angle_deg += 360.0;
                            }
                            if (angle_deg > angle_1 && angle_deg < angle_2) {
                                if (bogey_range > inner_range && bogey_range < outer_range) {
                                    bogey_count[z]++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    HandleBogeyCount(bogey_count);
}

void br24radar_pi::RenderSpectrum(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    int alpha;
    long scan_distribution[255];    // intensity distribution

    memset(&scan_distribution[0], 0, 255);

    for (int angle = 0 ; angle < LINES_PER_ROTATION ; angle++) {
        if (m_scan_range[angle][0] != 0) {
            for (int radius = 1; radius < 510; ++radius) {
                alpha = m_scan_buf[angle][radius];
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

    int x, y;
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

/**
 * Draw an OpenGL blob at a particular angle in radians, of a particular arc_length, also in radians.
 *
 * The minim distance from the center of the plan is radius, and it ends at radius + blob_heigth
 */
void br24radar_pi::draw_blob_gl(double angle, double radius, double arc_width, double blob_heigth)
{
     double ca = cos(angle);
     double sa = sin(angle);
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

//****************************************************************************
void br24radar_pi::RenderAlarmZone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp)
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    double ppNM;

    ppNM = v_scale_ppm * 1852.0 ;          // screen pixels per nautical mile to screen pixels per meter
    int start_bearing, end_bearing;
    int red = 0, green = 200, blue = 0, alpha = 50;

    for (size_t z = 0; z < GUARD_ZONES; z++) {
        
        if (guardZones[z].type != GZ_OFF) {
            if (guardZones[z].type == GZ_CIRCLE) {
                start_bearing = 0;
                end_bearing = 359;
            } else {
                start_bearing = guardZones[z].start_bearing;
                end_bearing = guardZones[z].end_bearing;
            }
            glColor4ub((GLubyte)red, (GLubyte)green, (GLubyte)blue, (GLubyte)alpha);
            DrawFilledArc(guardZones[z].outer_range * ppNM, guardZones[z].inner_range * ppNM, start_bearing, end_bearing);
        }
        
        red = 0; green = 0; blue = 200;
    }

    glPopAttrib();
}

void br24radar_pi::HandleBogeyCount(int *bogey_count)
{
    bool bogeysFound = false;
    
    for (int z = 0; z < GUARD_ZONES; z++) {
        if (bogey_count[z] > settings.alarm_zone_threshold) {
            bogeysFound = true;
            break;
        }
    }

    if (bogeysFound
        && (!m_pAlarmZoneDialog || !m_pAlarmZoneDialog->IsShown()) // Don't raise bogeys as long as control dialog is shown
        ) {
        // We have bogeys and there is no objection to showing the dialog

        if (!RadarAlarm.IsOk()) {
            RadarAlarm.Create(RadarAlertAudioFile);
        }
        if (!m_pAlarmZoneBogey) {
            // If this is the first time we have a bogey create & show the dialog immediately
            m_pAlarmZoneBogey = new AlarmZoneBogey;
            m_pAlarmZoneBogey->Create(m_parent_window, this);
            m_pAlarmZoneBogey->Show();
        }

        wxDateTime now = wxDateTime::Now();
        int delta_t = now.Subtract(alarm_sound_last).GetSeconds().ToLong();
        const int alarm_interval = 10;
        if (!alarm_bogey_confirmed && delta_t >= alarm_interval) {
            // If the last time is 10 seconds ago we ping a sound, unless the user confirmed
            alarm_sound_last = now;

            if (RadarAlarm.IsOk()) {
                RadarAlarm.Play();
            }
            else {
                wxBell();
            }
            m_pAlarmZoneBogey->Show();
            delta_t = alarm_interval;
        }
        m_pAlarmZoneBogey->SetBogeyCount(bogey_count, alarm_bogey_confirmed ? -1 : alarm_interval - delta_t);
    }
 
    if (!bogeysFound) {
        if (RadarAlarm.IsOk()) {
            RadarAlarm.Stop();
        }
        alarm_bogey_confirmed = false; // Reset for next time we see bogeys
        if (m_pAlarmZoneBogey && m_pAlarmZoneBogey->IsShown()) {
            m_pAlarmZoneBogey->Hide();
        }
    }
}

void br24radar_pi::DrawFilledArc(double r1, double r2, double a1, double a2)
{
    if (a1 > a2) {
        a2 += 360.0;
    }

    for (double n = a1; n <= a2; ++n ) {
        draw_blob_gl(deg2rad(n), r2, deg2rad(0.5), r1 - r2);
    }
}

//****************************************************************************

bool br24radar_pi::LoadConfig(void)
{

    wxFileConfig *pConf = m_pconfig;

    if (pConf) {
        pConf->SetPath(wxT("/Settings"));
        pConf->SetPath(wxT("/Plugins/BR24Radar"));
        if (pConf->Read(wxT("DisplayOption"), &settings.display_option, 0)) {
            pConf->Read(wxT("RangeUnits" ), &settings.range_units, 0 ); //0 = "Nautical miles"), 1 = "Kilometers"
            if (settings.range_units > 1) {
                settings.range_units = 1;
            }
            pConf->Read(wxT("DisplayMode"),  &settings.display_mode, 0);
            pConf->Read(wxT("VerboseLog"),  &settings.verbose, 0);
            pConf->Read(wxT("Transparency"),  &settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY);
            pConf->Read(wxT("Gain"),  &settings.gain, 50);
            pConf->Read(wxT("RainGain"),  &settings.rain_clutter_gain, 50);
            pConf->Read(wxT("ClutterGain"),  &settings.sea_clutter_gain, 25);
            pConf->Read(wxT("RangeCalibration"),  &settings.range_calibration, 1.0);
            pConf->Read(wxT("HeadingCorrection"),  &settings.heading_correction, 0);
            pConf->Read(wxT("Interface"), &settings.radar_interface, wxT("0.0.0.0"));
            pConf->Read(wxT("BeamWidth"), &settings.beam_width, 2);
            pConf->Read(wxT("InterferenceRejection"), &settings.rejection, 0);
            pConf->Read(wxT("TargetBoost"), &settings.target_boost, 0);
            pConf->Read(wxT("AlarmZonesThreshold"), &settings.alarm_zone_threshold, 5L);

            pConf->Read(wxT("ControlsDialogSizeX"), &m_BR24Controls_dialog_sx, 300L);
            pConf->Read(wxT("ControlsDialogSizeY"), &m_BR24Controls_dialog_sy, 540L);
            pConf->Read(wxT("ControlsDialogPosX"), &m_BR24Controls_dialog_x, 20L);
            pConf->Read(wxT("ControlsDialogPosY"), &m_BR24Controls_dialog_y, 170L);

            pConf->Read(wxT("Zone1StBrng"), &guardZones[0].start_bearing, 0);
            pConf->Read(wxT("Zone1EndBrng"), &guardZones[0].end_bearing, 0);
            pConf->Read(wxT("Zone1OutRng"), &guardZones[0].outer_range, 0);
            pConf->Read(wxT("Zone1InRng"), &guardZones[0].inner_range, 0);
            pConf->Read(wxT("Zone1ArcCirc"), &guardZones[0].type, 0);

            pConf->Read(wxT("Zone2StBrng"), &guardZones[1].start_bearing, 0);
            pConf->Read(wxT("Zone2EndBrng"), &guardZones[1].end_bearing, 0);
            pConf->Read(wxT("Zone2OutRng"), &guardZones[1].outer_range, 0);
            pConf->Read(wxT("Zone2InRng"), &guardZones[1].inner_range, 0);
            pConf->Read(wxT("Zone2ArcCirc"), &guardZones[1].type, 0);


            pConf->Read(wxT("RadarAlertAudioFile"), &RadarAlertAudioFile );
            SaveConfig();
            return true;
        }
    }

    // Read the old location with old paths
    if (pConf) {
        pConf->SetPath(wxT("/Settings"));
        if (pConf->Read(wxT("BR24RadarDisplayOption"), &settings.display_option, 0))
            pConf->DeleteEntry(wxT("BR24RadarDisplayOption"));
        if (pConf->Read(wxT("BR24RadarDisplayMode"),  &settings.display_mode, 0))
            pConf->DeleteEntry(wxT("BR24RadarDisplayMode"));
        settings.verbose = false;
        if (pConf->Read(wxT("BR24RadarTransparency"),  &settings.overlay_transparency, DEFAULT_OVERLAY_TRANSPARENCY))
            pConf->DeleteEntry(wxT("BR24RadarTransparency"));
        if (pConf->Read(wxT("BR24RadarGain"),  &settings.gain, 50))
            pConf->DeleteEntry(wxT("BR24RadarGain"));
        if (pConf->Read(wxT("BR24RadarRainGain"),  &settings.rain_clutter_gain, 50))
            pConf->DeleteEntry(wxT("BR24RadarRainGain"));
        if (pConf->Read(wxT("BR24RadarClutterGain"),  &settings.sea_clutter_gain, 50))
            pConf->DeleteEntry(wxT("BR24RadarClutterGain"));
        if (pConf->Read(wxT("BR24RadarRangeCalibration"),  &settings.range_calibration, 1.0))
            pConf->DeleteEntry(wxT("BR24RadarRangeCalibration"));
        if (pConf->Read(wxT("BR24RadarHeadingCorrection"),  &settings.heading_correction, 0))
            pConf->DeleteEntry(wxT("BR24RadarHeadingCorrection"));
        if (pConf->Read(wxT("BR24RadarInterface"), &settings.radar_interface, wxT("0.0.0.0")))
            pConf->DeleteEntry(wxT("BR24RadarInterface"));

        if (pConf->Read(wxT("BR24ControlsDialogSizeX"), &m_BR24Controls_dialog_sx, 300L))
            pConf->DeleteEntry(wxT("BR24ControlsDialogSizeX"));
        if (pConf->Read(wxT("BR24ControlsDialogSizeY"), &m_BR24Controls_dialog_sy, 540L))
            pConf->DeleteEntry(wxT("BR24ControlsDialogSizeY"));
        if (pConf->Read(wxT("BR24ControlsDialogPosX"), &m_BR24Controls_dialog_x, 20L))
            pConf->DeleteEntry(wxT("BR24ControlsDialogPosX"));
        if (pConf->Read(wxT("BR24ControlsDialogPosY"), &m_BR24Controls_dialog_y, 170L))
            pConf->DeleteEntry(wxT("BR24ControlsDialogPosY"));

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
        pConf->Write(wxT("DisplayMode"), settings.display_mode);
        pConf->Write(wxT("VerboseLog"), settings.verbose);
        pConf->Write(wxT("Transparency"), settings.overlay_transparency);
        pConf->Write(wxT("Gain"), settings.gain);
        pConf->Write(wxT("RainGain"), settings.rain_clutter_gain);
        pConf->Write(wxT("ClutterGain"), settings.sea_clutter_gain);
        pConf->Write(wxT("RangeCalibration"),  settings.range_calibration);
        pConf->Write(wxT("HeadingCorrection"),  settings.heading_correction);
        pConf->Write(wxT("Interface"),  settings.radar_interface);
        pConf->Write(wxT("BeamWidth"),  settings.beam_width);
        pConf->Write(wxT("InterferenceRejection"), settings.rejection);
        pConf->Write(wxT("TargetBoost"), settings.target_boost);
        pConf->Write(wxT("AlarmZonesThreshold"), settings.alarm_zone_threshold);

        pConf->Write(wxT("ControlsDialogSizeX"),  m_BR24Controls_dialog_sx);
        pConf->Write(wxT("ControlsDialogSizeY"),  m_BR24Controls_dialog_sy);
        pConf->Write(wxT("ControlsDialogPosX"),   m_BR24Controls_dialog_x);
        pConf->Write(wxT("ControlsDialogPosY"),   m_BR24Controls_dialog_y);


        pConf->Write(wxT("Zone1StBrng"), guardZones[0].start_bearing);
        pConf->Write(wxT("Zone1EndBrng"), guardZones[0].end_bearing);
        pConf->Write(wxT("Zone1OutRng"), guardZones[0].outer_range);
        pConf->Write(wxT("Zone1InRng"), guardZones[0].inner_range);
        pConf->Write(wxT("Zone1ArcCirc"), guardZones[0].type);

        pConf->Write(wxT("Zone2StBrng"), guardZones[1].start_bearing);
        pConf->Write(wxT("Zone2EndBrng"), guardZones[1].end_bearing);
        pConf->Write(wxT("Zone2OutRng"), guardZones[1].outer_range);
        pConf->Write(wxT("Zone2InRng"), guardZones[1].inner_range);
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
    br_ownship_lat = pfix.Lat;
    br_ownship_lon = pfix.Lon;

    br_bpos_set = true;
    bpos_warn_msg = false;
}

void br24radar_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix)
{
    br_ownship_lat = pfix.Lat;
    br_ownship_lon = pfix.Lon;

    if (!wxIsNaN(pfix.Hdt))
    {
        br_hdt = pfix.Hdt;
        if (m_hdt_source != 1) {
           wxLogMessage(wxT("BR24radar_pi: Heading source is now HDT"));
        }
        m_hdt_source = 1;
    }
    else if (!wxIsNaN(pfix.Hdm) && !wxIsNaN(pfix.Var))
    {
        br_hdt = pfix.Hdm + pfix.Var;
        if (m_hdt_source != 2) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now HDM"));
        }
        m_hdt_source = 2;
    }
    else if (!wxIsNaN(pfix.Cog))
    {
        br_hdt = pfix.Cog;
        if (m_hdt_source != 3) {
            wxLogMessage(wxT("BR24radar_pi: Heading source is now COG"));
        }
        m_hdt_source = 3;
    }

    br_bpos_set = true;
    bpos_warn_msg = false;
    watchdog = wxDateTime::Now();
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
void br24radar_pi::TransmitCmd(char* msg, int size)
{
    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl((236 << 24) | (6 << 16) | (7 << 8) | 10); // 236.6.7.10
    adr.sin_port=htons(6680);

    if (m_radar_socket == INVALID_SOCKET || sendto(m_radar_socket, msg, size, 0, (struct sockaddr *) &adr, sizeof(adr)) < size) {
        wxLogMessage(wxT("BR24radar_pi: unable to transmit command to radar: %s\n"), SOCKETERRSTR);
        return;
    } else if (settings.verbose) {
        logBinaryData(wxT("command"), msg, size);
    }
};

void br24radar_pi::RadarTxOff(void)
{
//    wxLogMessage(wxT("BR24 Radar turned Off manually."));

    char pck[3] = {(byte)0x00, (byte)0xc1, (byte)0x00};
    TransmitCmd(pck, sizeof(pck));

    pck[0] = (byte)0x01;
    TransmitCmd(pck, sizeof(pck));
}

void br24radar_pi::RadarTxOn(void)
{
//    wxLogMessage(wxT("BR24 Radar turned ON manually."));

    char pck[3] = {(byte)0x00, (byte)0xc1, (byte)0x01};               // ON
    TransmitCmd(pck, sizeof(pck));

    pck[0] = (byte)0x01;
    TransmitCmd(pck, sizeof(pck));
}

void br24radar_pi::RadarStayAlive(void)
{
    if (settings.master_mode && (br_radar_state == RADAR_ON)) {
        char pck[] = {(byte)0xA0, (byte)0xc1};
        TransmitCmd(pck, sizeof(pck));
    }
}

void br24radar_pi::SetRangeMeters(long meters)
{
    if (settings.master_mode) {
        if (meters >= 50 && meters <= 64000) {
            long decimeters = meters * 10L/1.762;
            char pck[] = { (byte) 0x03
                         , (byte) 0xc1
                         , (byte) ((decimeters >>  0) & 0XFFL)
                         , (byte) ((decimeters >>  8) & 0XFFL)
                         , (byte) ((decimeters >> 16) & 0XFFL)
                         , (byte) ((decimeters >> 24) & 0XFFL)
                         };
            if (settings.verbose) {
                wxLogMessage(wxT("SetRangeMeters: %ld meters\n"), meters);
            }
            TransmitCmd(pck, sizeof(pck));
        }
    }
}

void br24radar_pi::SetControlValue(ControlType controlType, int value)
{
    wxString msg;
    
    if (settings.master_mode || controlType == CT_TRANSPARENCY) {
        switch (controlType) {
            case CT_GAIN: {
                if (value < 0) {                // AUTO gain
                    char cmd[] = {
                        (byte)0x06,
                        (byte)0xc1,
                        0, 0, 0, 0, (byte)0x01,
                        0, 0, 0, (byte)0xa1
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("Gain: Auto"));
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                    break;
                } else {                        // Manual Gain
                    int v = value * 255 / 100;
                    if (v > 255) {
                        v = 255;
                    }
                    char cmd[] = {
                        (byte)0x06,
                        (byte)0xc1,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        (char) v
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("Gain: %d"), value);
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                    break;
                }
            }
            case CT_RAIN: {                       // Rain Clutter - Manual. Range is 0x01 to 0x50
                int v = value * 0x50 / 100;
                if (v > 255) {
                    v = 255;
                }
                
                char cmd[] = {
                    (byte)0x06,
                    (byte)0xc1,
                    (byte)0x04,
                    0, 0, 0, 0, 0, 0, 0,
                    (char) v
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("Rain: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_SEA: {
                if (value < 0) {                 // Sea Clutter - Auto
                    char cmd[11] = {
                        (byte)0x06,
                        (byte)0xc1,
                        (byte)0x02,
                        0, 0, 0, (byte)0x01,
                        0, 0, 0, (byte)0xd3
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("Sea: Auto"));
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                    break;
                } else {                       // Sea Clutter
                    int v = value * 255 / 100;
                    if (v > 255) {
                        v = 255;
                    }
                    char cmd[] = {
                        (byte)0x06,
                        (byte)0xc1,
                        (byte)0x02,
                        0, 0, 0, 0, 0, 0, 0, 
                       (char)v
                    };
                    if (settings.verbose) {
                        wxLogMessage(wxT("Sea: %d"), value);
                    }
                    TransmitCmd(cmd, sizeof(cmd));
                    break;
                }
            case CT_REJECTION: {
                settings.rejection = value;
                char cmd[] = {
                    (byte)0x08,
                    (byte)0xc1,
                    (char) settings.rejection
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("Rejection: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_TARGET_BOOST: {
                settings.target_boost = value;
                char cmd[] = {
                    (byte)0x0a,
                    (byte)0xc1,
                    (char) value
                };
                if (settings.verbose) {
                    wxLogMessage(wxT("Target boost: %d"), value);
                }
                TransmitCmd(cmd, sizeof(cmd));
                break;
            }
            case CT_TRANSPARENCY: {
                settings.overlay_transparency = value;
                break;
            }
            default: {
                wxLogMessage(wxT("Unhandled control setting for control %d"), controlType);
            }
            };
        }
    }
    else
    {
        wxLogMessage(wxT("Not master, so ignore SetControlValue: %d %d"), controlType, value);
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



// Ethernet packet stuff *************************************************************

MulticastRXThread::~MulticastRXThread()
{
}

void MulticastRXThread::OnExit()
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
        if (!isdigit(c))
        {
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
            if (pp >= parts + 3)
            {
                return (0);
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
    if (c != '\0' && (!isascii(c) || !isspace(c)))
    {
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
    fd_set fdin;
    struct timeval tv = { (long) timeout, 0 };

    FD_ZERO(&fdin);
    FD_SET(sockfd, &fdin);

    int r = select(sockfd + 1, &fdin, 0, &fdin, &tv);

    return r > 0;
}

void *MulticastRXThread::Entry(void)
{
    wxString msg;
    SOCKET rx_socket;
    struct sockaddr_in adr;
    int one = 1;
    int r = 0;

    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(6678);
    rx_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rx_socket == INVALID_SOCKET) {
        r = -1;
    }
    else {
        r = setsockopt(rx_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &one, sizeof(one));
    }

    if (!r)
    {
        r = bind(rx_socket, (struct sockaddr *) &adr, sizeof(adr));
    }

    if (r)
    {
        wxLogError(wxT("BR24radar_pi: Unable to create UDP sending socket"));
        // Might as well give up now
        if (rx_socket != INVALID_SOCKET) {
            closesocket(rx_socket);
        }
        return 0;
    }

    // Subscribe rx_socket to a multicast group
    struct in_addr recvAddr;

    if (!my_inet_aton(pPlugIn->settings.radar_interface.mb_str().data(), &recvAddr)) {
        wxLogError(wxT("Unable to determine address of %s"), pPlugIn->settings.radar_interface.mb_str().data());
        closesocket(rx_socket);
        return 0;
    }

    struct ip_mreq mreq;
    // listen to 236.6.7.8 on interface identified by recvAddr.
    mreq.imr_multiaddr.s_addr = htonl((236 << 24) | (6 << 16) | (7 << 8) | 8); // 236.6.7.8
    mreq.imr_interface = recvAddr;

    r = setsockopt(rx_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *) &mreq, sizeof(mreq));
    if (r) {
        wxLogError(wxT("Unable to listen to multicast group: %s"), SOCKETERRSTR);
        closesocket(rx_socket);
        return 0;
    }

    sockaddr_storage rx_addr;
    socklen_t        rx_len;

    //    Loop until we quit
    int n_rx_once = 0;
    while (!*m_quit) {
        if (socketReady(rx_socket, 1)) {
            packet_buf frame;
            rx_len = sizeof(rx_addr);
            r = recvfrom(rx_socket, (char *) frame.buf, sizeof(frame.buf), 0, (struct sockaddr *) &rx_addr, &rx_len);
            if (r) {
                if (0 == n_rx_once) {
                    if (pPlugIn->settings.verbose) {
                        wxLogMessage(wxT("BR24radar_pi: First Packet Received."));
                    }
                    n_rx_once++;
                }
                br_scan_packets_per_tick++;
                process_buffer(&frame.packet, r);
            }
        }
    }

    closesocket(rx_socket);
    return 0;
}

// process_buffer
// --------------
// Process one radar frame packet, which can contain up to 32 'spokes' or lines extending outwards
// from the radar up to the range indicated in the packet.
//
// We only get Data packets of fixed length from PORT (6678), see structure in .h file
// Sequence Header - 8 bytes
// 32 line header/data sets per packet
//      Line Header - 24 bytes
//      Line Data - 512 bytes
//
void MulticastRXThread::process_buffer(radar_frame_pkt * packet, int len)
{
    for (int scanline = 0; scanline < 32 ; scanline++) {
        radar_line * line = &packet->line[scanline];
        if ((char *) &packet->line[scanline + 1] > (char *) packet + len)
        {
          // ignore a truncated spoke
          break;
        }

        int range_raw = 0;
        int angle_raw;

        short int large_range;
        short int small_range;
        int range_meters;

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

        // Range change desired?
        if (range_meters != br_range_meters) {

            if (range_meters == 0) {
                wxLogMessage(wxT("BR24radar_pi:  Invalid range received, keeping %d meters"), br_range_meters);
            }
            else {
                wxLogMessage(wxT("BR24radar_pi:  Range Change: %d --> %d meters (raw value: %d"), br_range_meters, range_meters, range_raw);
            }

            br_range_meters = range_meters;

            // Set the control's value to the real range that we received, not a table idea
            if (pPlugIn->m_pControlDialog) {
                pPlugIn->m_pControlDialog->SetRangeIndex(convertMetersToRadarAllowedValue(&range_meters, pPlugIn->settings.range_units, br_radar_type));
            }
        }

        unsigned char *dest_data1 = pPlugIn->m_scan_buf[angle_raw];
        memcpy(dest_data1, line->data, 512);
        
        // The following line is a quick hack to confirm on-screen where the range ends, by putting a 'ring' of
        // returned radar energy at the max range line.
        // TODO: create nice actual range circles.
        pPlugIn->m_scan_buf[angle_raw][511] = (byte)0xff;
        
        pPlugIn->m_scan_range[angle_raw][2] = pPlugIn->m_scan_range[angle_raw][1];
        pPlugIn->m_scan_range[angle_raw][1] = pPlugIn->m_scan_range[angle_raw][0];
        pPlugIn->m_scan_range[angle_raw][0] = range_meters;
    }
}
