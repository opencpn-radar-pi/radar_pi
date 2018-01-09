/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  ShipDriver Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2017 by Mike Rossiter                                *
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

#include "ShipDriver_pi.h"
#include "ShipDrivergui_impl.h"
#include "ShipDrivergui.h"
#include "ocpn_plugin.h" 
#include "folder.xpm"

class ShipDriver_pi;
class Dlg;

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new ShipDriver_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    ShipDriver PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------




ShipDriver_pi::ShipDriver_pi(void *ppimgr)
      :opencpn_plugin_16 (ppimgr)
{
      // Create the PlugIn icons
      initialize_images();
	  m_bShowShipDriver = false;
}

ShipDriver_pi::~ShipDriver_pi(void)
{
     //delete _img_ShipDriver_pi;
     delete _img_ShipDriverIcon;
     
}

int ShipDriver_pi::Init(void)
{
      AddLocaleCatalog( _T("opencpn-ShipDriver_pi") );

      // Set some default private member parameters
      m_hr_dialog_x = 40;
      m_hr_dialog_y = 80;
	  m_hr_dialog_sx = 400;
	  m_hr_dialog_sy = 300;
      ::wxDisplaySize(&m_display_width, &m_display_height);

      //    Get a pointer to the opencpn display canvas, to use as a parent for the POI Manager dialog
      m_parent_window = GetOCPNCanvasWindow();

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();

      //    And load the configuration items
      LoadConfig();

      //    This PlugIn needs a toolbar icon, so request its insertion
	if(m_bShipDriverShowIcon)
		m_leftclick_tool_id = InsertPlugInTool(_T(""), _img_ShipDriverIcon, _img_ShipDriverIcon, wxITEM_CHECK,
            _("ShipDriver"), _T(""), NULL,
             ShipDriver_TOOL_POSITION, 0, this);

	wxMenu dummy_menu;
	m_position_menu_id = AddCanvasContextMenuItem
		(new wxMenuItem(&dummy_menu, -1, _("Select Vessel Start Position")), this);
	SetCanvasContextMenuItemViz(m_position_menu_id, true);



      m_pDialog = NULL;

      return (
			  WANTS_OVERLAY_CALLBACK |
			  WANTS_OPENGL_OVERLAY_CALLBACK |
              WANTS_TOOLBAR_CALLBACK    |
              INSTALLS_TOOLBAR_TOOL     |   
			  WANTS_CURSOR_LATLON |
			  WANTS_NMEA_SENTENCES|
			  WANTS_AIS_SENTENCES|
              WANTS_CONFIG           
           );
}

bool ShipDriver_pi::DeInit(void)
{
      //    Record the dialog position
      if (NULL != m_pDialog)
      {
            //Capture dialog position
            wxPoint p = m_pDialog->GetPosition();
			wxRect r = m_pDialog->GetRect();
            SetShipDriverDialogX(p.x);
            SetShipDriverDialogY(p.y);
			SetShipDriverDialogSizeX(r.GetWidth());
			SetShipDriverDialogSizeY(r.GetHeight());

			if ((m_pDialog->m_Timer != NULL) && (m_pDialog->m_Timer->IsRunning())){ // need to stop the timer or crash on exit
				m_pDialog->m_Timer->Stop();
			}
            m_pDialog->Close();
            delete m_pDialog;
            m_pDialog = NULL;

			m_bShowShipDriver = false;
			SetToolbarItemState( m_leftclick_tool_id, m_bShowShipDriver );
      }	
    
    SaveConfig();

    RequestRefresh(m_parent_window); // refresh main window

    return true;
}

int ShipDriver_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int ShipDriver_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int ShipDriver_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int ShipDriver_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *ShipDriver_pi::GetPlugInBitmap()
{
      return _img_ShipDriverIcon;
}

wxString ShipDriver_pi::GetCommonName()
{
      return _("ShipDriver");
}


wxString ShipDriver_pi::GetShortDescription()
{
      return _("ShipDriver player");
}

wxString ShipDriver_pi::GetLongDescription()
{
      return _("Almost a simulator");
}

int ShipDriver_pi::GetToolbarToolCount(void)
{
      return 1;
}

