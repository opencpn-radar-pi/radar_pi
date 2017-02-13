/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
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

#include "Kalman.h"
#include "RadarMarpa.h"

int main() {
  int ret = 0;
  int range = 4000;
  br24::Kalman_Filter *filter = new br24::Kalman_Filter(range);
  br24::Polar pol, expected;
  br24::LocalPosition x_local;

  pol.angle = 0;
  pol.r = 1000;
  pol.time = 1000;

  x_local.lat = 50;
  x_local.lon = -5;
  x_local.dlat_dt = 5;
  x_local.dlon_dt = 2;
  x_local.sd_speed_m_s = 0.2;

  expected.angle = 10;
  expected.r = 1050;
  expected.time = 6000;

  filter->SetMeasurement(&pol, &x_local, &expected, 4000);                // pol is measured position in polar coordinates
  filter->Predict(&x_local, (expected.time - pol.time).GetLo() / 1000.);  // x_local is new estimated local position of the target

  cout << "The predicted location is: lat=" << x_local.lat << " lon=" << x_local.lon << "\n";
  cout << "Delta lat=" << x_local.dlat_dt << " Delta lon=" << x_local.dlon_dt << "\n";
  cout << "StdDev speed=" << x_local.sd_speed_m_s << "\n";

  if (x_local.lat != 25) {
    cout << "Predicted lat is not expected value " << 25 << "\n";
    ret = 1;
  }

  if (x_local.lon != 10) {
    cout << "Predicted lat is not expected value " << 10 << "\n";
    ret = 1;
  }

  if (fabs(x_local.sd_speed_m_s - 0.360555) > 0.001) {
    cout << "Predicted StdDev is not expected value " << 0.360555 << "\n";
    ret = 1;
  }

  exit(ret);
}
