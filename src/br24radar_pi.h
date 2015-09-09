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

#include <stdint.h>
#include "wx/wxprec.h"
#include <wx/glcanvas.h>

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#include "jsonreader.h"

#include "version.h"

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    10

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
#include "nmea0183/nmea0183.h"


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

#ifndef __WXMSW__
# ifndef UINT8
#  define UINT8 uint8_t
# endif
# ifndef UINT16
#  define UINT16 uint16_t
# endif
# ifndef UINT32
#  define UINT32 uint32_t
# endif
# define wxTPRId64 wxT("ld")
#else
# define wxTPRId64 wxT("I64d")
#endif

#ifndef INT16_MIN
# define INT16_MIN (-32768)
#endif

# define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
# define MILLISECONDS_PER_SECOND (1000)


#define LINES_PER_ROTATION  (2048) // BR radars can generate up to 4096 lines per rotation, but use only 2048
#define RETURNS_PER_LINE     (512) // BR radars generate 512 separate values per range, at 8 bits each
#define DEGREES_PER_ROTATION (360) // Classical math

// Use the above to convert from 'raw' headings sent by the radar (0..4095) into classical degrees (0..359) and back
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * (double) DEGREES_PER_ROTATION / 4096)
#define SCALE_RAW_TO_DEGREES2048(raw) ((raw) * (double) DEGREES_PER_ROTATION / 2048)
#define SCALE_DEGREES_TO_RAW(angle) ((int)((angle) * (double) 4096 / DEGREES_PER_ROTATION))
#define SCALE_DEGREES_TO_RAW2048(angle) ((int)((angle) * (double) 2048 / DEGREES_PER_ROTATION))
#define MOD_DEGREES(angle) (fmod(angle + 720.0, 360.0))
#define MOD_ROTATION(raw) (((raw) + 2 * 4096) % 4096)
#define MOD_ROTATION2048(raw) (((raw) + 4096) % 2048)
#define displaysetting0_threshold_red (50)
#define displaysetting1_threshold_blue (50)  // should be < 100
#define displaysetting2_threshold_blue (20)  // should be < 100

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
    UINT8 headerLen;       //1 bytes
    UINT8 status;          //1 bytes
    UINT8 scan_number[2];  //2 bytes
    UINT8 mark[4];         //4 bytes 0x00, 0x44, 0x0d, 0x0e
    UINT8 angle[2];        //2 bytes
    UINT8 heading[2];      //2 bytes heading with RI-10/11?
    UINT8 range[4];        //4 bytes
    UINT8 u01[2];          //2 bytes blank
    UINT8 u02[2];          //2 bytes
    UINT8 u03[4];          //4 bytes blank
}; /* total size = 24 */

struct br4g_header {
    UINT8 headerLen;       //1 bytes
    UINT8 status;          //1 bytes
    UINT8 scan_number[2];  //2 bytes
    UINT8 u00[2];          //Always 0x4400 (integer)
    UINT8 largerange[2];   //2 bytes or -1
    UINT8 angle[2];        //2 bytes
    UINT8 heading[2];      //2 bytes heading with RI-10/11 or -1
    UINT8 smallrange[2];   //2 bytes or -1
    UINT8 rotation[2];     //2 bytes, looks like rotation/angle
    UINT8 u02[4];          //4 bytes signed integer, always -1
    UINT8 u03[4];          //4 bytes signed integer, mostly -1 (0x80 in last byte) or 0xa0 in last byte
}; /* total size = 24 */

struct radar_line {
    union {
        br24_header   br24;
        br4g_header   br4g;
    };
    UINT8 data[RETURNS_PER_LINE];
};


/* Normally the packets are have 32 spokes, or scan lines, but we assume nothing
 * so we take up to 120 spokes. This is the nearest round figure without going over
 * 64kB.
 */

struct radar_frame_pkt {
    UINT8      frame_hdr[8];
    radar_line line[120];    //  scan lines, or spokes
};
#pragma pack(pop)

struct receive_statistics {
    int packets;
    int broken_packets;
    int spokes;
    int broken_spokes;
    int missing_spokes;
};

static wxFont g_font;
static wxSize g_buttonSize;

