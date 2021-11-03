/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  DR Plugin
 * Author:   SaltyPaws/Mike Rossiter
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
#include "NavFunc.h"
#include <math.h>


#define DTOL                 1e-12

#define HALFPI  1.5707963267948966
#define SPI     3.14159265359
#define TWOPI   6.2831853071795864769
#define ONEPI   3.14159265358979323846
#define MERI_TOL 1e-9

double adjlon(double lon) {
	if (fabs(lon) <= SPI) return(lon);
	lon += ONEPI;  /* adjust to 0..2pi rad */
	lon -= TWOPI * floor(lon / TWOPI); /* remove integral # of 'revolutions'*/
	lon -= ONEPI;  /* adjust back to -pi..pi rad */
	return(lon);
}




//#include <string>
double toRad (double degree) {
  return degree * PI / 180;
}

double toDeg (double radians) {
  return radians * 180 / PI;
}

double sqr (double sqr2) {
  return sqr2*sqr2;
}

double max ( const double a, const double b ) {
  return (a<b)?b:a;     // or: return comp(a,b)?b:a; for the comp version
}

double min ( double a, double b ) {
  return !(b<a)?a:b;     // or: return !comp(b,a)?a:b; for the comp version
}

/*
Another potential implementation problem is that the arguments of asin and/or acos may, because of rounding error,
exceed one in magnitude. With perfect arithmetic this can't happen. You may need to use "safe" versions of asin and acos on the lines of:
*/
double asin_safe(double x){return asin(max(-1,min(x,1)));}
double acos_safe(double x){return acos(max(-1,min(x,1)));}

/*
Note on the mod function. This appears to be implemented differently in different languages, with differing conventions on whether
the sign of the result follows the sign of the divisor or the dividend. (We want the sign to follow the divisor or be Euclidean.
C's fmod and Java's % do not work.) In this document, Mod(y,x) is the remainder on dividing y by x and always lies in the range 0 <=mod <x.
For instance: mod(2.3,2.)=0.3 and mod(-2.3,2.)=1.7
*/
double mod(double y, double x){
    double mod=y - x * (int)(y/x);
    if ( mod < 0) mod = mod + x;
    return mod;
}

//convert radians to NM
double radtoNM(double distance_radians){
return((180*60)/PI)*distance_radians;
}
//convert NM to radians
double NMtorad(double distance_NM){
return (PI/(180*60))*distance_NM;
}


// convert metres to NM
double mtoNM(double metres){
return metres/1852;
}

// convert NM to metres
double NMtom(double NM){
return NM*1852;
}


/****************************************************************************/
/* Convert dd mm'ss.s" (DMS-Format) to degrees.                             */
/****************************************************************************/

double fromDMStodouble(char *dms)
{
    int d = 0, m = 0;
    double s = 0.0;
    char buf[20];

    buf[0] = '\0';

    sscanf(dms, "%d%[ ]%d%[ ']%lf%[ \"NSWEnswe]", &d, buf, &m, buf, &s, buf);

    s = (double) (abs(d)) + ((double) m + s / 60.0) / 60.0;

    if (d >= 0 && strpbrk(buf, "SWsw") == NULL)
      return s;
    else
      return -s;
}

/****************************************************************************/
/* Convert degrees to dd mm'ss.s" (DMS-Format)                              */
/****************************************************************************/
void doubletoDMS(double a, char *bufp, int bufplen)
{
    short neg = 0;
    int d, m, s;
    long n;

    if (a < 0.0) {
      a = -a;
      neg = 1;
    }
    d = (int) a;
    n = (long) ((a - (double) d) * 36000.0);

    m = n / 600;
    s = n % 600;

    if (neg)
      d = -d;

    sprintf(bufp, "%d%02d'%02d.%01d\"", d, m, s / 10, s % 10);
    return;
}

