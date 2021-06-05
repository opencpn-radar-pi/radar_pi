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
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
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

#include "RadarFactory.h"

#include "RadarType.h"
#include "pi_common.h"

PLUGIN_BEGIN_NAMESPACE

const wchar_t* RadarTypeName[RT_MAX] = {
#define DEFINE_RADAR(t, n, s, l, a, b, c, d) n,
#include "RadarType.h"
};

ControlsDialog* RadarFactory::MakeControlsDialog(size_t radarType, int radar) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t:                                    \
    return new a;
#include "RadarType.h"
  };
  return 0;
}

RadarReceive* RadarFactory::MakeRadarReceive(size_t radarType, radar_pi* pi, RadarInfo* ri) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t:                                    \
    return new b;
#include "RadarType.h"
  };
  return 0;
}

RadarControl* RadarFactory::MakeRadarControl(size_t radarType, radar_pi* pi, RadarInfo* ri) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t:                                    \
    return new c;
#include "RadarType.h"
  };
  return 0;
}

size_t RadarFactory::GetRadarRanges(size_t radarType, RangeUnits units, const int** ranges) {
  size_t n = 0;
  *ranges = 0;

  switch (units) {
    case RANGE_MIXED:
      switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_MIXED_##t;  \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }

#include "RadarType.h"
      };
      break;

    case RANGE_METRIC:
      switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_METRIC_##t; \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }
#include "RadarType.h"
      };
      break;

    case RANGE_UNITS_UNDEFINED:
      wxLogError(wxT("Internal error: RANGE_UNITS_UNDEFINED, RANGE_NAUTIC applied instead for radar type %d units %d"),
                 (int)radarType, (int)units);
    case RANGE_NAUTIC:
      switch (radarType) {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_NAUTIC_##t; \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }
#include "RadarType.h"
      };
      break;

    default:
      wxLogError(wxT("Internal error: illegal value for range units for radar type %d units %d"), (int)radarType, (int)units);
      static const int r[] = {0};
      *ranges = r;
      n = ARRAY_SIZE(r);
      break;
  }

  if (n == 0) {
    wxLogError(wxT("Internal error: Programmer forgot to define ranges for radar type %d units %d"), (int)radarType, (int)units);
    wxAbort();
  }
  return n;
}

size_t RadarFactory::GetRadarRanges(RadarInfo* ri, RangeUnits units, const int** ranges) {
  size_t n = 0;
  if (ri->m_radar_type == RM_E120 || ri->m_radar_type == RM_QUANTUM) {
    *ranges = ri->m_radar_ranges;
    if (ri->m_radar_type == RM_E120) return 11;
    if (ri->m_radar_type == RM_QUANTUM) return 20;
  }

  switch (units) {
    case RANGE_MIXED:
      switch (ri->m_radar_type) {
        case RT_MAX:
          *ranges = 0;
          break;

#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_MIXED_##t;  \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }

#include "RadarType.h"
      };
      break;

    case RANGE_METRIC:
      switch (ri->m_radar_type) {
        case RT_MAX:
          *ranges = 0;
          break;

#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_METRIC_##t; \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }
#include "RadarType.h"
      };
      break;

    case RANGE_UNITS_UNDEFINED:
      wxLogError(wxT("Internal error: RANGE_UNITS_UNDEFINED, RANGE_NAUTIC applied instead for radar type %d units %d"),
                 (int)ri->m_radar_type, (int)units);
    case RANGE_NAUTIC:
      switch (ri->m_radar_type) {
        case RT_MAX:
          *ranges = 0;
          break;

#define DEFINE_RADAR(t, x, s, l, a, b, c, d) \
  case t: {                                  \
    static const int r[] = RANGE_NAUTIC_##t; \
    *ranges = r;                             \
    n = ARRAY_SIZE(r);                       \
    break;                                   \
  }
#include "RadarType.h"
      };
      break;
    default:
      wxLogError(wxT("Internal error: illegal value for range units for radar type %d units %d"), (int)ri->m_radar_type,
                 (int)units);
      static const int r[] = {0};
      *ranges = r;
      n = ARRAY_SIZE(r);
      break;
  }

  if (n == 0) {
    wxLogError(wxT("Internal error: Programmer forgot to define ranges for radar type %d units %d"), (int)ri->m_radar_type,
               (int)units);
    wxAbort();
  }
  return n;
}

void RadarFactory::GetRadarTypes(wxArrayString& radarTypes) {
  wxString names[] = {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) x,
#include "RadarType.h"
  };

  radarTypes = wxArrayString(ARRAY_SIZE(names), names);
}

PLUGIN_END_NAMESPACE