void ShipDriver_pi::SetColorScheme(PI_ColorScheme cs)
{
      if (NULL == m_pDialog)
            return;

      DimeWindow(m_pDialog);
}

void ShipDriver_pi::OnToolbarToolCallback(int id)
{
	bool starting = false;

	double scale_factor = GetOCPNGUIToolScaleFactor_PlugIn();
	if (scale_factor != m_GUIScaleFactor) starting = true;
    
	if(NULL == m_pDialog)
      {
            m_pDialog = new Dlg(m_parent_window);
            m_pDialog->plugin = this;
			m_pDialog->m_Timer = new wxTimer(m_pDialog);			
            m_pDialog->Move(wxPoint(m_hr_dialog_x, m_hr_dialog_y));	
			m_pDialog->SetSize(m_hr_dialog_sx, m_hr_dialog_sy);			
      }

	 // m_pDialog->Fit();
	  //Toggle 
	  m_bShowShipDriver = !m_bShowShipDriver;	  

      //    Toggle dialog? 
      if(m_bShowShipDriver) {
		  m_pDialog->Move(wxPoint(m_hr_dialog_x, m_hr_dialog_y));
		  m_pDialog->SetSize(m_hr_dialog_sx, m_hr_dialog_sy);
          m_pDialog->Show();         
	  }
	  else {
		  m_pDialog->Hide();
	  }
     
      // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
      // to actual status to ensure correct status upon toolbar rebuild
      SetToolbarItemState( m_leftclick_tool_id, m_bShowShipDriver);

	  //Capture dialog position
	  wxPoint p = m_pDialog->GetPosition();
	  wxRect r = m_pDialog->GetRect();
	  SetShipDriverDialogX(p.x);
	  SetShipDriverDialogY(p.y);
	  SetShipDriverDialogSizeX(r.GetWidth());
	  SetShipDriverDialogSizeY(r.GetHeight());


      RequestRefresh(m_parent_window); // refresh main window
}

bool ShipDriver_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T( "/Settings/ShipDriver_pi" ) );
			pConf->Read ( _T( "ShowShipDriverIcon" ), &m_bShipDriverShowIcon, 1 );
           
            m_hr_dialog_x =  pConf->Read ( _T ( "DialogPosX" ), 40L );
            m_hr_dialog_y =  pConf->Read ( _T ( "DialogPosY" ), 140L);
			m_hr_dialog_sx = pConf->Read ( _T ( "DialogSizeX"), 300L);
			m_hr_dialog_sy = pConf->Read ( _T ( "DialogSizeY"), 540L);
         
            if((m_hr_dialog_x < 0) || (m_hr_dialog_x > m_display_width))
                  m_hr_dialog_x = 40;
            if((m_hr_dialog_y < 0) || (m_hr_dialog_y > m_display_height))
                  m_hr_dialog_y = 140;

            return true;
      }
      else
            return false;
}

bool ShipDriver_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T ( "/Settings/ShipDriver_pi" ) );
			pConf->Write ( _T ( "ShowShipDriverIcon" ), m_bShipDriverShowIcon );
          
            pConf->Write ( _T ( "DialogPosX" ),   m_hr_dialog_x );
            pConf->Write ( _T ( "DialogPosY" ),   m_hr_dialog_y );
			pConf->Write ( _T ( "DialogSizeX"),   m_hr_dialog_sx);
			pConf->Write ( _T ( "DialogSizeY"),   m_hr_dialog_sy);
            
            return true;
      }
      else
            return false;
}

void ShipDriver_pi::OnShipDriverDialogClose()
{
    m_bShowShipDriver = false;
    SetToolbarItemState( m_leftclick_tool_id, m_bShowShipDriver);
    m_pDialog->Hide();
   // SaveConfig();

    RequestRefresh(m_parent_window); // refresh main window
}

void ShipDriver_pi::OnContextMenuItemCallback(int id)
{

	if (!m_pDialog)
		return;

	if (id == m_position_menu_id){

		m_cursor_lat = GetCursorLat();
		m_cursor_lon = GetCursorLon();

		m_pDialog->OnContextMenu(m_cursor_lat, m_cursor_lon);
	}
}

void ShipDriver_pi::SetCursorLatLon(double lat, double lon)
{
	m_cursor_lat = lat;
	m_cursor_lon = lon;
}






