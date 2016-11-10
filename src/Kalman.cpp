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
 The filter used here is an "Extended Kalman Filter", for a general introduction see Wikipedia.

 */

#include "Kalman.h"

PLUGIN_BEGIN_NAMESPACE

//  Variables for the plot
double TRUERANGE;
double TRUEANGLE;
double MATRANGE;
double MATANGLE;
double MATX;
double MATY;
double ESTRANGE;
double ESTANGLE;
double DELTAANGLE;
double ESTX;
double ESTY;
double TRUEX;
double TRUEY;
double jac_a;
double jac_b;

Kalman_Filter::Kalman_Filter(Position init_position, double init_speed, double init_course) {
  Matrix Q1 = Matrix(4, 4);  // Error covariance matrix when not maneuvring
  Q1(1, 1) = 4.;
  Matrix Q2 = Matrix(4, 4);
  int hh = Q1.Size(1);
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman size of cc %i"), Q1.Size(1));
  Q1(3, 3) = 2.;
  Q1(4, 4) = 2.;

  //  Matrix Q2(4, 4);
  Q2(3, 3) = 20.;  // Error covariance matrix when maneuvring
  Q2(4, 4) = 20.;
  Q1 = Q1 + Q2;

  LOG_INFO(wxT("BR24radar_pi: $$$ XXXXXXXXXXXXXXXXXXXXXXXXXX %f"), Q1(4, 4));

  H = Matrix(2, 4);  // Observation matrix
  H(1, 1) = 1.;
  H(2, 2) = 1.;

  HT = Matrix(4, 2);  // Transpose of observation matrix
  HT(1, 1) = 1.;
  HT(2, 2) = 1.;

  H1 = Matrix(2, 4);  // Variable observation matrix
  Matrix H1T(4, 2);   // Transposed H1

  MetricPoint xx = Pos2Metric(init_position);
  Matrix X(4, 1);  // matrix of estimate position (X circumflex in literature)
  X(1, 1) = xx.lat;
  X(2, 1) = xx.lon;
  X(3, 1) = init_speed * 1852. / 3600.;   // speed in m / sec
  X(4, 1) = init_course * 1852. / 3600.;  // east speed in m / sec

  Matrix Z(2, 1);  //  measured target position and speed and course
  Z(1, 1) = xx.lat;
  Z(2, 1) = xx.lon;

  Matrix P(4, 4);  // Error covariance matrix, initial values
  P(1, 1) = 10;    // $$$ redifine!!
  P(2, 2) = 10.;
  P(3, 3) = 10.;
  P(4, 4) = 10.;
}

Kalman_Filter::~Kalman_Filter() {}

void Kalman_Filter::SetMeasurement(Position measured_pos) {
  // MetricPoint xx = Pos2Metric(measured_pos);
  /* X(1, 1) = xx.lat;
   X(2, 1) = xx.lon;*/
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman SetMeasurement"));
  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman size of X %i"), X.Size(1));
  int iii = Q1.Size(1);
  //  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman size of Q1 %i"), Q1.Size(1));
  return;
}

