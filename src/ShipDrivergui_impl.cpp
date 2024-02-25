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

#include <stdio.h>

#include <wx/progdlg.h>
#include <wx/textfile.h>
#include <wx/timer.h>
#include <wx/wx.h>
#include "wx/tglbtn.h"

#include "qtstylesheet.h"
#include "ShipDrivergui_impl.h"
#include "ShipDriver_pi.h"

#ifdef __ANDROID__
wxWindow* g_Window;
#endif

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(Plugin_WaypointExList);

class GribRecordSet;

void assign(char* dest, char* arrTest2) { strcpy(dest, arrTest2); }

#define BUFSIZE 0x10000

Dlg::Dlg(wxWindow* parent, wxWindowID id, const wxString& title,
         const wxPoint& pos, const wxSize& size, long style)
    : ShipDriverBase(parent, id, title, pos, size, style) {
  this->Fit();
  dbg = false;  // for debug output set to true
  initLat = 0;
  initLon = 0;
  m_interval = 500;
  m_bUseSetTime = false;
  m_bUseStop = true;
  m_bUsePause = false;
  m_sNmeaTime = wxEmptyString;

  m_bUsingWind = false;
  m_bUsingFollow = false;
  m_bInvalidPolarsFile = false;
  m_bInvalidGribFile = false;
  m_bShipDriverHasStarted = false;

  m_bSART = false;
  m_bDISTRESS = false;
  m_bCANCEL = false;
  m_bDISTRESSRELAY = false;
  m_bRELAYCANCEL = false;

  stop_count = 999;
  stop_countMOB = 999;
  stop_countEPIRB = 999;
  stop_countDISTRESS = 999;
  stop_countCANCEL = 999;

  alarm_id = 999;
  m_bGotAPB = false;

#ifdef __ANDROID__

  m_binResize = false;

  g_Window = this;
  GetHandle()->setStyleSheet(qtStyleSheet);
  Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Dlg::OnMouseEvent));
  Connect(wxEVT_LEFT_UP, wxMouseEventHandler(Dlg::OnMouseEvent));

  Connect(wxEVT_MOTION, wxMouseEventHandler(Dlg::OnMouseEvent));

#endif

  wxFileConfig* pConf = GetOCPNConfigObject();

  if (pConf) {
    pConf->SetPath("/PlugIns/ShipDriver_pi");

    pConf->Read("shipdriverUseAis", &m_bUseAis, 0);
    pConf->Read("shipdriverUseFile", &m_bUseFile, 0);
    pConf->Read("shipdriverMMSI", &m_tMMSI, "123456789");
  }
}

#ifdef __ANDROID__
wxPoint g_startPos;
wxPoint g_startMouse;
wxPoint g_mouse_pos_screen;

void Dlg::OnPopupClick(wxCommandEvent& evt) {
  switch (evt.GetId()) {
    case ID_SOMETHING:
      m_binResize = true;
      break;
      // case ID_SOMETHING_ELSE:
      //   break;
  }
}

void Dlg::OnDLeftClick(wxMouseEvent& event) {
  wxMenu mnu;
  mnu.Append(ID_SOMETHING, "Resize...");
  // mnu.Append(ID_SOMETHING_ELSE, "Do something else");
  mnu.Connect(wxEVT_COMMAND_MENU_SELECTED,
              wxCommandEventHandler(Dlg::OnPopupClick), NULL, this);
  PopupMenu(&mnu);
}

void Dlg::OnMouseEvent(wxMouseEvent& event) {
  if (m_binResize) {
    wxSize currentSize = g_Window->GetSize();
    wxSize par_size = GetOCPNCanvasWindow()->GetClientSize();
    wxPoint par_pos = g_Window->GetPosition();
    if (event.LeftDown()) {
      m_resizeStartPoint = event.GetPosition();
      m_resizeStartSize = currentSize;
      m_binResize2 = true;
    }

    if (m_binResize2) {
      if (event.Dragging()) {
        wxPoint p = event.GetPosition();

        wxSize dragSize = m_resizeStartSize;

        dragSize.y = p.y;  //  - m_resizeStartPoint.y;
        dragSize.x = p.x;  //  - m_resizeStartPoint.x;
        ;
        /*
        if ((par_pos.y + dragSize.y) > par_size.y)
            dragSize.y = par_size.y - par_pos.y;

        if ((par_pos.x + dragSize.x) > par_size.x)
            dragSize.x = par_size.x - par_pos.x;
*/
        // not too small
        dragSize.x = wxMax(dragSize.x, 150);
        dragSize.y = wxMax(dragSize.y, 150);

        int x = wxMax(0, m_resizeStartPoint.x);
        int y = wxMax(0, m_resizeStartPoint.y);
        int xmax = ::wxGetDisplaySize().x - GetSize().x;
        x = wxMin(x, xmax);
        int ymax =
            ::wxGetDisplaySize().y - (GetSize().y);  // Some fluff at the bottom
        y = wxMin(y, ymax);

        g_Window->Move(x, y);
      }
      if (event.LeftUp()) {
        wxPoint p = event.GetPosition();

        wxSize dragSize = m_resizeStartSize;

        dragSize.y = p.y;
        dragSize.x = p.x;

        // not too small
        dragSize.x = wxMax(dragSize.x, 150);
        dragSize.y = wxMax(dragSize.y, 150);

        g_Window->SetSize(dragSize);

        m_binResize = false;
        m_binResize2 = false;
      }
    }
  } else {
    if (event.Dragging()) {
      m_resizeStartPoint = event.GetPosition();
      int x = wxMax(0, m_resizeStartPoint.x);
      int y = wxMax(0, m_resizeStartPoint.y);
      int xmax = ::wxGetDisplaySize().x - GetSize().x;
      x = wxMin(x, xmax);
      int ymax =
          ::wxGetDisplaySize().y - (GetSize().y);  // Some fluff at the bottom
      y = wxMin(y, ymax);

      g_Window->Move(x, y);
    }
  }
}

#endif  // End of Android functions for move/resize

void Dlg::OnTimer(wxTimerEvent& event) { Notify(); }

void Dlg::SetNextStep(double inLat, double inLon, double inDir, double inSpd,
                      double& outLat, double& outLon) {
  PositionBearingDistanceMercator_Plugin(inLat, inLon, inDir, inSpd, &stepLat,
                                         &stepLon);
}

void Dlg::SetFollowStep(double inLat, double inLon, double inDir, double inSpd,
                        double& outLat, double& outLon) {
  double myBrg;
  PlugIn_Waypoint_Ex* myWaypoint;

  PositionBearingDistanceMercator_Plugin(inLat, inLon, inDir, inSpd, &stepLat,
                                         &stepLon);
  DistanceBearingMercator_Plugin(nextLat, nextLon, stepLat, stepLon, &myBrg,
                                 &myDist);

  if (myDist <= initSpd / 7200) {
    stepLat = nextLat;
    stepLon = nextLon;

    nextRoutePointIndex++;

    if (nextRoutePointIndex > (countRoutePoints - 1)) {
      SetStop();
      return;
    }

    for (size_t n = 0; n < theWaypoints.size(); n++) {
      if (static_cast<int>(n) == nextRoutePointIndex) {
        myWaypoint = theWaypoints[n];
        nextLat = myWaypoint->m_lat;
        nextLon = myWaypoint->m_lon;
      }
    }

    DistanceBearingMercator_Plugin(nextLat, nextLon, stepLat, stepLon,
                                   &followDir, &myDist);

    PositionBearingDistanceMercator_Plugin(stepLat, stepLon, followDir, inSpd,
                                           &stepLat, &stepLon);

    myDir = followDir;
  }
}

void Dlg::OnStart(wxCommandEvent& event) { StartDriving(); }

