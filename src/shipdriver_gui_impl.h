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

#include "AisMaker.h"
#include "shipdriver_pi.h"
#include "shipdriver_gui.h"
#include "ocpn_plugin.h"
#include "tinyxml.h"
#include "wx/process.h"
#include "json/reader.h"
#include "json/writer.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <wx/filedlg.h>
#include <wx/gdicmn.h>
#include <wx/listctrl.h>
#include <wx/stdpaths.h>
#include <wx/thread.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include <wx/arrstr.h>
#include <wx/vector.h>
#include <wx/dataobj.h>
#include <wx/list.h>
#include <wx/window.h>
#include <wx/menu.h>

#define ID_SOMETHING 2001
#define ID_SOMETHING_ELSE 2002

#ifdef __WXOSX__
#define SHIPDRIVER_DLG_STYLE \
  wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP
#else
#define SHIPDRIVER_DLG_STYLE \
  wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
#endif
using namespace std;

class ShipDriverPi;

class rtept {
public:
  wxString Name, m_GUID;
  int index;
  wxString lat, lon;
};

class rte {
public:
  wxString Name;

  vector<rtept> m_rteptList;
};

class MyThread : public wxThread {
public:
  MyThread();
  virtual ~MyThread();

  // thread execution starts here
  virtual void* Entry();

public:
  unsigned m_count;
};

// An identifier to notify the application when the // work is done #define
// ID_COUNTED_COLORS    100

class AisMaker;

class Dlg : public ShipDriverBase {
public:
  Dlg(wxWindow* parent, wxWindowID id = wxID_ANY,
      const wxString& title = _("ShipDriver"),
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize, long style = SHIPDRIVER_DLG_STYLE);
  ShipDriverPi* plugin;

#ifdef __ANDROID__
  void OnMouseEvent(wxMouseEvent& event);
  wxPoint m_resizeStartPoint;
  wxSize m_resizeStartSize;
  bool m_binResize;
  bool m_binResize2;

  void OnPopupClick(wxCommandEvent& evt);
  void OnDLeftClick(wxMouseEvent& event);

#endif

  wxString createVHWSentence(double stw, double hdg);
  wxString createMWVTSentence(double spd, double hdg, double winddirection,
                              double windspeed);
  wxString createMWVASentence(double spd, double hdg, double winddirection,
                              double windspeed);
  wxString createMWDSentence(double winddirection, double windspeed);
  wxString createRMCSentence(wxDateTime myTime, double myLat, double myLon,
                             double mySpd, double myDir);
  wxString createGLLSentence(wxDateTime myTime, double myLat, double myLon,
                             double mySpd, double myDir);
  wxString createVTGSentence(double mySpd, double myDir);
  wxString createHDTSentence(double myDir);

  // DSC Sentences
  wxString createDSCAlertSentence(double lat, double lon, int mmsi,
                                  wxString nature, wxString time);
  wxString createDSCExpansionSentence(double lat, double lon, int mmsi);
  wxString createDSCAlertCancelSentence(double lat, double lon, int mmsi,
                                        wxString nature, wxString time);
  wxString createDSCAlertRelaySentence(double lat, double lon, int mmsi,
                                       int dmmsi, wxString nature,
                                       wxString time);
  wxString createDSCAlertRelayCancelSentence(double lat, double lon, int mmsi,
                                             int dmmsi, wxString nature,
                                             wxString time);

  wxString LatitudeToString(double mLat);
  wxString LongitudeToString(double mLon);
  wxString DateTimeToTimeString(wxDateTime myDT);
  wxString DateTimeToDateString(wxDateTime myDT);
  void OnContextMenu(double m_lat, double m_lon);

  wxString makeCheckSum(wxString mySentence);

  wxTimer* m_timer;
  void OnTimer(wxTimerEvent& event);

  double GetLatitude() { return initLat; };
  double GetLongitude() { return initLon; };

  double initLat;
  double initLon;
  double nextLat;
  double nextLon;

  double stepLat;
  double stepLon;
  AisMaker* myAIS;
  wxTextFile* nmeafile;
  wxTextFile* nmeastream;

  bool m_bUseAis;
  bool m_bUseNMEA;
  bool m_bUseFile;
  wxString m_tMMSI;

  bool m_bAuto;

  wxDateTime m_GribTimelineTime;

  double myDir;
  void SetNMEAMessage(wxString sentence);
  bool m_bGotAPB;

protected:
  bool m_bNeedsGrib;

private:
  void Notify();
  wxString MWD, VHW, MWVA, MWVT, GLL, VTG, HDT, RMC;
  double initDir, initSpd, initRudder, myDist, followStepDistance;