// void Kalman_Filter::Kalman_Next_Estimate(int delta_t, Position* x) {
//  LOG_INFO(wxT("BR24radar_pi: $$$ Kalman entry"));
//
//  Matrix PP(4, 4);  // Error covariance matrix
//
//  Matrix X(4, 1);  // Target position and speed
//  // X(1, 1) = 950.;  //not in original
//  // X(2, 1) = 10000.;
//  X(3, 1) = 5.;  // Initial values of speed
//  X(4, 1) = -45.;
//
//  Matrix A(4, 4);   // The Jacobi state transition matrix
//  Matrix AT(4, 4);  // Transpose of A
//
//  Matrix XP(4, 1);  // Estimate (expected) position of target
//
//  // Matrix Z(1, 2);  // Observation vector
//  Matrix ZT(2, 1);  // Transpose of Z
//
//  Matrix E(4, 1);  //  The difference between estimate and measurement
//
//  double dist;     // radial  measured
//  double angular;  // angular  measured  in meters
//
//  double EX;
//  double EY;
//  double PX;
//  double PXX;
//  double PY;
//  double PYY;
//
//  ESTRANGE = 10045.;
//  ESTANGLE = 1.46;
//  DELTAANGLE = 0.;
//  ESTX = 900.;
//  ESTY = 10050.;
//
//  double RR = ESTRANGE * ESTRANGE / 36481;
//  Matrix R(2, 2);  // Measurement error covariance matrix, depends on radar charasteristics
//  R(1, 1) = 625.;
//  R(2, 2) = RR;
//
//  Matrix K(1, 4);  // Kalman gain
//  //  The main loop of the file
//  for (int k = 1; k <= 135; k++) {
//    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman start loop k= %i"), k);
//
//    //  To polar coordinates
//    TRUERANGE = sqrt(TRUEX * TRUEX + TRUEY * TRUEY);
//    TRUEANGLE = atan(TRUEY / TRUEX);
//    //  Adding measurement noise
//    //   MATRANGE = TRUERANGE + 25 * distr(gen);
//    //   MATANGLE = TRUEANGLE + 0.005 * distr(gen);
//    //  To Cartesian coordinates to be able to plot the measurements
//    MATX = MATRANGE * cos(MATANGLE);
//    MATY = MATRANGE * sin(MATANGLE);
//    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman start k=%i, TRUEX= %f, TRUEY= %f, MATX= %f, MATY= %f"), k, TRUEX, TRUEY, MATX, MATY);
//    //  Calculation of the Jacobi matrix for A
//    jac_a = cos(DELTAANGLE);
//    jac_b = sin(DELTAANGLE);
//    A(1, 1) = jac_a;
//    A(1, 2) = jac_b;
//    A(2, 1) = -jac_b;
//    A(2, 2) = jac_a;
//    A(3, 3) = jac_a;
//    A(3, 4) = jac_b;
//    A(4, 3) = -jac_b;
//    A(4, 4) = jac_a;
//
//    AT(1, 1) = jac_a;
//    AT(2, 1) = jac_b;
//    AT(1, 2) = -jac_b;
//    AT(2, 2) = jac_a;
//    AT(3, 3) = jac_a;
//    AT(4, 3) = jac_b;
//    AT(3, 4) = -jac_b;
//    AT(4, 4) = jac_a;
//
//    //  Predict state
//    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman X(1,1)= %f, X(2,1)= %f, X(3,1)= %f, X(4,1)= %f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
//    XP = A * X;
//    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman X(1,1)= %f, X(2,1)= %f, X(3,1)= %f, X(4,1)= %f"), X(1, 1), X(2, 1), X(3, 1), X(4, 1));
//    LOG_INFO(wxT("BR24radar_pi: $$$ Kalman XP(1,1)= %f, XP(2,1)= %f, XP(3,1)= %f, XP(4,1)= %f"), XP(1, 1), XP(2, 1), XP(3, 1),
//             XP(4, 1));
//    //  The measurement
//    dist = MATRANGE * cos(MATANGLE - ESTANGLE) - ESTRANGE;  // diff between measured and estimated
//    angular = MATRANGE * tan(MATANGLE - ESTANGLE);
//    ZT(1, 1) = dist;
//    ZT(2, 1) = angular;
//    // Z = [dist angular]';
//    //  The difference between estimate and measurement
//    E = ZT - H * XP;  // seems to be wrong
//    EX = E(1, 1);
//    EY = E(2, 1);
//  PX = P(1, 1);
// PXX = 5 * (sqrt(PX));
// PY = P(2, 1);
// PYY = 5 * (sqrt(PX));
////  Measurement to track association
// if (abs(EX) > (2 * PXX)) {
//  H1(1, 1) = 0.;
//  H1(2, 2) = 0.;
//} else if (abs(EY) > (2 * PYY)) {
//  H1(1, 1) = 0.;
//  H1(2, 2) = 0.;
//} else if (abs(EY) > 3000) {
//  H1(1, 1) = 0.;
//  H1(2, 2) = 0.;
//} else if (abs(EY) > 3000) {
//  H1(1, 1) = 0.;
//  H1(2, 2) = 0.;
//} else {
//  H1(1, 1) = 1.;
//  H1(2, 2) = 1.;
//}
////  A check if the target is maneuvering
// if (abs(EX) > PXX) {
//  maneuvring = true;
//} else if (abs(EY) > PYY) {
//  maneuvring = true;
//} else {
//  maneuvring = false;
//}
////  Predict error covariance
// if (!maneuvring) {  //  non maneuvering target
//  PP = A * P * AT + Q1;
//} else {  //  maneuvering target
//  PP = A * P * AT + Q2;
//}
//  Calculation of Kalman gain
//    H1T(1, 1) = H1(1, 1);
//    H1T(2, 1) = H1(1, 2);
//    H1T(3, 1) = H1(1, 3);
//    H1T(4, 1) = H1(1, 4);
//
//    H1T(1, 2) = H1(2, 1);
//    H1T(2, 2) = H1(2, 2);
//    H1T(3, 2) = H1(2, 3);
//    H1T(4, 2) = H1(2, 4);
//
//    K = PP * H1T * Inv(H * PP * HT + R);
//    //  Calculation of the estimate
//    X = XP + K * (ZT - H * XP);
//    //  Calculate the angle and range differences between old and new estimate
//    DELTAANGLE = atan(X(4, 1) / ESTRANGE);
//    double DELTARANGE = X(3, 1);
//
//    //  Set new estimate
//    ESTANGLE = ESTANGLE + DELTAANGLE;
//    ESTRANGE = ESTRANGE + DELTARANGE;
//    //  Calculation of error covariance
//    //   P = PP - K * H * PP;
//    //  To Cartesian coordinates to be able to plot the estimate
//    ESTX = ESTRANGE * cos(ESTANGLE);
//    ESTY = ESTRANGE * sin(ESTANGLE);
//    //  Plotting
//    // axis(axlar);
//    //  plot(TRUEX, TRUEY, 'b*', 'markersize', 8); The true target position
//    // plot(MATX, MATY, 'k*', 'markersize', 3); //  The measurement
//    // if maneuvring < 1
//    //    plot(ESTX, ESTY, 'g*', 'markersize', 3); //  The estimate
//    // else
//    //    plot(ESTX, ESTY, 'r*', 'markersize', 3); //  The estimate
//    // end
//    //    //  Pause to regulate the speed of the plotting
//    //    pause(1);
//  }
//}

PLUGIN_END_NAMESPACE
