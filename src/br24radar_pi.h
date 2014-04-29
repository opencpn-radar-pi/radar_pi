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

#ifndef _BR24RADARPI_H_
#define _BR24RADARPI_H_

#include "wx/wxprec.h"
#include <wx/glcanvas.h> 

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    40429

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    8

#ifndef PI
#define PI        3.1415926535897931160E0      /* pi */
#endif

#ifdef __WXGTK__
//#include "/home/dsr/Projects/opencpn_sf/opencpn/include/ocpn_plugin.h"
#endif

#ifdef __WXMSW__
//#include "../../opencpn_sf/opencpn/include/ocpn_plugin.h"
#endif

#include "ocpn_plugin.h"

#ifndef SOCKET
# define SOCKET int
#endif
#ifndef INVALID_SOCKET
# define INVALID_SOCKET ((SOCKET)~0)
#endif

#ifdef __WXMSW__
# define SOCKETERRSTR (strerror(WSAGetLastError()))
#else
# include <errno.h>
# define SOCKETERRSTR (strerror(errno))
# define closesocket(fd) close(fd)
#endif


enum {
    BM_ID_RED,
    BM_ID_RED_SLAVE,
    BM_ID_GREEN,
    BM_ID_GREEN_SLAVE,
    BM_ID_AMBER,
    BM_ID_AMBER_SLAVE,
    BM_ID_BLANK,
    BM_ID_BLANK_SLAVE

};

enum {
    RADAR_OFF,
    RADAR_ON,
};

#pragma pack(push,1)

struct br24_header {
    unsigned char status[2];       //2 bytes
    unsigned char scan_number[2];  //2 bytes
    unsigned char mark[4];         //4 bytes 0x00, 0x44, 0x0d, 0x0e
    unsigned char angle[2];        //2 bytes
    unsigned char u00[2];          //2 bytes blank
    unsigned char range[4];        //4 bytes
    unsigned char u01[2];          //2 bytes blank
    unsigned char u02[2];          //2 bytes
    unsigned char u03[4];          //4 bytes blank
}; /* total size = 24 */

struct br4g_header {
    unsigned char status[2];       //2 bytes
    unsigned char scan_number[2];  //2 bytes
    unsigned char u00[2];          //Always 0x4400 (integer)
    unsigned char largerange[2];   //2 bytes or -1
    unsigned char angle[2];        //2 bytes
    unsigned char u01[2];          //Always 0x8000 = -1
    unsigned char smallrange[2];   //2 bytes or -1
    unsigned char rotation[2];     //2 bytes, looks like rotation/angle
    unsigned char u02[4];          //4 bytes signed integer, always -1
    unsigned char u03[4];          //4 bytes signed integer, mostly -1 (0x80 in last byte) or 0xa0 in last byte
}; /* total size = 24 */

struct radar_line {
    union {
        br24_header   br24;
        br4g_header   br4g;
    };
    unsigned char data[512];
};


/* A single packet, 17160 = 8 + (24 + 512) * 32 bytes = 17156 ... */

struct radar_frame_pkt {
    unsigned char   frame_hdr[8];
    radar_line      line[32];          //  scan lines
};

#pragma pack(pop)

#define DEFAULT_OVERLAY_TRANSPARENCY (5)
#define MIN_OVERLAY_TRANSPARENCY (0)
#define MAX_OVERLAY_TRANSPARENCY (10)

struct radar_control_settings {
    int     radar_type;
    int      overlay_transparency;    // now 0-100, no longer a double
    bool     master_mode;
    bool     verbose;
    bool     auto_range_mode;
    int      range_index;
    int      display_option;
    int      display_mode;
    int      alarm_zone;            // active zone (0 = none,1,2)
    int      alarm_zone_threshold;  // How many blobs must be sent by radar before we fire alarm
    int      gain;
    int      rejection;
    int      filter_process;
    int      sea_clutter_gain;
    int      rain_clutter_gain;
    double   range_calibration;
    double   heading_correction;
    int      range_units;        // 0 = "Nautical miles"), 1 = "Statute miles", 2 = "Kilometers", 3 = "Meters"
    wxString radar_interface;        // IP address of interface to bind to (on UNIX)
    int      beam_width;
};

