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


#include "ShipDrivergui_impl.h"
#include <wx/progdlg.h>
#include <wx/wx.h>
#include "ShipDriver_pi.h"

#include "folder.xpm"
#include <windows.h>
#include <stdio.h> 
#include <wx/timer.h>

#define BUFSIZE 0x10000

Dlg::Dlg(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : ShipDriverBase(parent, id, title, pos, size, style)
{
	this->Fit();
	dbg = false; //for debug output set to true
	initLat = 0;
	initLon = 0;
}

void Dlg::OnTimer(wxTimerEvent& event){
  Notify();
}

void Dlg::SetNextStep(double inLat, double inLon, double inDir, double inSpd, double &outLat, double &outLon){
	PositionBearingDistanceMercator_Plugin(inLat, inLon, inDir, inSpd, &stepLat, &stepLon);
}

void Dlg::OnStart(wxCommandEvent& event) {

	if (initLat == 0.0){
		wxMessageBox(_("Please right-click and choose vessel start position"));
		return;
	}
	initSpd = 0; // 5 knots
	initDir = m_SliderCourse->GetValue();
	myDir = initDir;
	dt = dt.Now();
	GLL = createGLLSentence(dt, initLat, initLon, initSpd/3600, initDir);
	VTG = createVTGSentence(initSpd, initDir);
	m_interval = 1000;
	m_Timer->Start(m_interval, wxTIMER_CONTINUOUS); // start timer
	return;
}

void Dlg::OnStop(wxCommandEvent& event) {
	
	if (m_Timer->IsRunning()) m_Timer->Stop();

	m_SliderSpeed->SetValue(0);
    m_SliderRudder->SetValue(30);

	m_interval = m_Timer->GetInterval();
	m_bUseSetTime = false;
	m_bUseStop = true;
}

void Dlg::OnMidships(wxCommandEvent& event){
	m_SliderRudder->SetValue(30);
}

void Dlg::OnClose(wxCloseEvent& event)
{	
	if (m_Timer->IsRunning()) m_Timer->Stop();
	plugin->OnShipDriverDialogClose();
}

void Dlg::Notify()
{
	initSpd = m_SliderSpeed->GetValue();
	initRudder = m_SliderRudder->GetValue();

	double myRudder = initRudder - 30;
	if (myRudder < 0){
		initRudder -= 30;
		myRudder = abs(initRudder);
		myDir -= myRudder;
		double myPortRudder = 30 - abs(myRudder);
		m_gaugeRudderPort->SetValue(myPortRudder);
		m_gaugeRudderStbd->SetValue(0);
	}
	else if (myRudder >= 0){
		initRudder -= 30;
		myDir += initRudder;
		m_gaugeRudderStbd->SetValue(myRudder);
		m_gaugeRudderPort->SetValue(0);
	}

	if (myDir < 0){
		myDir += 360;
	}
	else if (myDir > 360){
		myDir -= 360;
	}

	m_SliderCourse->SetValue(myDir);

	SetNextStep(initLat, initLon, myDir, initSpd/3600, stepLat, stepLon);   

	int ss = 1;
	wxTimeSpan mySeconds = wxTimeSpan::Seconds(ss);
	wxDateTime mdt = dt.Add(mySeconds);

	GLL = createGLLSentence(mdt, initLat, initLon, initSpd, myDir);
	VTG = createVTGSentence(initSpd, myDir);
	
	PushNMEABuffer(GLL + _T("\r\n"));
	PushNMEABuffer(VTG + _T("\r\n"));

	initLat = stepLat;
	initLon = stepLon;

	dt = mdt;	
}

void Dlg::OnSliderUpdated(wxCommandEvent& event)
{
	initSpd = m_SliderSpeed->GetValue();
	initDir = m_SliderCourse->GetValue();
}

void Dlg::SetInterval(int interval)
{
	m_interval = interval;
	if (m_Timer->IsRunning()) // Timer started?
		m_Timer->Start(m_interval, wxTIMER_CONTINUOUS); // restart timer with new interval
}

wxString Dlg::createRMCSentence(wxDateTime myDateTime, double myLat, double myLon, double mySpd, double myDir){
	//$GPRMC, 110858.989, A, 4549.9135, N, 00612.2671, E, 003.7, 207.5, 050513, , , A * 60
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
	wxString nC = _T(",");
	wxString nA = _T("A,");
	nRMC = _T("GPRMC,");
	nValid = _T("A,A");
	wxString ndlr = _T("$");
	wxString nast = _T("*");

	nTime = DateTimeToTimeString(myDateTime);
	nNS = LatitudeToString(myLat);
	nEW = LongitudeToString(myLon);
	nSpd = wxString::Format(_T("%2.1f"),mySpd);
	nDir = wxString::Format(_T("%3.0f"), myDir);
	nDate = DateTimeToDateString(myDateTime);

	nForCheckSum = nRMC + nTime + nC + nNS + nEW + nSpd + nC + nDir + nC + nDate + _T(",,,A");
	nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
	return nFinal;
}

wxString Dlg::createGLLSentence(wxDateTime myDateTime, double myLat, double myLon, double mySpd, double myDir){
	
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
	wxString nC = _T(",");
	wxString nA = _T("A,");
	nGLL = _T("IIGLL,");
	nValid = _T("A,A");
	wxString ndlr = _T("$");
	wxString nast = _T("*");

	nTime = DateTimeToTimeString(myDateTime);
	nNS = LatitudeToString(myLat);
	nEW = LongitudeToString(myLon);
	nSpd = wxString::Format(_T("%2.1f"), mySpd);
	nDir = wxString::Format(_T("%3.0f"), myDir);
	nDate = DateTimeToDateString(myDateTime);

	nForCheckSum = nGLL + nNS + nEW + nTime + _T(",A");
	nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
	return nFinal;
}

wxString Dlg::createVTGSentence(double mySpd, double myDir){
	//$GPVTG, 054.7, T, 034.4, M, 005.5, N, 010.2, K * 48
	//$IIVTG, 307., T, , M, 08.5, N, 15.8, K, A * 2F
	wxString nSpd;
	wxString nDir;
	wxString nTime;
	wxString nDate;
	wxString nValid;
	wxString nForCheckSum;
	wxString nFinal;
	wxString nC = _T(",");
	wxString nA = _T("A");
	wxString nT = _T("T,");
	wxString nM = _T("M,");
	wxString nN = _T("N,");
	wxString nK = _T("K,");

	wxString nVTG = _T("IIVTG,");
	nValid = _T("A,A");
	wxString ndlr = _T("$");
	wxString nast = _T("*");

	nSpd = wxString::Format(_T("%2.1f"), mySpd);
	nDir = wxString::Format(_T("%3.0f"), myDir);

	nForCheckSum = nVTG + nDir + nC + nT + nC + nM + nSpd + nN + nC + nC + nA;

	nFinal = ndlr + nForCheckSum + nast + makeCheckSum(nForCheckSum);
	return nFinal;
}


wxString Dlg::makeCheckSum(wxString mySentence){
	int i;
	unsigned char XOR;

	wxString s(mySentence);
	wxCharBuffer buffer = s.ToUTF8();
	char *Buff = buffer.data();	// data() returns const char *
	unsigned long iLen = strlen(Buff);
	for (XOR = 0, i = 0; i < iLen; i++)
		XOR ^= (unsigned char)Buff[i];
	stringstream tmpss;
	tmpss << hex << (int)XOR << endl;
	wxString mystr = tmpss.str();
	return mystr;
}

double StringToLatitude(wxString mLat) {

	//495054
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

	wxString singlezero = _T("0");
	wxString mDegLat;

	int degLat = abs((int)mLat);
	wxString finalDegLat = wxString::Format(_T("%i"), degLat);

	int myL = finalDegLat.length();
	switch (myL){
		case(1) : {
			mDegLat = singlezero + finalDegLat;
			break;
		}
		case(2) : {
			mDegLat = finalDegLat;
			break;
		}
	}

	double minLat = abs(mLat) - degLat;
	double decLat = minLat * 60;

	wxString returnLat;

	if (mLat >= 0){
		if (decLat < 10){
			returnLat = mDegLat + _T("0") + wxString::Format(_T("%.2f"), decLat) + _T(",N,");
		}
		else {
			returnLat = mDegLat + wxString::Format(_T("%.2f"), decLat) + _T(",N,");
		}

	}
	else if (mLat < 0) {
		if (decLat < 10){
			returnLat = mDegLat + _T("0") + wxString::Format(_T("%.2f"), decLat) + _T(",S,");
		}
		else {
			returnLat = mDegLat + wxString::Format(_T("%.2f"), decLat) + _T(",S,");
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

	if (m_len == 7)
	{
		mBitLon = mLon(0, 3);
	}

	if (m_len == 6)
	{
		mBitLon = mLon(0, 2);
	}

	if (m_len == 5)
	{
		mBitLon = mLon(0, 1);
	}

	if (m_len == 4)
	{
		mBitLon = "00.00";
	}

	if (mBitLon == "-")
	{
		value1 = -0.00001;
	}
	else
	{
		mBitLon.ToDouble(&value1);
	}

	mDecLon = mLon(mLon.length() - 4, mLon.length());
	mDecLon.ToDouble(&decValue1);

	if (value1 < 0)
	{
		returnLon = value1 - decValue1 / 100 / 60;
	}
	else
	{
		returnLon = value1 + decValue1 / 100 / 60;
	}

	return returnLon;
}

wxString Dlg::LongitudeToString(double mLon) {

	wxString mDecLon;
	wxString mDegLon;
	double decValue;
	wxString returnLon;
	wxString doublezero = _T("00");
	wxString singlezero = _T("0");
	
	int degLon = abs((int)mLon);
	wxString inLon = wxString::Format(_T("%i"), degLon);
	
	int myL = inLon.length();
	switch (myL){
		case(1) : {
			mDegLon = doublezero + inLon;
			break;
		}
		case(2) : {
			mDegLon = singlezero + inLon;
			break;
		}
		case(3) : {
			mDegLon = inLon;
			break;
		}
	}
	decValue = abs(mLon) - degLon;
	double decLon = decValue * 60;

	if (mLon >= 0){
		if (decLon < 10){
			returnLon = mDegLon + _T("0") + wxString::Format(_T("%.2f"), decLon) + _T(",E,");
		}
		else {
			returnLon = mDegLon + wxString::Format(_T("%.2f"), decLon) + _T(",E,");
		}

	} else  {
		if (decLon < 10){
			returnLon = mDegLon + _T("0") + wxString::Format(_T("%.2f"), decLon) + _T(",W,");
		}
		else {
			returnLon = mDegLon + wxString::Format(_T("%.2f"), decLon) + _T(",W,");
		}
	}
	return returnLon;
}

wxString Dlg::DateTimeToTimeString(wxDateTime myDT) {
	wxString sHours, sMinutes, sSecs;
	sHours = myDT.Format(_T("%H"));
	sMinutes = myDT.Format(_T("%M"));
	sSecs = myDT.Format(_T("%S"));
	wxString dtss = sHours + sMinutes + sSecs;
	return dtss;
}

wxString Dlg::DateTimeToDateString(wxDateTime myDT) {

	wxString sDay, sMonth, sYear;
	sDay = myDT.Format(_T("%d"));
	sMonth = myDT.Format(_T("%m"));
	sYear = myDT.Format(_T("%y"));

	return sDay + sMonth + sYear;
}

void Dlg::OnContextMenu(double m_lat, double m_lon){

	initLat = m_lat;
	initLon = m_lon;
}