void Dlg::StartDriving() {
  if (initLat == 0.0) {
    wxMessageBox(_("Please right-click and choose vessel start position"));
    return;
  }

  bool bIsDigits = m_tMMSI.IsNumber();

  if (m_tMMSI.Len() != 9 || !bIsDigits) {
    wxMessageBox(_("MMSI must be nine digits\nEdit using Preferences"));
    return;
  }

  m_bShipDriverHasStarted = true;
  m_bUsingWind = false;

  if (m_bUseFile) {
    wxString caption = wxT("Choose a file");
    wxString wildcard = wxT("Text files (*.txt)|*.txt|All files (*.*)|*.*");

    wxString s = "/";
    const char* pName = "ShipDriver_pi";
    wxString defaultDir = GetPluginDataDir(pName) + s + "data" + s;

    wxString defaultFilename = wxEmptyString;
    wxFileDialog filedlg(this->m_parent, caption, defaultDir, defaultFilename,
                         wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (filedlg.ShowModal() != wxID_OK) {
      wxMessageBox(_("ShipDriver has been stopped"));
      return;
    } else {
      nmeafile = new wxTextFile(filedlg.GetPath());
      nmeafile->Open();
      nmeafile->Clear();
    }
  }

  m_textCtrlRudderStbd->SetValue("");
  m_textCtrlRudderPort->SetValue("");
  initSpd = 0;  // 5 knots

  if (!m_bUsingFollow) {
    wxString myHeading = m_stHeading->GetLabel();
    myHeading.ToDouble(&initDir);
    myDir = initDir;

  } else {
    myDir = followDir;
  }

  dt = dt.Now();
  GLL = createGLLSentence(dt, initLat, initLon, initSpd, initDir);
  VTG = createVTGSentence(initSpd, initDir);

  m_interval = 500;
  m_Timer->Start(m_interval, wxTIMER_CONTINUOUS);  // start timer
  m_bAuto = false;

  myAIS = new AisMaker();
}

void Dlg::OnStop(wxCommandEvent& event) { SetStop(); }

void Dlg::SetStop() {
  if (m_Timer->IsRunning()) m_Timer->Stop();

  wxMessageBox(_("Vessel stopped"));

  m_SliderSpeed->SetValue(0);
  m_SliderRudder->SetValue(30);
  m_textCtrlRudderStbd->SetValue("");
  m_textCtrlRudderPort->SetValue("");

  m_interval = m_Timer->GetInterval();
  m_bUseSetTime = false;
  m_bUseStop = true;
  m_bAuto = false;
  m_bUsingWind = false;
  m_bUsingFollow = false;
  m_bShipDriverHasStarted = false;
  m_buttonStandby->SetBackgroundColour(wxColour(0, 255, 0));
  m_buttonAuto->SetBackgroundColour(wxColour(255, 255, 255));
  m_buttonWind->SetBackgroundColour(wxColour(0, 255, 0));

  if (m_bUseFile) {
    nmeafile->Write();
    nmeafile->Close();
  }
  initSpd = 0.0;
  m_stSpeed->SetLabel(wxString::Format("%3.1f", initSpd));

  Refresh();
}

void Dlg::OnMidships(wxCommandEvent& event) { m_SliderRudder->SetValue(30); }

void Dlg::OnMinus10(wxCommandEvent& event) {
  m_bAuto = false;
  GoToStandby();
  myDir -= 10;
  wxString mystring = wxString::Format("%03.0f", myDir);
  m_stHeading->SetLabel(mystring);
}

void Dlg::OnPlus10(wxCommandEvent& event) {
  m_bAuto = false;
  GoToStandby();
  myDir += 10;
  wxString mystring = wxString::Format("%03.0f", myDir);
  m_stHeading->SetLabel(mystring);
}

void Dlg::OnMinus1(wxCommandEvent& event) {
  m_bAuto = false;
  GoToStandby();
  myDir -= 1;
  wxString mystring = wxString::Format("%03.0f", myDir);
  m_stHeading->SetLabel(mystring);
}

void Dlg::OnPlus1(wxCommandEvent& event) {
  m_bAuto = false;
  GoToStandby();
  myDir += 1;
  wxString mystring = wxString::Format("%03.0f", myDir);
  m_stHeading->SetLabel(mystring);
}

void Dlg::OnAuto(wxCommandEvent& event) {
  if (m_bShipDriverHasStarted) {
    if (m_bGotAPB) {
      m_bAuto = true;
      m_buttonStandby->SetBackgroundColour(wxColour(255, 0, 0));
      m_buttonAuto->SetBackgroundColour(wxColour(0, 255, 0));
      Refresh();
    } else {
      m_bAuto = false;
      m_buttonStandby->SetBackgroundColour(wxColour(0, 255, 0));
      m_buttonAuto->SetBackgroundColour(wxColour(255, 255, 255));

      wxMessageBox(
          _("Route or waypoint must be activated\nHas a "
            "connection been made in Options->Connections?"));
      return;
    }
  }
}

void Dlg::SetNMEAMessage(wxString sentence) {
  // $GPAPB,A,A,0.10,R,N,V,V,011,M,DEST,011,M,011,M*3C
  wxString token[40];
  wxString s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
  token[0] = "";

  wxStringTokenizer tokenizer(sentence, ",");
  int i = 0;
  while (tokenizer.HasMoreTokens()) {
    token[i] = tokenizer.GetNextToken();
    i++;
  }
  if (token[0].Right(3) == "APB") {
    s11 = token[11];

    m_bGotAPB = true;
    if (m_bAuto) {
      double value;
      s11.ToDouble(&value);
      myDir = value;
    }
  }
}

void Dlg::OnStandby(wxCommandEvent& event) { GoToStandby(); }

void Dlg::GoToStandby() {
  m_bAuto = false;

  m_buttonStandby->SetBackgroundColour(wxColour(0, 255, 0));
  m_buttonAuto->SetBackgroundColour(wxColour(255, 255, 255));
  Refresh();
}

// Now for the distress alarms

void Dlg::OnSART(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonSART->GetValue();
    alarm_id = 970;
    if (active) {
      PositionBearingDistanceMercator_Plugin(initLat, initLon, 200.0, 10.0,
                                             &m_latSART, &m_lonSART);
      m_bSART = true;
      m_buttonSART->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_count = 0;
      m_bSART = false;
      m_buttonSART->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnMOB(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonMOB->GetValue();
    alarm_id = 972;
    if (active) {
      m_latMOB = initLat;
      m_lonMOB = initLon;
      m_bMOB = true;
      m_buttonMOB->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_countMOB = 0;
      m_bMOB = false;
      m_buttonMOB->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnEPIRB(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonEPIRB->GetValue();
    alarm_id = 974;
    if (active) {
      PositionBearingDistanceMercator_Plugin(initLat, initLon, 300.0, 10.0,
                                             &m_latEPIRB, &m_lonEPIRB);
      m_bEPIRB = true;
      m_buttonEPIRB->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_countEPIRB = 0;
      m_bEPIRB = false;
      m_buttonEPIRB->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnDistressAlert(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonDistressAlert->GetValue();
    alarm_id = 980;
    if (active) {
      m_bDISTRESS = true;
      m_buttonDistressAlert->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_countDISTRESS = 0;
      m_bDISTRESS = false;
      m_buttonDistressAlert->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnDistressCancel(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonDistressCancel->GetValue();
    alarm_id = 982;
    if (active) {
      if (!m_bDISTRESS) {
        wxMessageBox(_("No DSC Distress activated"));
        return;
      }
      m_bCANCEL = true;
      m_buttonDistressCancel->SetBackgroundColour(wxColour(255, 0, 0));
      if (m_bDISTRESS) {
        m_bDISTRESS = false;
        m_buttonDistressAlert->SetBackgroundColour(wxColour(0, 255, 0));
      }

    } else {
      m_bCANCEL = false;
      m_buttonDistressCancel->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnDistressRelay(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonDistressRelay->GetValue();
    alarm_id = 984;
    if (active) {
      m_bDISTRESSRELAY = true;
      m_buttonDistressRelay->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_countDISTRESSRELAY = 0;
      m_bDISTRESSRELAY = false;
      m_buttonDistressRelay->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnRelayCancel(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonRelayCancel->GetValue();
    alarm_id = 986;
    if (active) {
      if (!m_bDISTRESSRELAY) {
        wxMessageBox(_("No DSC Distress activated"));
        return;
      }
      m_bRELAYCANCEL = true;
      m_buttonRelayCancel->SetBackgroundColour(wxColour(255, 0, 0));
      if (m_bDISTRESSRELAY) {
        m_bDISTRESSRELAY = false;
        m_buttonDistressRelay->SetBackgroundColour(wxColour(0, 255, 0));
      }

    } else {
      m_bRELAYCANCEL = false;
      m_buttonRelayCancel->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnCollision(wxCommandEvent& event) {
  if (m_Timer->IsRunning()) {
    bool active = m_buttonCollision->GetValue();
    alarm_id = 990;
    if (active) {
      PositionBearingDistanceMercator_Plugin(initLat, initLon, 20.0, 6.0,
                                             &m_latCollision, &m_lonCollision);
      DistanceBearingMercator_Plugin(initLat, initLon, m_latCollision,
                                     m_lonCollision, &m_collisionDir, &myDist);

      m_bCOLLISION = true;
      m_buttonCollision->SetBackgroundColour(wxColour(255, 0, 0));
    } else {
      stop_countCOLLISION = 0;
      m_bCOLLISION = false;
      m_buttonCollision->SetBackgroundColour(wxColour(0, 255, 0));
    }
  } else
    wxMessageBox(_("ShipDriver has not been started"));
}

void Dlg::OnPause(wxCommandEvent& event) {
  bool active = m_buttonPause->GetValue();
  if (active) {
    m_Timer->Stop();
    m_buttonPause->SetLabel("Resume");
    m_buttonPause->SetBackgroundColour(wxColour(255, 0, 0));
  } else {
    StartDriving();
    m_buttonPause->SetLabel("Pause");
    m_buttonPause->SetBackgroundColour(wxColour(0, 255, 0));
  }
}

void Dlg::ResetPauseButton() {
  m_buttonPause->SetValue(0);
  m_buttonPause->SetLabel("Pause");
  m_buttonPause->SetBackgroundColour(wxColour(0, 255, 0));
}

void Dlg::OnClose(wxCloseEvent& event) {
  if (m_Timer->IsRunning()) m_Timer->Stop();
  plugin->OnShipDriverDialogClose();
}
void Dlg::Notify() {
  wxString mySentence;
  plugin->SetNMEASentence(mySentence);
  initSpd = m_SliderSpeed->GetValue();
  initRudder = m_SliderRudder->GetValue();

  double myRudder = initRudder - 30;
  if (myRudder < 0) {
    initRudder -= 30.0;
    myRudder = std::abs(initRudder);
    myDir -= myRudder;
    double myPortRudder = 30 - std::abs(myRudder);
    m_gaugeRudderPort->SetValue(myPortRudder);
    m_textCtrlRudderPort->SetValue(wxString::Format("%.0f", myRudder) + " P");
    m_gaugeRudderStbd->SetValue(0);
    m_textCtrlRudderStbd->SetValue("");
  } else if (myRudder >= 0) {
    initRudder -= 30;
    myDir += initRudder;
    m_gaugeRudderStbd->SetValue(myRudder);
    if (myRudder == 0) {
      m_textCtrlRudderStbd->SetValue("");
    } else {
      m_textCtrlRudderStbd->SetValue(wxString::Format("%.0f", myRudder) + " S");
    }
    m_gaugeRudderPort->SetValue(0);
    m_textCtrlRudderPort->SetValue("");
  }

  if (myDir < 0) {
    myDir += 360;
  } else if (myDir > 360) {
    myDir -= 360;
  }

  wxString mystring = wxString::Format("%03.0f", myDir);
  m_stHeading->SetLabel(mystring);

  if (m_bUsingWind) {
    double polarBoatSpeed = GetPolarSpeed(initLat, initLon, myDir);
    if (polarBoatSpeed != -1) {
      initSpd = polarBoatSpeed;
    }
  }

  m_stSpeed->SetLabel(wxString::Format("%3.1f", initSpd));

  if (!m_bUsingFollow) {
    SetNextStep(initLat, initLon, myDir, initSpd / 7200, stepLat, stepLon);

  } else {
    SetFollowStep(initLat, initLon, myDir, initSpd / 7200, stepLat, stepLon);
  }

  wxString timeStamp = wxString::Format("%i", wxGetUTCTime());

  wxString myNMEAais = myAIS->nmeaEncode("18", m_iMMSI, "5", initSpd, initLat,
                                         initLon, myDir, myDir, "B", timeStamp);

  wxString notMID = m_tMMSI.Mid(3);

  switch (alarm_id) {
    case 970:
      SARTid = "970111111";
      SARTint = wxAtoi(SARTid);

      if (m_bSART) {
        myNMEA_SART = myAIS->nmeaEncode1_2_3(1, SARTint, 14, 0, m_latSART,
                                             m_lonSART, 3600, 511, "B");

        m_textCtrlSART->SetValue(myNMEA_SART);
        PushNMEABuffer(myNMEA_SART + "\r\n");
      } else if (stop_count < 5) {
        stop_count++;

        myNMEA_SART = myAIS->nmeaEncode1_2_3(1, SARTint, 15, 0, m_latSART,
                                             m_lonSART, 3600, 511, "B");

        m_textCtrlSART->SetValue(myNMEA_SART);  // for analysis of sentence
        PushNMEABuffer(myNMEA_SART + "\r\n");
      }
      break;

    case 972:
      MOBid = "972" + notMID;
      MOBint = wxAtoi(MOBid);

      if (m_bMOB) {
        myNMEA_MOB = myAIS->nmeaEncode1_2_3(1, MOBint, 14, 0, m_latMOB,
                                            m_lonMOB, 3600, 511, "B");
        m_textCtrlSART->SetValue(myNMEA_MOB);
        PushNMEABuffer(myNMEA_MOB + "\r\n");
      } else if (stop_countMOB < 5) {
        stop_countMOB++;
        myNMEA_MOB = myAIS->nmeaEncode1_2_3(1, MOBint, 15, 0, m_latMOB,
                                            m_lonMOB, 3600, 511, "B");
        m_textCtrlSART->SetValue(myNMEA_MOB);  // for analysis of sentence
        PushNMEABuffer(myNMEA_MOB + "\r\n");
      }
      break;

    case 974:
      EPIRBid = "974222222";
      EPIRBint = wxAtoi(EPIRBid);

      if (m_bEPIRB) {
        myNMEA_EPIRB = myAIS->nmeaEncode1_2_3(1, EPIRBint, 14, 0, m_latEPIRB,
                                              m_lonEPIRB, 3600, 511, "B");

        m_textCtrlSART->SetValue(myNMEA_EPIRB);
        PushNMEABuffer(myNMEA_EPIRB + "\r\n");
      } else if (stop_countEPIRB < 5) {
        stop_countEPIRB++;

        myNMEA_EPIRB = myAIS->nmeaEncode1_2_3(1, EPIRBint, 15, 0, m_latEPIRB,
                                              m_lonEPIRB, 3600, 511, "B");

        m_textCtrlSART->SetValue(myNMEA_EPIRB);  // for analysis of sentence
        PushNMEABuffer(myNMEA_EPIRB + "\r\n");
      }
      break;
    case 980:
      if (m_bDISTRESS) {
        myNMEA_DISTRESS =
            createDSCAlertSentence(initLat, initLon, m_iMMSI, "05", timeStamp);

        m_textCtrlSART->SetValue(myNMEA_DISTRESS);

        wxString DSCexpansion =
            createDSCExpansionSentence(initLat, initLon, m_iMMSI);

        PushNMEABuffer(myNMEA_DISTRESS + "\r\n");
        PushNMEABuffer(DSCexpansion + "\r\n");
      }
      break;
    case 982:
      if (m_bCANCEL && m_bDISTRESS) {
        myNMEA_CANCEL = createDSCAlertCancelSentence(initLat, initLon, m_iMMSI,
                                                     "05", timeStamp);

        m_textCtrlSART->SetValue(myNMEA_CANCEL);
        PushNMEABuffer(myNMEA_CANCEL + "\r\n");
      }
      break;
    case 984:
      if (m_bDISTRESSRELAY) {
        myNMEA_DISTRESSRELAY = createDSCAlertRelaySentence(
            49.9, -5.9, m_iMMSI, 503110520, "05", "1800");
        m_textCtrlSART->SetValue(myNMEA_DISTRESSRELAY);

        wxString DSCexpansion =
            createDSCExpansionSentence(initLat, initLon, m_iMMSI);

        PushNMEABuffer(myNMEA_DISTRESSRELAY + "\r\n");
        PushNMEABuffer(DSCexpansion + "\r\n");
      }
      break;
    case 986:
      if (m_bRELAYCANCEL && m_bDISTRESSRELAY) {
        myNMEA_RELAYCANCEL = createDSCAlertRelayCancelSentence(
            49.9, -5.9, m_iMMSI, 503110520, "05", "1800");

        m_textCtrlSART->SetValue(myNMEA_RELAYCANCEL);
        PushNMEABuffer(myNMEA_RELAYCANCEL + "\r\n");
      }
      break;
    case 990:
      MOBid = "870112233";
      MOBint = wxAtoi(MOBid);

      if (m_bCOLLISION) {
        DistanceBearingMercator_Plugin(initLat, initLon, m_latCollision,
                                       m_lonCollision, &m_collisionDir,
                                       &myDist);
        myNMEA_Collision = myAIS->nmeaEncode1_2_3(
            1, MOBint, 0, 20, m_latCollision, m_lonCollision, m_collisionDir,
            m_collisionDir, "B");
        m_textCtrlSART->SetValue(myNMEA_Collision);
        PushNMEABuffer(myNMEA_Collision + "\r\n");
      } else if (stop_countCOLLISION < 5) {
        stop_countCOLLISION++;
        myNMEA_Collision = myAIS->nmeaEncode1_2_3(
            1, MOBint, 0, 0, m_latCollision, m_lonCollision, m_collisionDir,
            m_collisionDir, "B");
        m_textCtrlSART->SetValue(myNMEA_MOB);  // for analysis of sentence
        PushNMEABuffer(myNMEA_Collision + "\r\n");
      }
      break;
  }

  if (m_bUseFile) nmeafile->AddLine(myNMEAais);

  int ss = 1;
  wxTimeSpan mySeconds = wxTimeSpan::Seconds(ss);
  wxDateTime mdt = dt.Add(mySeconds);

  bool m_bGrib = false;
  double wspd, wdir;
  if (m_bUsingWind) {
    m_bGrib = GetGribSpdDir(dt, initLat, initLon, wspd, wdir);
  }

  if (m_bGrib && m_bUsingWind) {
    MWVT = createMWVTSentence(initSpd, myDir, wdir, wspd);
    MWVA = createMWVASentence(initSpd, myDir, wdir, wspd);
    //MWD = createMWDSentence(wdir, wspd);

    PushNMEABuffer(MWVA + "\r\n");
    PushNMEABuffer(MWVT + "\r\n");
    PushNMEABuffer(MWD + "\r\n");
  }

  GLL = createGLLSentence(mdt, initLat, initLon, initSpd, myDir);
  VTG = createVTGSentence(initSpd, myDir);
  VHW = createVHWSentence(initSpd, myDir);
  RMC = createRMCSentence(mdt, initLat, initLon, initSpd, myDir);
  HDT = createHDTSentence(myDir);

  PushNMEABuffer(GLL + "\r\n");
  PushNMEABuffer(VTG + "\r\n");
  PushNMEABuffer(VHW + "\r\n");
  PushNMEABuffer(RMC + "\r\n");
  PushNMEABuffer(HDT + "\r\n");

  if (m_bUseAis) {
    PushNMEABuffer(myNMEAais + "\r\n");
  }
  initLat = stepLat;
  initLon = stepLon;

  dt = mdt;

  GetParent()->Refresh();
}

void Dlg::SetInterval(int interval) {
  m_interval = interval;
  if (m_Timer->IsRunning())  // Timer started?
    m_Timer->Start(m_interval,
                   wxTIMER_CONTINUOUS);  // restart timer with new interval
}

wxString Dlg::createMWDSentence(double winddirection, double windspeed) {
  /*
  1    2   3   4        5
  |    |   |   |        |
  $--MWV, x.x, a, x.x, a*hh

  Field Number :
  1. Wind Angle, 0 to 360 degrees
  2.Reference, R = Relative, T = True
  3.Wind Speed
  4.Wind Speed Units, K / M / N
  5.Status, A = Data Valid
  Checksum
  */

  /*
  +     * $WIMWD,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>*hh
  +     *
  +     * NMEA 0183 standard Wind Direction and Speed, with respect to north.
  +     *
  +     * <1> Wind direction, 0.0 to 359.9 degrees True, to the nearest 0.1
  degree
  +     * <2> T = True
  +     * <3> Wind direction, 0.0 to 359.9 degrees Magnetic, to the nearest
  0.1 degree
  +     * <4> M = Magnetic
  +     * <5> Wind speed, knots, to the nearest 0.1 knot.
  +     * <6> N = Knots
  +     * <7> Wind speed, meters/second, to the nearest 0.1 m/s.
  +     * <8> M = Meters/second
  +     */

  wxString nMWV;
  wxString nMWD;
  wxString nDir;
  wxString nRelTrue;
  wxString nSpd;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nUnits;
  wxString nC = ",";
  wxString nA = "A";
  nUnits = "N";
  nMWV = "WIMWV";
  nMWD = "WIMWD";
  nRelTrue = "T";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nSpd = wxString::Format("%f", windspeed);
  nDir = wxString::Format("%f", winddirection);

  // nForCheckSum = nMWV + nC + nDir + nC + nRelTrue + nC + nSpd + nC + nUnits
  // + nC + nA;
  nForCheckSum = nMWD + nC + nDir + nC + nRelTrue + nC + nC + nC + nSpd + nC +
                 nUnits + nC + nC;
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  return nFinal;
}

wxString Dlg::createVHWSentence(double stw, double hdg) {
  /*
  VHW Water Speed and Heading
  1   2   3   4   5   6   7   8 9
  |   |   |   |   |   |   |   | |
  $--VHW, x.x, T, x.x, M, x.x, N, x.x, K*hh
  1) Degress  True
  2) T = True
  3) Degrees Magnetic
  4) M = Magnetic
  5) Knots(speed of vessel relative to the water)
  6) N = Knots
  7) Kilometers (speed of vessel relative to the water)
  8) K = Kilometres
  9) Checksum
  */
  wxString nVHW;
  wxString nDir;
  wxString nTrue;
  wxString nMag;
  wxString nSpd;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nUnits;
  wxString nC = ",";
  wxString nA = "A";
  nUnits = "N";
  nVHW = "IIVHW";
  nTrue = "T";
  nMag = "M";
  wxString ndlr = "$";
  wxString nast = "*";

  nSpd = wxString::Format("%f", stw);
  nDir = wxString::Format("%f", hdg);

  nForCheckSum = nVHW + nC + nDir + nC + nTrue + nC + nC + nMag + nC + nSpd + nC + nUnits +
                 nC + nC + "K";
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  return nFinal;
}

wxString Dlg::createMWVTSentence(double spd, double hdg, double winddirection,
                                 double windspeed) {
  /*
  1    2   3   4        5
  |    |   |   |        |
  $--MWV, x.x, a, x.x, a*hh

  Field Number :
  1. Wind Angle, 0 to 360 degrees
  2.Reference, R = Relative, T = True (theoretical)
  3.Wind Speed
  4.Wind Speed Units, K / M / N
  5.Status, A = Data Valid
  Checksum
  */

  /*
  +     * $WIMWD,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>*hh
  +     *
  +     * NMEA 0183 standard Wind Direction and Speed, with respect to north.
  +     *
  +     * <1> Wind direction, 0.0 to 359.9 degrees True, to the nearest 0.1
  degree
  +     * <2> T = True
  +     * <3> Wind direction, 0.0 to 359.9 degrees Magnetic, to the nearest
  0.1 degree
  +     * <4> M = Magnetic
  +     * <5> Wind speed, knots, to the nearest 0.1 knot.
  +     * <6> N = Knots
  +     * <7> Wind speed, meters/second, to the nearest 0.1 m/s.
  +     * <8> M = Meters/second
  +
  */

  double twa = 360 - ((hdg - winddirection) - 360);
  if (twa > 360) {
    twa -= 360;
    if (twa > 360) {
      twa -= 360;
    }
  }

  double tws = windspeed;

  wxString nMWV;
  wxString nMWD;
  wxString nDir;
  wxString nRelTrue;
  wxString nSpd;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nUnits;
  wxString nC = ",";
  wxString nA = "A";
  nUnits = "K";
  nMWV = "WIMWV";
  nMWD = "WIMWD";
  nRelTrue = "T";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nSpd = wxString::Format("%f", tws);
  nDir = wxString::Format("%f", twa);

  nForCheckSum =
      nMWV + nC + nDir + nC + nRelTrue + nC + nSpd + nC + nUnits + nC + nA;
  //$--MWD, x.x, T, x.x, M, x.x, N, x.x, M*hh<CR><LF>
  // MWD,270.7,T,,,20.5,N,,
  // nForCheckSum = nMWD + nC + nDir + nC + nRelTrue + nC  + nC + nC + nSpd +
  // nC + nUnits + nC + nC ;
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  return nFinal;
}

wxString Dlg::createMWVASentence(double spd, double hdg, double winddirection,
                                 double windspeed) {
  /*
  1    2   3   4        5
  |    |   |   |        |
  $--MWV, x.x, a, x.x, a*hh

  Field Number :
  1. Wind Angle, 0 to 360 degrees
  2.Reference, R = Relative, T = True (theoretical)
  3.Wind Speed
  4.Wind Speed Units, K / M / N
  5.Status, A = Data Valid
  Checksum
  */

  /*
  +     * $WIMWD,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>*hh
  +     *
  +     * NMEA 0183 standard Wind Direction and Speed, with respect to north.
  +     *
  +     * <1> Wind direction, 0.0 to 359.9 degrees True, to the nearest 0.1
  degree
  +     * <2> T = True
  +     * <3> Wind direction, 0.0 to 359.9 degrees Magnetic, to the nearest
  0.1 degree
  +     * <4> M = Magnetic
  +     * <5> Wind speed, knots, to the nearest 0.1 knot.
  +     * <6> N = Knots
  +     * <7> Wind speed, meters/second, to the nearest 0.1 m/s.
  +     * <8> M = Meters/second
  +
  */

  double twa = 360 - ((hdg - winddirection) - 360);
  if (twa > 360) {
    twa -= 360;
    if (twa > 360) {
      twa -= 360;
    }
  }
  wxString leftright = wxEmptyString;

  if (twa <= 180) {
    leftright = "R";
  }
  if (twa > 180) {
    leftright = "L";
    twa = 360 - twa;
  }

  // double aws, twd, tws, awd, awa;
  double aws, awa;
  twa = 180 - twa;  // we need the complement of the twa for the internal angle
                    // of the triangle
  twa = twa * M_PI / 180;  // convert to radians
  // tws = windspeed;
  // double alpha, bravo, charlie, delta;
  double alpha, charlie;
  alpha = pow(spd, 2) + pow(windspeed, 2) - 2 * spd * windspeed * cos(twa);
  aws = sqrt(alpha);

  // spd / charlie = aws / twa;

  charlie = spd * sin(twa) / aws;
  charlie = asin(charlie);
  twa = M_PI - twa;  // complement in radians
  awa = twa - charlie;
  awa = awa * 180 / M_PI;  // back to degrees

  if (leftright == "L") {
    awa = 360 - awa;
  }

  wxString nMWV;
  wxString nMWD;
  wxString nDir;
  wxString nRelTrue;
  wxString nSpd;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nUnits;
  wxString nC = ",";
  wxString nA = "A";
  nUnits = "K";
  nMWV = "WIMWV";
  nMWD = "WIMWD";
  nRelTrue = "R";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nSpd = wxString::Format("%f", aws);
  nDir = wxString::Format("%f", awa);

  nForCheckSum =
      nMWV + nC + nDir + nC + nRelTrue + nC + nSpd + nC + nUnits + nC + nA;
  //$--MWD, x.x, T, x.x, M, x.x, N, x.x, M*hh<CR><LF>
  // MWD,270.7,T,,,20.5,N,,
  // nForCheckSum = nMWD + nC + nDir + nC + nRelTrue + nC  + nC + nC + nSpd +
  // nC + nUnits + nC + nC ;
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  return nFinal;
}
wxString Dlg::createRMCSentence(wxDateTime myDateTime, double myLat,
                                double myLon, double mySpd, double myDir) {
  //$GPRMC, 110858.989, A, 4549.9135, N, 00612.2671, E, 003.7, 207.5, 050513,
  //, , A * 60
  //$GPRMC,110858.989,A,4549.9135,N,00612.2671,E,003.7,207.5,050513,,,A*60

  wxString nlat;
  wxString nlon;
  wxString nRMC;
  wxString nNS;
  wxString nEW;
  wxString nSpd;
  wxString nDir;
  wxString nTime;
  wxString nDate;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nC = ",";
  wxString nA = "A,";
  nRMC = "GPRMC,";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nTime = DateTimeToTimeString(myDateTime);
  nNS = LatitudeToString(myLat);
  nEW = LongitudeToString(myLon);
  nSpd = wxString::Format("%f", mySpd);
  nDir = wxString::Format("%f", myDir);
  nDate = DateTimeToDateString(myDateTime);

  nForCheckSum =
      nRMC + nTime + nC + nNS + nEW + nSpd + nC + nDir + nC + nDate + ",,,A";
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  return nFinal;
}

wxString Dlg::createGLLSentence(wxDateTime myDateTime, double myLat,
                                double myLon, double mySpd, double myDir) {
  //$IIGLL,5027.776667,N,412.690754,W,123327,A*26

  wxString nlat;
  wxString nlon;
  wxString nGLL;
  wxString nNS;
  wxString nEW;
  wxString nSpd;
  wxString nDir;
  wxString nTime;
  wxString nDate;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nC = ",";
  wxString nA = "A,";
  nGLL = "IIGLL,";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nTime = DateTimeToTimeString(myDateTime);
  nNS = LatitudeToString(myLat);
  nEW = LongitudeToString(myLon);
  nSpd = wxString::Format("%f", mySpd);
  nDir = wxString::Format("%f", myDir);
  nDate = DateTimeToDateString(myDateTime);

  nForCheckSum = nGLL + nNS + nEW + nTime + ",A";
  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  // wxMessageBox(nFinal);
  return nFinal;
}

wxString Dlg::createVTGSentence(double mySpd, double myDir) {
  //$GPVTG, 054.7, T, 034.4, M, 005.5, N, 010.2, K * 48
  //$IIVTG, 307., T, , M, 08.5, N, 15.8, K, A * 2F
  wxString nSpd;
  wxString nDir;
  wxString nTime;
  wxString nDate;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nC = ",";
  wxString nA = "A";
  wxString nT = "T,";
  wxString nM = "M,";
  wxString nN = "N,";
  wxString nK = "K,";

  wxString nVTG = "IIVTG,";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nSpd = wxString::Format("%f", mySpd);
  nDir = wxString::Format("%f", myDir);

  nForCheckSum = nVTG + nDir + nC + nT + nC + nM + nSpd + nN + nC + nC + nA;

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  // wxMessageBox(nFinal);
  return nFinal;
}

wxString Dlg::createHDTSentence(double myDir) {
  /*
  1   2 3
  |   | |
  $--HDT, x.x, T*hh<CR><LF>
  */
  wxString nSpd;
  wxString nDir;
  wxString nTime;
  wxString nDate;
  wxString nValid;
  wxString nForCheckSum;
  wxString nFinal;
  wxString nC = ",";
  wxString nA = "A";
  wxString nT = "T,";
  wxString nM = "M,";
  wxString nN = "N,";
  wxString nK = "K,";

  wxString nHDG = "IIHDT,";
  nValid = "A,A";
  wxString ndlr = "$";
  wxString nast = "*";

  nDir = wxString::Format("%f", myDir);

  nForCheckSum = nHDG + nDir + nC + nT;

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
  // wxMessageBox(nFinal);
  return nFinal;
}

wxString Dlg::createDSCAlertSentence(double lat, double lon, int mmsi,
                                     wxString nature, wxString time) {
  // 1      2  3          4  5  6  7          8    9 10 11 12 13
  // |      |  |          |  |  |  |          |    | |  |  |  |
  // $CDDSC,12,3380400790,12,06,00,1423108312,2019, , , S, E* 6A

  wxString nNS;
  wxString nEW;
  wxString nLatLon;
  wxString nMMSI;
  wxString nTime;
  wxString nQLat;
  wxString nQLon;

  wxString ndlr = "$";
  wxString nast = "*";

  wxString nForCheckSum;
  wxString nFinal;

  wxString nC = ",";
  wxString nDSC = "CDDSC";

  nNS = LatitudeToString(lat);
  nEW = LongitudeToString(lon);

  nQLat = nNS.at(nNS.length() - 2);
  nQLon = nEW.at(nEW.length() - 2);

  wxString nQ = nQLat + nQLon;
  wxString quadrant;

  /*
     - quadrant NE is indicated by the digit 0,
      quadrant NW is indicated by the digit 1,
      quadrant SE is indicated by the digit 2,
      quadrant SW is indicated by the digit 3
 */
  if (nQ == "NE")
    quadrant = "0";
  else if (nQ == "NW")
    quadrant = "1";
  else if (nQ == "SE")
    quadrant = "2";
  else if (nQ == "SW")
    quadrant = "3";

  nNS = nNS.Mid(0, 4);
  nEW = nEW.Mid(0, 5);
  nLatLon = quadrant + nNS + nEW;

  nMMSI = wxString::Format("%i", mmsi) + "0";

  nTime = time;
  nTime = nTime.Mid(0, 4);

  nForCheckSum = nDSC + nC + "12" + nC + nMMSI + nC + "12" + nC + "06" + nC +
                 "00" + nC + nLatLon + nC + nC + nC + "S" + nC + "E";

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);

  return nFinal;
}

wxString Dlg::createDSCExpansionSentence(double lat, double lon, int mmsi) {
  //    1     2  3  4  5           6   7          8
  //    |     |  |  |  |           |   |          |
  //   $CDDSE,1, 1, A, 3380400790, 00, 45894494 * 1B

  wxString nNS;
  wxString nEW;
  wxString nDecNS;
  wxString nDecEW;
  wxString nMMSI;
  wxString nDec;

  wxString nDSC = "CDDSE";

  wxString ndlr = "$";
  wxString nast = "*";

  wxString nForCheckSum;
  wxString nFinal;

  wxString nC = ",";

  nNS = LatitudeToString(lat);
  nEW = LongitudeToString(lon);

  nDecNS = nNS.substr(nNS.find('.') + 1);
  nDecEW = nEW.substr(nEW.find('.') + 1);

  nDec = nDecNS.Mid(0, 4) + nDecEW.Mid(0, 4);

  nMMSI = wxString::Format("%i", mmsi) + "0";

  nForCheckSum = nDSC + nC + "1" + nC + "1" + nC + "A" + nC + nMMSI + nC +
                 "00" + nC + nDec;

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);

  return nFinal;
}

wxString Dlg::createDSCAlertCancelSentence(double lat, double lon, int mmsi,
                                           wxString nature, wxString time) {
  //    1      2  3           4   5   6   7           8     9          10 11 12
  //    13 |      |  |           |   |   |   |           |     |          |  |
  //    |    |
  //   $CDDSC,12,3381581370, 12, 06, 00, 1423108312, 0236, 3381581370, , S, *20

  wxString nNS;
  wxString nEW;
  wxString nLatLon;
  wxString nMMSI;
  wxString nTime;
  wxString nQLat;
  wxString nQLon;

  wxString ndlr = "$";
  wxString nast = "*";

  wxString nForCheckSum;
  wxString nFinal;

  wxString nC = ",";
  wxString nDSC = "CDDSC";

  nNS = LatitudeToString(lat);
  nEW = LongitudeToString(lon);

  nQLat = nNS.at(nNS.length() - 2);
  nQLon = nEW.at(nEW.length() - 2);

  wxString nQ = nQLat + nQLon;
  wxString quadrant;

  if (nQ == "NE")
    quadrant = "0";
  else if (nQ == "NW")
    quadrant = "1";
  else if (nQ == "SE")
    quadrant = "2";
  else if (nQ == "SW")
    quadrant = "3";

  nNS = nNS.Mid(0, 4);
  nEW = nEW.Mid(0, 5);
  nLatLon = quadrant + nNS + nEW;

  nMMSI = wxString::Format("%i", mmsi) + "0";

  nForCheckSum = nDSC + nC + "12" + nC + nMMSI + nC + nature + nC + "00" + nC +
                 nLatLon + nC + time + nC + nMMSI + nC + nC + "S" + nC;

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);

  return nFinal;
}

wxString Dlg::createDSCAlertRelaySentence(double lat, double lon, int mmsi,
                                          int dmmsi, wxString nature,
                                          wxString time) {
  // 1      2  3          4  5  6  7          8    9          10 11 12 13
  // |      |  |          |  |  |  |          |    |          |  |  |  |
  // $CDDSC,12,3380400790,12,12,00,1423108312,2019,5031105200,05,S, E* 6A

  wxString nNS;
  wxString nEW;
  wxString nLatLon;
  wxString nMMSI;
  wxString dMMSI;
  wxString nTime;
  wxString nQLat;
  wxString nQLon;

  wxString ndlr = "$";
  wxString nast = "*";

  wxString nForCheckSum;
  wxString nFinal;

  wxString nC = ",";
  wxString nDSC = "CDDSC";

  nNS = LatitudeToString(lat);
  nEW = LongitudeToString(lon);

  nQLat = nNS.at(nNS.length() - 2);
  nQLon = nEW.at(nEW.length() - 2);

  wxString nQ = nQLat + nQLon;
  wxString quadrant;

  /*
     - quadrant NE is indicated by the digit 0,
      quadrant NW is indicated by the digit 1,
      quadrant SE is indicated by the digit 2,
      quadrant SW is indicated by the digit 3
 */
  if (nQ == "NE")
    quadrant = "0";
  else if (nQ == "NW")
    quadrant = "1";
  else if (nQ == "SE")
    quadrant = "2";
  else if (nQ == "SW")
    quadrant = "3";

  nNS = nNS.Mid(0, 4);
  nEW = nEW.Mid(0, 5);
  nLatLon = quadrant + nNS + nEW;

  nMMSI = wxString::Format("%i", mmsi) + "0";
  dMMSI = wxString::Format("%i", dmmsi) + "0";

  nTime = time;
  nTime = nTime.Mid(0, 4);

  nForCheckSum = nDSC + nC + "16" + nC + nMMSI + nC + "12" + nC + "12" + nC +
                 "00" + nC + nLatLon + nC + time + nC + dMMSI + nC + nature +
                 nC + "S" + nC + "E";

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);

  return nFinal;
}

wxString Dlg::createDSCAlertRelayCancelSentence(double lat, double lon,
                                                int mmsi, int dmmsi,
                                                wxString nature,
                                                wxString time) {
  //    1      2  3           4   5   6   7           8     9          10 11
  //    12  13 |      |  |           |   |   |   |           |     | |  |  | |
  //   $CDDSC,12,3381581370, 12, 06, 00, 1423108312, 0236, 3381581370, , S,
  //   *20

  wxString nNS;
  wxString nEW;
  wxString nLatLon;
  wxString nMMSI;
  wxString dMMSI;
  wxString nTime;
  wxString nQLat;
  wxString nQLon;

  wxString ndlr = "$";
  wxString nast = "*";

  wxString nForCheckSum;
  wxString nFinal;

  wxString nC = ",";
  wxString nDSC = "CDDSC";

  nNS = LatitudeToString(lat);
  nEW = LongitudeToString(lon);

  nQLat = nNS.at(nNS.length() - 2);
  nQLon = nEW.at(nEW.length() - 2);

  wxString nQ = nQLat + nQLon;
  wxString quadrant;

  if (nQ == "NE")
    quadrant = "0";
  else if (nQ == "NW")
    quadrant = "1";
  else if (nQ == "SE")
    quadrant = "2";
  else if (nQ == "SW")
    quadrant = "3";

  nNS = nNS.Mid(0, 4);
  nEW = nEW.Mid(0, 5);
  nLatLon = quadrant + nNS + nEW;

  nMMSI = wxString::Format("%i", mmsi) + "0";
  dMMSI = wxString::Format("%i", dmmsi) + "0";

  nForCheckSum = nDSC + nC + "12" + nC + nMMSI + nC + nature + nC + "00" + nC +
                 nLatLon + nC + time + nC + dMMSI + nC + nC + "S" + nC;

  nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);

  return nFinal;
}

wxString Dlg::makeCheckSum(wxString mySentence) {
  size_t i;
  unsigned char XOR;

  wxString s(mySentence);
  wxCharBuffer buffer = s.ToUTF8();
  char* Buff = buffer.data();  // data() returns const char *
  size_t iLen = strlen(Buff);
  for (XOR = 0, i = 0; i < iLen; i++) XOR ^= (unsigned char)Buff[i];
  stringstream tmpss;
  tmpss << hex << (int)XOR;
  wxString mystr = tmpss.str();
  return mystr;
}

double StringToLatitude(wxString mLat) {
  // 495054
  double returnLat;
  wxString mBitLat = mLat(0, 2);
  double degLat;
  mBitLat.ToDouble(&degLat);
  wxString mDecLat = mLat(2, mLat.length());
  double decValue;
  mDecLat.ToDouble(&decValue);

  returnLat = degLat + decValue / 100 / 60;

  return returnLat;
}

wxString Dlg::LatitudeToString(double mLat) {
  wxString singlezero = "0";
  wxString mDegLat;

  int degLat = std::abs(mLat);
  wxString finalDegLat = wxString::Format("%i", degLat);

  int myL = finalDegLat.length();
  switch (myL) {
    case (1): {
      mDegLat = singlezero + finalDegLat;
      break;
    }
    case (2): {
      mDegLat = finalDegLat;
      break;
    }
  }

  double minLat = std::abs(mLat) - degLat;
  double decLat = minLat * 60;

  wxString returnLat;

  if (mLat >= 0) {
    if (decLat < 10) {
      returnLat = mDegLat + "0" + wxString::Format("%.6f", decLat) + ",N,";
    } else {
      returnLat = mDegLat + wxString::Format("%.6f", decLat) + ",N,";
    }

  } else if (mLat < 0) {
    if (decLat < 10) {
      returnLat = mDegLat + "0" + wxString::Format("%.6f", decLat) + ",S,";
    } else {
      returnLat = mDegLat + wxString::Format("%.6f", decLat) + ",S,";
    }
  }

  return returnLat;
}
double StringToLongitude(wxString mLon) {
  wxString mBitLon = "";
  wxString mDecLon;
  double value1;
  double decValue1;

  double returnLon;

  int m_len = mLon.length();

  if (m_len == 7) {
    mBitLon = mLon(0, 3);
  }

  if (m_len == 6) {
    mBitLon = mLon(0, 2);
  }

  if (m_len == 5) {
    mBitLon = mLon(0, 1);
  }

  if (m_len == 4) {
    mBitLon = "00.00";
  }

  if (mBitLon == "-") {
    value1 = -0.00001;
  } else {
    mBitLon.ToDouble(&value1);
  }

  mDecLon = mLon(mLon.length() - 4, mLon.length());
  mDecLon.ToDouble(&decValue1);

  if (value1 < 0) {
    returnLon = value1 - decValue1 / 100 / 60;
  } else {
    returnLon = value1 + decValue1 / 100 / 60;
  }

  return returnLon;
}

wxString Dlg::LongitudeToString(double mLon) {
  wxString mDecLon;
  wxString mDegLon;
  double decValue;
  wxString returnLon;
  wxString doublezero = "00";
  wxString singlezero = "0";

  int degLon = fabs(mLon);
  wxString inLon = wxString::Format("%i", degLon);

  int myL = inLon.length();
  switch (myL) {
    case (1): {
      mDegLon = doublezero + inLon;
      break;
    }
    case (2): {
      mDegLon = singlezero + inLon;
      break;
    }
    case (3): {
      mDegLon = inLon;
      break;
    }
  }
  decValue = std::abs(mLon) - degLon;
  double decLon = decValue * 60;

  if (mLon >= 0) {
    if (decLon < 10) {
      returnLon = mDegLon + "0" + wxString::Format("%.6f", decLon) + ",E,";
    } else {
      returnLon = mDegLon + wxString::Format("%.6f", decLon) + ",E,";
    }

  } else {
    if (decLon < 10) {
      returnLon = mDegLon + "0" + wxString::Format("%.6f", decLon) + ",W,";
    } else {
      returnLon = mDegLon + wxString::Format("%.6f", decLon) + ",W,";
    }
  }
  return returnLon;
}

wxString Dlg::DateTimeToTimeString(wxDateTime myDT) {
  wxString sHours, sMinutes, sSecs;
  sHours = myDT.Format("%H");
  sMinutes = myDT.Format("%M");
  sSecs = myDT.Format("%S");
  wxString dtss = sHours + sMinutes + sSecs;
  return dtss;
}

wxString Dlg::DateTimeToDateString(wxDateTime myDT) {
  wxString sDay, sMonth, sYear;
  sDay = myDT.Format("%d");
  sMonth = myDT.Format("%m");
  sYear = myDT.Format("%y");

  return sDay + sMonth + sYear;
}

void Dlg::OnContextMenu(double m_lat, double m_lon) {
  m_buttonWind->SetBackgroundColour(wxColour(0, 255, 0));
  m_bUsingWind = false;

  initLat = m_lat;
  initLon = m_lon;
}

void Dlg::RequestGrib(wxDateTime time) {
  Json::Value value;

  time = time.FromUTC();

  value["Day"] = time.GetDay();
  value["Month"] = time.GetMonth();
  value["Year"] = time.GetYear();
  value["Hour"] = time.GetHour();
  value["Minute"] = time.GetMinute();
  value["Second"] = time.GetSecond();

  wxString out;

  Json::StreamWriterBuilder builder;
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  std::ostringstream outStream;
  writer->write(value, &outStream);
  std::string str = outStream.str();

  SendPluginMessage(wxString("GRIB_TIMELINE_RECORD_REQUEST"), str);

  Lock();
  m_bNeedsGrib = false;
  Unlock();
}

bool Dlg::GetGribSpdDir(wxDateTime dt, double lat, double lon, double& spd,
                        double& dir) {
  wxDateTime dtime = dt;

  plugin->m_grib_lat = lat;
  plugin->m_grib_lon = lon;
  RequestGrib(dtime);
  if (plugin->m_bGribValid) {
    spd = plugin->m_tr_spd;
    dir = plugin->m_tr_dir;
    return true;
  } else {
    return false;
  }
}

void Dlg::OnWind(wxCommandEvent& event) {
  m_bUsingFollow = false;
  ResetPauseButton();

  if (initLat == 0.0) {
    wxMessageBox(_("Please right-click and choose vessel start position"));
    return;
  }
  if (!m_bShipDriverHasStarted) {
    wxMessageBox(_("Please start ShipDriver"));
    return;
  }

  m_SliderSpeed->SetValue(0);
  double scale_factor = GetOCPNGUIToolScaleFactor_PlugIn();
  JumpToPosition(initLat, initLon, scale_factor);

  if (!m_bUsingWind) {
    m_buttonWind->SetBackgroundColour(wxColour(255, 0, 0));
    m_bUsingWind = true;
    double myPolarSpeed = GetPolarSpeed(initLat, initLon, initDir);
    if (myPolarSpeed == -1) {
      if (m_bInvalidPolarsFile) {
        wxMessageBox(_("Invalid Boat Polars file"));
      }

      if (m_bInvalidGribFile) {
        wxMessageBox(
            _("Grib data is not available for the present date/time or "
              "location"));
      }
      m_buttonWind->SetBackgroundColour(wxColour(0, 255, 0));
      m_bUsingWind = false;
    }
  } else {
    m_buttonWind->SetBackgroundColour(wxColour(0, 255, 0));
    m_bUsingWind = false;
  }
}

double Dlg::GetPolarSpeed(double lat, double lon, double cse) {
  double lati = lat;
  double loni = lon;
  double spd;
  double dir;

  wxDateTime dt;
  dt = wxDateTime::UNow();

  bool m_bGrib = GetGribSpdDir(dt, lati, loni, spd, dir);
  if (!m_bGrib) {
    m_bInvalidGribFile = true;
    return -1;
  }

  wxString error;
  wxString s = "/";

  const char* pName = "ShipDriver_pi";

  wxString polars_path = GetPluginDataDir(pName) + s + "data" + s;
  wxString myFile = polars_path + "arcona.xml";

  double twa = 360 - ((cse - dir) - 360);
  if (twa > 360) {
    twa -= 360;
    if (twa > 360) {
      twa -= 360;
    }
  }

  if (twa > 180) {
    twa = 360 - twa;
  }

  double polarSpeed = ReadPolars(myFile, twa, spd);
  return polarSpeed;
}

double Dlg::ReadPolars(wxString filename, double windangle, double windspeed) {
  bool foundWindAngle = false;
  bool foundWindSpeed = false;
  // bool foundPreviousWindAngle = false;

  double myWindAngle = -1;
  double myWindSpeed = -1;
  double prevAngle = -1;
  double prevSpeed = -1;
  // double dSpeed = -1;
  double prevPolarSpeed = -1;
  wxString myPolarSpeed;

  wxString theWindAngle;

  TiXmlDocument doc;
  wxString error;

  wxFileName fn(filename);

  if (!doc.LoadFile(filename.mb_str())) {
    m_bInvalidPolarsFile = true;
    return -1;
  } else {
    TiXmlHandle root(doc.RootElement());

    if (strcmp(root.Element()->Value(), "ShipDriver")) {
      m_bInvalidPolarsFile = true;
      return -1;
    }

    int count = 0;
    for (TiXmlElement* e = root.FirstChild().Element(); e;
         e = e->NextSiblingElement())
      count++;

    int i = 0;
    for (TiXmlElement* e = root.FirstChild().Element(); e;
         e = e->NextSiblingElement(), i++) {
      if (!strcmp(e->Value(), "TWA") && windangle > myWindAngle &&
          !foundWindAngle && !foundWindSpeed) {
        myWindAngle = AttributeDouble(e, "WindAngle", NAN);
        if (prevAngle < windangle && windangle < myWindAngle) {
          theWindAngle = wxString::Format("%5.2f", prevAngle);
          foundWindAngle = true;
          break;
        }
        prevAngle = myWindAngle;
      }
    }

    if (foundWindAngle) {
      // we have found the polar for the next highest wind speed
      // need to move back to the previous polar ... given by windAngle

      TiXmlElement* e;
      for (e = root.FirstChild().Element(); e;
           e = e->NextSiblingElement(), i++) {
        if (!strcmp(e->Value(), "TWA")) {
          myWindAngle = AttributeDouble(e, "WindAngle", NAN);
          wxString angleOut = wxString::Format("%5.2f", myWindAngle);
          if (angleOut == theWindAngle) {  // we have found the correct section
                                           // of the polars file    for the
                                           // relative wind
            for (TiXmlElement* g = e->FirstChildElement(); g;
                 g = g->NextSiblingElement()) {
              if (!strcmp(g->Value(), "SPD") && windspeed > myWindSpeed) {
                myWindSpeed = AttributeDouble(g, "WindSpeed", NAN);
                wxString myPolarSpeed = g->GetText();
                double dSpeed;
                myPolarSpeed.ToDouble(&dSpeed);

                if (prevSpeed < windspeed && windspeed < myWindSpeed) {
                  return prevPolarSpeed;
                }

                prevSpeed = myWindSpeed;  // attribute for wind speed
                prevPolarSpeed = dSpeed;  // value for boat speed
              }
            }
          }
        }
      }
    }
  }

  m_bInvalidPolarsFile = true;
  return -1;
}

double Dlg::AttributeDouble(TiXmlElement* e, const char* name, double def) {
  const char* attr = e->Attribute(name);
  if (!attr) return def;
  char* end;
  double d = strtod(attr, &end);
  if (end == attr) return def;
  return d;
}

double Dlg::ReadNavobj() {
  rte myRte;
  rtept myRtePt;
  vector<rtept> my_points;

  my_routes.clear();

  wxString rte_lat;
  wxString rte_lon;

  wxString wpt_guid;

  wxString navobj_path = Dlg::StandardPath();
  wxString myFile = navobj_path + "navobj.xml";

  // wxMessageBox(myFile);

  TiXmlDocument doc;
  wxString error;

  if (!doc.LoadFile(myFile.mb_str())) {
    wxMessageBox(_("Unable to read navobj file"));
    return -1;
  } else {
    TiXmlElement* root = doc.RootElement();
    if (!strcmp(root->Value(), "rte")) {
      wxMessageBox(_("Invalid xml file"));
      return -1;
    }

    int i = 0;
    int myIndex = 0;
    bool nameFound = false;

    for (TiXmlElement* e = root->FirstChildElement(); e;
         e = e->NextSiblingElement(), i++) {
      if (!strcmp(e->Value(), "rte")) {
        nameFound = false;
        my_points.clear();

        for (TiXmlElement* f = e->FirstChildElement(); f;
             f = f->NextSiblingElement()) {
          if (!strcmp(f->Value(), "name")) {
            myRte.Name = wxString::FromUTF8(f->GetText());
            nameFound = true;
          }

          if (!strcmp(f->Value(), "rtept")) {
            rte_lat = wxString::FromUTF8(f->Attribute("lat"));
            rte_lon = wxString::FromUTF8(f->Attribute("lon"));

            myRtePt.lat = rte_lat;
            myRtePt.lon = rte_lon;

            for (TiXmlElement* i = f->FirstChildElement(); i;
                 i = i->NextSiblingElement()) {
              if (!strcmp(i->Value(), "extensions")) {
                for (TiXmlElement* j = i->FirstChildElement(); j;
                     j = j->NextSiblingElement()) {
                  if (!strcmp(j->Value(), "opencpn:guid")) {
                    wpt_guid = wxString::FromUTF8(j->GetText());

                    myRtePt.m_GUID = wpt_guid;
                  }
                }
              }
            }

            myRtePt.index = myIndex;
            myIndex++;
            my_points.push_back(myRtePt);
          }
        }
        myRte.m_rteptList = my_points;
        if (!nameFound) {
          myRte.Name = "Unnamed";
        }
        my_routes.push_back(myRte);
        myIndex = 0;
        my_points.clear();
      }

      my_points.clear();
      myIndex = 0;
    }
  }

  return -1;
}

void Dlg::OnFollow(wxCommandEvent& event) {
  std::vector<std::unique_ptr<PlugIn_Route_Ex>> routes;
  auto uids = GetRouteGUIDArray();
  for (size_t i = 0; i < uids.size(); i++) {
    routes.push_back(std::move(GetRouteEx_Plugin(uids[i])));
  }

  GetRouteDialog RouteDialog(this, -1, _("Select the route to follow"),
                             wxPoint(200, 200), wxSize(300, 200),
                             wxRESIZE_BORDER);

  RouteDialog.dialogText->InsertColumn(0, "", 0, wxLIST_AUTOSIZE);
  RouteDialog.dialogText->SetColumnWidth(0, 290);
  RouteDialog.dialogText->InsertColumn(1, "", 0, wxLIST_AUTOSIZE);
  RouteDialog.dialogText->SetColumnWidth(1, 0);
  RouteDialog.dialogText->DeleteAllItems();

  int in = 0;
  std::vector<std::string> names;
  for (const auto& r : routes) names.push_back(r->m_NameString.ToStdString());

  for (size_t n = 0; n < names.size(); n++) {
    wxString routeName = names[in];

    RouteDialog.dialogText->InsertItem(in, "", -1);
    RouteDialog.dialogText->SetItem(in, 0, routeName);
    in++;
  }

  // ReadNavobj();
  long si = -1;
  long itemIndex = -1;
  // int f = 0;

  wxListItem row_info;
  wxString cell_contents_string = wxEmptyString;
  bool foundRoute = false;

  if (RouteDialog.ShowModal() != wxID_OK) {
    m_bUsingFollow = false;
  } else {
    ResetPauseButton();
    for (;;) {
      itemIndex = RouteDialog.dialogText->GetNextItem(
          itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

      if (itemIndex == -1) break;

      // Got the selected item index
      if (RouteDialog.dialogText->IsSelected(itemIndex)) {
        si = itemIndex;
        foundRoute = true;
        break;
      }
    }

    if (foundRoute) {
      // Set what row it is (m_itemId is a member of the regular
      // wxListCtrl class)
      row_info.m_itemId = si;
      // Set what column of that row we want to query for information.
      row_info.m_col = 0;
      // Set text mask
      row_info.m_mask = wxLIST_MASK_TEXT;

      // Get the info and store it in row_info variable.
      RouteDialog.dialogText->GetItem(row_info);
      // Extract the text out that cell
      cell_contents_string = row_info.m_text;
      rtept initPoint;
      nextRoutePointIndex = 0;
      bool foundRoute = false;

      for (size_t i = 0; i < uids.size(); i++) {
        thisRoute = GetRouteEx_Plugin(uids[i]);

        if (thisRoute->m_NameString == cell_contents_string) {
          foundRoute = true;
          break;
        }
      }
      if (foundRoute) {
        m_bUsingFollow = true;
        countRoutePoints = thisRoute->pWaypointList->size();
        myList = thisRoute->pWaypointList;

        PlugIn_Waypoint_Ex* myWaypoint;
        theWaypoints.clear();

        wxPlugin_WaypointExListNode* pwpnode = myList->GetFirst();
        while (pwpnode) {
          myWaypoint = pwpnode->GetData();

          theWaypoints.push_back(myWaypoint);

          pwpnode = pwpnode->GetNext();
        }

        for (size_t n = 0; n < theWaypoints.size(); n++) {
          if (n == 0) {
            double dlat = theWaypoints[n]->m_lat;
            double dlon = theWaypoints[n]->m_lon;

            initLat = dlat;
            initLon = dlon;

            nextRoutePointIndex = 1;
          }

          if (n == 1) {
            double dlat1 = theWaypoints[n]->m_lat;
            double dlon1 = theWaypoints[n]->m_lon;

            nextLat = dlat1;
            nextLon = dlon1;

            DistanceBearingMercator_Plugin(nextLat, nextLon, initLat, initLon,
                                           &followDir, &myDist);
          }
        }

      } else
        wxMessageBox("Route not found");
    }

    double scale_factor = GetOCPNGUIToolScaleFactor_PlugIn();
    JumpToPosition(initLat, initLon, scale_factor);
    StartDriving();
  }
}

wxString Dlg::StandardPath() {
  wxStandardPathsBase& std_path = wxStandardPathsBase::Get();
  wxString s = wxFileName::GetPathSeparator();

#if defined(__WXMSW__)
  wxString stdPath = std_path.GetConfigDir();
#elif defined(__WXGTK__) || defined(__WXQT__)
  wxString stdPath = std_path.GetUserDataDir();
#elif defined(__WXOSX__)
  wxString stdPath = (std_path.GetUserConfigDir() + s + "opencpn");
#endif

#ifdef __WXOSX__
  // Compatibility with pre-OCPN-4.2; move config dir to
  // ~/Library/Preferences/opencpn if it exists
  wxString oldPath = (std_path.GetUserConfigDir() + s);
  if (wxDirExists(oldPath) && !wxDirExists(stdPath)) {
    wxLogMessage("ShipDriver_pi: moving config dir %s to %s", oldPath, stdPath);
    wxRenameFile(oldPath, stdPath);
  }
#endif

  stdPath += s;  // is this necessary?
  return stdPath;
}

GetRouteDialog::GetRouteDialog(wxWindow* parent, wxWindowID id,
                               const wxString& title, const wxPoint& position,
                               const wxSize& size, long style)
    : wxDialog(parent, id, title, position, size, style) {
  wxPoint p;
  wxSize sz;

  sz.SetWidth(size.GetWidth() - 20);
  sz.SetHeight(size.GetHeight() - 70);

  p.x = 6;
  p.y = 2;

  dialogText = new wxListView(this, wxID_ANY, p, sz,
                              wxLC_NO_HEADER | wxLC_REPORT | wxLC_SINGLE_SEL,
                              wxDefaultValidator, wxT(""));
  wxFont pVLFont(wxFontInfo(12).FaceName("Arial"));
  dialogText->SetFont(pVLFont);

  auto sizerlist = new wxBoxSizer(wxVERTICAL);
  sizerlist->Add(-1, -1, 100, wxEXPAND);
  sizerlist->Add(dialogText);

  auto sizer = new wxBoxSizer(wxHORIZONTAL);
  auto flags = wxSizerFlags().Bottom().Border();
  sizer->Add(1, 1, 100, wxEXPAND);  // Expanding spacer
  auto cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
  sizer->Add(cancel, flags);
  auto m_ok = new wxButton(this, wxID_OK, _("OK"));
  m_ok->Enable(true);
  sizer->Add(m_ok, flags);
  sizerlist->Add(sizer);
  SetSizer(sizerlist);
  Fit();
  SetFocus();
};