typedef enum ControlType {
    CT_RANGE,
    CT_GAIN,
    CT_SEA,
    CT_RAIN,
    CT_TRANSPARENCY,
    CT_INTERFERENCE_REJECTION,
    CT_TARGET_SEPARATION,
    CT_NOISE_REJECTION,
    CT_TARGET_BOOST,
    CT_REFRESHRATE,
    CT_PASSHEADING,
    CT_SCAN_SPEED,
    CT_SCAN_AGE,
    CT_TIMED_IDLE
} ControlType;

typedef enum GuardZoneType {
    GZ_OFF,
    GZ_ARC,
    GZ_CIRCLE
} GuardZoneType;

typedef enum RadarType {
    RT_UNKNOWN,
    RT_BR24,
    RT_4G
} RadarType;

extern size_t convertRadarMetersToIndex(int *range_meters, int units, RadarType radar_type);
extern size_t convertMetersToRadarAllowedValue(int *range_meters, int units, RadarType radar_type);

typedef enum DisplayModeType {
    DM_CHART_OVERLAY,
    DM_CHART_BLACKOUT,
    DM_SPECTRUM,
    DM_EMULATOR
} DisplayModeType;

static wxString DisplayModeStrings[] = {
    _("Radar Chart Overlay"),
    _("Spectrum"),
    _("Emulator"),
};

static const int RangeUnitsToMeters[2] = {
    1852,
    1000
};

#define DEFAULT_OVERLAY_TRANSPARENCY (5)
#define MIN_OVERLAY_TRANSPARENCY (0)
#define MAX_OVERLAY_TRANSPARENCY (10)
#define MIN_AGE (4)
#define MAX_AGE (12)

struct pi_control_settings {
    int      overlay_transparency;    // now 0-100, no longer a double
	bool     auto_range_mode[2];    // for A and B
	int      range_index;
    int      verbose;
    int      display_option;
    DisplayModeType display_mode[2];
    int      guard_zone;            // active zone (0 = none,1,2)
    int      guard_zone_threshold;  // How many blobs must be sent by radar before we fire alarm
    int      guard_zone_render_style;
 //   int      filter_process;
    double   range_calibration;
    double   heading_correction;
    double   skew_factor;
    int      range_units;       // 0 = Nautical miles, 1 = Kilometers
    int      range_unit_meters; // ... 1852 or 1000, depending on range_units
 //   int      beam_width;
    int      max_age;
    int      timed_idle;
    int      idle_run_time;
    int      draw_algorithm;
    int      scan_speed;
    int      refreshrate;   
	int      passHeadingToOCPN;      
	int      enable_dual_radar;
	int      multi_sweep_filter[2][3];   //  0: guard zone 1 filter state;
                                      //  1: guard zone 2 filter state;
                                      //  2: display filter state, modified in gain control;
                                      //  these values are not saved, for safety reasons user must set them after each start
	int      selectRadarB;
	int      showRadar;
    wxString alert_audio_file;
};

class radar_control_item{
public:
	int value;
	int button;
	bool mod;
	void Update(int v);
};

struct radar_control_setting{
	bool                    mod;
	radar_control_item      range;
	radar_control_item      gain;
	radar_control_item      interference_rejection;
	radar_control_item      target_separation;
	radar_control_item      noise_rejection;
	radar_control_item      target_boost;
	radar_control_item      sea;
	radar_control_item      rain;
	radar_control_item      scan_speed;
};
struct guard_zone_settings {
    int type;                   // 0 = circle, 1 = arc
    int inner_range;            // now in meters
    int outer_range;            // now in meters
    double start_bearing;
    double end_bearing;
};

struct scan_line {
    int range;                        // range of this scan line in decimeters
    wxLongLong age;                   // how old this scan line is. We keep old scans on-screen for a while
    UINT8 data[RETURNS_PER_LINE + 1]; // radar return strength, data[512] is an additional element, accessed in drawing the spokes
	UINT8 history[RETURNS_PER_LINE + 1]; // contains per bit the history of previous scans. 
	   //Each scan this byte is left shifted one bit. If the strength (=level) of a return is above the threshold
	   // a 1 is added in the rightmost position, if below threshold, a 0.
};

