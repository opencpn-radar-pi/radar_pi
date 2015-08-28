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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif                          //precompiled headers

#include <wx/socket.h>
#include "wx/apptrait.h"
//#include <wx/glcanvas.h>
#include "wx/datetime.h"
//#include <wx/fileconf.h>
//#include <fstream>
//#include "chart1.h"
using namespace std;

#ifdef __WXGTK__
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

#ifdef __WXMSW__
#include "GL/glu.h"
#endif

#include "br24radar_pi.h"


enum {                                      // process ID's
    ID_TEXTCTRL1 = 10000,
    ID_BACK,
    ID_PLUS_TEN,
    ID_PLUS,
    ID_VALUE,
    ID_MINUS,
    ID_MINUS_TEN,
    ID_AUTO,
	ID_MULTISWEEP,

    ID_MSG_BACK,
	ID_RDRONLY,

    ID_ADVANCED_BACK,
    ID_TRANSPARENCY,
    ID_INTERFERENCE_REJECTION,
    ID_TARGET_BOOST,
    ID_NOISE_REJECTION,
    ID_TARGET_SEPARATION,
    ID_REFRESHRATE,
    ID_SCAN_SPEED,
    ID_SCAN_AGE,
    ID_TIMED_IDLE,

    ID_RADAR_ONLY,
	ID_RANGE,
	ID_RADAR_AB,
    ID_GAIN,
    ID_SEA,
    ID_RAIN,
    ID_ADVANCED,
    ID_ZONE1,
    ID_ZONE2,

    ID_MESSAGE,
    ID_BPOS,
    ID_HEADING,
    ID_RADAR,
    ID_DATA

};

//---------------------------------------------------------------------------------------
//          Radar Control Implementation
//---------------------------------------------------------------------------------------
IMPLEMENT_CLASS(BR24ControlsDialog, wxDialog)

BEGIN_EVENT_TABLE(BR24ControlsDialog, wxDialog)

EVT_CLOSE(BR24ControlsDialog::OnClose)
EVT_BUTTON(ID_BACK, BR24ControlsDialog::OnBackClick)
EVT_BUTTON(ID_PLUS_TEN,  BR24ControlsDialog::OnPlusTenClick)
EVT_BUTTON(ID_PLUS,  BR24ControlsDialog::OnPlusClick)
EVT_BUTTON(ID_MINUS, BR24ControlsDialog::OnMinusClick)
EVT_BUTTON(ID_MINUS_TEN, BR24ControlsDialog::OnMinusTenClick)
EVT_BUTTON(ID_AUTO,  BR24ControlsDialog::OnAutoClick)
EVT_BUTTON(ID_MULTISWEEP,  BR24ControlsDialog::OnMultiSweepClick)

EVT_BUTTON(ID_MSG_BACK, BR24ControlsDialog::OnMessageBackButtonClick)
EVT_BUTTON(ID_RDRONLY, BR24ControlsDialog::OnRdrOnlyButtonClick)

EVT_BUTTON(ID_ADVANCED_BACK,  BR24ControlsDialog::OnAdvancedBackButtonClick)
EVT_BUTTON(ID_TRANSPARENCY, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_INTERFERENCE_REJECTION, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_BOOST, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_NOISE_REJECTION, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TARGET_SEPARATION, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_REFRESHRATE, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SCAN_SPEED, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_SCAN_AGE, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_TIMED_IDLE, BR24ControlsDialog::OnRadarControlButtonClick)

EVT_BUTTON(ID_RADAR_ONLY, BR24ControlsDialog::OnRadarOnlyButtonClick)
EVT_BUTTON(ID_RANGE, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_RADAR_AB, BR24ControlsDialog::OnRadarABButtonClick)
EVT_BUTTON(ID_GAIN, BR24ControlsDialog::OnRadarGainButtonClick)
EVT_BUTTON(ID_SEA, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_RAIN, BR24ControlsDialog::OnRadarControlButtonClick)
EVT_BUTTON(ID_ADVANCED, BR24ControlsDialog::OnAdvancedButtonClick)
EVT_BUTTON(ID_ZONE1, BR24ControlsDialog::OnZone1ButtonClick)
EVT_BUTTON(ID_ZONE2, BR24ControlsDialog::OnZone2ButtonClick)
EVT_BUTTON(ID_MESSAGE, BR24ControlsDialog::OnMessageButtonClick)

EVT_MOVE(BR24ControlsDialog::OnMove)
EVT_SIZE(BR24ControlsDialog::OnSize)

END_EVENT_TABLE()

//Ranges are metric for BR24 - the hex codes are little endian = 10 X range value

struct RadarRanges {
  int meters;
  int actual_meters;
  const char * name;
};

static const RadarRanges g_ranges_metric[] =
{
  /* Nautical (mixed) first */
    {    50,    98, "50 m" },
    {    75,   146, "75 m" },
    {   100,   195, "100 m" },
    {   250,   488, "1/4 km" },
    {   500,   808, "1/2 km" },
    {   750,  1154, "3/4 km" },
    {  1000,  1616, "1 km" },
    {  1500,  2308, "1.5 km" },
    {  2000,  3366, "2 km" },
    {  3000,  4713, "3 km" },
    {  4000,  5655, "4 km" },
    {  6000,  9408, "6 km" },
    {  8000, 12096, "8 km" },
    { 12000, 18176, "12 km" },
    { 16000, 26240, "16 km" },
    { 24000, 36352, "24 km" },
    { 36000, 52480, "36 km" },
    { 48000, 72704, "48 km" }
};

