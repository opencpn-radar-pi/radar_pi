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

#ifndef SDR_PI_H_
#define SDR_PI_H_

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include <wx/glcanvas.h>
#endif

#include <wx/fileconf.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>

#include "config.h"

#include "json/reader.h"
#include "json/writer.h"

#include "ocpn_plugin.h"  //Required for OCPN plugin functions
#include "shipdriver_gui_impl.h"
#include "GribRecordSet.h"

// Define minimum and maximum versions of the grib plugin supported
#define GRIB_MAX_MAJOR 4
#define GRIB_MAX_MINOR 1
#define GRIB_MIN_MAJOR 4
#define GRIB_MIN_MINOR 1

class Dlg;

static inline bool GribCurrent(GribRecordSet* grib, double lat, double lon,
                               double& c, double& vc) {
  if (!grib) return false;

  if (!GribRecord::getInterpolatedValues(
          vc, c, grib->m_GribRecordPtrArray[Idx_WIND_VX],
          grib->m_GribRecordPtrArray[Idx_WIND_VY], lon, lat))
    return false;

  vc *= 3.6 / 1.852;  // knots

  // C += 180;
  if (c > 360) c -= 360;
  return true;
}

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define ShipDriver_TOOL_POSITION (-1)  ///< toolbar tool default positioning

class ShipDriverPi : public opencpn_plugin_118 {
public:
  explicit ShipDriverPi(void* ppimgr);
  ~ShipDriverPi() override;

  //    The required PlugIn Methods
  int Init() override;
  bool DeInit() override;

  int GetAPIVersionMajor() override;
  int GetAPIVersionMinor() override;
  int GetPlugInVersionMajor() override;
  int GetPlugInVersionMinor() override;
  int GetPlugInVersionPatch() override;
  int GetPlugInVersionPost() override;
  const char* GetPlugInVersionPre() override;
  const char* GetPlugInVersionBuild() override;

  wxBitmap* GetPlugInBitmap() override;
  wxString GetCommonName() override;
  wxString GetShortDescription() override;
  wxString GetLongDescription() override;

  //    The required override PlugIn Methods
  int GetToolbarToolCount() override;
  void OnToolbarToolCallback(int id) override;

  //    Optional plugin overrides
  void SetColorScheme(PI_ColorScheme cs) override;

  //    The override PlugIn Methods
  void OnContextMenuItemCallback(int id) override;
  void SetCursorLatLon(double lat, double lon) override;
  void SetNMEASentence(wxString& sentence) override;

  //    Other public methods
  void SetShipDriverDialogX(int x) { m_hr_dialog_x = x; };
  void SetShipDriverDialogY(int x) { m_hr_dialog_y = x; };
  void SetShipDriverDialogWidth(int x) { m_hr_dialog_width = x; };
  void SetShipDriverDialogHeight(int x) { m_hr_dialog_height = x; };
  void SetShipDriverDialogSizeX(int x) { m_hr_dialog_sx = x; }
  void SetShipDriverDialogSizeY(int x) { m_hr_dialog_sy = x; }
  void OnShipDriverDialogClose();

  int m_hr_dialog_x, m_hr_dialog_y;

  [[nodiscard]] double GetCursorLat() const { return m_cursor_lat; }
  [[nodiscard]] double GetCursorLon() const { return m_cursor_lon; }

  void ShowPreferencesDialog(wxWindow* parent) override;
  void SetPluginMessage(wxString& message_id, wxString& message_body) override;
  [[maybe_unused]] bool GribWind(GribRecordSet* grib, double lat, double lon,
                                 double& wg, double& vwg);

  bool m_is_grib_valid;
  double m_grib_lat, m_grib_lon;
  double m_tr_spd;
  double m_tr_dir;
  wxBitmap m_panel_bitmap;

private:
  double m_cursor_lat;
  double m_cursor_lon;

  int m_position_menu_id;
  double m_gui_scale_factor;

  ShipDriverPi* plugin;

  Dlg* m_dialog;

  wxFileConfig* m_config;
  wxWindow* m_parent_window;
  bool LoadConfig();
  bool SaveConfig();

  int m_hr_dialog_width, m_hr_dialog_height;
  int m_hr_dialog_sx, m_hr_dialog_sy;
  int m_display_width, m_display_height;
  int m_leftclick_tool_id;
  bool m_show_shipdriver_icon;
  bool m_show_shipdriver;

  bool m_copy_use_ais;
  bool m_copy_use_file;
  wxString m_copy_mmsi;
  bool m_copy_use_nmea;
};

#endif