//    Forward definitions
class RadarDataReceiveThread;
class RadarCommandReceiveThread;
class RadarReportReceiveThread;
class BR24ControlsDialog;
class BR24MessageBox;
class GuardZoneDialog;
class GuardZoneBogey;
class BR24DisplayOptionsDialog;
class Idle_Dialog;

//ofstream outfile("C:/ProgramData/opencpn/BR24DataDump.dat",ofstream::binary);

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define BR24RADAR_TOOL_POSITION    -1          // Request default positioning of toolbar tool

class br24radar_pi : public opencpn_plugin_110
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
    void SetPluginMessage(wxString &message_id, wxString &message_body);
    void SetCursorLatLon(double lat, double lon);
    void OnContextMenuItemCallback(int id);
    void SetNMEASentence(wxString &sentence);
	bool data_seenAB[2];

    void SetDefaults(void);
    int GetToolbarToolCount(void);
    void OnToolbarToolCallback(int id);
    void ShowPreferencesDialog(wxWindow* parent);
	bool control_box_closed, control_box_opened;

    // Other public methods

    void OnBR24ControlDialogClose(); 
	void OnBR24MessageBoxClose();
    void SetDisplayMode(DisplayModeType mode);
    void UpdateDisplayParameters(void);

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

	void SetBR24MessageBoxX(long x) {
		m_BR24Message_box_x = x;
	}
	void SetBR24MessageBoxY(long y) {
		m_BR24Message_box_y = y;
	}
	void SetBR24MessageBoxSizeX(long sx) {
		m_BR24Message_box_sx = sx;
	}
	void SetBR24MessageBoxSizeY(long sy) {
		m_BR24Message_box_sy = sy;
	}
    void Select_Guard_Zones(int zone);
    void OnGuardZoneDialogClose();
    void OnGuardZoneBogeyClose();
    void OnGuardZoneBogeyConfirm();



    void SetControlValue(ControlType controlType, int value);

    bool LoadConfig(void);
    bool SaveConfig(void);

    long GetRangeMeters();
    long GetOptimalRangeMeters();
    void SetRangeMeters(long range);

    pi_control_settings settings;
	radar_control_setting radar_setting[2];

    scan_line                 m_scan_line[2][LINES_PER_ROTATION];

#define GUARD_ZONES (2)
    guard_zone_settings guardZones[2][GUARD_ZONES];
    
    BR24DisplayOptionsDialog *m_pOptionsDialog;
    BR24ControlsDialog       *m_pControlDialog;
	BR24MessageBox           *m_pMessageBox;
    GuardZoneDialog          *m_pGuardZoneDialog;
    GuardZoneBogey           *m_pGuardZoneBogey;
    Idle_Dialog              *m_pIdleDialog;
    receive_statistics          m_statistics[2];