static const RadarRanges g_ranges_nautic[] =
{
   {    50,            98, "50 m" },
   {    75,           146, "75 m" },
   {   100,           195, "100 m" },
   {  1852 / 8,       451, "1/8 NM" },
   {  1852 / 4,       673, "1/4 NM" },
   {  1852 / 2,      1346, "1/2 NM" },
   {  1852 * 3 / 4,  2020, "3/4 NM" },
   {  1852 * 1,      2693, "1 NM" },
   {  1852 * 3 / 2,  4039, "1.5 NM" },
   {  1852 * 2,      5655, "2 NM" },
   {  1852 * 3,      8079, "3 NM" },
   {  1852 * 4,     10752, "4 NM" },
   {  1852 * 6,     16128, "6 NM" },
   {  1852 * 8,     22208, "8 NM" },
   { 1852 * 12,     36352, "12 NM" },
   { 1852 * 16,     44416, "16 NM" },
   { 1852 * 24,     72704, "24 NM" }
};

static const int METRIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_metric);
static const int NAUTIC_RANGE_COUNT = ARRAY_SIZE(g_ranges_nautic);

static const int g_range_maxValue[2] = { NAUTIC_RANGE_COUNT - 1, METRIC_RANGE_COUNT - 1 };

wxString interference_rejection_names[4];
wxString target_separation_names[4];
wxString noise_rejection_names[3];
wxString target_boost_names[3];
wxString scan_speed_names[2];
wxString timed_idle_times[8];

extern size_t convertRadarMetersToIndex(int * range_meters, int units, RadarType radarType)
{
    const RadarRanges * ranges;
    int myrange = *range_meters;
    size_t n;

    n = g_range_maxValue[units];
    ranges = units ? g_ranges_metric : g_ranges_nautic;

    if (radarType != RT_4G) {
        n--; // only 4G has longest ranges
    }
    for (; n > 0; n--) {
        if (ranges[n].actual_meters <= myrange) {  // step down until past the right range value
            break;
        }
    }
    *range_meters = ranges[n].meters;

    return n;
}

extern size_t convertMetersToRadarAllowedValue(int * range_meters, int units, RadarType radarType)
{
    const RadarRanges * ranges;
    int myrange = *range_meters;
    size_t n;

    n = g_range_maxValue[units];
    ranges = units ? g_ranges_metric : g_ranges_nautic;

    if (radarType != RT_4G) {
        n--; // only 4G has longest ranges
    }
    for (; n > 0; n--) {
        if (ranges[n].meters <= myrange) {  // step down until past the right range value
            break;
        }
    }
    *range_meters = ranges[n].meters;

    return n;
}

void RadarControlButton::SetValue(int newValue)
{
    if (newValue < minValue) {
        value = minValue;
    } else if (newValue > maxValue) {
        value = maxValue;
    } else {
        value = newValue;
    }
    wxString label;

    if (names) {
        label.Printf(wxT("%s\n%s"), firstLine.c_str(), names[value].c_str());
    } else {
        label.Printf(wxT("%s\n%d"), firstLine.c_str(), value);
    }

    this->SetLabel(label);

    pPlugIn->SetControlValue(controlType, value);
}

void RadarControlButton::SetValueX(int newValue)
{            // sets value in the button without sending new value to the radar
	if (newValue < minValue) {
		value = minValue;
	}
	else if (newValue > maxValue) {
		value = maxValue;
	}
	else {
		value = newValue;
	}
	wxString label;

	if (names) {
		label.Printf(wxT("%s\n%s"), firstLine.c_str(), names[value].c_str());
	}
	else {
		label.Printf(wxT("%s\n%d"), firstLine.c_str(), value);
	}

	this->SetLabel(label);

}

void RadarControlButton::SetAuto()
{
    wxString label;

    label << firstLine << wxT("\n") << _("Auto");
    this->SetLabel(label);

    pPlugIn->SetControlValue(controlType, -1);
}

void RadarControlButton::SetAutoX()
{      // sets auto in the button without sending new value to the radar
	wxString label;

	label << firstLine << wxT("\n") << _("Auto");
	this->SetLabel(label);

}

int RadarRangeControlButton::SetValueInt(int newValue)
{
    // newValue is the new range index number
    // sets the new range label in the button and returns the new range in meters
    int units = pPlugIn->settings.range_units;

    maxValue = g_range_maxValue[units];
    int oldValue = value;  // for debugging only
    if (newValue >= minValue && newValue <= maxValue) {
        value = newValue;
    } else if (pPlugIn->settings.auto_range_mode) {
        value = auto_range_index;
    } else if (value > maxValue) {
        value = maxValue;
    }

    int meters = units ? g_ranges_metric[value].meters : g_ranges_nautic[value].meters;
    wxString label;
    wxString rangeText = value < 0 ? wxT("?") : wxString::FromUTF8(units ? g_ranges_metric[value].name : g_ranges_nautic[value].name);
    if (isRemote) {
        label << firstLine << wxT("\n") << _("Remote") << wxT(" (") << rangeText << wxT(")");
    }
    else if (pPlugIn->settings.auto_range_mode) {
        label << firstLine << wxT("\n") << _("Auto") << wxT(" (") << rangeText << wxT(")");
    }
    else {
        label << firstLine << wxT("\n") << rangeText;
    }
    this->SetLabel(label);
    if (pPlugIn->settings.verbose > 0) {
        wxLogMessage(wxT("BR24radar_pi: Range label '%s' auto=%d remote=%d unit=%d max=%d new=%d old=%d"), rangeText.c_str(), pPlugIn->settings.auto_range_mode, isRemote, units, maxValue, newValue, oldValue);
    }

    return meters;
}



void RadarRangeControlButton::SetValue(int newValue)
{
    // newValue is the index of the new range
    // sends the command for the new range to the radar
    isRemote = false;
	pPlugIn->settings.auto_range_mode[pPlugIn->A_B] = false;

    int meters = SetValueInt(newValue);   // do not display the new value now, will be done by receive thread when frame with new range is received
    pPlugIn->SetRangeMeters(meters);        // send new value to the radar
}

void RadarRangeControlButton::SetAuto()
{
    pPlugIn->settings.auto_range_mode[pPlugIn->A_B] = true;
}

BR24ControlsDialog::BR24ControlsDialog()
{
    //      printf("BR24BUIDialog ctor\n");
    Init();
}

