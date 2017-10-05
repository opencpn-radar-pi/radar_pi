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

PLUGIN_BEGIN_NAMESPACE

ControlsDialog* RadarFactory::makeControlsDialog(size_t radarType, int radar) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, a, b, c) \
  case t:                           \
    return new a;
#include "RadarType.h"
  };
  return 0;
}

RadarReceive* RadarFactory::makeRadarReceive(size_t radarType, radar_pi* pi, RadarInfo* ri) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, a, b, c) \
  case t:                           \
    return new b;
#include "RadarType.h"
  };
  return 0;
}

RadarControl* RadarFactory::makeRadarControl(size_t radarType) {
  switch (radarType) {
#define DEFINE_RADAR(t, x, a, b, c) \
  case t:                           \
    return new c;
#include "RadarType.h"
  };
  return 0;
}

void RadarFactory::GetRadarTypes(wxArrayString& radarTypes) {
  wxString names[] = {
#define DEFINE_RADAR(t, x, a, b, c) x,
#include "RadarType.h"
  };

  radarTypes = wxArrayString(ARRAY_SIZE(names), names);
}

PLUGIN_END_NAMESPACE