private:
    void TransmitCmd(UINT8 * msg, int size);
	void TransmitCmd(int AB, UINT8 * msg, int size);   // overcharged
    void RadarTxOff(void);
    void RadarTxOn(void);
    void RadarSendState(void);
    void RadarStayAlive(void);
    void UpdateState(void);
    void DoTick(void);
    void Select_Clutter(int req_clutter_index);
    void Select_Rejection(int req_rejection_index);
    void RenderRadarOverlay(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
    void RenderSpectrum(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp);
	void Guard(int max_range, int AB);
    void RenderRadarBuffer(wxDC *pdc, int width, int height);
    void DrawRadarImage(int max_range, wxPoint radar_center);
    void RenderGuardZone(wxPoint radar_center, double v_scale_ppm, PlugIn_ViewPort *vp, int AB);
    void HandleBogeyCount(int *bogey_count);
    void draw_histogram_column(int x, int y);

    void CacheSetToolbarToolBitmaps(int bm_id_normal, int bm_id_rollover);
    void ShowRadarControl(bool show = true);

    wxFileConfig             *m_pconfig;
    wxWindow                 *m_parent_window;
    wxMenu                   *m_pmenu;

    int                       m_display_width, m_display_height;
    int                       m_tool_id;
    wxBitmap                 *m_pdeficon;

    //    Controls added to Preferences panel
    wxCheckBox               *m_pShowIcon;

    RadarDataReceiveThread   *m_dataReceiveThreadA;
    RadarCommandReceiveThread *m_commandReceiveThreadA;
    RadarReportReceiveThread *m_reportReceiveThreadA;
	RadarDataReceiveThread   *m_dataReceiveThreadB;
	RadarCommandReceiveThread *m_commandReceiveThreadB;
	RadarReportReceiveThread *m_reportReceiveThreadB;

    SOCKET                    m_radar_socket;

    int                       m_BR24Controls_dialog_sx, m_BR24Controls_dialog_sy ;
    int                       m_BR24Controls_dialog_x, m_BR24Controls_dialog_y ;

	int                       m_BR24Message_box_sx, m_BR24Message_box_sy;
	int                       m_BR24Message_box_x,  m_BR24Message_box_y;

    int                        m_GuardZoneBogey_x, m_GuardZoneBogey_y ;

    int                       m_Guard_dialog_sx, m_Guard_dialog_sy ;
    int                       m_Guard_dialog_x, m_Guard_dialog_y ;


    wxBitmap                 *m_ptemp_icon;
    //    wxLogWindow              *m_plogwin;
    int                       m_sent_bm_id_normal;
    int                       m_sent_bm_id_rollover;

    volatile bool             m_quit;
	volatile int				AB;
    enum HeadingSource { HEADING_NONE, HEADING_HDM, HEADING_HDT, HEADING_COG, HEADING_RADAR };
    HeadingSource             m_heading_source;

    br_NMEA0183               m_NMEA0183;

    double                    llat, llon, ulat, ulon, dist_y, pix_y, v_scale_ppm;
};

class RadarDataReceiveThread: public wxThread
{

public:

    RadarDataReceiveThread(br24radar_pi *ppi, volatile bool * quit, int ab)
    : wxThread(wxTHREAD_JOINABLE)
    , pPlugIn(ppi)
    , m_quit(quit)
	, AB(ab)

    {
        //      wxLogMessage(_T("BR24 radar thread starting for multicast address %ls port %ls"), m_ip.c_str(), m_service_port.c_str());
        Create(1024 * 1024);
    };

    ~RadarDataReceiveThread(void);
    void *Entry(void);

    void OnExit(void);

private:
    void process_buffer(radar_frame_pkt * packet, int len);
    void emulate_fake_buffer(void);

    br24radar_pi      *pPlugIn;
    wxString           m_ip;
    volatile bool    * m_quit;
    wxIPV4address      m_myaddr;
	int AB;

};

class RadarCommandReceiveThread: public wxThread
{

public:

    RadarCommandReceiveThread(br24radar_pi *ppi, volatile bool * quit, int ab)
    : wxThread(wxTHREAD_JOINABLE)
    , pPlugIn(ppi)
    , m_quit(quit)
	, AB(ab)
    {
        Create(64 * 1024);
    };

    ~RadarCommandReceiveThread(void);
    void *Entry(void);
    void OnExit(void);

private:
    br24radar_pi      *pPlugIn;
    wxString           m_ip;
    volatile bool    * m_quit;
    wxIPV4address      m_myaddr;
	int                AB;
};

class RadarReportReceiveThread: public wxThread
{

public:

    RadarReportReceiveThread(br24radar_pi *ppi, volatile bool * quit, int ab)
    : wxThread(wxTHREAD_JOINABLE)
    , pPlugIn(ppi)
    , m_quit(quit)
	, AB(ab)
    {
        Create(64 * 1024);
    };

    ~RadarReportReceiveThread(void);
    void *Entry(void);
    void OnExit(void);

private:
    bool ProcessIncomingReport( UINT8 * command, int len );

    br24radar_pi      *pPlugIn;
    wxString           m_ip;
    volatile bool    * m_quit;
    wxIPV4address      m_myaddr;
	int                AB;
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
    void OnIntervalSlider(wxCommandEvent& event);
    void OnDisplayModeClick(wxCommandEvent& event);
    void OnGuardZoneStyleClick(wxCommandEvent& event);
    void OnHeading_Calibration_Value(wxCommandEvent& event);
    void OnSelectSoundClick(wxCommandEvent& event);
    void OnTestSoundClick(wxCommandEvent& event);
    void OnPassHeadingClick(wxCommandEvent& event);
	void OnEnableDualRadarClick(wxCommandEvent& event);
    