struct alarm_zone_settings {
    int type;                   // 0 = circle, 1 = arc
    double inner_range;
    double outer_range;
    double start_bearing;
    double end_bearing;
};

//    Forward definitions
class MulticastRXThread;
class BR24ControlsDialog;
class AlarmZoneDialog;
class AlarmZoneBogey;
class BR24DisplayOptionsDialog;

//ofstream outfile("C:/ProgramData/opencpn/BR24DataDump.dat",ofstream::binary);

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define BR24RADAR_TOOL_POSITION    -1          // Request default positioning of toolbar tool

class br24radar_pi : public opencpn_plugin_18
{
public:
    br24radar_pi(void *ppimgr);

//    The required PlugIn Methods
    int Init(void);
    bool DeInit(void);

    int GetAPIVersionMajor();
    int GetAPIVersionMinor();
    int GetPlugInVersionMajor();
    int GetPlugInVersionMinor();

    wxBitmap *GetPlugInBitmap();
    wxString GetCommonName();
    wxString GetShortDescription();
    wxString GetLongDescription();

//    The required override PlugIn Methods
    bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
    bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
    void SetPositionFix(PlugIn_Position_Fix &pfix);
    void SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix);
    void SetCursorLatLon(double lat, double lon);
    void OnContextMenuItemCallback(int id);

    void SetDefaults(void);
    int GetToolbarToolCount(void);
    void OnToolbarToolCallback(int id);
    void ShowPreferencesDialog(wxWindow* parent);

// Other public methods

    void OnBR24ControlDialogClose();         // Control dialog
    void SetDisplayMode(int mode);
    void UpdateDisplayParameters(void);
    void SetRangeMode(int mode);

    void SetBR24ControlsDialogX(long x) {
        m_BR24Controls_dialog_x = x;
    }
    void SetBR24ControlsDialogY(long y) {
        m_BR24Controls_dialog_y = y;
    }
    void SetBR24ControlsDialogSizeX(long sx) {
        m_BR24Controls_dialog_sx = sx;
    }
    void SetBR24ControlsDialogSizeY(long sy) {
        m_BR24Controls_dialog_sy = sy;
    }
    void Select_Alarm_Zones(int zone);
    void OnAlarmZoneDialogClose();
    void OnAlarmZoneBogeyClose();
    void OnAlarmZoneBogeyConfirm();

    void SetFilterProcess(int br_process, int sel_gain);
    void SetGainMode(int mode);
    void SetRejectionMode(int mode);
    bool LoadConfig(void);
    bool SaveConfig(void);

    long GetRangeMeters();
    void SetRangeMeters(long range);

    radar_control_settings settings;

    alarm_zone_settings Zone1;
    alarm_zone_settings Zone2;


#define LINES_PER_ROTATION (4096)
    unsigned char             m_scan_buf[LINES_PER_ROTATION][512];  // scan buffer that contains raw radar scan image
    int                       m_scan_range[LINES_PER_ROTATION][3];  // range in decimeters for the corresponding line in m_scan_buf
                                                                    // kept for 3 rotations, so a scanline is discarded after 3.

    BR24DisplayOptionsDialog *m_pOptionsDialog;
    BR24ControlsDialog       *m_pControlDialog;
    AlarmZoneDialog          *m_pAlarmZoneDialog;
    AlarmZoneBogey           *m_pAlarmZoneBogey;