////////////////////////////
/**
 * Calculates rhumb line distance between two points specified by latitude/longitude
 * http://williams.best.vwh.net/avform.htm#Rhumb
 * ->   double lat1, lon1: first point in decimal degrees
 * ->   double lat2, lon2: second point in decimal degrees
 * <-   double dist: distance along bearing in nautical miles
 * <-   double bearing in decimal degrees
 * As stated in the introduction, North latitudes and West longitudes are treated as positive,
 * and South latitudes and East longitudes negative. It's easier to go with the flow, but if
 * you prefer another convention you can change the signs in the formulae.

void distRhumb(double lat1,double lon1, double lat2, double lon2, double *distance, double *brng){
  lat1=toRad(lat1);
  lat2=toRad(lat2);
  lon1=toRad(lon1);
  lon2=toRad(lon2);

  double dlon_W=mod(lon2-lon1,(2*PI));
  double dlon_E=mod(lon1-lon2,(2*PI));
  double dphi=log(tan(lat2/2+PI/4)/tan(lat1/2+PI/4));
  double q=0;
  if (std::abs(lat2-lat1) < sqrt(Tol())){
     q=cos(lat1);
  } else {
     q= (lat2-lat1)/dphi;
  }
  if (dlon_W < dlon_E){// Westerly rhumb line is the shortest
      *brng=toDeg(mod(atan2(-dlon_W,dphi),(2*PI)));
      *distance= radtoNM(sqrt(sqr(q)*(sqr(dlon_W)) + sqr(lat2-lat1)));
  } else{
      *brng=toDeg(mod(atan2(dlon_E,dphi),(2*PI)));
      *distance= radtoNM(sqrt(sqr(q)*(sqr(dlon_E)) + sqr(lat2-lat1)));
      }
}

*/

/**
 * To find the lat/lon of a point on true course brng, distance dist from (lat1,lon1)
 * along a rhumbline (initial point cannot be a pole!):
 *
 * ->   double lat1, lon1: first point in decimal degrees
 * ->   double brng: bearing in decimal degrees
 * ->   double dist: distance along bearing in nautical miles
 * <-   double lat2, lat2 final point in decimal degrees



bool destRhumb(double lat1, double lon1, double brng, double dist, double* lat2, double* lon2) {
  lat1=toRad(lat1);
  lon1=toRad(lon1);
  double d=NMtorad(dist);
  double tc=toRad(brng);
  double lat= lat1+d*cos(tc);
    if (std::abs(lat) > PI/2) return false;//"d too large. You can't go this far along this rhumb line!"
    double q;
    if (std::abs(lat-lat1) < sqrt(Tol())){
     q=cos(lat1);
  } else {
     double dphi=log(tan(lat/2+PI/4)/tan(lat1/2+PI/4));
     q= (lat-lat1)/dphi;
  }
    double dlon=-d*sin(tc)/q;
    *lon2=toDeg(mod(lon1+dlon+PI,2*PI)-PI);
    *lat2=toDeg(lat);
  return true;
}
 */
