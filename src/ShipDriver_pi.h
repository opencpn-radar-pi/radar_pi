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

#ifndef _SDR_PI_H_
#define _SDR_PI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
  #include <wx/glcanvas.h>
#endif //precompiled headers

#include <wx/fileconf.h>

#include "ocpn_plugin.h" //Required for OCPN plugin functions
#include "ShipDrivergui_impl.h"

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    2

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    6

class Dlg;

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define ShipDriver_TOOL_POSITION    -1          // Request default positioning of toolbar tool

class ShipDriver_pi : public opencpn_plugin_16
{
public:
      ShipDriver_pi(void *ppimgr);
	   ~ShipDriver_pi(void);

//    The required PlugIn Methods
      int Init(void);
      bool DeInit(void);

      int GetAPIVersionMajor();
      int GetAPIVersionMinor();
      int GetPlugInVersionMajor();
      int GetPlugInVersionMinor();
      wxBitmap *GetPlugInBitmap();
      wxString GetCommonName();
      wxString GetShortDescription();
      wxString GetLongDescription();

//    The required override PlugIn Methods
      int GetToolbarToolCount(void);
      void OnToolbarToolCallback(int id);
     

//    Optional plugin overrides
      void SetColorScheme(PI_ColorScheme cs);


//    The override PlugIn Methods
	  void OnContextMenuItemCallback(int id);
	  void SetCursorLatLon(double lat, double lon);

//    Other public methods
      void SetShipDriverDialogX         (int x){ m_hr_dialog_x = x;};
      void SetShipDriverDialogY         (int x){ m_hr_dialog_y = x;};
      void SetShipDriverDialogWidth     (int x){ m_hr_dialog_width = x;};
      void SetShipDriverDialogHeight    (int x){ m_hr_dialog_height = x;};      
	  void OnShipDriverDialogClose();
	  

	  int  m_hr_dialog_x, m_hr_dialog_y;

      double GetCursorLat(void) { return m_cursor_lat; }
	  double GetCursorLon(void) { return m_cursor_lon; }
	  
	  
	  
private:

	double m_cursor_lat;
	double m_cursor_lon;

	int				m_position_menu_id;
	  double           m_GUIScaleFactor;
	  void OnClose( wxCloseEvent& event );
	  
	  ShipDriver_pi *plugin;
	  
      Dlg         *m_pDialog;

	  wxFileConfig      *m_pconfig;
      wxWindow          *m_parent_window;
      bool              LoadConfig(void);
      bool              SaveConfig(void);
	  
      int				m_hr_dialog_width,m_hr_dialog_height;
      int               m_display_width, m_display_height;      
      int				m_leftclick_tool_id;
	  bool				m_bShipDriverShowIcon;
	  bool				m_bShowShipDriver;
};



#endif
