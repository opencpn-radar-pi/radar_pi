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

#ifndef _NAVICOCAPABILITIES_H_
#define _NAVICOCAPABILITIES_H_

#include "pi_common.h"

PLUGIN_BEGIN_NAMESPACE

//
// The exact model, as reported in report 03 C4. This is how the chartplotters
// tell a HALO 20 from a HALO 20+ or 24; nothing else in the protocol does.
//
enum NavicoScannerType {
    NST_BR24 = 10,
    NST_3G = 12,
    NST_4G = 13,
    NST_HALO = 14,
    NST_HALO24 = 16,
    NST_HALO20 = 17,
    NST_HALO20PLUS = 18,
    NST_HALO2000 = 19,
    NST_HALO3000 = 20,
    NST_HALO5000 = 21,
    NST_HALO4000 = 22,
    NST_HALO6000 = 23,
};

// Name of a scanner type for logging, "Unknown" when we don't know it.
extern wxString NavicoScannerTypeName(uint32_t scanner_type);

// False only for the models that we know cannot do Doppler.
extern bool NavicoScannerHasDoppler(uint32_t scanner_type);

//
// The capabilities that a HALO radar advertises in report 09 C4.
//
// The 'supported use modes' bitmask has one bit per use mode, numbered the
// same way as the mode values that the radar reports, so bit n means mode n.
// The BR24, 3G and 4G never send this report.
//
class NavicoCapabilities {
public:
    NavicoCapabilities()
        : m_valid(false)
        , m_supported_modes(0)
        , m_min_range_m(0)
        , m_max_range_m(0)
        , m_dome(true) {};

    // Parse the TLV stream that follows the two byte report header.
    // Returns false if the stream is malformed, in which case the object is
    // left invalid.
    bool Parse(const uint8_t* data, size_t len);

    uint32_t GetMaxRangeMeters() const { return m_max_range_m; }
    uint32_t GetSupportedModes() const { return m_supported_modes; }

    bool operator==(const NavicoCapabilities& other) const
    {
        return m_valid == other.m_valid
            && m_supported_modes == other.m_supported_modes
            && m_min_range_m == other.m_min_range_m
            && m_max_range_m == other.m_max_range_m
            && m_dome == other.m_dome;
    }

    wxString to_string() const;

private:
    bool m_valid; // A report has been parsed successfully
    uint32_t m_supported_modes; // Bit n is set when the radar has use mode n
    uint32_t m_min_range_m; // Shortest instrumented range
    uint32_t m_max_range_m; // Longest instrumented range
    bool m_dome; // Dome antenna, else open array
};

PLUGIN_END_NAMESPACE

#endif /* _NAVICOCAPABILITIES_H_ */
