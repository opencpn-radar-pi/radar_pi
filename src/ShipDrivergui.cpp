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
	fgSizer2 = new wxFlexGridSizer( 4, 0, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxVERTICAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_gaugeRudderPort = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( 150,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderPort->SetValue( 0 ); 
	m_gaugeRudderPort->SetForegroundColour( wxColour( 255, 0, 0 ) );
	m_gaugeRudderPort->SetBackgroundColour( wxColour( 255, 0, 0 ) );
	
	fgSizer8->Add( m_gaugeRudderPort, 0, 0, 0 );
	
	m_gaugeRudderStbd = new wxGauge( this, wxID_ANY, 30, wxDefaultPosition, wxSize( 150,-1 ), wxGA_HORIZONTAL );
	m_gaugeRudderStbd->SetValue( 0 ); 
	m_gaugeRudderStbd->SetForegroundColour( wxColour( 0, 255, 0 ) );
	m_gaugeRudderStbd->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	
	fgSizer8->Add( m_gaugeRudderStbd, 0, 0, 0 );
	
	
	fgSizer2->Add( fgSizer8, 100, wxEXPAND, 0 );
	
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_SliderRudder = new wxSlider( this, wxID_ANY, 30, 0, 60, wxDefaultPosition, wxSize( 300,-1 ), wxSL_HORIZONTAL );
	m_SliderRudder->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_SliderRudder->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderRudder->SetToolTip( wxT("Tiller control") );
	
	fgSizer9->Add( m_SliderRudder, 0, wxALIGN_LEFT, 5 );
	
	m_staticTextKnots = new wxStaticText( this, wxID_ANY, wxT("         Knots"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKnots->Wrap( -1 );
	m_staticTextKnots->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer9->Add( m_staticTextKnots, 0, 0, 0 );
	
	
	fgSizer2->Add( fgSizer9, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	wxFlexGridSizer* fgSizer81;
	fgSizer81 = new wxFlexGridSizer( 6, 3, 0, 0 );
	fgSizer81->SetFlexibleDirection( wxBOTH );
	fgSizer81->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_textCtrlRudderPort = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE );
	m_textCtrlRudderPort->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer12->Add( m_textCtrlRudderPort, 0, wxALIGN_RIGHT, 0 );
	
	m_buttonMid = new wxButton( this, wxID_ANY, wxT("|"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonMid->SetFont( wxFont( 11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_buttonMid->SetBackgroundColour( wxColour( 250, 203, 107 ) );
	m_buttonMid->SetToolTip( wxT("Midships") );
	
	fgSizer12->Add( m_buttonMid, 0, wxALIGN_LEFT, 0 );
	
	m_textCtrlRudderStbd = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE );
	m_textCtrlRudderStbd->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer12->Add( m_textCtrlRudderStbd, 0, 0, 0 );
	
	
	fgSizer12->Add( 0, 0, 1, wxEXPAND, 0 );
	
	
	fgSizer12->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpPlay = new wxButton( this, wxID_ANY, wxT("Start"), wxDefaultPosition, wxSize( 140,60 ), 0 );
	m_bpPlay->SetFont( wxFont( 14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_bpPlay->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	
	fgSizer12->Add( m_bpPlay, 0, wxALIGN_LEFT|wxALL, 0 );
	
	
	fgSizer12->Add( 0, 0, 1, wxEXPAND, 0 );
	
	
	fgSizer12->Add( 0, 0, 1, wxEXPAND, 0 );
	
	m_bpStop = new wxButton( this, wxID_ANY, wxT("Emerg. Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bpStop->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_bpStop->SetBackgroundColour( wxColour( 255, 0, 0 ) );
	m_bpStop->SetMinSize( wxSize( 140,30 ) );
	
	fgSizer12->Add( m_bpStop, 0, wxALIGN_LEFT|wxALL, 0 );
	
	m_staticTextHeading = new wxStaticText( this, wxID_ANY, wxT("Heading:  "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeading->Wrap( -1 );
	m_staticTextHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer12->Add( m_staticTextHeading, 0, wxALIGN_RIGHT, 0 );
	
	m_stHeading = new wxStaticText( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHeading->Wrap( -1 );
	m_stHeading->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
	fgSizer12->Add( m_stHeading, 0, 0, 5 );
	
	
	fgSizer81->Add( fgSizer12, 1, wxEXPAND, 5 );
	
	
	fgSizer5->Add( fgSizer81, 1, wxEXPAND, 5 );
	
	
	fgSizer3->Add( fgSizer5, 1, wxEXPAND, 0 );
	
	m_SliderSpeed = new wxSlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize( 90,200 ), wxSL_AUTOTICKS|wxSL_INVERSE|wxSL_LABELS|wxSL_LEFT|wxSL_VERTICAL );
	m_SliderSpeed->SetFont( wxFont( 9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_SliderSpeed->SetForegroundColour( wxColour( 0, 0, 0 ) );
	m_SliderSpeed->SetBackgroundColour( wxColour( 201, 201, 201 ) );
	m_SliderSpeed->SetToolTip( wxT("Engine control") );
	
	fgSizer3->Add( m_SliderSpeed, 0, wxALL, 0 );
	
	
	fgSizer2->Add( fgSizer3, 1, wxEXPAND, 0 );
	
	wxFlexGridSizer* fgSizer82;
	fgSizer82 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer82->SetFlexibleDirection( wxVERTICAL );
	fgSizer82->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_buttonStandby = new wxButton( this, wxID_ANY, wxT("Standby"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonStandby->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	m_buttonStandby->SetBackgroundColour( wxColour( 0, 255, 0 ) );
	
	fgSizer82->Add( m_buttonStandby, 0, wxALL, 0 );
	
	m_buttonAuto = new wxButton( this, wxID_ANY, wxT("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAuto->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial") ) );
	
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
	
	
	fgSizer2->Add( fgSizer82, 1, wxEXPAND, 5 );
	
	
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