BR24ControlsDialog::~BR24ControlsDialog()
{
}


void BR24ControlsDialog::Init()
{
    // Initialize all members that need initialization
}

bool BR24ControlsDialog::Create(wxWindow *parent, br24radar_pi *ppi, wxWindowID id,
                                const wxString& caption,
                                const wxPoint& pos, const wxSize& size, long style)
{

    pParent = parent;
    pPlugIn = ppi;

#ifdef wxMSW
    long wstyle = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN;
#else
    long wstyle =                 wxCLOSE_BOX | wxCAPTION | wxRESIZE_BORDER;
#endif

    if (!wxDialog::Create(parent, id, caption, pos, wxDefaultSize, wstyle)) {
        return false;
    }
    g_font = *OCPNGetFont(_("Dialog"), 12);

    CreateControls();
    return true;
}


void BR24ControlsDialog::CreateControls()
{
    static int BORDER = 0;
    wxFont fatFont = g_font;
    fatFont.SetWeight(wxFONTWEIGHT_BOLD);
    fatFont.SetPointSize(g_font.GetPointSize() + 1);

    // A top-level sizer
    topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    /*
     * Here be dragons... Since I haven't been able to create buttons that adapt up, and at the same time
     * calculate the biggest button, and I want all buttons to be the same width I use a trick.
     * I generate a multiline StaticText containing all the (long) button labels and find out what the width
     * of that is, and then generate the buttons using that width.
     * I know, this is a hack, but this way it works relatively nicely even with translations.
     */
    wxBoxSizer * testBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(testBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    wxString label;
    label << _("Transparency") << wxT("\n");
    label << _("Interference rejection") << wxT("\n");
    label << _("Target separation") << wxT("\n");
    label << _("Noise rejection") << wxT("\n");
    label << _("Target boost") << wxT("\n");
    label << _("Scan speed") << wxT("\n");
    label << _("Scan age") << wxT("\n");
    label << _("Timed Transmit") << wxT("\n");
    label << _("Gain") << wxT("\n");
    label << _("Sea clutter") << wxT("\n");
    label << _("Rain clutter") << wxT("\n");
    label << _("Auto") << wxT(" (1/20 NM)\n");

    wxStaticText * testMessage = new wxStaticText(this, ID_BPOS, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    testBox->Add(testMessage, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    testMessage->SetFont(g_font);

    topSizer->Fit(this);
    topSizer->Layout();
    int width = topSizer->GetSize().GetWidth() + 10;
    wxSize bestSize = GetBestSize();
    if (width < bestSize.GetWidth()) {
        width = bestSize.GetWidth();
    }
    if (width < 100) {
        width = 100;
    }
    if (width > 300) {
        width = 300;
    }
    g_buttonSize = wxSize(width, 40);  // was 50, buttons a bit lower now
    if (pPlugIn->settings.verbose) {
        wxLogMessage(wxT("BR24radar_pi: Dynamic button width = %d"), g_buttonSize.GetWidth());
    }
    topSizer->Hide(testBox);
    topSizer->Remove(testBox);
    // Determined desired button width


    //**************** MESSAGE BOX ******************//
    // A box sizer to contain warnings

    messageBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(messageBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    tMessage = new wxStaticText(this, ID_BPOS, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    messageBox->Add(tMessage, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    tMessage->SetLabel(_("Radar requires the following"));
    tMessage->SetFont(g_font);

    wxStaticBox* optionsBox = new wxStaticBox(this, wxID_ANY, _("OpenCPN options"));
    optionsBox->SetFont(g_font);
    wxStaticBoxSizer* optionsSizer = new wxStaticBoxSizer(optionsBox, wxVERTICAL);
    messageBox->Add(optionsSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    cbOpenGL = new wxCheckBox(this, ID_BPOS, _("Accelerated Graphics (OpenGL)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    optionsSizer->Add(cbOpenGL, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbOpenGL->SetFont(g_font);
    cbOpenGL->Disable();

    ipBox = new wxStaticBox(this, wxID_ANY, _("ZeroConf via Ethernet"));
    ipBox->SetFont(g_font);
    wxStaticBoxSizer* ipSizer = new wxStaticBoxSizer(ipBox, wxVERTICAL);
    messageBox->Add(ipSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    cbRadar = new wxCheckBox(this, ID_RADAR, _("Radar present"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    ipSizer->Add(cbRadar, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbRadar->SetFont(g_font);
    cbRadar->Disable();

    cbData = new wxCheckBox(this, ID_DATA, _("Radar sending data"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    ipSizer->Add(cbData, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbData->SetFont(g_font);
    cbData->Disable();

	nmeaBox = new wxStaticBox(this, wxID_ANY, _("For radar overlay also required"));
    nmeaBox->SetFont(g_font);

 //   wxStaticBoxSizer* nmeaSizer = new wxStaticBoxSizer(nmeaBox, wxVERTICAL);
	nmeaSizer = new wxStaticBoxSizer(nmeaBox, wxVERTICAL);
    messageBox->Add(nmeaSizer, 0, wxEXPAND | wxALL, BORDER * 2);

    cbBoatPos = new wxCheckBox(this, ID_BPOS, _("Boat position"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbBoatPos, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbBoatPos->SetFont(g_font);
    cbBoatPos->Disable();

    cbHeading = new wxCheckBox(this, ID_HEADING, _("Heading"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbHeading, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbHeading->SetFont(g_font);
    cbHeading->Disable();

    cbVariation = new wxCheckBox(this, ID_HEADING, _("Variation"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    nmeaSizer->Add(cbVariation, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    cbVariation->SetFont(g_font);
    cbVariation->Disable();

    tStatistics = new wxStaticText(this, ID_VALUE, _("Statistics"), wxDefaultPosition, g_buttonSize, 0);
    tStatistics->SetFont(*OCPNGetFont(_("Dialog"), 8));
    messageBox->Add(tStatistics, 0, wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE, BORDER);

    // The <Radar Only> button
    bRdrOnly = new wxButton(this, ID_RDRONLY, _("Radar Only"), wxDefaultPosition, wxDefaultSize, 0);
    messageBox->Add(bRdrOnly, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bRdrOnly->SetFont(g_font);
    messageBox->Hide(bRdrOnly);
	
	// The <Close> button
    bMsgBack = new wxButton(this, ID_MSG_BACK, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    messageBox->Add(bMsgBack, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMsgBack->SetFont(g_font);
    messageBox->Hide(bMsgBack);

    // topSizer->Hide(messageBox);

    //**************** EDIT BOX ******************//
	 // A box sizer to contain RANGE button
    editBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(editBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    // A box sizer to contain RANGE button
    editBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(editBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    // The <<Back button
    bBack = new wxButton(this, ID_BACK, _("<<\nBack"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bBack, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bBack->SetFont(g_font);

    // The +10 button
    bPlusTen = new wxButton(this, ID_PLUS_TEN, _("+10"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bPlusTen, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bPlusTen->SetFont(g_font);

    // The + button
    bPlus = new wxButton(this, ID_PLUS, _("+"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bPlus, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bPlus->SetFont(g_font);

    // The VALUE button
    tValue = new wxStaticText(this, ID_VALUE, _("Value"), wxDefaultPosition, g_buttonSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    editBox->Add(tValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    tValue->SetFont(fatFont);
    tValue->SetBackgroundColour(*wxLIGHT_GREY);

    // The - button
    bMinus = new wxButton(this, ID_MINUS, _("-"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMinus->SetFont(g_font);

    // The -10 button
    bMinusTen = new wxButton(this, ID_MINUS_TEN, _("-10"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bMinusTen, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMinusTen->SetFont(g_font);

    // The Auto button
    bAuto = new wxButton(this, ID_AUTO, _("Auto"), wxDefaultPosition, g_buttonSize, 0);
    editBox->Add(bAuto, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bAuto->SetFont(g_font);

	// The Multi Sweep Filter button
	wxString labelMS;
    labelMS << _("Multi Sweep Filter") << wxT("\n") << _("OFF");
    bMultiSweep = new wxButton(this, ID_MULTISWEEP, labelMS, wxDefaultPosition, wxSize(width, 40), 0);
    editBox->Add(bMultiSweep, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMultiSweep->SetFont(g_font);

    topSizer->Hide(editBox);
    

    //**************** ADVANCED BOX ******************//
    // These are the controls that the users sees when the Advanced button is selected

    advancedBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(advancedBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

    // The Back button
    wxString backButtonStr;
    backButtonStr << wxT("<<\n") << _("Back");
    bAdvancedBack = new wxButton(this, ID_ADVANCED_BACK, backButtonStr, wxDefaultPosition, g_buttonSize, 0);
    advancedBox->Add(bAdvancedBack, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bAdvancedBack->SetFont(g_font);

    // The TRANSPARENCY button
    bTransparency = new RadarControlButton(this, ID_TRANSPARENCY, _("Transparency"), pPlugIn, CT_TRANSPARENCY, false, pPlugIn->settings.overlay_transparency);
    advancedBox->Add(bTransparency, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bTransparency->minValue = MIN_OVERLAY_TRANSPARENCY;
    bTransparency->maxValue = MAX_OVERLAY_TRANSPARENCY;

    // The REJECTION button

    interference_rejection_names[0] = _("Off");
    interference_rejection_names[1] = _("Low");
    interference_rejection_names[2] = _("Medium");
    interference_rejection_names[3] = _("High");

    bInterferenceRejection = new RadarControlButton(this, ID_INTERFERENCE_REJECTION, _("Interference rejection"), pPlugIn, 
		CT_INTERFERENCE_REJECTION, false, pPlugIn->radar_setting[pPlugIn->A_B].interference_rejection.button);
    advancedBox->Add(bInterferenceRejection, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bInterferenceRejection->minValue = 0;
    bInterferenceRejection->maxValue = ARRAY_SIZE(interference_rejection_names) - 1;
    bInterferenceRejection->names = interference_rejection_names;
	bInterferenceRejection->SetValueX(pPlugIn->radar_setting[pPlugIn->A_B].interference_rejection.button); // redraw after adding names

    // The TARGET BOOST button
    target_boost_names[0] = _("Off");
    target_boost_names[1] = _("Low");
    target_boost_names[2] = _("High");
	bTargetBoost = new RadarControlButton(this, ID_TARGET_BOOST, _("Target boost"), pPlugIn, CT_TARGET_BOOST, false, 
		pPlugIn->radar_setting[pPlugIn->A_B].target_boost.button);
    advancedBox->Add(bTargetBoost, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bTargetBoost->minValue = 0;
    bTargetBoost->maxValue = ARRAY_SIZE(target_boost_names) - 1;
    bTargetBoost->names = target_boost_names;
	bTargetBoost->SetValueX(pPlugIn->radar_setting[pPlugIn->A_B].target_boost.button); // redraw after adding names


    // The NOISE REJECTION button
    noise_rejection_names[0] = _("Off");
    noise_rejection_names[1] = _("Low");
    noise_rejection_names[2] = _("High");

    bNoiseRejection = new RadarControlButton(this, ID_NOISE_REJECTION, _("Noise rejection"), pPlugIn, CT_NOISE_REJECTION, false, 
		pPlugIn->radar_setting[pPlugIn->A_B].noise_rejection.button);
    advancedBox->Add(bNoiseRejection, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bNoiseRejection->minValue = 0;
    bNoiseRejection->maxValue = ARRAY_SIZE(noise_rejection_names) - 1;
    bNoiseRejection->names = noise_rejection_names;
	bNoiseRejection->SetValueX(pPlugIn->radar_setting[pPlugIn->A_B].noise_rejection.button); // redraw after adding names

    advanced4gBox = new wxBoxSizer(wxVERTICAL);
    advancedBox->Add(advanced4gBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

    // The TARGET SEPARATION button

    target_separation_names[0] = _("Off");
    target_separation_names[1] = _("Low");
    target_separation_names[2] = _("Medium");
    target_separation_names[3] = _("High");

    bTargetSeparation = new RadarControlButton(this, ID_TARGET_SEPARATION, _("Target separation"), 
		pPlugIn, CT_TARGET_SEPARATION, false, pPlugIn->radar_setting[pPlugIn->A_B].target_separation.button);
    advanced4gBox->Add(bTargetSeparation, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bTargetSeparation->minValue = 0;
    bTargetSeparation->maxValue = ARRAY_SIZE(target_separation_names) - 1;
    bTargetSeparation->names = target_separation_names;
	bTargetSeparation->SetValueX(pPlugIn->radar_setting[pPlugIn->A_B].target_separation.button); // redraw after adding names

    // The SCAN SPEED button
    scan_speed_names[0] = _("Normal");
    scan_speed_names[1] = _("Fast");
    bScanSpeed = new RadarControlButton(this, ID_SCAN_SPEED, _("Scan speed"), pPlugIn, CT_SCAN_SPEED, false, pPlugIn->settings.scan_speed);
    advanced4gBox->Add(bScanSpeed, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bScanSpeed->minValue = 0;
    bScanSpeed->maxValue = ARRAY_SIZE(scan_speed_names) - 1;
    bScanSpeed->names = scan_speed_names;
    bScanSpeed->SetValueX(pPlugIn->settings.scan_speed); // redraw after adding names
	   
    // The REFRESHRATE button
	bRefreshrate = new RadarControlButton(this, ID_REFRESHRATE, _("Refresh rate"), pPlugIn, CT_REFRESHRATE, false, pPlugIn->settings.refreshrate[pPlugIn->A_B]);
    advancedBox->Add(bRefreshrate, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bRefreshrate->minValue = 1;
    bRefreshrate->maxValue = 5;

    // The SCAN AGE button
    bScanAge = new RadarControlButton(this, ID_SCAN_AGE, _("Scan age"), pPlugIn, CT_SCAN_AGE, false, pPlugIn->settings.max_age);
    advancedBox->Add(bScanAge, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bScanAge->minValue = MIN_AGE;
    bScanAge->maxValue = MAX_AGE;

    // The TIMED TRANSMIT button
    timed_idle_times[0] = _("Off");
    timed_idle_times[1] = _("5 min");
    timed_idle_times[2] = _("10 min");
    timed_idle_times[3] = _("15 min");
    timed_idle_times[4] = _("20 min");
    timed_idle_times[5] = _("25 min");
    timed_idle_times[6] = _("30 min");
    timed_idle_times[7] = _("35 min");

    bTimedIdle = new RadarControlButton(this, ID_TIMED_IDLE, _("Timed Transmit"), pPlugIn, CT_TIMED_IDLE, false, pPlugIn->settings.timed_idle); //HakanToDo new setting
    advancedBox->Add(bTimedIdle, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bTimedIdle->minValue = 0;
    bTimedIdle->maxValue = ARRAY_SIZE(timed_idle_times) - 1;
    bTimedIdle->names = timed_idle_times;
    bTimedIdle->SetValue(pPlugIn->settings.timed_idle); // redraw after adding names

    topSizer->Hide(advancedBox);

    //**************** CONTROL BOX ******************//
    // These are the controls that the users sees when the dialog is started

    // A box sizer to contain RANGE, GAIN etc button
    controlBox = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(controlBox, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, BORDER);

	// The RADAR ONLY / OVERLAY button
    bRadarOnly_Overlay = new RadarRangeControlButton(this, ID_RADAR_ONLY, _("Radar Only / Overlay"), pPlugIn);
    controlBox->Add(bRadarOnly_Overlay, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
	if (pPlugIn->settings.display_mode == DM_CHART_BLACKOUT) {
		wxString label; 
		label << _("Overlay / Radar") << wxT("\n") << _("Radar Only, Head Up") ;
        bRadarOnly_Overlay->SetLabel(label);
	}
	else {
		wxString label;
		label << _("Overlay / Radar") << wxT("\n") << _("Radar Overlay");
        bRadarOnly_Overlay->SetLabel(label);
	}

	// The RADAR A / B button
	wxString labelab;
	labelab << _("Radar A / B") << wxT("\n") << _("Radar A");
	bRadarAB = new RadarRangeControlButton(this, ID_RADAR_AB, labelab, pPlugIn);
	controlBox->Add(bRadarAB, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
	if (pPlugIn->settings.selectRadarB == 1) {
		wxString labelab1;
		labelab1 << _("Radar A / B") << wxT("\n") << _("Radar B");
		bRadarAB->SetLabel(labelab1);
	}
	
    // The RANGE button
    bRange = new RadarRangeControlButton(this, ID_RANGE, _("Range"), pPlugIn);
    controlBox->Add(bRange, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);

    // The GAIN button
	bGain = new RadarControlButton(this, ID_GAIN, _("Gain"), pPlugIn, CT_GAIN, true, pPlugIn->radar_setting[pPlugIn->A_B].gain.button);
    controlBox->Add(bGain, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);

    // The SEA button
	bSea = new RadarControlButton(this, ID_SEA, _("Sea clutter"), pPlugIn, CT_SEA, true, pPlugIn->radar_setting[pPlugIn->A_B].sea.button);
    controlBox->Add(bSea, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);

    // The RAIN button
    bRain = new RadarControlButton(this, ID_RAIN, _("Rain clutter"), pPlugIn, CT_RAIN, false, pPlugIn->radar_setting[pPlugIn->A_B].rain.button);
    controlBox->Add(bRain, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);

    // The ADVANCED button
    bAdvanced = new wxButton(this, ID_ADVANCED, _("Advanced\ncontrols"), wxDefaultPosition, g_buttonSize, 0);
    controlBox->Add(bAdvanced, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bAdvanced->SetFont(g_font);

    // The GUARD ZONE 1 button
    wxString label1;
    label1 << _("Guard zone") << wxT(" 1\n");
    bGuard1 = new wxButton(this, ID_ZONE1, label1, wxDefaultPosition, g_buttonSize, 0);
    controlBox->Add(bGuard1, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bGuard1->SetFont(g_font);

    // The GUARD ZONE 2 button
    wxString label2;
    label2 << _("Guard zone") << wxT(" 2\n");
    bGuard2 = new wxButton(this, ID_ZONE2, label2, wxDefaultPosition, g_buttonSize, 0);
    controlBox->Add(bGuard2, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bGuard2->SetFont(g_font);

    // The INFO button
    bMessage = new wxButton(this, ID_MESSAGE, _("Info"), wxDefaultPosition, wxSize(width, 25), 0);
    controlBox->Add(bMessage, 0, wxALIGN_CENTER_VERTICAL | wxALL, BORDER);
    bMessage->SetFont(g_font);
    wantShowMessage = false;

    fromBox = controlBox;
    topSizer->Hide(controlBox);

    UpdateGuardZoneState();

    pPlugIn->UpdateDisplayParameters();
    DimeWindow(this); // Call OpenCPN to change colours depending on day/night mode
    Fit();
    //wxSize size_min = GetBestSize();
    //SetMinSize(size_min);
    //SetSize(size_min);
}

void BR24ControlsDialog::UpdateGuardZoneState()
{
    wxString label1, label2;

    wxString GuardZoneNames[] = {
        _("Off"),
        _("Arc"),
        _("Circle")
    };

    label1 << _("Guard zone") << wxT(" 1\n") << GuardZoneNames[pPlugIn->guardZones[0].type];
    bGuard1->SetLabel(label1);

    label2 << _("Guard zone") << wxT(" 2\n") << GuardZoneNames[pPlugIn->guardZones[1].type];
    bGuard2->SetLabel(label2);
}

void BR24ControlsDialog::SetRemoteRangeIndex(size_t index)
{
    bRange->isRemote = true;
    bRange->SetValueInt(index); // set and recompute the range label
}

void BR24ControlsDialog::SetRangeIndex(size_t index)
{
    bRange->isRemote = false;
    bRange->SetValueInt(index); // set and recompute the range label
}

void BR24ControlsDialog::SetTimedIdleIndex(int index)
{
    bTimedIdle->SetValue(index) ; // set and recompute the Timed Idle label
    if (pPlugIn->m_pIdleDialog && index == 0) pPlugIn->m_pIdleDialog->Close();
}

void BR24ControlsDialog::OnZone1ButtonClick(wxCommandEvent &event)
{
    pPlugIn->Select_Guard_Zones(0);
}

void BR24ControlsDialog::OnZone2ButtonClick(wxCommandEvent &event)
{
    pPlugIn->Select_Guard_Zones(1);
}

void BR24ControlsDialog::OnClose(wxCloseEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
}


void BR24ControlsDialog::OnIdOKClick(wxCommandEvent& event)
{
    pPlugIn->OnBR24ControlDialogClose();
}

void BR24ControlsDialog::OnPlusTenClick(wxCommandEvent& event)
{
    fromControl->SetValue(fromControl->value + 10);
    wxString label = fromControl->GetLabel();

    tValue->SetLabel(label);
}

void BR24ControlsDialog::OnPlusClick(wxCommandEvent& event)
{
    fromControl->SetValue(fromControl->value + 1);
    wxString label = fromControl->GetLabel();

    tValue->SetLabel(label);
}

void BR24ControlsDialog::OnBackClick(wxCommandEvent &event)
{
    extern RadarType br_radar_type;

    topSizer->Hide(editBox);
    topSizer->Show(fromBox);
    if (fromBox == advancedBox) {
        if (br_radar_type == RT_4G) {
            advancedBox->Show(advanced4gBox);
        } else {
            advancedBox->Hide(advanced4gBox);
        }
    }

    topSizer->Layout();
}

void BR24ControlsDialog::OnAutoClick(wxCommandEvent &event)
{
    fromControl->SetAuto();

    OnBackClick(event);
}

void BR24ControlsDialog::OnMultiSweepClick(wxCommandEvent &event)
{
	wxString labelSweep; 
	if ((pPlugIn->settings.multi_sweep_filter[2]) != 1) 
	{
		labelSweep << _("Multi Sweep Filter") << wxT("\n") << _("ON");
		pPlugIn->settings.multi_sweep_filter[2] = 1;
	}
	else
	{
		labelSweep << _("Multi Sweep Filter") << wxT("\n") << _("OFF");
		pPlugIn->settings.multi_sweep_filter[2] = 0;  // reset bit 2
        wxLogMessage(wxT("BR24radar_pi: Multi Sweep Filter OFF %d"), pPlugIn->settings.multi_sweep_filter[2]);
	}
	bMultiSweep->SetLabel(labelSweep);
	bMultiSweep->SetFont(g_font);
}



void BR24ControlsDialog::OnMinusClick(wxCommandEvent& event)
{
    fromControl->SetValue(fromControl->value - 1);

    wxString label = fromControl->GetLabel();
    tValue->SetLabel(label);
}

void BR24ControlsDialog::OnMinusTenClick(wxCommandEvent& event)
{
    fromControl->SetValue(fromControl->value - 10);

    wxString label = fromControl->GetLabel();
    tValue->SetLabel(label);
}

void BR24ControlsDialog::OnAdvancedBackButtonClick(wxCommandEvent& event)
{
    fromBox = controlBox;
    topSizer->Hide(advancedBox);
    topSizer->Show(controlBox);
    advancedBox->Layout();
    Fit();
    topSizer->Layout();
}

void BR24ControlsDialog::OnAdvancedButtonClick(wxCommandEvent& event)
{
    extern RadarType br_radar_type;

    fromBox = advancedBox;
    topSizer->Show(advancedBox);
    if (br_radar_type == RT_4G) {
        advancedBox->Show(advanced4gBox);
    } else {
        advancedBox->Hide(advanced4gBox);
    }
    advancedBox->Layout();
    topSizer->Hide(controlBox);
    controlBox->Layout();
    Fit();
    topSizer->Layout();
}


void BR24ControlsDialog::OnMessageBackButtonClick(wxCommandEvent& event)
{
    wantShowMessage = false;
    fromBox = messageBox;
    topSizer->Hide(messageBox);
    topSizer->Show(controlBox);
    Fit();
    topSizer->Layout();
}

void BR24ControlsDialog::OnRdrOnlyButtonClick(wxCommandEvent& event)
{
	pPlugIn->settings.display_mode = DM_CHART_BLACKOUT;
	messageBox->Hide(bRdrOnly);
	wxString label;
	label << _("Overlay / Radar") << wxT("\n") << _("Radar Only, Head Up") ;
    bRadarOnly_Overlay->SetLabel(label);
 //   Fit();
 //   topSizer->Layout();
	
}

void BR24ControlsDialog::OnMessageButtonClick(wxCommandEvent& event)
{
    wantShowMessage = true;
    topSizer->Hide(controlBox);
    topSizer->Show(messageBox);
	messageBox->Hide(bRdrOnly);
	messageBox->Show(bMsgBack);
    Fit();
    topSizer->Layout();
}

void BR24ControlsDialog::EnterEditMode(RadarControlButton * button)
{
    fromControl = button; // Keep a record of which button was clicked
    tValue->SetLabel(button->GetLabel());
    topSizer->Hide(controlBox);
    topSizer->Hide(advancedBox);
    topSizer->Show(editBox);
    if (fromControl->hasAuto) {
        bAuto->Show();
    }
    else {
        bAuto->Hide();
    }
	if (fromControl == bGain){
		bMultiSweep->Show();
	}
	else{
		bMultiSweep->Hide();
	}
    if (fromControl->maxValue > 20) {
        bPlusTen->Show();
        bMinusTen->Show();
    }
    else {
        bPlusTen->Hide();
        bMinusTen->Hide();
    }
    editBox->Layout();
    topSizer->Layout();
}


void BR24ControlsDialog::OnRadarControlButtonClick(wxCommandEvent& event)
{
    EnterEditMode((RadarControlButton *) event.GetEventObject());
}

void BR24ControlsDialog::OnRadarOnlyButtonClick(wxCommandEvent& event)
{
    if (pPlugIn->settings.display_mode == DM_CHART_BLACKOUT) {
		pPlugIn->settings.display_mode = DM_CHART_OVERLAY;
		wxString label ; 
		label << _("Overlay / Radar") << wxT("\n") << _("Radar Overlay");
        bRadarOnly_Overlay->SetLabel(label);
	}
	else {
		pPlugIn->settings.display_mode = DM_CHART_BLACKOUT;
		wxString label;
		label << _("Overlay / Radar") << wxT("\n") << _("Radar Only, Head Up") ;
        bRadarOnly_Overlay->SetLabel(label);
	}
}

void BR24ControlsDialog::OnRadarGainButtonClick(wxCommandEvent& event)
{
	EnterEditMode((RadarControlButton *) event.GetEventObject());
}

void BR24ControlsDialog::OnRadarABButtonClick(wxCommandEvent& event)
{
	
	if (pPlugIn->settings.selectRadarB == 0){
		wxString labels;
		pPlugIn->settings.selectRadarB = 1;
		pPlugIn->A_B = 1;
		labels << _("Radar A / B") << wxT("\n") << _("Radar B");
		bRadarAB->SetLabel(labels);
	}
	else{
		wxString labels;
		pPlugIn->settings.selectRadarB = 0;
		pPlugIn->A_B = 0;
		labels << _("Radar A / B") << wxT("\n") << _("Radar A");
		bRadarAB->SetLabel(labels);
	}
	
	UpdateControl(true);
}

void BR24ControlsDialog::OnMove(wxMoveEvent& event)
{
    //    Record the dialog position
    wxPoint p =  GetPosition();
    pPlugIn->SetBR24ControlsDialogX(p.x);
    pPlugIn->SetBR24ControlsDialogY(p.y);

    event.Skip();
}

void BR24ControlsDialog::OnSize(wxSizeEvent& event)
{
    //    Record the dialog size
    wxSize p = event.GetSize();
    pPlugIn->SetBR24ControlsDialogSizeX(p.GetWidth());
    pPlugIn->SetBR24ControlsDialogSizeY(p.GetHeight());

    event.Skip();
}

void BR24ControlsDialog::UpdateControl(bool refreshAll)
{
	if (!topSizer->IsShown(controlBox)) return;
	if (pPlugIn->radar_setting[0].gain.button == -1){
		wxLogMessage(wxT("BR24radar_pi: XX gain auto"));
		bGain->SetAutoX();
	}
	else{
		bGain->SetValueX(pPlugIn->radar_setting[0].gain.button);
	}
	bRain->SetValueX(pPlugIn->radar_setting[0].rain.button);
	if (pPlugIn->radar_setting[0].sea.button == -1){
		wxLogMessage(wxT("BR24radar_pi: XX sea auto"));
		bSea->SetAutoX();
	}
	else{
		bSea->SetValueX(pPlugIn->radar_setting[0].sea.button);
	}
	bTargetBoost->SetValueX(pPlugIn->radar_setting[0].target_boost.button);
	bNoiseRejection->SetValueX(pPlugIn->radar_setting[0].noise_rejection.button);
	bTargetSeparation->SetValueX(pPlugIn->radar_setting[0].target_separation.button);
}

void BR24ControlsDialog::UpdateMessage(bool haveOpenGL, bool haveGPS, bool haveHeading, bool haveVariation, bool haveRadar, bool haveData)
{
    cbOpenGL->SetValue(haveOpenGL);
    cbBoatPos->SetValue(haveGPS);
    cbHeading->SetValue(haveHeading);
    cbVariation->SetValue(haveVariation);
    cbRadar->SetValue(haveRadar);
    cbData->SetValue(haveData);

	bool radarOn = haveOpenGL && haveRadar && haveData;
	bool navOn = haveGPS && haveHeading && haveVariation;
	bool black = pPlugIn->settings.display_mode == DM_CHART_BLACKOUT;
	/*
	Decision table to select the message or control box
	- means not relevant

    case nr        1   2   3   4   5   6   7   8   9   10  11
	box type       m1  m2  m3  m3  m1  c   c   c   mb  mb  mb
	_________________________________________________________
	radarOn        0   0   0   0   1   1   1   1   1   1   1   
	navOn          0   1   0   1   0   1   0   1   1   0   1
	black          0   0   1   1   0   0   1   1   0   1   1
	want_message   -   -   -   -   -   0   0   0   1   1   1

	m1    message box plus radar only button 
	m2    message box
	m3    message box without NMEA (no buttons)
	c     control box
	mb    message box with back button

	*/
	if (! black && !navOn)               // case 1 and 5
	{                                    // m1    message box plus radar only button 
		if (!topSizer->IsShown(messageBox)) {
		topSizer->Show(messageBox);
		}
		messageBox->Show(bRdrOnly);
		messageBox->Hide(bMsgBack);
		messageBox->Show(nmeaBox);
	//	messageBox->Show(cbHeading);
	//	messageBox->Show(cbBoatPos);
	//	messageBox->Show(cbVariation);
		topSizer->Hide(controlBox);
        topSizer->Hide(advancedBox);
        topSizer->Hide(editBox);
        messageBox->Layout();
        Fit();
        topSizer->Layout();
	}
	else if (!radarOn && ! black && navOn)   // case 2
	{                                        // m2    message box
		if (!topSizer->IsShown(messageBox)) {
		topSizer->Show(messageBox);
		}
		messageBox->Hide(bRdrOnly);
		messageBox->Hide(bMsgBack);
		messageBox->Show(nmeaBox);
	//	messageBox->Show(cbHeading);
	//	messageBox->Show(cbBoatPos);
	//	messageBox->Show(cbVariation);
		topSizer->Hide(controlBox);
        topSizer->Hide(advancedBox);
        topSizer->Hide(editBox);
        messageBox->Layout();
        Fit();
        topSizer->Layout();
	}
	else if (!radarOn && black)           // case 3 and 4
	{                                     // m3    message box without NMEA (no buttons)
		if (!topSizer->IsShown(messageBox)) {
		topSizer->Show(messageBox);
		}
		messageBox->Hide(nmeaSizer);
	//	messageBox->Hide(cbHeading);
	//	messageBox->Hide(cbBoatPos);
		messageBox->Hide(bRdrOnly);
		messageBox->Hide(bMsgBack);
		topSizer->Hide(controlBox);
        topSizer->Hide(advancedBox);
        topSizer->Hide(editBox);
        messageBox->Layout();
        Fit();
        topSizer->Layout();
	}
	else if ((navOn || black) && !wantShowMessage)     //  case 6, 7 and 8
	{                                                  //  c     control box
		if (topSizer->IsShown(messageBox))    
        {
		topSizer->Hide(messageBox);
        topSizer->Show(controlBox);
        Fit();
        topSizer->Layout();
		}
	}
	else if (wantShowMessage)          // case 9, 10 and 11
	{                                  // mb    message box with back button
		if (!topSizer->IsShown(messageBox)) {
		topSizer->Show(messageBox);
		}
		messageBox->Hide(bRdrOnly);
		messageBox->Show(bMsgBack);
		messageBox->Show(nmeaBox);
	//	messageBox->Show(cbHeading);
	//	messageBox->Show(cbBoatPos);
	//	messageBox->Show(cbVariation);
		topSizer->Hide(controlBox);
        topSizer->Hide(advancedBox);
        topSizer->Hide(editBox);
        messageBox->Layout();
        Fit();
        topSizer->Layout();
	}
	else 
	{
		 wxLogMessage(wxT("BR24radar_pi: XX Update message, case not covered"));
	}

	editBox->Layout();
    topSizer->Layout();
}
	

void BR24ControlsDialog::SetErrorMessage(wxString &msg)
{
    tMessage->SetLabel(msg);
    topSizer->Show(messageBox);
//	messageBox->Show(bRdrOnly);
    topSizer->Hide(controlBox);
    topSizer->Hide(advancedBox);
    topSizer->Hide(editBox);
    messageBox->Layout();
    Fit();
    topSizer->Layout();
}

void BR24ControlsDialog::SetRadarIPAddress(wxString &msg)
{
    if (cbRadar) {
        wxString label;

        label << _("Radar IP") << wxT(" ") << msg;
        cbRadar->SetLabel(label);
    }
}

void BR24ControlsDialog::SetMcastIPAddress(wxString &msg)
{
    if (ipBox) {
        wxString label;

        label << _("ZeroConf Ethernet") << wxT(" ") << msg;
        ipBox->SetLabel(label);
    }
}

void BR24ControlsDialog::SetHeadingInfo(wxString &msg)
{
    if (cbHeading && topSizer->IsShown(messageBox)) {
        wxString label;

        label << _("Heading") << wxT(" ") << msg;
        cbHeading->SetLabel(label);
    }
}

void BR24ControlsDialog::SetVariationInfo(wxString &msg)
{
    if (cbVariation && topSizer->IsShown(messageBox)) {
        wxString label;

        label << _("Variation") << wxT(" ") << msg;
        cbVariation->SetLabel(label);
    }
}

void BR24ControlsDialog::SetRadarInfo(wxString &msg)
{
    if (tStatistics && topSizer->IsShown(messageBox)) {
        tStatistics->SetLabel(msg);
    }
}
