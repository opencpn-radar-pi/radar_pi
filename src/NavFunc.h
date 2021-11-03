/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  DR Plugin
 * Author:   SaltyPaws
 *
 ***************************************************************************
 *   Copyright (C) 2012 by Brazil BrokeTail                                *
 *   $EMAIL$                                                               *
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
#ifndef ROUTE_NAV_FUNC_H
#define ROUTE_NAV_FUNC_H

#include <iostream>
//#include <cmath>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <wx/msgdlg.h>



#ifndef PI
#define PI        3.1415926535897931160E0      /* pi */
#endif
#define DEGREE    (PI/180.0)

const inline double Tol(void) { return 1.0e-9; }

static const double WGS84_semimajor_axis_meters = 6378137.0;           // WGS84 semimajor axis
static const double mercator_k0 = 0.9996;
static const double WGSinvf = 298.257223563;       /* WGS84 1/f */

double fromDMStodouble(char *dms);
void doubletoDMS(double a, char *bufp, int bufplen);

double toRad (double degree);
double toDeg (double radians);
double sqr (double radians);

double radtoNM(double distance_radians);
double NMtorad(double distance_NM);
double mtoNM(double metres);
double NMtom(double NM);

//void DestVincenty(double lat1, double lon1, double brng, double dist, double* lat2, double* lon2, double* revAz);
//bool DistVincenty(double lat1, double lon1, double lat2, double lon2, double *dist, double *fwdAz, double *revAz);
//void distRhumb(double lat1,double lon1, double lat2, double lon2, double *distance, double *brng);
//bool destRhumb(double lat1, double lon1, double brng, double dist, double* lat2, double* lon2);
bool destLoxodrome(double lat1, double lon1, double brng, double dist, double* lat2, double* lon2);

// New functions
void DistanceBearingMercator(double lat0, double lon0, double lat1, double lon1, double *dist, double *brg);
void toSM_ECC(double lat, double lon, double lat0, double lon0, double *x, double *y);
//void PositionBearingDistanceMercator(double lat, double lon, double brg, double dist,
	//double *dlat, double *dlon);
//void ll_gc_ll(double lat, double lon, double brg, double dist, double *dlat, double *dlon);

double DistGreatCircle(double slat, double slon, double dlat, double dlon);


#endif/* #define ROUTE_NAV_FUNC_H*/
