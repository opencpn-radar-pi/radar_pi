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
 *   Copyright (C) 2012-2025 by Kees Verruijt         canboat@verruijt.net *
 *   Copyright (C) 2013-2025 by Douwe Fokkema              df@percussion.nl*
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

#ifndef _DOPPLER_H_
#define _DOPPLER_H_

#include "radar_pi.h"

PLUGIN_BEGIN_NAMESPACE

/*
Doppler states of the target.
A Doppler state of a target is an attribute of the target that determines the
search method for the target in the history array, according to the following
table:

x means don't care, bit0 is the above threshold bit, bit2 is the APPROACHING
bit, bit3 is the RECEDING bit.

                  bit0   bit2   bit3
ANY                  1      x      x
NO_DOPPLER           1      0      0
APPROACHING          1      1      0
RECEDING             1      0      1
ANY_DOPPLER          1      1      0   or
                     1      0      1
NOT_RECEDING         1      x      0
NOT_APPROACHING      1      0      x

ANY is typical non Dopper target
NOT_RECEDING and NOT_APPROACHING are only used to check countour length in the
transition of APPROACHING or RECEDING -> ANY ANY_DOPPLER is only used in the
search for targets and converted to APPROACHING or RECEDING in the first refresh
cycle

State transitions:
ANY -> APPROACHING or RECEDING  (not yet implemented)
APPROACHING or RECEDING -> ANY  (based on length of contours)
ANY_DOPPLER -> APPROACHING or RECEDING

*/

enum Doppler {
    ANY, // any target above threshold
    NO_DOPPLER, // a target without a Doppler bit
    APPROACHING,
    RECEDING,
    ANY_DOPPLER, // APPROACHING or RECEDING
    NOT_RECEDING, // that is NO_DOPPLER or APPROACHING
    NOT_APPROACHING, // that is NO_DOPPLER or RECEDING
    ANY_PLUS // will also check bits that have been cleared
};

PLUGIN_END_NAMESPACE

#endif /* _DOPPLER_H_ */