private:
    void TransmitCmd(char* msg, int size);
    void RadarTxOff(void);
    void RadarTxOn(void);
    void RadarStayAlive(void);
    void UpdateState(void);
    void DoTick(void);
    void Select_Clutter(int req_clutter_index);
    void Select_Rejection(int req_rejection_index);
    void RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
    void RenderRadarStandalone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
    void RenderSpectrum(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
    void OpenGL3_Render_Overlay();
    void RenderRadarBuffer(wxDC *pdc, int width, int height);
    void DrawRadarImage(int max_range, wxPoint radar_center);
    void RenderAlarmZone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
    void HandleBogeyCount(int bogey_count);
    void DrawFilledArc(double r1, double r2, double a1, double a2);
    void draw_blob_dc(wxDC &dc, double angle, double radius, double blob_r, double arc_length,
                      double scale, int xoff, int yoff);
    void draw_blob_gl(double angle, double radius, double blob_width, double blob_heigth);
    void draw_histogram_column(int x, int y);

    void CacheSetToolbarToolBitmaps(int bm_id_normal, int bm_id_rollover);

    wxFileConfig             *m_pconfig;
    wxWindow                 *m_parent_window;
    wxMenu                   *m_pmenu;

    int                       m_display_width, m_display_height;
    int                       m_tool_id;
    bool                      m_bShowIcon;
    wxBitmap                 *m_pdeficon;

    //    Controls added to Preferences panel
    wxCheckBox               *m_pShowIcon;

    MulticastRXThread        *m_receiveThread;

    SOCKET                    m_radar_socket;
    wxDateTime                m_dt_last_render;

    long                      m_BR24Controls_dialog_sx, m_BR24Controls_dialog_sy ;
    long                      m_BR24Controls_dialog_x, m_BR24Controls_dialog_y ;

    long                      m_Alarm_dialog_sx, m_Alarm_dialog_sy ;
    long                      m_Alarm_dialog_x, m_Alarm_dialog_y ;


    wxBitmap                 *m_ptemp_icon;
//    wxLogWindow              *m_plogwin;
    int                       m_sent_bm_id_normal;
    int                       m_sent_bm_id_rollover;

    volatile bool             m_quit;

    int                       m_hdt_source;
    int                       m_hdt_prev_source;

    double                    llat, llon, ulat, ulon, dist_y, pix_y, v_scale_ppm;

};

class MulticastRXThread: public wxThread
{

public:

    MulticastRXThread(br24radar_pi *ppi, volatile bool * quit)
    : wxThread(wxTHREAD_JOINABLE)
    , pPlugIn(ppi)
    , m_quit(quit)
    {
//      wxLogMessage(_T("BR24 radar thread starting for multicast address %ls port %ls"), m_ip.c_str(), m_service_port.c_str());
      Create(1024 * 1024);
    };

    ~MulticastRXThread(void);
    void *Entry(void);

    void OnExit(void);

private:
    void process_buffer(radar_frame_pkt * packet, int len);

    br24radar_pi      *pPlugIn;
    wxString           m_ip;
    volatile bool    * m_quit;
    wxIPV4address      m_myaddr;
};

//----------------------------------------------------------------------------------------------------------
//    BR24Radar DisplayOptions Dialog Specification
//----------------------------------------------------------------------------------------------------------

class BR24DisplayOptionsDialog: public wxDialog
{
    DECLARE_CLASS(BR24DisplayOptionsDialog)
    DECLARE_EVENT_TABLE()

public:

    BR24DisplayOptionsDialog();

    ~BR24DisplayOptionsDialog();
    void Init();

    bool Create(wxWindow *parent, br24radar_pi *ppi);

    void CreateDisplayOptions();

private:
    void OnClose(wxCloseEvent& event);
    void OnIdOKClick(wxCommandEvent& event);
    void OnRangeUnitsClick(wxCommandEvent& event);
    void OnDisplayOptionClick(wxCommandEvent& event);
    void OnRange_Calibration_Value(wxCommandEvent& event);
    void OnIntervalSlider(wxCommandEvent& event);
    void OnDisplayModeClick(wxCommandEvent& event);
    void OnHeading_Calibration_Value(wxCommandEvent& event);

    wxWindow          *pParent;
    br24radar_pi      *pPlugIn;

    // DisplayOptions
    wxRadioBox        *pRangeUnits;
    wxRadioBox        *pOverlayDisplayOptions;
    wxRadioBox        *pDisplayMode;
    wxTextCtrl        *pText_Range_Calibration_Value;
    wxSlider          *pIntervalSlider;
    wxTextCtrl        *pText_Heading_Correction_Value;
};

//----------------------------------------------------------------------------------------------------------
//    BR24Radar Control Dialog Specification
//----------------------------------------------------------------------------------------------------------
class BR24ControlsDialog: public wxDialog
{
    DECLARE_CLASS(BR24ControlsDialog)
    DECLARE_EVENT_TABLE()

public:

    BR24ControlsDialog();

    ~BR24ControlsDialog();
    void Init();

