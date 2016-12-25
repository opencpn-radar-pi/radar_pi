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
class LocalPosition;
class Matrix;
class ArpaTarget;
class Polar;

class Kalman_Filter {
 public:
  Kalman_Filter(int range);
  Kalman_Filter::Kalman_Filter();
  ~Kalman_Filter();
  void SetMeasurement(Polar* p, LocalPosition* x, Polar* expected, int range);
  void Predict(LocalPosition* x, double delta_time);  // measured position and expected position
  void ResetFilter();

  Matrix A;
  Matrix AT;
  Matrix W;
  Matrix WT;
  Matrix H;
  Matrix HT;
  Matrix V;
  Matrix VT;
  Matrix P;
  Matrix Q;
  Matrix FT;
  Matrix R;
  Matrix K;
  Matrix I;
};

PLUGIN_END_NAMESPACE
#endif
