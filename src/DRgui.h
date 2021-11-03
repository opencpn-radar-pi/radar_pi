///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class m_Dialog
///////////////////////////////////////////////////////////////////////////////
class m_Dialog : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText1511;
		wxStaticText* m_staticText32111112;
		wxStaticText* m_staticText33111111;
		wxStaticText* m_staticText32111111;
		wxStaticText* m_staticText3311111;
		wxStaticText* m_staticText7;
		wxStaticLine* m_staticline1;
		wxButton* m_button3111;
		wxStaticText* m_staticText71;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnPSGPX( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxTextCtrl* m_Speed_PS;
		wxChoice* m_Nship;
		wxTextCtrl* m_Route;

		m_Dialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("DR"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~m_Dialog();

};

