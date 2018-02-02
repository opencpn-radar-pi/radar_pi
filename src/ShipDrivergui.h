///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan 25 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SHIPDRIVERGUI_H__
#define __SHIPDRIVERGUI_H__

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
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/dialog.h>

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
		wxStaticText* m_staticTextHeading;
		wxStaticText* m_staticTextKnots;
		wxStaticText* m_staticText81;
		wxStaticText* m_staticText7;
		wxStaticText* m_staticText8;
		wxButton* m_buttonStandby;
		wxButton* m_buttonAuto;
		wxButton* m_button7;
		wxButton* m_button8;
		wxButton* m_buttonMinus1;
		wxButton* m_buttonMinus10;
		wxButton* m_buttonPlus10;
		wxButton* m_buttonPlus1;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnMidships( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStart( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStop( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStandby( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMinus1( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMinus10( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlus10( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlus1( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTimer( wxTimerEvent& event ) { event.Skip(); }
		
	
	public:
		wxGauge* m_gaugeRudderPort;
		wxGauge* m_gaugeRudderStbd;
		wxSlider* m_SliderRudder;
		wxStaticText* m_stHeading;
		wxStaticText* m_stSpeed;
		wxSlider* m_SliderSpeed;
		wxTimer m_timer1;
		
		ShipDriverBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 330,240 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~ShipDriverBase();
	
};

#endif //__SHIPDRIVERGUI_H__
