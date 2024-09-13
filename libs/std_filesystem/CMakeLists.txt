# ************************************************************************
#    Copyright (C) 2023  Alec Leamas                                  *
#                                                                          *
#    This program is free software; you can redistribute it and/or modify  *
#    it under the terms of the GNU General Public License as published by  *
#    the Free Software Foundation; either version 2 of the License, or     *
#    (at your option) any later version.                                   *
#                                                                          *
#    This program is distributed in the hope that it will be useful,       *
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#    GNU General Public License for more details.                          *
#                                                                          *
#    You should have received a copy of the GNU General Public License     *
#    along with this program; if not, write to the                         *
#    Free Software Foundation, Inc.,                                       *
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
# *************************************************************************/

#
# Wrapper for std::filesystem which handles also clang < 15
# (NDK < 26, MacOS < 10.15) which does not have a usable std::filesystem
#
# Exports:
#   - ocpn::filesystem transitive link target
#   - Header "std_filesystem.h" which does the right thing on all platforms.
#

if (TARGET ocpn::filesystem)
  return ()
endif ()

add_library(_FILESYS INTERFACE)
add_library(ocpn::filesystem ALIAS _FILESYS)
target_include_directories(_FILESYS INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