    wxWindow          *pParent;
    br24radar_pi      *pPlugIn;

    // DisplayOptions
    wxRadioBox        *pRangeUnits;
    wxRadioBox        *pOverlayDisplayOptions;
    wxRadioBox        *pDisplayMode;
    wxRadioBox        *pGuardZoneStyle;
    wxSlider          *pIntervalSlider;
    wxTextCtrl        *pText_Heading_Correction_Value;
    wxCheckBox        *cbPassHeading;
	wxCheckBox        *cbEnableDualRadar;
};


//----------------------------------------------------------------------------------------------------------
//    BR24Radar Control Dialog Helpers Specification
//----------------------------------------------------------------------------------------------------------

class RadarControlButton: public wxButton
{
public:
    RadarControlButton()
    {

    };

    RadarControlButton(wxWindow *parent,
                       wxWindowID id,
                       const wxString& label,
                       br24radar_pi *ppi,
                       ControlType ct,
                       bool newHasAuto,
                       int newValue
                       )
    {
        Create(parent, id, label + wxT("\n"), wxDefaultPosition, g_buttonSize, 0, wxDefaultValidator, label);
        minValue = 0;
        maxValue = 100;
        value = 0;
        if (ct == CT_GAIN)
        {
          value = 40;
        }
        hasAuto = newHasAuto;
        pPlugIn = ppi;
        firstLine = label;
        names = 0;
        controlType = ct;
        if (hasAuto) {
            SetAuto();
        } else {
            SetValue(newValue);
        }

        this->SetFont(g_font);
    }

    virtual void SetValue(int value);
    virtual void SetAuto();
	virtual void SetValueX(int newValue);
	virtual void SetAutoX();
    const wxString  *names;

    wxString   firstLine;

    br24radar_pi *pPlugIn;

    int        value;

    int        minValue;
    int        maxValue;
    bool       hasAuto;
    ControlType controlType;

};

class RadarRangeControlButton: public RadarControlButton
{
public:
    RadarRangeControlButton(wxWindow *parent,
                            wxWindowID id,
                            const wxString& label,
                            br24radar_pi *ppi
                            )
    {
        Create(parent, id, label + wxT("\n"), wxDefaultPosition, g_buttonSize, 0, wxDefaultValidator, label);
        minValue = 0;
        maxValue = 0;
        auto_range_index = 0;
        value = -1; // means: never set
        hasAuto = true;
        pPlugIn = ppi;
        firstLine = label;
        names = 0;
        controlType = CT_RANGE;

        this->SetFont(g_font);
    }

    virtual void SetValue(int value);
    virtual void SetAuto();

    int SetValueInt(int value);

    bool isRemote; // Set by some other computer or MFD
    int auto_range_index;
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
                const wxString& caption = _("Radar"),
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX)
                );

    void CreateControls();
    void SetRemoteRangeIndex(size_t index);
    void SetRangeIndex(size_t index);
    void SetTimedIdleIndex(int index);
    void UpdateGuardZoneState();
    void UpdateControl(bool haveOpenGL, bool haveGPS, bool haveHeading, bool haveVariation, bool haveRadar, bool haveData);
	void BR24ControlsDialog::UpdateControlValues(bool refreshAll);
	void SetErrorMessage(wxString &msg);
	bool wantShowMessage; // If true, don't hide messagebox automatically

	RadarControlButton *bRadarAB;

