/**************************************************************************
 *   Copyright (C) 2024 Alec Leamas                                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************/

/** \file plug_utils.cpp Implement plug_utils.h. */

#include "wx/log.h"
#include "ocpn_plugin.h"
#include "plug_utils.h"
#include "std_filesystem.h"

IconPath GetPluginIcon(const std::string& basename,
                       const std::string& pkg_name) {
  fs::path path(GetPluginDataDir(pkg_name.c_str()).ToStdString());
  auto root = path / "data" / basename;
  auto svg_path = fs::path(root.string() + ".svg");
  wxLogDebug("GetPluginIcon: trying path: %s", svg_path.string().c_str());
  if (fs::exists(svg_path))
    return IconPath(IconPath::Type::Svg, svg_path.string());

  auto png_path = fs::path(root.string() + ".png");
  wxLogDebug("GetPluginIcon: trying path: %s", png_path.string().c_str());
  if (fs::exists(png_path))
    return IconPath(IconPath::Type::Png, png_path.string());

  return IconPath(IconPath::Type::NotFound, "");
}

wxBitmap LoadPngIcon(const char* path) {
  if (!wxImage::CanRead(path)) {
    wxLogDebug("Initiating image handlers.");
    wxInitAllImageHandlers();
  }
  wxImage image(path);
  return wxBitmap(image);
}

wxBitmap LoadSvgIcon(const char* path) {
  wxLogDebug("Loading SVG icon");
  const static int kIconSize = 48;  // FIXME: Needs size from GUI
  auto bitmap = GetBitmapFromSVGFile(path, kIconSize, kIconSize);
  return bitmap;
}
