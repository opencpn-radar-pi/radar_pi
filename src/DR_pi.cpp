/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  DR Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2013 by Mike Rossiter                                *
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "DR_pi.h"
#include "DRgui_impl.h"
#include "DRgui.h"

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new DR_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    DR PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

DR_pi::DR_pi(void *ppimgr)
      :opencpn_plugin_116 (ppimgr)
{
      // Create the PlugIn icons
      initialize_images();

	  wxFileName fn;
	  wxString tmp_path;

	  tmp_path = GetPluginDataDir("DR_pi");
	  fn.SetPath(tmp_path);
	  fn.AppendDir(_T("data"));
	  fn.SetFullName("dr_pi_panel_icon.png");

	  wxString shareLocn = fn.GetFullPath();
	  wxImage panelIcon(shareLocn);
	  
	  if (panelIcon.IsOk())
		  m_panelBitmap = wxBitmap(panelIcon);
	  else
		  wxLogMessage(_T("    DR_pi panel icon NOT loaded"));
	  m_bShowDR = false;
}

DR_pi::~DR_pi(void)
{
     delete _img_DR_pi;
     delete _img_DR;
     
}

int DR_pi::Init(void)
{
      AddLocaleCatalog( _T("opencpn-DR_pi") );

      // Set some default private member parameters
      m_route_dialog_x = 0;
      m_route_dialog_y = 0;
      ::wxDisplaySize(&m_display_width, &m_display_height);

      //    Get a pointer to the opencpn display canvas, to use as a parent for the POI Manager dialog
      m_parent_window = GetOCPNCanvasWindow();

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();

      //    And load the configuration items
      LoadConfig();

      //    This PlugIn needs a toolbar icon, so request its insertion
	if(m_bDRShowIcon)

#ifdef DR_USE_SVG
		m_leftclick_tool_id = InsertPlugInToolSVG(_T("DR"), _svg_dr, _svg_dr, _svg_dr_toggled,
			wxITEM_CHECK, _("DR"), _T(""), NULL, CALCULATOR_TOOL_POSITION, 0, this);
#else
		m_leftclick_tool_id = InsertPlugInTool(_T(""), _img_DR, _img_DR, wxITEM_CHECK,
			_("DR"), _T(""), NULL,
			CALCULATOR_TOOL_POSITION, 0, this);
#endif
    

      m_pDialog = NULL;

      return (WANTS_OVERLAY_CALLBACK |
              WANTS_OPENGL_OVERLAY_CALLBACK |
		  
		      WANTS_CURSOR_LATLON      |
              WANTS_TOOLBAR_CALLBACK    |
              INSTALLS_TOOLBAR_TOOL     |
              WANTS_CONFIG             |
			  WANTS_PLUGIN_MESSAGING

           );
}

bool DR_pi::DeInit(void)
{
      //    Record the dialog position
      if (NULL != m_pDialog)
      {
            //Capture dialog position
            wxPoint p = m_pDialog->GetPosition();
            SetCalculatorDialogX(p.x);
            SetCalculatorDialogY(p.y);
            m_pDialog->Close();
            delete m_pDialog;
            m_pDialog = NULL;

			m_bShowDR = false;
			SetToolbarItemState( m_leftclick_tool_id, m_bShowDR );

      }	
    
    SaveConfig();

    RequestRefresh(m_parent_window); // refresh main window

    return true;
}

int DR_pi::GetAPIVersionMajor()
{
      return atoi(API_VERSION);
}

int DR_pi::GetAPIVersionMinor()
{
      std::string v(API_VERSION);
    size_t dotpos = v.find('.');
    return atoi(v.substr(dotpos + 1).c_str());
}

int DR_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int DR_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *DR_pi::GetPlugInBitmap()
{
      return &m_panelBitmap;
}

wxString DR_pi::GetCommonName()
{
      return _("DR");
}


wxString DR_pi::GetShortDescription()
{
      return _("DR Positions using GPX files");
}

wxString DR_pi::GetLongDescription()
{
      return _("Creates GPX files with\n\
DR Positions");
}

int DR_pi::GetToolbarToolCount(void)
{
      return 1;
}

void DR_pi::SetColorScheme(PI_ColorScheme cs)
{
      if (NULL == m_pDialog)
            return;

      DimeWindow(m_pDialog);
}

void DR_pi::OnToolbarToolCallback(int id)
{
    
	if(NULL == m_pDialog)
      {
            m_pDialog = new Dlg(m_parent_window, this);
            m_pDialog->Move(wxPoint(m_route_dialog_x, m_route_dialog_y));
      }

	  m_pDialog->Fit();
	  //Toggle 
	  m_bShowDR = !m_bShowDR;	  

      //    Toggle dialog? 
      if(m_bShowDR) {
          m_pDialog->Show();         
      } else
          m_pDialog->Hide();
     
      // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
      // to actual status to ensure correct status upon toolbar rebuild
      SetToolbarItemState( m_leftclick_tool_id, m_bShowDR );

      RequestRefresh(m_parent_window); // refresh main window
}

bool DR_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T( "/Settings/DR_pi" ) );
			 pConf->Read ( _T( "ShowDRIcon" ), &m_bDRShowIcon, 1 );
           
            m_route_dialog_x =  pConf->Read ( _T ( "DialogPosX" ), 20L );
            m_route_dialog_y =  pConf->Read ( _T ( "DialogPosY" ), 20L );
         
            if((m_route_dialog_x < 0) || (m_route_dialog_x > m_display_width))
                  m_route_dialog_x = 5;
            if((m_route_dialog_y < 0) || (m_route_dialog_y > m_display_height))
                  m_route_dialog_y = 5;
            return true;
      }
      else
            return false;
}

bool DR_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T ( "/Settings/DR_pi" ) );
			pConf->Write ( _T ( "ShowDRIcon" ), m_bDRShowIcon );
          
            pConf->Write ( _T ( "DialogPosX" ),   m_route_dialog_x );
            pConf->Write ( _T ( "DialogPosY" ),   m_route_dialog_y );
            
            return true;
      }
      else
            return false;
}

void DR_pi::OnDRDialogClose()
{
    m_bShowDR = false;
    SetToolbarItemState( m_leftclick_tool_id, m_bShowDR );
    m_pDialog->Hide();
    SaveConfig();

    RequestRefresh(m_parent_window); // refresh main window

}


