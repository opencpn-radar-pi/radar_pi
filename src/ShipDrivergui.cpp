///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ShipDrivergui.h"

///////////////////////////////////////////////////////////////////////////

ShipDriverBase::ShipDriverBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxSize( -1,-1 ) );
	this->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	this->SetBackgroundColour( wxColour( 201, 201, 201 ) );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer92;
	bSizer92 = new wxBoxSizer( wxHORIZONTAL );

	m_gaugeRudderPort = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( -1,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderPort->SetValue( 0 );
	m_gaugeRudderPort->SetForegroundColour( wxColour( 255, 0, 0 ) );
	m_gaugeRudderPort->SetBackgroundColour( wxColour( 255, 0, 0 ) );
  m_gaugeRudderPort->SetToolTip(_("Rudder Port"));

	bSizer92->Add( m_gaugeRudderPort, 1, wxEXPAND|wxTOP, 5 );

	m_gaugeRudderStbd = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( -1,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderStbd->SetValue( 0 );
	m_gaugeRudderStbd->SetForegroundColour( wxColour( 0, 255, 0 ) );
	m_gaugeRudderStbd->SetBackgroundColour( wxColour( 0, 255, 0 ) );
  m_gaugeRudderStbd->SetToolTip(_("Rudder Starboard"));

	bSizer92->Add( m_gaugeRudderStbd, 1, wxEXPAND|wxTOP, 5 );


	bSizer14->Add( bSizer92, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	m_SliderRudder = new wxSlider( this, wxID_ANY, 30, 0, 60, wxDefaultPosition, wxSize( 300,-1 ), wxSL_AUTOTICKS|wxSL_BOTH|wxSL_HORIZONTAL );
	m_SliderRudder->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_SliderRudder->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderRudder->SetToolTip( _("Tiller Control") );

	bSizer13->Add( m_SliderRudder, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer14->Add( bSizer13, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );


	bSizer131->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlRudderPort = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER );
	m_textCtrlRudderPort->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
  m_textCtrlRudderPort->SetToolTip(_("Rudder Port"));

	bSizer131->Add( m_textCtrlRudderPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

	m_buttonMid = new wxButton( this, wxID_ANY, _("|"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMid->SetFont( wxFont( 11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonMid->SetBackgroundColour( wxColour( 250, 203, 107 ) );
	m_buttonMid->SetToolTip( _("Midships") );

	bSizer131->Add( m_buttonMid, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

	m_textCtrlRudderStbd = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER );
	m_textCtrlRudderStbd->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
  m_textCtrlRudderStbd->SetToolTip( _("Rudder Starboard") );

	bSizer131->Add( m_textCtrlRudderStbd, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );


	bSizer131->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer6->Add( bSizer131, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );


	bSizer16->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bpPlay = new wxButton( this, wxID_ANY, _("Start"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_bpPlay->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_bpPlay->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_bpPlay->SetBackgroundColour( wxColour( 0, 255, 0 ) );

	bSizer16->Add( m_bpPlay, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

	m_bpStop = new wxButton( this, wxID_ANY, _("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bpStop->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_bpStop->SetBackgroundColour( wxColour( 255, 0, 0 ) );

	bSizer16->Add( m_bpStop, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );


	bSizer16->Add( 25, 0, 0, wxEXPAND, 5 );

	m_buttonPause = new wxToggleButton( this, wxID_ANY, _("Pause"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPause->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonPause->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonPause->SetToolTip( _("MOB") );

	bSizer16->Add( m_buttonPause, 0, wxALL, 5 );


	bSizer16->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizer6->Add( bSizer16, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );


	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextHeading = new wxStaticText( this, wxID_ANY, _("  Heading:  "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeading->Wrap( -1 );
	m_staticTextHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer7->Add( m_staticTextHeading, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stHeading = new wxStaticText( this, wxID_ANY, _("000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHeading->Wrap( -1 );
	m_stHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer7->Add( m_stHeading, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextKnots = new wxStaticText( this, wxID_ANY, _("       Speed: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKnots->Wrap( -1 );
	m_staticTextKnots->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer7->Add( m_staticTextKnots, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stSpeed = new wxStaticText( this, wxID_ANY, _("00.0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSpeed->Wrap( -1 );
	m_stSpeed->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer7->Add( m_stSpeed, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText81 = new wxStaticText( this, wxID_ANY, _("        Kts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	m_staticText81->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer7->Add( m_staticText81, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );


	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer6->Add( bSizer7, 1, wxEXPAND, 5 );


	bSizer14->Add( bSizer6, 1, wxEXPAND, 5 );


	bSizer10->Add( bSizer14, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer12->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_SliderSpeed = new wxSlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_SliderSpeed->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_SliderSpeed->SetForegroundColour( wxColour( 0, 0, 0 ) );
	m_SliderSpeed->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderSpeed->SetToolTip( _("Speed Control") );

	bSizer12->Add( m_SliderSpeed, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _("100"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	m_staticText8->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );

	bSizer12->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer10->Add( bSizer12, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer82;
	fgSizer82 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer82->AddGrowableCol( 0 );
	fgSizer82->AddGrowableCol( 1 );
	fgSizer82->AddGrowableCol( 2 );
	fgSizer82->AddGrowableCol( 3 );
	fgSizer82->AddGrowableRow( 0 );
	fgSizer82->AddGrowableRow( 1 );
	fgSizer82->SetFlexibleDirection( wxVERTICAL );
	fgSizer82->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_buttonStandby = new wxButton( this, wxID_ANY, _("Stby"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonStandby->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonStandby->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonStandby->SetToolTip( _("Hand Steering") );
	m_buttonStandby->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonStandby, 1, wxALL|wxEXPAND, 2 );

	m_buttonAuto = new wxButton( this, wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAuto->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonAuto->SetBackgroundColour( wxColour( 255, 255, 255 ) );
	m_buttonAuto->SetToolTip( _("Autopilot Control") );
	m_buttonAuto->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonAuto, 1, wxALL|wxEXPAND, 2 );

	m_button7 = new wxButton( this, wxID_ANY, _("Follow"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button7->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_button7->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_button7, 1, wxALL|wxEXPAND, 2 );

	m_buttonWind = new wxButton( this, wxID_ANY, _("Sail"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonWind->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonWind->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonWind->SetToolTip( _("Using Sail") );
	m_buttonWind->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonWind, 1, wxALL|wxEXPAND, 2 );

	m_buttonMinus1 = new wxButton( this, wxID_ANY, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMinus1->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonMinus1->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonMinus1, 1, wxALL|wxEXPAND, 2 );

	m_buttonMinus10 = new wxButton( this, wxID_ANY, _("-10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMinus10->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonMinus10->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonMinus10, 1, wxALL|wxEXPAND, 2 );

	m_buttonPlus10 = new wxButton( this, wxID_ANY, _("+10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPlus10->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonPlus10->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonPlus10, 1, wxALL|wxEXPAND, 2 );

	m_buttonPlus1 = new wxButton( this, wxID_ANY, _("+1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPlus1->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonPlus1->SetMinSize( wxSize( 50,-1 ) );

	fgSizer82->Add( m_buttonPlus1, 1, wxALL|wxEXPAND, 2 );


	bSizer10->Add( fgSizer82, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Alarms") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 1, 3, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableCol( 2 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_buttonSART = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("SART"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSART->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonSART->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonSART->SetToolTip( _("SART") );

	fgSizer3->Add( m_buttonSART, 0, wxALL|wxEXPAND, 5 );

	m_buttonMOB = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("MOB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMOB->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonMOB->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonMOB->SetToolTip( _("MOB") );

	fgSizer3->Add( m_buttonMOB, 0, wxALL|wxEXPAND, 5 );

	m_buttonEPIRB = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("EPIRB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonEPIRB->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonEPIRB->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonEPIRB->SetToolTip( _("EPIRB") );

	fgSizer3->Add( m_buttonEPIRB, 0, wxALL|wxEXPAND, 5 );


	sbSizer3->Add( fgSizer3, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer(2, wxSize(0,0));
	fgSizer5->AddGrowableCol( 0 );
	fgSizer5->AddGrowableCol( 1 );
	fgSizer5->AddGrowableRow( 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_buttonDistressAlert = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("DSC Distress Alert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonDistressAlert->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonDistressAlert->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonDistressAlert->SetToolTip( _("Distress Alert") );

	fgSizer5->Add( m_buttonDistressAlert, 0, wxALL|wxEXPAND, 5 );

	m_buttonDistressCancel = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("DSC Distress Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonDistressCancel->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonDistressCancel->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonDistressCancel->SetToolTip( _("Distress Cancel") );

	fgSizer5->Add( m_buttonDistressCancel, 0, wxALL|wxEXPAND, 5 );

	m_buttonDistressRelay = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Distress Relay"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonDistressRelay->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonDistressRelay->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonDistressRelay->SetToolTip( _("Distress Relay") );

	fgSizer5->Add( m_buttonDistressRelay, 0, wxALL|wxEXPAND, 5 );

	m_buttonRelayCancel = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Relay Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRelayCancel->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonRelayCancel->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonRelayCancel->SetToolTip( _("Relay Cancel") );

	fgSizer5->Add( m_buttonRelayCancel, 0, wxALL|wxEXPAND, 5 );


	sbSizer3->Add( fgSizer5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxVERTICAL );

	m_buttonCollision = new wxToggleButton( sbSizer3->GetStaticBox(), wxID_ANY, _("AIS Collision Warning"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonCollision->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ) );
	m_buttonCollision->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonCollision->SetToolTip( _("MOB") );

	bSizer111->Add( m_buttonCollision, 0, wxALIGN_CENTER|wxALL, 5 );


	sbSizer3->Add( bSizer111, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_textCtrlSART = new wxTextCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_textCtrlSART, 0, wxALL|wxEXPAND, 5 );


	sbSizer3->Add( bSizer11, 0, wxEXPAND, 5 );


	bSizer10->Add( sbSizer3, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );
	m_timer1.SetOwner( this, wxID_ANY );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ShipDriverBase::OnClose ) );
	this->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( ShipDriverBase::OnDLeftClick ) );
	m_buttonMid->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	m_buttonPause->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPause ), NULL, this );
	m_buttonStandby->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStandby ), NULL, this );
	m_buttonAuto->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnAuto ), NULL, this );
	m_button7->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnFollow ), NULL, this );
	m_buttonWind->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnWind ), NULL, this );
	m_buttonMinus1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus1 ), NULL, this );
	m_buttonMinus10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus10 ), NULL, this );
	m_buttonPlus10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus10 ), NULL, this );
	m_buttonPlus1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus1 ), NULL, this );
	m_buttonSART->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnSART ), NULL, this );
	m_buttonMOB->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMOB ), NULL, this );
	m_buttonEPIRB->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnEPIRB ), NULL, this );
	m_buttonDistressAlert->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressAlert ), NULL, this );
	m_buttonDistressCancel->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressCancel ), NULL, this );
	m_buttonDistressRelay->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressRelay ), NULL, this );
	m_buttonRelayCancel->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnRelayCancel ), NULL, this );
	m_buttonCollision->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnCollision ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( ShipDriverBase::OnTimer ) );
}

ShipDriverBase::~ShipDriverBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ShipDriverBase::OnClose ) );
	this->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( ShipDriverBase::OnDLeftClick ) );
	m_buttonMid->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	m_buttonPause->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPause ), NULL, this );
	m_buttonStandby->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStandby ), NULL, this );
	m_buttonAuto->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnAuto ), NULL, this );
	m_button7->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnFollow ), NULL, this );
	m_buttonWind->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnWind ), NULL, this );
	m_buttonMinus1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus1 ), NULL, this );
	m_buttonMinus10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus10 ), NULL, this );
	m_buttonPlus10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus10 ), NULL, this );
	m_buttonPlus1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus1 ), NULL, this );
	m_buttonSART->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnSART ), NULL, this );
	m_buttonMOB->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMOB ), NULL, this );
	m_buttonEPIRB->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnEPIRB ), NULL, this );
	m_buttonDistressAlert->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressAlert ), NULL, this );
	m_buttonDistressCancel->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressCancel ), NULL, this );
	m_buttonDistressRelay->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnDistressRelay ), NULL, this );
	m_buttonRelayCancel->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnRelayCancel ), NULL, this );
	m_buttonCollision->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnCollision ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( ShipDriverBase::OnTimer ) );

}

shipdriverPreferences::shipdriverPreferences( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("AIS") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbTransmitAis = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Transmit"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_cbTransmitAis, 0, wxALL, 5 );

	m_cbAisToFile = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Save to file"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_cbAisToFile, 0, wxALL, 5 );


	sbSizer1->Add( fgSizer2, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( sbSizer1->GetStaticBox(), wxID_ANY, _("MMSI") ), wxVERTICAL );

	m_textCtrlMMSI = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_textCtrlMMSI, 0, wxALL, 5 );


	sbSizer1->Add( sbSizer2, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( sbSizer1->GetStaticBox(), wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( sbSizer1->GetStaticBox(), wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	sbSizer1->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	this->SetSizer( sbSizer1 );
	this->Layout();
	sbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

shipdriverPreferences::~shipdriverPreferences()
{
}
