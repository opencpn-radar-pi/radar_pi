/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2013 by Kees Verruijt         canboat@verruijt.net *
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

#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

void GuardZone::ResetBogeys() {
  for (size_t r = 0; r < ARRAY_SIZE(bogeyCount); r++) {
    bogeyCount[r] = 0;
  }
  m_bogey_count = -1;
}

void GuardZone::ProcessSpoke(SpokeBearing angle, UINT8* data, UINT8* hist, size_t len, int range) {
  int bogeys = 0;
  size_t range_start, range_end;

  // We can't check whether the data is in the correct angle here since the boat may be changing
  // course. So we store it anyway (if the guard zone is active, but we're not called otherwise)
  if (type != GZ_OFF) {
    range_start = inner_range * RETURNS_PER_LINE / range;  // Convert from meters to 0..511
    range_end = outer_range * RETURNS_PER_LINE / range;    // Convert from meters to 0..511
    if (range_start < RETURNS_PER_LINE) {
      if (range_end > RETURNS_PER_LINE) {
        range_end = RETURNS_PER_LINE;
      }

      for (size_t r = range_start; r <= range_end; r++) {
        if (!multi_sweep_filter || HISTORY_FILTER_ALLOW(hist[r])) {
          if (m_pi->m_color_map[data[r]] != BLOB_NONE) {
            bogeys++;
          }
        }
      }
    }
  }

  bogeyCount[angle] = bogeys;
}

int GuardZone::GetBogeyCount(SpokeBearing current_hdt) {
  int bogeys = 0;
  SpokeBearing begin_arc, end_arc, arc, angle;

  switch (type) {
    case GZ_CIRCLE:
      begin_arc = 0;
      end_arc = LINES_PER_ROTATION;
      break;
    case GZ_ARC:
      begin_arc = MOD_ROTATION2048(start_bearing + current_hdt);
      end_arc = MOD_ROTATION2048(end_bearing + current_hdt);
      if (begin_arc > end_arc) {
        end_arc += LINES_PER_ROTATION;  // now end_arc may be larger than LINES_PER_ROTATION!
      }
      break;
    case GZ_OFF:
    default:
      bogeys = -1;
      m_bogey_count = bogeys;
      return bogeys;
  }

  for (arc = begin_arc; arc < end_arc; arc++) {
    angle = MOD_ROTATION2048(arc);

    bogeys += bogeyCount[angle];
  }

  m_bogey_count = bogeys;
  return bogeys;
}

PLUGIN_END_NAMESPACE
