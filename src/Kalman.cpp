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
 The filter used here is an "Extended Kalman Filter"  For a general introduction see
 Wikipedia.
 For the formula used here see "An Introduction to the Kalman Filter"
 Greg Welch and Gary Bishop, TR45-041, Department of Computer Science,
 University of North Carolina at Chapel Hill
 July 24, 2006

 */

#include "Kalman.h"

PLUGIN_BEGIN_NAMESPACE

Kalman_Filter::Kalman_Filter(int range) {
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman_Filter created"));
  // as the measurement to state transformation is non-linear, the extended Kalman filter should be used
  // as the state transformation is linear, the state transformation matrix F is equal to the jacobian A
  // f is the state transformation function Xk <- Xk-1
  // Ai,j is jacobian matrix dfi / dxj

  A.Extend(4, 4);
  for (int i = 1; i <= 4; i++) {
    A(i, i) = 1.;
  }

  AT.Extend(4, 4);  // transpose of A
  AT = A;

  W.Extend(4, 2);
  W(3, 1) = 1.;
  W(4, 2) = 1.;

  WT.Extend(2, 4);  // transpose of W
  WT(1, 3) = 1.;
  WT(2, 4) = 1.;

  // Observation matrix, jacobian of observation function
  // dhi / dvj
  // angle = atan2 (lat,lon) * 2048 / (2 * pi) + v1
  // r = sqrt(x * x + y * y) + v2
  // v is measurement noise
  H.Extend(2, 4);

  HT.Extend(4, 2);  // Transpose of observation matrix

  // Jacobian V, dhi / dvj
  V.Extend(2, 2);
  V(1, 1) = 1.;
  V(2, 2) = 1.;

  VT.Extend(2, 2);
  VT(1, 1) = 1.;
  VT(2, 2) = 1.;

  // P estimate error covariance
  P.Extend(4, 4);
  // initial values follow
  // P(1, 1) = .0000027 * range * range;   ???
  P(1, 1) = 20.;
  P(2, 2) = P(1, 1);
  P(3, 3) = 4.;
  P(4, 4) = 4.;

  // Q Process noise covariance matrix
  Q.Extend(2, 2);
  Q(1, 1) = .1;  // variance in lat speed, (m / sec)2
  Q(2, 2) = .1;  // variance in lon speed, (m / sec)2

  // R measurement noise covariance matrix
  R.Extend(2, 2);
  R(1, 1) = 100.0;  // variance in the angle 3.0
  R(2, 2) = 25.;    // variance in radius  .5

  K.Extend(4, 2);  // initial Kalman gain

  // identity matrix
  I.Extend(4, 4);
  for (int i = 1; i <= 4; i++) {
    I(i, i) = 1.;
  }
}

Kalman_Filter::~Kalman_Filter() {  // clean up all matrices
  A.~Matrix();
  AT.~Matrix();
  W.~Matrix();
  WT.~Matrix();
  H.~Matrix();
  HT.~Matrix();
  V.~Matrix();
  VT.~Matrix();
  P.~Matrix();
  Q.~Matrix();
  FT.~Matrix();
  R.~Matrix();
  K.~Matrix();
  I.~Matrix();
}

void Kalman_Filter::Predict(LocalPosition* xx, double delta_time) {
  Matrix X(4, 1);
  X(1, 1) = xx->lat;
  X(2, 1) = xx->lon;
  X(3, 1) = xx->dlat_dt;
  X(4, 1) = xx->dlon_dt;
  A(1, 3) = delta_time;  // time in seconds
  A(2, 4) = delta_time;

  AT(3, 1) = delta_time;
  AT(4, 2) = delta_time;

  X = A * X;
  xx->lat = X(1, 1);
  xx->lon = X(2, 1);
  xx->dlat_dt = X(3, 1);
  xx->dlon_dt = X(4, 1);
  // calculate apriori P
  //   LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Predict P before"));
  /* for (int i = 1; i < 5; i++){
       LOG_INFO(wxT("BR24radar_pi: $$$ Kalman P   %f %f %.9f %.9f"), P(i, 1), P(i, 2), P(i,3), P(i,4));
}*/
  P = A * P * AT + W * Q * WT;
  /*LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Predict P After"));
  for (int i = 1; i < 5; i++){
      LOG_INFO(wxT("BR24radar_pi: $$$ Kalman P   %f %f %.9f %.9f"), P(i, 1), P(i, 2), P(i, 3), P(i, 4));
  }*/
  /*LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Predict "));*/
  X.~Matrix();
  return;
}

void Kalman_Filter::SetMeasurement(Polar* pol, LocalPosition* x, Polar* expected, int range) {
// pol measured angular position
// x expected local position
// expected, same but in polar coordinates
#define LON x->lon
#define LAT x->lat
  double q_sum = LON * LON + LAT * LAT;

  double c = 2048. / (2. * PI);
  H(1, 1) = -c * LON / q_sum;
  H(1, 2) = c * LAT / q_sum;
  HT(1, 1) = H(1, 1);
  HT(2, 1) = H(1, 2);

  q_sum = sqrt(q_sum);
  H(2, 1) = LAT / q_sum * 512. / (double)range;
  H(2, 2) = LON / q_sum * 512. / (double)range;
  HT(1, 2) = H(2, 1);
  HT(2, 2) = H(2, 2);

  /*for (int i = 1; i < 3; i++){
      LOG_INFO(wxT("BR24radar_pi: $$$ Kalman H   %f %f %f %f"), H(i, 1), H(i, 2), H(i, 3), H(i, 4));
  }*/

  Matrix Z(2, 1);
  Z(1, 1) = (double)(pol->angle - expected->angle);  // Z is  difference between measured and expected
  Z(2, 1) = (double)(pol->r - expected->r);

  Matrix X(4, 1);
  X(1, 1) = x->lat;
  X(2, 1) = x->lon;
  X(3, 1) = x->dlat_dt;
  X(4, 1) = x->dlon_dt;

  // calculate Kalman gain
  Matrix Inverse(4, 4);

  Inverse = Inv(H * P * HT + R);  // V left out, only valid if V = I
  K = P * HT * Inverse;

  /*LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Gain "));
  for (int i = 1; i < 5; i++){
      LOG_INFO(wxT("BR24radar_pi: $$$ Kalman Gain  K %f %f "), K(i, 1), K(i, 2));
  }*/

  X = X + K * Z;
  // LOG_INFO(wxT("BR24radar_pi: $$$ Kalman SetMeasurement after  X %f %f %.9f %.9f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
  x->lat = X(1, 1);
  x->lon = X(2, 1);
  x->dlat_dt = X(3, 1);
  x->dlon_dt = X(4, 1);
  x->sd_speed_m_s = sqrt((P(3, 3) + P(4, 4)) / 2.);  // rough approximation of standard dev of speed
  // update covariance P
  P = (I - K * H) * P;
  /*LOG_INFO(wxT("BR24radar_pi: $$$ Kalman  P After"));
  for (int i = 1; i < 5; i++){
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman P   %f %f %.9f %.9f"), P(i, 1), P(i, 2), P(i, 3), P(i, 4));
  }*/

  X.~Matrix();
  Z.~Matrix();
  Inverse.~Matrix();
  return;
}

PLUGIN_END_NAMESPACE