//Loxodrome (Mercator) Destination the WGS ellipsoid
//http://koti.mbnet.fi/jukaukor/loxodrom.html
/**
 * To find the lat/lon of a point on true course brng, distance dist from (lat1,lon1)
 * along a rhumbline (initial point cannot be a pole!):
 *
 * ->   double lat1, lon1: first point in decimal degrees
 * ->   double brng: bearing in decimal degrees
 * ->   double dist: distance along bearing in nautical miles
 * <-   double lat2, lat2 final point in decimal degrees

*/
bool destLoxodrome(double lat1, double lon1, double brng, double dist, double* lat2, double* lon2) {

	bool dbg = false;
	if (dbg) std::cout << "dL lat1 " << lat1 << " lon1: " << lon1 << "brng " << brng << " dist " << dist << std::endl;
	//double ecc = 0.08181919084255; //The eccentricity ecc of the WGS84 ellipsoid is
	double ecc2 = 0.00669437999012962; //(ecc²)/1
	double ecc4 = 4.48147234522478E-05;//(ecc⁴)/3
	double ecc6 = 3.0000678794192E-07;//(ecc⁶)/5
	double ecc8 = 2.00835943810145E-09;//(ecc⁸)/7 (check)
	double fromlat = toRad(lat1);//fromlat = fromlatdeg*Math.PI/180;
	//double fromlong = toRad(lon1);// fromlongdeg*Math.PI/180;
	double course = toRad(brng);//course = coursedeg*Math.PI/180;

	// Destination latitude
	double tolatmin = lat1 * 60 + dist*cos(course);
	double tolatdegree = tolatmin / 60;
	// Check Poles
	if (tolatdegree > 90) {
		tolatdegree = 90;
		if (dbg) std::cout << "You are on the North Pole" << std::endl;
	}
	if (tolatdegree < -90) {
		if (dbg) std::cout << "You are on the South Pole. tolatdegree" << tolatdegree << std::endl;
		tolatdegree = -90;
	}

	double tolat = toRad(tolatdegree);

	double tolongdegree = 0;
	if ((tolatdegree != 90) && (tolatdegree != -90)) {
		double meridional1 = log(tan(PI / 4 + fromlat / 2)) - ecc2*sin(fromlat) - (ecc4 / 3)*pow(sin(fromlat), 3) - (ecc6 / 5)*pow(sin(fromlat), 5) - (ecc8 / 7)*pow(sin(fromlat), 7);
		double meridionalmin1 = 10800 * meridional1 / PI;
		double meridional2 = log(tan(PI / 4 + tolat / 2)) - ecc2*sin(tolat) - (ecc4 / 3)*pow(sin(tolat), 3) - (ecc6 / 5)*pow(sin(tolat), 5) - (ecc8 / 7)*pow(sin(tolat), 7);
		double meridionalmin2 = 10800 * meridional2 / PI;
		// deltaM is meridional difference in minutes
		double deltaM = meridionalmin2 - meridionalmin1;

		// Destination longitude, non parallel sailing
		if ((brng != 90) && (brng != 270))
			tolongdegree = lon1 + (deltaM*tan(course)) / 60;

		// parallel navigation, distance is in nautical miles = equator minutes;
		if (fabs(brng - 90)< 1e-10) {
			tolongdegree = lon1 + (dist / cos(fromlat)) / 60;
			tolatdegree = lat1 + 1e-8;
		}
		if (fabs(brng - 270) < 1e-10) {
			tolongdegree = lon1 - (dist / cos(fromlat)) / 60;
			tolatdegree = lat1 + 1e-8;
		}
	}
	else
		return false;
	*lat2 = tolatdegree;
	*lon2 = tolongdegree;
	return true;
}
/// New functions
void DistanceBearingMercator(double lat0, double lon0, double lat1, double lon1, double *dist, double *brg)
{
	//    Calculate bearing by conversion to SM (Mercator) coordinates, then simple trigonometry

	double lon0x = lon0;
	double lon1x = lon1;

	//    Make lon points the same phase
	if ((lon0x * lon1x) < 0.)
	{
		lon0x < 0.0 ? lon0x += 360.0 : lon1x += 360.0;
		//    Choose the shortest distance
		if (fabs(lon0x - lon1x) > 180.)
		{
			lon0x > lon1x ? lon0x -= 360.0 : lon1x -= 360.0;
		}

		//    Make always positive
		lon1x += 360.;
		lon0x += 360.;
	}

	//    Classic formula, which fails for due east/west courses....
	if (dist)
	{
		//    In the case of exactly east or west courses
		//    we must make an adjustment if we want true Mercator distances

		//    This idea comes from Thomas(Cagney)
		//    We simply require the dlat to be (slightly) non-zero, and carry on.
		//    MAS022210 for HamishB from 1e-4 && .001 to 1e-9 for better precision
		//    on small latitude diffs
		const double mlat0 = fabs(lat1 - lat0) < 1e-9 ? lat0 + 1e-9 : lat0;

		double east, north;
		toSM_ECC(lat1, lon1x, mlat0, lon0x, &east, &north);
		const double C = atan2(east, north);
		if (cos(C))
		{
			const double dlat = (lat1 - mlat0) * 60.;              // in minutes
			*dist = (dlat / cos(C));
		}
		else
		{
			*dist = DistGreatCircle(lat0, lon0, lat1, lon1);
		}
	}

	//    Calculate the bearing using the un-adjusted original latitudes and Mercator Sailing
	if (brg)
	{
		double east, north;
		toSM_ECC(lat1, lon1x, lat0, lon0x, &east, &north);

		const double C = atan2(east, north);
		const double brgt = 180. + (C * 180. / PI);
		if (brgt < 0)
			*brg = brgt + 360.;
		else if (brgt >= 360.)
			*brg = brgt - 360.;
		else
			*brg = brgt;
	}
}

