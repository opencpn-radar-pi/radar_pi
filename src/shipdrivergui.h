///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/tglbtn.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/timer.h>
#include <wx/dialog.h>
#include <wx/checkbox.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class ShipDriverBase
///////////////////////////////////////////////////////////////////////////////
class ShipDriverBase : public wxDialog
{
	private:

	protected:
		wxTextCtrl* m_textCtrlRudderPort;
		wxButton* m_buttonMid;
		wxTextCtrl* m_textCtrlRudderStbd;
		wxButton* m_bpPlay;
		wxButton* m_bpStop;
		wxToggleButton* m_buttonPause;
		wxStaticText* m_staticTextHeading;
		wxStaticText* m_staticTextKnots;
		wxStaticText* m_staticText81;
		wxStaticText* m_staticText7;
		wxStaticText* m_staticText8;
		wxButton* m_buttonStandby;
		wxButton* m_buttonAuto;
		wxButton* m_button7;
		wxButton* m_buttonWind;
		wxButton* m_buttonMinus1;
		wxButton* m_buttonMinus10;
		wxButton* m_buttonPlus10;
		wxButton* m_buttonPlus1;
		wxToggleButton* m_buttonSART;
		wxToggleButton* m_buttonMOB;
		wxToggleButton* m_buttonEPIRB;
		wxToggleButton* m_buttonDistressAlert;
		wxToggleButton* m_buttonDistressCancel;
		wxToggleButton* m_buttonDistressRelay;
		wxToggleButton* m_buttonRelayCancel;
		wxToggleButton* m_buttonCollision;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnDLeftClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnMidships( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStart( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStop( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPause( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStandby( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFollow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWind( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMinus1( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMinus10( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlus10( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlus1( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSART( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMOB( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEPIRB( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDistressAlert( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDistressCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDistressRelay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRelayCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCollision( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTimer( wxTimerEvent& event ) { event.Skip(); }


	public:
		wxGauge* m_gaugeRudderPort;
		wxGauge* m_gaugeRudderStbd;
		wxSlider* m_SliderRudder;
		wxStaticText* m_stHeading;
		wxStaticText* m_stSpeed;
		wxSlider* m_SliderSpeed;
		wxTextCtrl* m_textCtrlSART;
		wxTimer m_timer1;

		ShipDriverBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~ShipDriverBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class shipdriverPreferences
///////////////////////////////////////////////////////////////////////////////
class shipdriverPreferences : public wxDialog
{
	private:

	protected:
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

	public:
		wxCheckBox* m_cbTransmitAis;
		wxCheckBox* m_cbAisToFile;
		wxTextCtrl* m_textCtrlMMSI;
		wxCheckBox* m_cbNMEAToFile;

		shipdriverPreferences( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Preferences"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxRESIZE_BORDER );

		~shipdriverPreferences();

};

