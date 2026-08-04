/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2025 by Kees Verruijt         canboat@verruijt.net *
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

#include "navico/NavicoCapabilities.h"

#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

/*
 * Report 09 C4 is a list of TLV entries, each headed by a type byte, a
 * reserved byte and a length byte. Only HALO radars send it.
 *
 * The types below are the ones that we act upon. The radar sends many more,
 * up to type 67 on recent firmware, but even the Navico MFD software that we
 * have seen only understands types 2 to 12.
 */
#define TLV_HEADER_LEN (3)

#define TLV_SUPPORTED_USE_MODES (2)  // Bit n set = the radar has use mode n
#define TLV_SUPPORTED_ANTENNAS (10)  // Dome or list of open array sizes
#define TLV_INSTRUMENTED_RANGE (11)  // Min and max range in decimeters

#define DECIMETERS_PER_METER (10)

// Indexed by use mode, so also by the bit number in the supported modes mask.
static const wxString ModeNames[] = {wxT("Custom"), wxT("Harbor"), wxT("Offshore"),
                                     wxT("Buoy"),   wxT("Weather"), wxT("Bird")};

wxString NavicoScannerTypeName(uint32_t scanner_type) {
  switch (scanner_type) {
    case NST_BR24:
      return wxT("BR24");
    case NST_3G:
      return wxT("3G");
    case NST_4G:
      return wxT("4G");
    case NST_HALO:
      return wxT("HALO");
    case NST_HALO24:
      return wxT("HALO24");
    case NST_HALO20:
      return wxT("HALO20");
    case NST_HALO20PLUS:
      return wxT("HALO20+");
    case NST_HALO2000:
      return wxT("HALO2000");
    case NST_HALO3000:
      return wxT("HALO3000");
    case NST_HALO5000:
      return wxT("HALO5000");
    case NST_HALO4000:
      return wxT("HALO4000");
    case NST_HALO6000:
      return wxT("HALO6000");
    default:
      return wxT("Unknown");
  }
}

bool NavicoScannerHasDoppler(uint32_t scanner_type) {
  // The HALO 20 is the small brother of the HALO 20+; it has no Doppler where
  // every other HALO that we know of has. Assume that an unknown model has it,
  // that is what the plugin has always done.
  return scanner_type != NST_HALO20;
}

static uint32_t GetUInt32LE(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool NavicoCapabilities::Parse(const uint8_t *data, size_t len) {
  size_t offset = 0;

  while (offset + TLV_HEADER_LEN <= len) {
    uint8_t type = data[offset];
    size_t length = data[offset + 2];
    const uint8_t *payload = data + offset + TLV_HEADER_LEN;

    offset += TLV_HEADER_LEN + length;
    if (offset > len) {
      LOG_RECEIVE(wxT("capability report truncated in type %u at offset %u of %u"), type, (unsigned)offset, (unsigned)len);
      return false;
    }

    switch (type) {
      case TLV_SUPPORTED_USE_MODES: {
        if (length >= 4) {
          m_supported_modes = GetUInt32LE(payload);
        }
        break;
      }

      case TLV_SUPPORTED_ANTENNAS: {
        // A dome sends a single zero byte, an open array the sizes it supports
        m_dome = (length == 1 && payload[0] == 0);
        break;
      }

      case TLV_INSTRUMENTED_RANGE: {
        if (length >= 8) {
          m_min_range_m = GetUInt32LE(payload) / DECIMETERS_PER_METER;
          m_max_range_m = GetUInt32LE(payload + 4) / DECIMETERS_PER_METER;
        }
        break;
      }
    }
  }

  m_valid = true;
  return true;
}

wxString NavicoCapabilities::to_string() const {
  wxString s;

  s << wxT("modes=");
  for (size_t mode = 0; mode < ARRAY_SIZE(ModeNames); mode++) {
    if (m_supported_modes & (1 << mode)) {
      s << ModeNames[mode] << wxT(",");
    }
  }
  s << wxString::Format(wxT(" range=%u-%um antenna=%s"), m_min_range_m, m_max_range_m,
                        m_dome ? wxT("dome") : wxT("open array"));

  return s;
}

PLUGIN_END_NAMESPACE