    bool Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id = wxID_ANY,
                const wxString& caption = _("BR24 Radar Control"),
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU);

    void CreateControls();
    void SetActualRange(long range);

private:
    void OnClose(wxCloseEvent& event);
    void OnIdOKClick(wxCommandEvent& event);
    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnTransSlider(wxCommandEvent &event);
    void OnRangeModeClick(wxCommandEvent &event);
    void OnRangeValue(wxCommandEvent &event);
    void OnFilterProcessClick(wxCommandEvent &event);
    void OnRejectionModeClick(wxCommandEvent &event);
    void OnGainSlider(wxCommandEvent &event);
    void OnAlarmDialogClick(wxCommandEvent &event);
    void OnLogModeClick(wxCommandEvent &event);

    wxWindow          *pParent;
    br24radar_pi      *pPlugIn;

    // Controls
    wxSlider          *pTranSlider;
    wxRadioBox        *pRangeMode;
    wxChoice          *pRange;
    wxTextCtrl        *pCommandRange;
    wxTextCtrl        *pActualRange;
    wxRadioBox        *pRejectionMode;
    wxRadioBox        *pFilterProcess;
    wxSlider          *pGainSlider;
    wxRadioBox        *pAlarmZones;
    wxCheckBox        *pCB_log;
};

/*
 =======================================================================================================================
    BR24Radar Alarm Zone Dialog Specification ;
 =======================================================================================================================
 */
class AlarmZoneDialog :    public wxDialog
{
    DECLARE_CLASS(AlarmZoneDialog)
    DECLARE_EVENT_TABLE()

public:
    AlarmZoneDialog();

    ~AlarmZoneDialog();
    void    Init();

    bool    Create
            (
                wxWindow        *parent,
                br24radar_pi    *ppi,
                wxWindowID      id = wxID_ANY,
                const wxString  &m_caption = _(" Alarm Zone Control"),
                const wxPoint   &pos = wxDefaultPosition,
                const wxSize    &size = wxDefaultSize,
                long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
            );


    void    CreateControls();
    void    OnContextMenuAlarmCallback(double mark_rng, double mark_brg);
    void    OnAlarmZoneDialogShow(int zone);

private:
    void            OnAlarmZoneModeClick(wxCommandEvent &event);
    void            OnInner_Range_Value(wxCommandEvent &event);
    void            OnOuter_Range_Value(wxCommandEvent &event);
    void            OnStart_Bearing_Value(wxCommandEvent &event);
    void            OnEnd_Bearing_Value(wxCommandEvent &event);
    void            OnClose(wxCloseEvent &event);
    void            OnIdOKClick(wxCommandEvent &event);

    wxWindow        *pParent;
    br24radar_pi    *pPlugIn;

    /* Controls */
    wxTextCtrl      *pZoneNumber;
    wxRadioBox      *pAlarmZoneType;
    wxTextCtrl      *pInner_Range;
    wxTextCtrl      *pOuter_Range;
    wxTextCtrl      *pStart_Bearing_Value;
    wxTextCtrl      *pEnd_Bearing_Value;
};

/*
 =======================================================================================================================
    BR24Radar Alarm Zone Bogey Dialog Specification ;
 =======================================================================================================================
 */
class AlarmZoneBogey :    public wxDialog
{
    DECLARE_CLASS(AlarmZoneBogey)
    DECLARE_EVENT_TABLE()

public:
    AlarmZoneBogey();

    ~AlarmZoneBogey();
    void    Init();

    bool    Create
            (
                wxWindow        *parent,
                br24radar_pi    *ppi,
                wxWindowID      id = wxID_ANY,
                const wxString  &m_caption = _("Alarm Zone Active"),
                const wxPoint   &pos = wxDefaultPosition,
                const wxSize    &size = wxDefaultSize,
                long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
            );


    void    CreateControls();
    void    SetBogeyCount(int bogey_count, int next_alarm);

private:
    void            OnClose(wxCloseEvent &event);
    void            OnIdConfirmClick(wxCommandEvent &event);
    void            OnIdCloseClick(wxCommandEvent &event);

    wxWindow        *pParent;
    br24radar_pi    *pPlugIn;

    /* Controls */
    wxStaticText    *pBogeyCountText;
};

#endif
