///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ShipDrivergui.h"

///////////////////////////////////////////////////////////////////////////

ShipDriverBase::ShipDriverBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	this->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	wxFlexGridSizer* fgSizer14;
	fgSizer14 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer14->SetFlexibleDirection( wxBOTH );
	fgSizer14->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_gaugeRudderPort = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( -1,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderPort->SetValue( 0 ); 
	m_gaugeRudderPort->SetForegroundColour( wxColour( 255, 0, 0 ) );
	m_gaugeRudderPort->SetBackgroundColour( wxColour( 255, 0, 0 ) );
	
	fgSizer14->Add( m_gaugeRudderPort, 0, wxALIGN_RIGHT, 0 );
	
	m_gaugeRudderStbd = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( -1,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderStbd->SetValue( 0 ); 
	m_gaugeRudderStbd->SetForegroundColour( wxColour( 0, 255, 0 ) );
	m_gaugeRudderStbd->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	
	fgSizer14->Add( m_gaugeRudderStbd, 0, 0, 0 );
	
	
	fgSizer8->Add( fgSizer14, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer14->Add( fgSizer8, 0, wxALIGN_CENTER_HORIZONTAL, 0 );
	
	
	fgSizer2->Add( bSizer14, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
	m_SliderRudder = new wxSlider( this, wxID_ANY, 30, 0, 60, wxDefaultPosition, wxSize( 300,-1 ), wxSL_HORIZONTAL );
	m_SliderRudder->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_SliderRudder->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderRudder->SetToolTip( _("Tiller Control") );
	
	bSizer13->Add( m_SliderRudder, 1, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	fgSizer2->Add( bSizer13, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer17;
	fgSizer17 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer17->SetFlexibleDirection( wxBOTH );
	fgSizer17->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxFlexGridSizer* fgSizer21;
	fgSizer21 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer21->SetFlexibleDirection( wxBOTH );
	fgSizer21->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_textCtrlRudderPort = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE );
	m_textCtrlRudderPort->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer21->Add( m_textCtrlRudderPort, 0, wxALIGN_RIGHT, 0 );
	
	m_buttonMid = new wxButton( this, wxID_ANY, wxT("|"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMid->SetFont( wxFont( 11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_buttonMid->SetBackgroundColour( wxColour( 250, 203, 107 ) );
	m_buttonMid->SetToolTip( _("Midships") );
	
	fgSizer21->Add( m_buttonMid, 0, wxALIGN_CENTER, 0 );
	
	m_textCtrlRudderStbd = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE );
	m_textCtrlRudderStbd->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer21->Add( m_textCtrlRudderStbd, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM, 0 );
	
	
	fgSizer17->Add( fgSizer21, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	
	fgSizer17->Add( bSizer16, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer30;
	fgSizer30 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer30->SetFlexibleDirection( wxBOTH );
	fgSizer30->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bpPlay = new wxButton( this, wxID_ANY, _("Start"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_bpPlay->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_bpPlay->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	
	fgSizer30->Add( m_bpPlay, 1, 0, 0 );
	
	m_bpStop = new wxButton( this, wxID_ANY, _("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bpStop->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_bpStop->SetBackgroundColour( wxColour( 255, 0, 0 ) );
	
	fgSizer30->Add( m_bpStop, 1, 0, 0 );
	
	
	fgSizer17->Add( fgSizer30, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer6->Add( fgSizer17, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer20;
	fgSizer20 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer20->SetFlexibleDirection( wxBOTH );
	fgSizer20->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextHeading = new wxStaticText( this, wxID_ANY, _("  Heading:  "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeading->Wrap( -1 );
	m_staticTextHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer20->Add( m_staticTextHeading, 0, 0, 5 );
	
	m_stHeading = new wxStaticText( this, wxID_ANY, wxT("000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHeading->Wrap( -1 );
	m_stHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer20->Add( m_stHeading, 0, 0, 5 );
	
	m_staticTextKnots = new wxStaticText( this, wxID_ANY, _("             Speed: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKnots->Wrap( -1 );
	m_staticTextKnots->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer20->Add( m_staticTextKnots, 0, 0, 5 );
	
	m_stSpeed = new wxStaticText( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSpeed->Wrap( -1 );
	m_stSpeed->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer20->Add( m_stSpeed, 0, 0, 5 );
	
	m_staticText81 = new wxStaticText( this, wxID_ANY, _("        Kts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	m_staticText81->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer20->Add( m_staticText81, 0, wxALL, 0 );
	
	
	bSizer7->Add( fgSizer20, 1, wxEXPAND|wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer6->Add( bSizer7, 1, wxEXPAND|wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	fgSizer2->Add( bSizer6, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizer101;
	fgSizer101 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer101->SetFlexibleDirection( wxBOTH );
	fgSizer101->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer101->Add( m_staticText7, 0, wxALL, 5 );
	
	m_SliderSpeed = new wxSlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize( 300,-1 ), 0 );
	m_SliderSpeed->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_SliderSpeed->SetForegroundColour( wxColour( 0, 0, 0 ) );
	m_SliderSpeed->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderSpeed->SetToolTip( _("Speed Control") );
	
	fgSizer101->Add( m_SliderSpeed, 100, wxEXPAND, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, wxT("100"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	m_staticText8->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer101->Add( m_staticText8, 0, wxALL, 5 );
	
	
	fgSizer2->Add( fgSizer101, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxVERTICAL );
	
	
	fgSizer2->Add( bSizer91, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer82;
	fgSizer82 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer82->SetFlexibleDirection( wxVERTICAL );
	fgSizer82->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_buttonStandby = new wxButton( this, wxID_ANY, _("Standby"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonStandby->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_buttonStandby->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	m_buttonStandby->SetToolTip( _("Hand Steering") );
	
	fgSizer82->Add( m_buttonStandby, 0, wxALL, 0 );
	
	m_buttonAuto = new wxButton( this, wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAuto->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_buttonAuto->SetBackgroundColour( wxColour( 255, 255, 255 ) );
	m_buttonAuto->SetToolTip( _("Autopilot Control") );
	
	fgSizer82->Add( m_buttonAuto, 0, wxALL, 0 );
	
	m_button7 = new wxButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer82->Add( m_button7, 0, wxALL, 0 );
	
	m_button8 = new wxButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer82->Add( m_button8, 0, wxALL, 0 );
	
	m_buttonMinus1 = new wxButton( this, wxID_ANY, wxT("-1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMinus1->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer82->Add( m_buttonMinus1, 0, wxALL, 0 );
	
	m_buttonMinus10 = new wxButton( this, wxID_ANY, wxT("-10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMinus10->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer82->Add( m_buttonMinus10, 0, wxALL, 0 );
	
	m_buttonPlus10 = new wxButton( this, wxID_ANY, wxT("+10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPlus10->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer82->Add( m_buttonPlus10, 0, wxALL, 0 );
	
	m_buttonPlus1 = new wxButton( this, wxID_ANY, wxT("+1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPlus1->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer82->Add( m_buttonPlus1, 0, wxALL, 0 );
	
	
	bSizer9->Add( fgSizer82, 1, wxEXPAND, 5 );
	
	
	fgSizer2->Add( bSizer9, 1, wxEXPAND|wxALIGN_BOTTOM, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 1, 1, 0, 0 );
	fgSizer10->SetFlexibleDirection( wxBOTH );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	bSizer10->Add( fgSizer10, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	fgSizer2->Add( bSizer10, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( fgSizer2 );
	this->Layout();
	m_timer1.SetOwner( this, wxID_ANY );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonMid->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	m_buttonStandby->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStandby ), NULL, this );
	m_buttonAuto->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnAuto ), NULL, this );
	m_buttonMinus1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus1 ), NULL, this );
	m_buttonMinus10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus10 ), NULL, this );
	m_buttonPlus10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus10 ), NULL, this );
	m_buttonPlus1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus1 ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( ShipDriverBase::OnTimer ) );
}

ShipDriverBase::~ShipDriverBase()
{
	// Disconnect Events
	m_buttonMid->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	m_buttonStandby->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStandby ), NULL, this );
	m_buttonAuto->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnAuto ), NULL, this );
	m_buttonMinus1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus1 ), NULL, this );
	m_buttonMinus10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMinus10 ), NULL, this );
	m_buttonPlus10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus10 ), NULL, this );
	m_buttonPlus1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnPlus1 ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( ShipDriverBase::OnTimer ) );
	
}
