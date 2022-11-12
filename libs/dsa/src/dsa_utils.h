/******************************************************************************
 *
 * Project:  s63_pi
 * Purpose:  DSA Utility Functions
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2014 by David S. Register   *
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

#ifndef _DSA_UTILS_H_
#define _DSA_UTILS_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "mp_math.h"

//      Miscellaneous prototypes

extern "C" bool validate_enc_signature( const wxString &sig_file_name, const wxString &key_file_name);
extern "C" bool validate_enc_cell( const wxString &sig_file_name, const wxString &cell_file_name);
extern "C" bool check_enc_signature_format( const wxString &sig_file_name );



//---------------------------------------------------------------------------------------------------
//
//      Public key class
//      Definition
//
//----------------------------------------------------------------------------------------------------

class pub_key
{
public:
    pub_key();
    ~pub_key();

    void ReadKey(const wxArrayString &key_array);
    
    mp_int      m_p;
    mp_int      m_q;
    mp_int      m_g;
    mp_int      m_y;
    
    bool        m_OK;
    
};


#endif