  vector<rte> my_routes;
  vector<rtept> routePoints;
  unique_ptr<PlugIn_Route_Ex> thisRoute;
  vector<PlugIn_Waypoint_Ex*> theWaypoints;
  int nextRoutePointIndex;
  double nextRoutePoint;
  double followDir;
  int countRoutePoints;

  wxDateTime dt;
  void SetInterval(int interval);
  int m_interval;

  wxString m_sTimeSentence;
  wxString m_sTimeID;
  wxString m_sNmeaTime;

  bool dbg;

  bool m_bUseSetTime;
  bool m_bUseStop;
  bool m_bUsePause;

  bool SART_active;

  wxString ParseNMEAIdentifier(wxString sentence);
  wxString ParseNMEASentence(wxString sentence, wxString id);

  void SetNextStep(double inLat, double inLon, double inDir, double inSpd,
                   double& outLat, double& outLon);
  void SetFollowStep(double inLat, double inLon, double inDir, double inSpd,
                     double& outLat, double& outLon);

  void OnStart(wxCommandEvent& event);
  void OnStop(wxCommandEvent& event);
  void OnClose(wxCloseEvent& event);

  void SetStop();
  void StartDriving();

  void OnFollow(wxCommandEvent& event);
  int mainTest(int argc, char* argv[]);
  // void SendAIS(double cse, double spd, double lat, double lon);

  void OnMidships(wxCommandEvent& event);
  void OnMinus10(wxCommandEvent& event);
  void OnPlus10(wxCommandEvent& event);
  void OnMinus1(wxCommandEvent& event);
  void OnPlus1(wxCommandEvent& event);

  void OnStandby(wxCommandEvent& event);
  void GoToStandby();

  void OnAuto(wxCommandEvent& event);

  // Distress alarms
  int alarm_id;

  bool m_bSART;
  bool m_bMOB;
  bool m_bEPIRB;
  bool m_bDISTRESS;
  bool m_bCANCEL;
  bool m_bDISTRESSRELAY;
  bool m_bRELAYCANCEL;
  bool m_bCOLLISION;

  wxString SARTid;
  wxString MOBid;
  wxString EPIRBid;
  wxString ALERTid;
  wxString CANCELid;

  int SARTint;
  int MOBint;
  int EPIRBint;
  int DISTRESSint;
  int CANCELint;

  wxString myNMEA_SART;
  wxString myNMEA_MOB;
  wxString myNMEA_EPIRB;
  wxString myNMEA_DISTRESS;
  wxString myNMEA_CANCEL;
  wxString myNMEA_DISTRESSRELAY;
  wxString myNMEA_RELAYCANCEL;
  wxString myNMEA_Collision;

  int stop_count;
  int stop_countMOB;
  int stop_countEPIRB;
  int stop_countDISTRESS;
  int stop_countCANCEL;
  int stop_countDISTRESSRELAY;
  int stop_countRELAYCANCEL;
  int stop_countCOLLISION;

  double m_latSART;
  double m_lonSART;
  double m_latMOB;
  double m_lonMOB;
  double m_latEPIRB;
  double m_lonEPIRB;
  double m_latCollision;
  double m_lonCollision;
  double m_collisionDir;

  void OnSART(wxCommandEvent& event);
  void OnMOB(wxCommandEvent& event);
  void OnEPIRB(wxCommandEvent& event);
  void OnDistressAlert(wxCommandEvent& event);
  void OnDistressCancel(wxCommandEvent& event);
  void OnDistressRelay(wxCommandEvent& event);
  void OnRelayCancel(wxCommandEvent& event);
  void OnCollision(wxCommandEvent& event);
  void OnPause(wxCommandEvent& event);
  void ResetPauseButton();

  long m_iMMSI;

  virtual void Lock() { routemutex.Lock(); }
  virtual void Unlock() { routemutex.Unlock(); }
  wxMutex routemutex;

  void RequestGrib(wxDateTime time);
  bool GetGribSpdDir(wxDateTime dt, double lat, double lon, double& spd,
                     double& dir);
  void OnWind(wxCommandEvent& event);
  double GetPolarSpeed(double lat, double lon, double cse);

  double AttributeDouble(TiXmlElement* e, const char* name, double def);
  double ReadPolars(wxString filename, double windangle, double windspeed);

  double ReadNavobj();
  static wxString StandardPath();

  bool m_bUsingWind;
  bool m_bUsingFollow;
  bool m_bInvalidPolarsFile;
  bool m_bInvalidGribFile;
  bool m_bShipDriverHasStarted;

  Plugin_WaypointExList* myList;
};

class GetRouteDialog : public wxDialog {
public:
  GetRouteDialog(wxWindow* parent, wxWindowID id, const wxString& title,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_DIALOG_STYLE);

  wxListView* dialogText;

private:
};

#endif