private:
    void OnClose(wxCloseEvent& event);
    void OnIdOKClick(wxCommandEvent& event);
    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnPlusTenClick(wxCommandEvent& event);
    void OnPlusClick(wxCommandEvent& event);
    void OnBackClick(wxCommandEvent& event);
    void OnMinusClick(wxCommandEvent& event);
    void OnMinusTenClick(wxCommandEvent& event);
    void OnAutoClick(wxCommandEvent& event);
	void OnMultiSweepClick(wxCommandEvent& event);

    void OnAdvancedBackButtonClick(wxCommandEvent& event);
    void OnAdvancedButtonClick(wxCommandEvent& event);
	void OnRadarGainButtonClick(wxCommandEvent& event);
	void OnRadarABButtonClick(wxCommandEvent& event);
		
    void OnRdrOnlyButtonClick(wxCommandEvent& event);
    void OnMessageButtonClick(wxCommandEvent& event);

    void OnRadarControlButtonClick(wxCommandEvent& event);
	void OnRadarOnlyButtonClick(wxCommandEvent& event);

    void OnZone1ButtonClick(wxCommandEvent &event);
    void OnZone2ButtonClick(wxCommandEvent &event);

    void EnterEditMode(RadarControlButton * button);

    wxWindow          *pParent;
    br24radar_pi      *pPlugIn;
    wxBoxSizer        *topSizer;

    wxBoxSizer        *editBox;
    wxBoxSizer        *advancedBox;
    wxBoxSizer        *advanced4gBox;
    wxBoxSizer        *controlBox;

    wxBoxSizer        *fromBox; // If on edit control, this is where the button is from
    
	
    // Edit Controls

    RadarControlButton *fromControl; // Only set when in edit mode

    // The 'edit' control has these buttons:
    wxButton           *bBack;
    wxButton           *bPlusTen;
    wxButton           *bPlus;
    wxStaticText       *tValue;
    wxButton           *bMinus;
    wxButton           *bMinusTen;
    wxButton           *bAuto;
	wxButton           *bMultiSweep;

    // Advanced controls
    wxButton           *bAdvancedBack;
    RadarControlButton *bTransparency;
    RadarControlButton *bInterferenceRejection;
    RadarControlButton *bTargetSeparation;
    RadarControlButton *bNoiseRejection;
    RadarControlButton *bTargetBoost;
    RadarControlButton *bRefreshrate;
    RadarControlButton *bScanSpeed;
    RadarControlButton *bScanAge;
    RadarControlButton *bTimedIdle;

    // Show Controls

    RadarRangeControlButton *bRange;
	RadarControlButton *bRadarOnly_Overlay;
    RadarControlButton *bGain;
    RadarControlButton *bSea;
    RadarControlButton *bRain;
    wxButton           *bAdvanced;
    wxButton           *bGuard1;
    wxButton           *bGuard2;
    wxButton           *bMessage;
};

class BR24MessageBox : public wxDialog
{
	DECLARE_CLASS(BR24MessageBox)
	DECLARE_EVENT_TABLE()

public:

	BR24MessageBox();

	~BR24MessageBox();
	void Init();

	bool Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id = wxID_ANY,
		const wxString& caption = _("Radar"),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX)
		);

	void CreateControls();
	void UpdateMessage(bool haveOpenGL, bool haveGPS, bool haveHeading, bool haveVariation, bool haveRadar, bool haveData);
	void SetErrorMessage(wxString &msg);
	void SetRadarIPAddress(wxString &msg);
	void SetMcastIPAddress(wxString &msg);
	void SetHeadingInfo(wxString &msg);
	void SetVariationInfo(wxString &msg);
	void SetRadarInfo(wxString &msg);
	wxBoxSizer        *topSizeM;

private:
	void OnClose(wxCloseEvent& event);
	void OnIdOKClick(wxCommandEvent& event);
	void OnMove(wxMoveEvent& event);
	void OnSize(wxSizeEvent& event);
	
	void OnMessageBackButtonClick(wxCommandEvent& event);

	wxWindow          *pParent;
	br24radar_pi      *pPlugIn;
	wxBoxSizer        *nmeaSizer;
	wxBoxSizer        *infoSizer;


	wxBoxSizer        *messageBox;   // Contains NO HDG and/or NO GPS
	wxStaticBox       *ipBox;
	wxStaticBox	      *nmeaBox;
	wxStaticBox	      *infoBox;

	bool              wantShowMessage; // If true, don't hide messagebox automatically

	// MessageBox
	wxButton           *bMsgBack;
	wxStaticText       *tMessage;
	wxCheckBox         *cbOpenGL;
	wxCheckBox         *cbBoatPos;
	wxCheckBox         *cbHeading;
	wxCheckBox         *cbVariation;
	wxCheckBox         *cbRadar;
	wxCheckBox         *cbData;
	wxStaticText       *tStatistics;

};

/*
 =======================================================================================================================
 BR24Radar Guard Zone Dialog Specification ;
 =======================================================================================================================
 */
