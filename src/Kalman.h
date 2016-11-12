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
 */

#ifndef _BR24KALMAN_H_
#define _BR24KALMAN_H_

#include "Matrix.h"
#include "RadarMarpa.h"
#include "br24radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class Position;
class Matrix;
class ArpaTarget;
class MetricPoint;

class Kalman_Filter {
 public:
  Kalman_Filter(Position init_position, double init_speed, double init_course);
  ~Kalman_Filter();
  MetricPoint SetMeasurement(MetricPoint Z, MetricPoint X);
  MetricPoint Predict(MetricPoint x);  // measured position and expected position
  

  Matrix Q1;   // Error covariance matrix when not maneuvring
  Matrix Q2;   // Error covariance matrix when maneuvring
  Matrix H;    // Observation matrix
  Matrix HT;   // Transpose of observation matrix
  Matrix H1;   // Variable observation matrix
  Matrix H1T;  // Transposed H1
  Matrix X;    // estimated target position and speed and course
  Matrix Z;    // measured target position and speed and course
  Matrix P;    // Error covariance matrix, initial values
  Matrix K;    // Kalman gain
  Matrix F;    // State transition operator, from one measurement to the next

  bool maneuvring;
};

PLUGIN_END_NAMESPACE
#endif
