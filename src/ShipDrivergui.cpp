///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ShipDrivergui.h"

///////////////////////////////////////////////////////////////////////////

ShipDriverBase::ShipDriverBase(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	this->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	this->SetBackgroundColour(wxColour(201, 201, 201));

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer(3, 0, 0, 0);
	fgSizer2->SetFlexibleDirection(wxVERTICAL);
	fgSizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer(0, 3, 0, 0);
	fgSizer8->SetFlexibleDirection(wxBOTH);
	fgSizer8->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);

	m_gaugeRudderPort = new wxGauge(this, wxID_ANY, 30, wxDefaultPosition, wxSize(150, -1), wxGA_HORIZONTAL);
	m_gaugeRudderPort->SetValue(0);
	m_gaugeRudderPort->SetForegroundColour(wxColour(255, 0, 0));
	m_gaugeRudderPort->SetBackgroundColour(wxColour(255, 0, 0));

	fgSizer8->Add(m_gaugeRudderPort, 0, wxALIGN_RIGHT | wxALL, 0);

	m_gaugeRudderStbd = new wxGauge(this, wxID_ANY, 30, wxDefaultPosition, wxSize(150, -1), wxGA_HORIZONTAL);
	m_gaugeRudderStbd->SetValue(0);
	m_gaugeRudderStbd->SetForegroundColour(wxColour(0, 255, 0));

	fgSizer8->Add(m_gaugeRudderStbd, 0, wxALL, 0);

	m_staticTextKnots = new wxStaticText(this, wxID_ANY, wxT("Knots"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticTextKnots->Wrap(-1);
	m_staticTextKnots->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

	fgSizer8->Add(m_staticTextKnots, 0, 0, 0);


	fgSizer2->Add(fgSizer8, 1, wxEXPAND, 0);

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer(1, 2, 0, 0);
	fgSizer3->SetFlexibleDirection(wxBOTH);
	fgSizer3->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer(wxHORIZONTAL);


	fgSizer3->Add(bSizer1, 1, wxEXPAND, 5);


	fgSizer3->Add(0, 0, 1, wxEXPAND, 5);


	fgSizer3->Add(0, 0, 1, wxEXPAND, 5);


	fgSizer3->Add(0, 0, 1, wxEXPAND, 5);

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer(0, 1, 0, 0);
	fgSizer5->SetFlexibleDirection(wxBOTH);
	fgSizer5->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer(2, 0, 0, 0);

	m_SliderRudder = new wxSlider(this, wxID_ANY, 30, 0, 60, wxDefaultPosition, wxSize(300, -1), wxSL_HORIZONTAL);
	m_SliderRudder->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	m_SliderRudder->SetBackgroundColour(wxColour(201, 201, 201));

	gSizer1->Add(m_SliderRudder, 0, wxALL, 5);

	m_buttonMid = new wxButton(this, wxID_ANY, wxT("^"), wxDefaultPosition, wxDefaultSize, 0);
	m_buttonMid->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	m_buttonMid->SetBackgroundColour(wxColour(250, 203, 107));
	m_buttonMid->SetToolTip(wxT("Midships"));

	gSizer1->Add(m_buttonMid, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);


	fgSizer5->Add(gSizer1, 1, wxEXPAND, 5);

	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer(0, 2, 0, 0);

	gSizer2->SetMinSize(wxSize(-1, 100));

	gSizer2->Add(0, 0, 1, wxEXPAND, 0);

	m_bpPlay = new wxButton(this, wxID_ANY, wxT("Start"), wxDefaultPosition, wxSize(140, 60), 0);
	m_bpPlay->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	m_bpPlay->SetBackgroundColour(wxColour(0, 255, 0));

	gSizer2->Add(m_bpPlay, 0, wxALIGN_RIGHT | wxALL, 0);


	gSizer2->Add(0, 0, 1, wxEXPAND, 0);

	m_bpStop = new wxButton(this, wxID_ANY, wxT("Emerg. Stop"), wxDefaultPosition, wxDefaultSize, 0);
	m_bpStop->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	m_bpStop->SetBackgroundColour(wxColour(255, 0, 0));
	m_bpStop->SetMinSize(wxSize(140, 30));

	gSizer2->Add(m_bpStop, 0, wxALIGN_RIGHT | wxALL, 0);


	fgSizer5->Add(gSizer2, 1, wxEXPAND, 0);

	wxGridSizer* gSizer5;
	gSizer5 = new wxGridSizer(0, 1, 0, 0);

	m_staticTextHeading = new wxStaticText(this, wxID_ANY, wxT("Heading"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticTextHeading->Wrap(-1);
	m_staticTextHeading->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

	gSizer5->Add(m_staticTextHeading, 0, wxALIGN_CENTER | wxALIGN_TOP | wxALL, 0);

	m_SliderCourse = new wxSlider(this, wxID_ANY, 0, 0, 360, wxDefaultPosition, wxSize(300, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	m_SliderCourse->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	m_SliderCourse->SetBackgroundColour(wxColour(250, 203, 107));

	gSizer5->Add(m_SliderCourse, 0, wxALIGN_BOTTOM | wxALIGN_RIGHT | wxALIGN_TOP | wxALL, 0);


	fgSizer5->Add(gSizer5, 1, wxEXPAND, 0);


	fgSizer3->Add(fgSizer5, 1, wxEXPAND, 0);

	m_SliderSpeed = new wxSlider(this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(90, 200), wxSL_AUTOTICKS | wxSL_INVERSE | wxSL_LABELS | wxSL_VERTICAL);
	m_SliderSpeed->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));
	m_SliderSpeed->SetForegroundColour(wxColour(0, 0, 0));
	m_SliderSpeed->SetBackgroundColour(wxColour(201, 201, 201));

	fgSizer3->Add(m_SliderSpeed, 0, wxALL, 0);


	fgSizer2->Add(fgSizer3, 1, wxEXPAND, 0);

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer(4, 0, 0, 0);
	fgSizer4->SetFlexibleDirection(wxBOTH);
	fgSizer4->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);


	fgSizer2->Add(fgSizer4, 1, wxEXPAND, 5);


	this->SetSizer(fgSizer2);
	this->Layout();

	this->Centre(wxBOTH);

	
	// Connect Events
	m_buttonMid->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	this->Connect(wxEVT_TIMER, wxTimerEventHandler(ShipDriverBase::OnTimer), NULL, this);
}

ShipDriverBase::~ShipDriverBase()
{
	// Disconnect Events
	m_buttonMid->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnMidships ), NULL, this );
	m_bpPlay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStart ), NULL, this );
	m_bpStop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ShipDriverBase::OnStop ), NULL, this );
	this->Disconnect(wxEVT_TIMER, wxTimerEventHandler(ShipDriverBase::OnTimer), NULL, this);
	
}