class GuardZoneDialog :    public wxDialog
{
    DECLARE_CLASS(GuardZoneDialog)
    DECLARE_EVENT_TABLE()

public:
    GuardZoneDialog();

    ~GuardZoneDialog();
    void    Init();

    bool    Create
    (
     wxWindow        *parent,
     br24radar_pi    *ppi,
     wxWindowID      id = wxID_ANY,
     const wxString  &m_caption = _(" Guard Zone Control"),
     const wxPoint   &pos = wxDefaultPosition,
     const wxSize    &size = wxDefaultSize,
     long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
     );


    void    CreateControls();
    void    OnContextMenuGuardCallback(double mark_rng, double mark_brg);
    void    OnGuardZoneDialogShow(int zone);

private:
    void            SetVisibility();
    void            OnGuardZoneModeClick(wxCommandEvent &event);
    void            OnInner_Range_Value(wxCommandEvent &event);
    void            OnOuter_Range_Value(wxCommandEvent &event);
    void            OnStart_Bearing_Value(wxCommandEvent &event);
    void            OnEnd_Bearing_Value(wxCommandEvent &event);
    void            OnFilterClick(wxCommandEvent &event);
    void            OnClose(wxCloseEvent &event);
    void            OnIdOKClick(wxCommandEvent &event);

    wxWindow        *pParent;
    br24radar_pi    *pPlugIn;

    /* Controls */
    wxStaticBox     *pBoxGuardZone;
    wxRadioBox      *pGuardZoneType;
    wxTextCtrl      *pInner_Range;
    wxTextCtrl      *pOuter_Range;
    wxTextCtrl      *pStart_Bearing_Value;
    wxTextCtrl      *pEnd_Bearing_Value;
    wxCheckBox      *cbFilter;
};

/*
 =======================================================================================================================
 BR24Radar Guard Zone Bogey Dialog Specification ;
 =======================================================================================================================
 */
class GuardZoneBogey :    public wxDialog
{
    DECLARE_CLASS(GuardZoneBogey)
    DECLARE_EVENT_TABLE()

public:
    GuardZoneBogey();

    ~GuardZoneBogey();
    void    Init();

    bool    Create
    (
     wxWindow        *parent,
     br24radar_pi    *ppi,
     wxWindowID      id = wxID_ANY,
     const wxString  &m_caption = _("Guard Zone Active"),
     const wxPoint   &pos = wxDefaultPosition,
     const wxSize    &size = wxDefaultSize,
     long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
     );


    void    CreateControls();
    void    SetBogeyCount(int *bogey_count, int next_alarm);

private:
    void            OnClose(wxCloseEvent &event);
    void            OnIdConfirmClick(wxCommandEvent &event);
    void            OnIdCloseClick(wxCommandEvent &event);

    wxWindow        *pParent;
    br24radar_pi    *pPlugIn;

    /* Controls */
    wxStaticText    *pBogeyCountText;
};

/*
 =======================================================================================================================
 BR24Radar Timed Idle Dialog Specification ;
 =======================================================================================================================
 */

// Class Idle_Dialog
class Idle_Dialog : public wxDialog
{
    DECLARE_CLASS(Idle_Dialog)
    DECLARE_EVENT_TABLE()

public:
    Idle_Dialog();

    ~Idle_Dialog();

    void    Init();

    bool Create
    (
     wxWindow        *parent,
     br24radar_pi    *ppi,
     wxWindowID      id = wxID_ANY,
     const wxString  &m_caption = _("Timed Transmit"),
     const wxPoint   &pos = wxPoint(0 ,0),
     const wxSize    &size = wxDefaultSize,
     long            style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU
     );

    void    CreateControls();
    void    SetIdleTimes(int IdleTime, int IdleTimeLeft);

private:

    void            OnClose(wxCloseEvent &event);
    void            OnIdStopIdleClick(wxCommandEvent &event);

    wxWindow        *pParent;

    br24radar_pi    *pPlugIn;

    /* Controls  */
    wxStaticText *p_Idle_Mode;
    wxStaticText *p_IdleTimeLeft;
    wxGauge *m_Idle_gauge;
    wxButton *m_btnStopIdle;

};

#endif