void toSM_ECC(double lat, double lon, double lat0, double lon0, double *x, double *y)
{
	const double f = 1.0 / WGSinvf;       // WGS84 ellipsoid flattening parameter
	const double e2 = 2 * f - f * f;      // eccentricity^2  .006700
	const double e = sqrt(e2);

	const double z = WGS84_semimajor_axis_meters * mercator_k0;

	*x = (lon - lon0) * DEGREE * z;

	// y =.5 ln( (1 + sin t) / (1 - sin t) )
	const double s = sin(lat * DEGREE);
	const double y3 = (.5 * log((1 + s) / (1 - s))) * z;

	const double s0 = sin(lat0 * DEGREE);
	const double y30 = (.5 * log((1 + s0) / (1 - s0))) * z;
	const double y4 = y3 - y30;

	//Add eccentricity terms

	const double falsen = z *log(tan(PI / 4 + lat0 * DEGREE / 2)*pow((1. - e * s0) / (1. + e * s0), e / 2.));
	const double test = z *log(tan(PI / 4 + lat  * DEGREE / 2)*pow((1. - e * s) / (1. + e * s), e / 2.));
	*y = test - falsen;
}
/*
void PositionBearingDistanceMercator(double lat, double lon, double brg, double dist, double *dlat, double *dlon)
{
	ll_gc_ll(lat, lon, brg, dist, dlat, dlon);
}


void ll_gc_ll(double lat, double lon, double brg, double dist, double *dlat, double *dlon)
{

	double th1, costh1, sinth1, sina12, cosa12, M, N, c1, c2, D, P, s1;
	int merid, signS;

	//   Input/Output from geodesic functions   //
	double al12;         //   Forward azimuth //
	double al21;         //   Back azimuth    //
	double geod_S;       //   Distance        //
	double phi1, lam1, phi2, lam2;

	int ellipse;
	double geod_f;
	double geod_a;
	double es, onef, f, f4;

	//      Setup the static parameters
	phi1 = lat * DEGREE;           //  Initial Position
	lam1 = lon * DEGREE;
	al12 = brg * DEGREE;           //  Forward azimuth
	geod_S = dist * 1852.0;        //  Distance


	//void geod_pre(struct georef_state *state)
	{

		//  Stuff the WGS84 projection parameters as necessary
		//      To avoid having to include <geodesic,h>
		//
		ellipse = 1;
		f = 1.0 / WGSinvf;     //  WGS84 ellipsoid flattening parameter
		geod_a = WGS84_semimajor_axis_meters;

		es = 2 * f - f * f;
		onef = sqrt(1. - es);
		geod_f = 1 - onef;
		//        f2 = geod_f/2;
		f4 = geod_f / 4;
		//        f64 = geod_f*geod_f/64;

		al12 = adjlon(al12);//  reduce to  +- 0-PI
		signS = fabs(al12) > HALFPI ? 1 : 0;
		th1 = ellipse ? atan(onef * tan(phi1)) : phi1;
		costh1 = cos(th1);
		sinth1 = sin(th1);
		if ((merid = fabs(sina12 = sin(al12)) < MERI_TOL)) {
			sina12 = 0.;
			cosa12 = fabs(al12) < HALFPI ? 1. : -1.;
			M = 0.;
		}
		else {
			cosa12 = cos(al12);
			M = costh1 * sina12;
		}
		N = costh1 * cosa12;
		if (ellipse) {
			if (merid) {
				c1 = 0.;
				c2 = f4;
				D = 1. - c2;
				D *= D;
				P = c2 / D;
			}
			else {
				c1 = geod_f * M;
				c2 = f4 * (1. - M * M);
				D = (1. - c2)*(1. - c2 - c1 * M);
				P = (1. + .5 * c1 * M) * c2 / D;
			}
		}
		if (merid) s1 = HALFPI - th1;
		else {
			s1 = (fabs(M) >= 1.) ? 0. : acos(M);
			s1 = sinth1 / sin(s1);
			s1 = (fabs(s1) >= 1.) ? 0. : acos(s1);
		}
	}


	//void  geod_for(struct georef_state *state)
	{
		double d, sind, u, V, X, ds, cosds, sinds, ss, de;

		ss = 0.;

		if (ellipse) {
			d = geod_S / (D * geod_a);
			if (signS) d = -d;
			u = 2. * (s1 - d);
			V = cos(u + d);
			X = c2 * c2 * (sind = sin(d)) * cos(d) * (2. * V * V - 1.);
			ds = d + X - 2. * P * V * (1. - 2. * P * cos(u)) * sind;
			ss = s1 + s1 - ds;
		}
		else {
			ds = geod_S / geod_a;
			if (signS) ds = -ds;
		}
		cosds = cos(ds);
		sinds = sin(ds);
		if (signS) sinds = -sinds;
		al21 = N * cosds - sinth1 * sinds;
		if (merid) {
			phi2 = atan(tan(HALFPI + s1 - ds) / onef);
			if (al21 > 0.) {
				al21 = PI;
				if (signS)
					de = PI;
				else {
					phi2 = -phi2;
					de = 0.;
				}
			}
			else {
				al21 = 0.;
				if (signS) {
					phi2 = -phi2;
					de = 0;
				}
				else
					de = PI;
			}
		}
		else {
			al21 = atan(M / al21);
			if (al21 > 0)
				al21 += PI;
			if (al12 < 0.)
				al21 -= PI;
			al21 = adjlon(al21);
			phi2 = atan(-(sinth1 * cosds + N * sinds) * sin(al21) /
				(ellipse ? onef * M : M));
			de = atan2(sinds * sina12,
				(costh1 * cosds - sinth1 * sinds * cosa12));
			if (ellipse){
				if (signS)
					de += c1 * ((1. - c2) * ds +
					c2 * sinds * cos(ss));
				else
					de -= c1 * ((1. - c2) * ds -
					c2 * sinds * cos(ss));
			}
		}
		lam2 = adjlon(lam1 + de);
	}


	*dlat = phi2 / DEGREE;
	*dlon = lam2 / DEGREE;
}

*/

