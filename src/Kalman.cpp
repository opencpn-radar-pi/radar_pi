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
 This specific implementation is based on the paper: Kalman filtering. With a radar tracking implementation.
 by Fredrik Svanström, Linaeus Univerity, Kalmar, Sweden. This paper describes the tracking used in the
 Saab military surveillance radar PS-90, see: http://www.diva-portal.org/smash/get/diva2:668963/fulltext01.pdf
 The paper above gives an implementation of the tracker in Matlab code, which was used for this implementation.
 */

#include "Kalman.h"
#include <random>  // $$$ remove later
#include "Matrix.cpp"
#include "br24radar_pi.h"
PLUGIN_BEGIN_NAMESPACE

//  tracker.m The MATLAB - file for the paper on Kalman filtering
//  Made by Fredrik Svanström 2013
// Converted and adapted for C++ by Douwe Fokkema

// $$$ for test only
std::random_device rd;
std::mt19937 gen(rd());
std::normal_distribution<> distr(0, 1);

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

void Kalman_Filter::KalmanDemo() {
  Matrix axlar(4, 1);
  axlar(1, 1) = 0.;
  axlar(2, 1) = 12000.;
  axlar(3, 1) = 0.;
  axlar(4, 1) = 12000.;

  // hold on
  //  The target start position

  TRUEX = 950.;
  TRUEY = 10000.;
  //  Variables used to create the target track
  int ACC = 1;
  float SVANGVINKEL1 = 0.14;
  float SVANGVINKEL2 = 0.45;

  //  Initial values
  bool maneuvring = false;

  Matrix Q1(4, 4);  // Error covariance matrix when not maneuvring
  Q1(3, 3) = 100.;
  Q1(4, 4) = 100.;

  Matrix Q2(4, 4);
  Q2(3, 3) = 3600.;  // Error covariance matrix when maneuvring
  Q2(4, 4) = 3600.;

  Matrix H(2, 4);  // Observation matrix
  H(1, 1) = 1.;
  H(2, 2) = 1.;

  Matrix HT(4, 2);  // Transpose of observation matrix
  HT(1, 1) = 1.;
  HT(2, 2) = 1.;

  Matrix H1(2, 4);   // Variable observation matrix
  Matrix H1T(4, 2);  // Translated H1

  Matrix P(4, 4);  // Error covariance matrix, initial values
  P(1, 1) = 1000.;
  P(2, 2) = 1000.;
  P(3, 3) = 3600.;
  P(4, 4) = 3600.;

  Matrix PP(4, 4);  // Error covariance matrix

  Matrix X(4, 1);  // Target position and speed
  X(3, 1) = 5.;    // Initial values
  X(4, 1) = -45.;

  Matrix A(4, 4);   // The Jacobi state transition matrix
  Matrix AT(4, 4);  // Transpose of A

  Matrix XP(4, 1);  // Estimate (expected) position of target

  Matrix Z(1, 2);  // Observation vector

  Matrix E(4, 1);  //  The difference between estimate and measurement

  double dist;     // radial  measured
  double angular;  // angular  measured  in meters

  double EX;
  double EY;
  double PX;
  double PXX;
  double PY;
  double PYY;

  ESTRANGE = 10045.;
  ESTANGLE = 1.46;
  DELTAANGLE = 0.;
  ESTX = 900.;
  ESTY = 10050.;

  double RR = ESTRANGE * ESTRANGE / 36481;
  Matrix R(1, 4);  // Measurement error covariance matrix, depends on radar charasteristics
  R(1, 1) = 625.;
  R(2, 2) = RR;

  Matrix K(1, 4);  // Kalman gain

  //  The main loop of the file
  for (int k = 1; k <= 135; k++) {
    //  Part 1 of the target path
    if (k < 20) {
      TRUEX = TRUEX + 50;
    }
    //  Part 2 of the target path, acceleration
    if (k >= 20) {
      if (k < 27) {
        TRUEX = TRUEX + 50 + 20 * ACC;
        ACC = ACC + 1;
      }
    }
    //  Part 3 of the target path
    if (k >= 27) {
      if (k < 60) {
        TRUEX = TRUEX + 200;
      }
    }
    //  Part 4 of the target path, turning at 3g
    if (k >= 60) {
      if (k < 83) {
        double SVANGMITTX1 = 9410;
        double SVANGMITTY1 = 8550;
        TRUEX = SVANGMITTX1 + 1450 * sin(SVANGVINKEL1);
        TRUEY = SVANGMITTY1 + 1450 * cos(SVANGVINKEL1);
        SVANGVINKEL1 = SVANGVINKEL1 + 0.14;
      }
    }
    //  Part 5 of the target path
    if (k >= 83) {
      if (k < 95) {
        TRUEX = TRUEX - 200;
      }
    }
    //  Part 6 of the target path when the target is not visible
    if (k >= 95) {
      if (k < 103) {
        TRUEX = TRUEX - 200000;
        TRUEY = TRUEX - 200000;
      }
    }
    //  Part 7 of the target path, the target is now visible again
    if (k == 103) {
      TRUEX = 5296;
      TRUEY = 7104;
    }
    if (k >= 103) {
      if (k < 115) {
        TRUEX = TRUEX - 200;
        TRUEY = TRUEY;
      }
    }
    //  Part 8 of the target path, turning at 9g
    if (k >= 115) {
      if (k < 122) {
        int SVANGMITTX2 = 2896;
        int SVANGMITTY2 = 6654;
        TRUEX = SVANGMITTX2 - 455 * sin(SVANGVINKEL2);
        TRUEY = SVANGMITTY2 + 455 * cos(SVANGVINKEL2);
        SVANGVINKEL2 = SVANGVINKEL2 + 0.45;
      }
    }
    //  Part 9 of the target path
    if (k >= 122) {
      TRUEX = TRUEX + 200.;
    }

    //  To polar coordinates
    TRUERANGE = sqrt(TRUEX * TRUEX + TRUEY * TRUEY);
    TRUEANGLE = atan(TRUEY / TRUEX);
    //  Adding measurement noise
    MATRANGE = TRUERANGE + 25 * distr(gen);
    MATANGLE = TRUEANGLE + 0.005 * distr(gen);
    //  To Cartesian coordinates to be able to plot the measurements
    MATX = MATRANGE * cos(MATANGLE);
    MATY = MATRANGE * sin(MATANGLE);

    //  Calculation of the Jacobi matrix for A
    jac_a = cos(DELTAANGLE);
    jac_b = sin(DELTAANGLE);
    A(1, 3) = jac_a;
    A(1, 4) = jac_b;
    A(2, 3) = -jac_b;
    A(2, 4) = jac_a;
    A(3, 3) = jac_a;
    A(3, 4) = jac_b;
    A(4, 3) = -jac_b;
    A(4, 4) = jac_a;

    AT(3, 1) = jac_a;
    AT(4, 1) = jac_b;
    AT(3, 2) = -jac_b;
    AT(4, 2) = jac_a;
    AT(3, 3) = jac_a;
    AT(4, 3) = jac_b;
    AT(3, 4) = -jac_b;
    AT(4, 4) = jac_a;

    //  Predict state
    XP = A * X;
    //  The measurement
    dist = MATRANGE * cos(MATANGLE - ESTANGLE) - ESTRANGE;
    angular = MATRANGE * tan(MATANGLE - ESTANGLE);

    Z(1, 1) = dist;
    Z(2, 1) = angular;
    // Z = [dist angular]';
    //  The difference between estimate and measurement
    E = Z - H * XP;
    EX = E(1, 1);
    EY = E(2, 1);
    PX = P(1, 1);

    PXX = 5 * (sqrt(PX));
    PY = P(2, 1);
    PYY = 5 * (sqrt(PX));
    //  Measurement to track association
    if (abs(EX) > (2 * PXX)) {
      H1(1, 1) = 0.;
      H1(2, 2) = 0.;
    } else if (abs(EY) > (2 * PYY)) {
      H1(1, 1) = 0.;
      H1(2, 2) = 0.;
    } else if (abs(EY) > 3000) {
      H1(1, 1) = 0.;
      H1(2, 2) = 0.;
    } else if (abs(EY) > 3000) {
      H1(1, 1) = 0.;
      H1(2, 2) = 0.;
    } else {
      H1(1, 1) = 1.;
      H1(2, 2) = 1.;
    }
    //  A check if the target is maneuvering
    if (abs(EX) > PXX) {
      maneuvring = true;
    } else if (abs(EY) > PYY) {
      maneuvring = true;
    } else {
      maneuvring = false;
    }
    //  Predict error covariance
    if (!maneuvring) {  //  non maneuvering target
      PP = A * P * AT + Q1;
    } else {  //  maneuvering target
      PP = A * P * AT + Q2;
    }

    //  Calculation of Kalman gain
    H1T(1, 1) = H1(1, 1);
    H1T(2, 1) = H1(1, 2);
    H1T(3, 1) = H1(1, 3);
    H1T(4, 1) = H1(1, 4);

    H1T(1, 2) = H1(2, 1);
    H1T(2, 2) = H1(2, 2);
    H1T(3, 2) = H1(2, 3);
    H1T(4, 2) = H1(2, 4);

    K = PP * H1T * Inv(H * PP * HT + R);
    //  Calculation of the estimate
    X = XP + K * (Z - H * XP);
    //  Calculate the angle and range differences between old and new estimate
    DELTAANGLE = atan(X(4, 1) / ESTRANGE);
    double DELTARANGE = X(3, 1);

    //  Set new estimate
    ESTANGLE = ESTANGLE + DELTAANGLE;
    ESTRANGE = ESTRANGE + DELTARANGE;
    //  Calculation of error covariance
    P = PP - K * H * PP;
    //  To Cartesian coordinates to be able to plot the estimate
    ESTX = ESTRANGE * cos(ESTANGLE);
    ESTY = ESTRANGE * sin(ESTANGLE);
    //  Plotting
    //axis(axlar);
    //  plot(TRUEX, TRUEY, 'b*', 'markersize', 8); The true target position
    // plot(MATX, MATY, 'k*', 'markersize', 3); //  The measurement
    // if maneuvring < 1
    //    plot(ESTX, ESTY, 'g*', 'markersize', 3); //  The estimate
    // else
    //    plot(ESTX, ESTY, 'r*', 'markersize', 3); //  The estimate
    // end
    //    //  Pause to regulate the speed of the plotting
    //    pause(1);
  }
}

PLUGIN_END_NAMESPACE
