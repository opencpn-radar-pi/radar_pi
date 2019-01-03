/******************************************************************************
*
* Project:  OpenCPN
* Purpose:  ShipDriver Plugin
* Author:   Mike Rossiter
*
***************************************************************************
*   Copyright (C) 2017 by Mike Rossiter                                   *
*   $EMAIL$                                                               *
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

#ifndef _HRGUI_IMPL_H_
#define _HRGUI_IMPL_H_

#ifdef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "ShipDrivergui.h"
#include "ShipDriver_pi.h"
#include "AisMaker.h"
#include <wx/utils.h>
#include <wx/gdicmn.h>
#include <sstream>
#include <cmath>
#include <wx/stdpaths.h>
#include "wx/process.h"
#include "wx/jsonreader.h"
#include "wx/jsonwriter.h"
#include <wx/thread.h>
#include "tinyxml.h"
#include "tinystr.h"
#include <wx/filedlg.h>
#include "ocpn_plugin.h"

#ifdef __WXOSX__
#define SHIPDRIVER_DLG_STYLE wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSTAY_ON_TOP
#else
#define SHIPDRIVER_DLG_STYLE wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER
#endif
using namespace std;

class ShipDriver_pi;

// ----------------------------------------------------------------------------
// a simple thread
// ----------------------------------------------------------------------------



class MyThread : public wxThread
{
public:
	MyThread();
	virtual ~MyThread();

	// thread execution starts here
	virtual void *Entry();

public:
	unsigned m_count;
};

// An identifier to notify the application when the // work is done #define ID_COUNTED_COLORS    100

class AisMaker;

class Dlg : public ShipDriverBase
{
public:
	Dlg(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("ShipDriver"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SHIPDRIVER_DLG_STYLE);
	ShipDriver_pi *plugin;

	wxString createVHWSentence(double stw, double hdg);
	wxString createMWVTSentence(double spd, double hdg, double winddirection, double windspeed);
	wxString createMWVASentence(double spd, double hdg, double winddirection, double windspeed);
	wxString createMWDSentence(double winddirection, double windspeed);
	wxString createRMCSentence(wxDateTime myTime, double myLat, double myLon, double mySpd, double myDir);
	wxString createGLLSentence(wxDateTime myTime, double myLat, double myLon, double mySpd, double myDir);
	wxString createVTGSentence(double mySpd, double myDir);
	wxString createHDGSentence(double myDir);

	wxString LatitudeToString(double mLat);
	wxString LongitudeToString(double mLon);
	wxString DateTimeToTimeString(wxDateTime myDT);
	wxString DateTimeToDateString(wxDateTime myDT);
	void OnContextMenu(double m_lat, double m_lon);

	wxString makeCheckSum(wxString mySentence);

	wxTimer *   m_Timer;
	void OnTimer(wxTimerEvent& event);

	double GetLatitude()  { return initLat; };
	double GetLongitude() { return initLon; };

	double initLat;
	double initLon;
	double stepLat;
	double stepLon;
	AisMaker* myAIS;
	wxTextFile* nmeafile;

	bool m_bUseAis;
	bool m_bUseFile;
	wxString m_tMMSI;

	wxDateTime m_GribTimelineTime;

protected:
	bool m_bNeedsGrib;

private:
	void Notify();
	wxString MWD, VHW, MWVA, MWVT, GLL, VTG, HDG;
	double initDir, initSpd, initRudder, myDir;

	wxDateTime dt;
	void SetInterval(int interval);
	int  m_interval;

	wxString m_sTimeSentence;
	wxString m_sTimeID;
	wxString m_sNmeaTime;

	bool dbg;

	bool m_bUseSetTime;
	bool m_bUseStop;
	bool m_bUsePause;

	wxString ParseNMEAIdentifier(wxString sentence);
	wxString ParseNMEASentence(wxString sentence, wxString id);

	void SetNextStep(double inLat, double inLon, double inDir, double inSpd, double &outLat, double &outLon);

	void OnStart(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	void OnTest(wxCommandEvent& event);
	int mainTest(int argc, char *argv[]);
	//void SendAIS(double cse, double spd, double lat, double lon);

	void OnMidships(wxCommandEvent& event);
	void OnMinus10(wxCommandEvent& event);
	void OnPlus10(wxCommandEvent& event);
	void OnMinus1(wxCommandEvent& event);
	void OnPlus1(wxCommandEvent& event);
	void OnStandby(wxCommandEvent& event);
	void OnAuto(wxCommandEvent& event);

	bool m_bAuto;
	long m_iMMSI;

	virtual void Lock() { routemutex.Lock(); }
	virtual void Unlock() { routemutex.Unlock(); }
	wxMutex routemutex;


	void RequestGrib(wxDateTime time);
	bool GetGribSpdDir(wxDateTime dt, double lat, double lon, double &spd, double &dir);
	void OnWind(wxCommandEvent& event);
	double GetPolarSpeed(double lat, double lon, double cse);

	double AttributeDouble(TiXmlElement *e, const char *name, double def);
	double ReadPolars(wxString filename, double windangle, double windspeed);

	bool m_bUsingWind;
	bool m_bInvalidPolarsFile;
	bool m_bInvalidGribFile;
	bool m_bShipDriverHasStarted;

};

#endif