double DistGreatCircle(double slat, double slon, double dlat, double dlon)
{
	//    double th1,costh1,sinth1,sina12,cosa12,M,N,c1,c2,D,P,s1;
	//    int merid, signS;

	//   Input/Output from geodesic functions
	double al12;           // Forward azimuth
	double al21;           // Back azimuth
	double geod_S;         // Distance
	double phi1, lam1, phi2, lam2;

	int ellipse;
	double geod_f;
	double geod_a;
	double es, onef, f, f64, f2, f4;

	double d5;
	phi1 = slat * DEGREE;
	lam1 = slon * DEGREE;
	phi2 = dlat * DEGREE;
	lam2 = dlon * DEGREE;

	//void geod_inv(struct georef_state *state)
	{
		double      th1, th2, thm, dthm, dlamm, dlam, sindlamm, costhm, sinthm, cosdthm,
			sindthm, L, E, cosd, d, X, Y, T, sind, tandlammp, u, v, D, A, B;


		//   Stuff the WGS84 projection parameters as necessary
		//      To avoid having to include <geodesic,h>
		//

		ellipse = 1;
		f = 1.0 / WGSinvf;       // WGS84 ellipsoid flattening parameter //
		geod_a = WGS84_semimajor_axis_meters;

		es = 2 * f - f * f;
		onef = sqrt(1. - es);
		geod_f = 1 - onef;
		f2 = geod_f / 2;
		f4 = geod_f / 4;
		f64 = geod_f*geod_f / 64;


		if (ellipse) {
			th1 = atan(onef * tan(phi1));
			th2 = atan(onef * tan(phi2));
		}
		else {
			th1 = phi1;
			th2 = phi2;
		}
		thm = .5 * (th1 + th2);
		dthm = .5 * (th2 - th1);
		dlamm = .5 * (dlam = adjlon(lam2 - lam1));
		if (fabs(dlam) < DTOL && fabs(dthm) < DTOL) {
			al12 = al21 = geod_S = 0.;
			return 0.0;
		}
		sindlamm = sin(dlamm);
		costhm = cos(thm);      sinthm = sin(thm);
		cosdthm = cos(dthm);    sindthm = sin(dthm);
		L = sindthm * sindthm + (cosdthm * cosdthm - sinthm * sinthm)
			* sindlamm * sindlamm;
		d = acos(cosd = 1 - L - L);
		if (ellipse) {
			E = cosd + cosd;
			sind = sin(d);
			Y = sinthm * cosdthm;
			Y *= (Y + Y) / (1. - L);
			T = sindthm * costhm;
			T *= (T + T) / L;
			X = Y + T;
			Y -= T;
			T = d / sind;
			D = 4. * T * T;
			A = D * E;
			B = D + D;
			geod_S = geod_a * sind * (T - f4 * (T * X - Y) +
				f64 * (X * (A + (T - .5 * (A - E)) * X) -
				Y * (B + E * Y) + D * X * Y));
			tandlammp = tan(.5 * (dlam - .25 * (Y + Y - E * (4. - X)) *
				(f2 * T + f64 * (32. * T - (20. * T - A)
				* X - (B + 4.) * Y)) * tan(dlam)));
		}
		else {
			geod_S = geod_a * d;
			tandlammp = tan(dlamm);
		}
		u = atan2(sindthm, (tandlammp * costhm));
		v = atan2(cosdthm, (tandlammp * sinthm));
		al12 = adjlon(TWOPI + v - u);
		al21 = adjlon(TWOPI - v - u);
	}

	d5 = geod_S / 1852.0;

	return d5;
}
