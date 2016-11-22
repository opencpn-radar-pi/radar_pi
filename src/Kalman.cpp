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
 *   Copyright (C) 2013-2016 by Douwe Fokkkema             df@percussion.nl*
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
 The filter used here is an "Extended Kalman Filter" temmporarily with a fixed Kalman gain. For a general introduction see
 Wikipedia.

 */

#include "Kalman.h"

PLUGIN_BEGIN_NAMESPACE

Kalman_Filter::Kalman_Filter() {
  // Error covariance matrix when not maneuvring
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman_Filter created"));
  Q1.Extend(4, 4);
  Q2.Extend(4, 4);
  Q1(3, 3) = 2.;
  Q1(4, 4) = 2.;

  Q2.Extend(4, 4);
  Q2(3, 3) = 20.;  // Error covariance matrix when maneuvring
  Q2(4, 4) = 20.;

  H.Extend(4, 4);  // Observation matrix
  H(1, 1) = 1.;
  H(2, 2) = 1.;
  H(3, 3) = 1.;  // $$$
  H(4, 4) = 1.;

  HT.Extend(4, 2);  // Transpose of observation matrix
  HT(1, 1) = 1.;
  HT(2, 2) = 1.;

  H1.Extend(2, 4);  // Variable observation matrix

  H1T.Extend(4, 2);  // Transposed H1

  P.Extend(4, 4);  // Error covariance matrix, initial values
  P(1, 1) = 10;    // $$$ redefine!!
  P(2, 2) = 10.;
  P(3, 3) = 10.;
  P(4, 4) = 10.;

  K.Extend(4, 4);  // initial Kalman gain
  double gain = .2;
  K(1, 1) = gain;
  K(2, 2) = gain;
  K(3, 3) = .2;
  K(4, 4) = .2;

  F.Extend(4, 4);
  for (int i = 1; i <= 4; i++) {
    F(i, i) = 1.;
  }
  F(1, 3) = 1.;
  F(2, 4) = 1.;
}

Kalman_Filter::~Kalman_Filter() {  // clean up all matrices
  Q1.~Matrix();
  Q2.~Matrix();
  H.~Matrix();
  HT.~Matrix();
  H1.~Matrix();
  H1T.~Matrix();
  P.~Matrix();
  K.~Matrix();
}

void Kalman_Filter::SetMeasurement(Position* zz, Position* xx, double gain_p, double gain_s) {
  // zz measured position, xx estimated position
  K(1, 1) = gain_p;
  K(2, 2) = gain_p;
  K(3, 3) = gain_s;
  K(4, 4) = gain_s;
  Matrix Z(4, 1);
  Z(1, 1) = zz->lat;
  Z(2, 1) = zz->lon;
  Z(3, 1) = zz->dlat_dt;
  Z(4, 1) = zz->dlon_dt;
  Matrix X(4, 1);
  X(1, 1) = xx->lat;
  X(2, 1) = xx->lon;
  X(3, 1) = xx->dlat_dt;
  X(4, 1) = xx->dlon_dt;
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman SetMeasurement before Z %f %f %.9f %.9f"), Z(1, 1), Z(2, 1), Z(3, 1), Z(4, 1));
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman SetMeasurement before X %f %f %.9f %.9f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
  X = X + K * (Z - H * X);
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman SetMeasurement after  X %f %f %.9f %.9f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
  xx->lat = X(1, 1);
  xx->lon = X(2, 1);
  xx->dlat_dt = X(3, 1);
  xx->dlon_dt = X(4, 1);
  xx->time = zz->time;
  X.~Matrix();
  Z.~Matrix();
  return;
}

void Kalman_Filter::Predict(Position* xx, int delta_time) {
  Matrix X(4, 1);
  X(1, 1) = xx->lat;
  X(2, 1) = xx->lon;
  X(3, 1) = xx->dlat_dt;
  X(4, 1) = xx->dlon_dt;
  F(1, 3) = ((double)delta_time) / 1000.;
  F(2, 4) = ((double)delta_time) / 1000.;
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Predict before X %f %f %.9f %.9f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
  X = F * X;
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Predict after  X %f %f %.9f %.9f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
  xx->lat = X(1, 1);
  xx->lon = X(2, 1);
  xx->dlat_dt = X(3, 1);
  xx->dlon_dt = X(4, 1);
  X.~Matrix();
  return;
}

PLUGIN_END_NAMESPACE
