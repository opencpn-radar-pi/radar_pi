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

/** \file plug_utils.h Miscellaneous plugin support stuff. */
#include <string>

/**
 * Result of a icon path lookup
 */
struct IconPath {
  const enum class Type { NotFound, Png, Svg } type;
  const std::string path;
  IconPath(Type t, const std::string p) : type(t), path(std::move(p)){}
};

/**
 * Find an icon matching basename in data/ directory
 * @param basename Icon name without extension.
 * @param pkg_name Argument to GetPluginDataDir()
 * @return IconPath with type == Svg if a svg path is found, otherwise a
 *   type == Png IconPath If no path exists return (NotFound, "");
 */
IconPath GetPluginIcon(const std::string& basename,
                       const std::string& pkg_name);

/**
 * Load .png file into memory.
 * @param path  png file path
 * @return  loaded wxBitmap object
 */
wxBitmap LoadPngIcon(const char* path);

/**
 * Load .svg file into memory.
 * @param path  png file path
 * @return  loaded wxBitmap object
 */
wxBitmap LoadSvgIcon(const char* path);
