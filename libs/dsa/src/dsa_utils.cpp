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


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "dsa_utils.h"
#include "sha1.h"

#include <wx/textfile.h>
#include "wx/tokenzr.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/file.h"
#include "wx/wfstream.h"


//---------------------------------------------------------------------------------------------------
//
//      Public key class
//      Implementation
//
//----------------------------------------------------------------------------------------------------

pub_key::pub_key()
{
    mp_init( &m_p );
    mp_init( &m_q );
    mp_init( &m_g );
    mp_init( &m_y );
    
    m_OK = false;
}

pub_key::~pub_key()
{
}


//      Initialize the class members from an array string that looks like this:
/*
 // Big p
 C16C BAD3 4D47 5EC5 3966 95D6 94BC 8BC4 7E59 8E23 B5A9 D7C5 CEC8 2D65 B682 7D44 E953 7848 4730 C0BF F1F4 CB56 F47C 6E51 054B E892 00F3 0D43 DC4F EF96 24D4 665B.*
 // Big q
 B7B8 10B5 8C09 34F6 4287 8F36 0B96 D7CC 26B5 3E4D.
 // Big g
 4C53 C726 BDBF BBA6 549D 7E73 1939 C6C9 3A86 9A27 C5DB 17BA 3CAC 589D 7B3E 003F A735 F290 CFD0 7A3E F10F 3515 5F1A 2EF7 0335 AF7B 6A52 11A1 1035 18FB A44E 9718.
 // Big y
 063A C955 F639 B2F9 202E 070C 4A10 E82F 877A BC7F D928 D5F4 55C2 A3BF E928 92C5 9EB5 5DB0 ED6A 9555 ED8F 1C6E F218 DB62 FFFD F74E 5755 A989 44C7 6B50 9C41 B022.

 */
void pub_key::ReadKey(const wxArrayString &key_array)
{
    for(unsigned int i=0 ; i < key_array.Count() ; i++){
        wxString line = key_array[i];
        if( line.Upper().Find(_T("BIG P")) != wxNOT_FOUND ){
            if( (i+1) < key_array.Count() ){
                wxString key_line = key_array[i+1];
                key_line.Replace(_T(" "), _T("") );
                wxCharBuffer lbuf = key_line.ToUTF8();
                mp_read_radix(&m_p, lbuf.data(), 16);
            }
        }
        
        else if( line.Upper().Find(_T("BIG Q")) != wxNOT_FOUND ){
            if( (i+1) < key_array.Count() ){
                wxString key_line = key_array[i+1];
                key_line.Replace(_T(" "), _T("") );
                wxCharBuffer lbuf = key_line.ToUTF8();
                mp_read_radix(&m_q, lbuf.data(), 16);
            }
        }
        
        else if( line.Upper().Find(_T("BIG G")) != wxNOT_FOUND ){
            if( (i+1) < key_array.Count() ){
                wxString key_line = key_array[i+1];
                key_line.Replace(_T(" "), _T("") );
                wxCharBuffer lbuf = key_line.ToUTF8();
                mp_read_radix(&m_g, lbuf.data(), 16);
            }
        }
        
        else if( line.Upper().Find(_T("BIG Y")) != wxNOT_FOUND ){
            if( (i+1) < key_array.Count() ){
                wxString key_line = key_array[i+1];
                key_line.Replace(_T(" "), _T("") );
                wxCharBuffer lbuf = key_line.ToUTF8();
                mp_read_radix(&m_y, lbuf.data(), 16);
            }
        }
    }
    
    if( !mp_iszero(&m_p) && !mp_iszero(&m_q) && !mp_iszero(&m_g) && !mp_iszero(&m_y) )
        m_OK = true;
}

#define MP_OP(op) if ((ret = (op)) != MP_OKAY) goto error;

int _dsa_verify_hash (mp_int *r, mp_int *s, mp_int *hash,
                mp_int *keyG, mp_int *keyP, mp_int *keyQ, mp_int *keyY)
{
        mp_int w, v, u1, u2;
        int ret;
        
        MP_OP(mp_init_multi(&w, &v, &u1, &u2, NULL));
        
        // neither r or s can be 0 or >q
        if (mp_iszero(r) == MP_YES || mp_iszero(s) == MP_YES || mp_cmp(r, keyQ) != MP_LT || mp_cmp(s, keyQ) != MP_LT) {
           ret = -1;
           goto error;
        }
        
        // w = 1/s mod q
        MP_OP(mp_invmod(s, keyQ, &w));
        
        // u1 = m * w mod q
        MP_OP(mp_mulmod(hash, &w, keyQ, &u1));
        
        // u2 = r*w mod q
        MP_OP(mp_mulmod(r, &w, keyQ, &u2));
        
        // v = g^u1 * y^u2 mod p mod q
        MP_OP(mp_exptmod(keyG, &u1, keyP, &u1));
        MP_OP(mp_exptmod(keyY, &u2, keyP, &u2));
        MP_OP(mp_mulmod(&u1, &u2, keyP, &v));
        MP_OP(mp_mod(&v, keyQ, &v));
        
        // if r = v then we're set
        ret = 0;
        if (mp_cmp(r, &v) == MP_EQ) ret = 1;
        
error:
        mp_clear_multi(&w, &v, &u1, &u2, NULL);
        return ret;
}
    
 
 
//      Validate an ENC signature file against a Data server or IHO Public Key file (*.PUB)

//      Example of an ENC signature file
/*
  // Signature part R:
  3E94 FA3E 4600 B649 BC0A 3861 CB5E DC43 D34E D3A9.
  // Signature part S:
  A1E5 A1CF 54AC C380 CF8B FCFD 3A70 A1FE D761 2E59.
  // Signature part R:
  630A 2ADC 91FA AD4C 0B94 5B0C FE26 491E 29C6 0919.
  // Signature part S:
  097C 0019 403F E828 7326 4697 2FB2 D3F4 2621 9CD3.
  // Big p
  C16C BAD3 4D47 5EC5 3966 95D6 94BC 8BC4 7E59 8E23 B5A9 D7C5 CEC8 2D65 B682 7D44 E953 7848 4730 C0BF F1F4 CB56 F47C 6E51 054B E892 00F3 0D43 DC4F EF96 24D4 665B.
  // Big q
  B7B8 10B5 8C09 34F6 4287 8F36 0B96 D7CC 26B5 3E4D.
  // Big g
  4C53 C726 BDBF BBA6 549D 7E73 1939 C6C9 3A86 9A27 C5DB 17BA 3CAC 589D 7B3E 003F A735 F290 CFD0 7A3E F10F 3515 5F1A 2EF7 0335 AF7B 6A52 11A1 1035 18FB A44E 9718.
  // Big y
  063A C955 F639 B2F9 202E 070C 4A10 E82F 877A BC7F D928 D5F4 55C2 A3BF E928 92C5 9EB5 5DB0 ED6A 9555 ED8F 1C6E F218 DB62 FFFD F74E 5755 A989 44C7 6B50 9C41 B022.
  
*/

bool check_enc_signature_format( const wxString &sig_file_name )
{
    //  check the format of the ENC signature file according to Spec 5.4.2.7
    
    //  Read the file into a string array
    if( wxFileName::FileExists(sig_file_name) ){
        wxTextFile sig_file( sig_file_name );
        if( sig_file.Open() ){
            
            wxArrayString sig_array;
            wxString line = sig_file.GetFirstLine();
            
            while( !sig_file.Eof() ){
                sig_array.Add(line);
                line = sig_file.GetNextLine();
            }
            
            
            // Check the lines for length
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 50)
                            return false;
                    }
                    else
                        return false;
                }
                
                if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 50)
                            return false;
                    }
                    else
                        return false;
                }
                
                if( line.Upper().Find(_T("BIG P")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 160)
                            return false;
                    }
                    else
                        return false;
                }
                
                if( line.Upper().Find(_T("BIG Q")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 50)
                            return false;
                    }
                    else
                        return false;
                }
                
                if( line.Upper().Find(_T("BIG G")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 160)
                            return false;
                    }
                    else
                        return false;
                }
                
                if( line.Upper().Find(_T("BIG Y")) != wxNOT_FOUND ){
                    if(i+1 < sig_array.Count()){
                        if(sig_array[i+1].Length() != 160)
                            return false;
                    }
                    else
                        return false;
                }
                
            }
            
            return true;
        }
    }
    
    return false;
}
    
            
    
bool validate_enc_signature( const wxString &sig_file_name, const wxString &key_file_name)
{
    bool ret_val = false;
    
    // Read the key_file_name, and create an instance of pub_key
    pub_key public_key;
    if( wxFileName::FileExists(key_file_name) ){
        wxTextFile key_file( key_file_name );
        if( key_file.Open() ){
            
            wxArrayString key_array;
            wxString line = key_file.GetFirstLine();
            
            while( !key_file.Eof() ){
                key_array.Add(line);
                line = key_file.GetNextLine();
            }
            
            public_key.ReadKey( key_array );
        }
    }

    if( !public_key.m_OK )
        return false;
    
    //  Validate the ENC signature file according to Spec 5.4.2.7 and 10.6.2

    //  Read the file into a string array
    if( wxFileName::FileExists(sig_file_name) ){
        wxTextFile sig_file( sig_file_name );
        if( sig_file.Open() ){
            
            wxArrayString sig_array;
            wxString line = sig_file.GetFirstLine();
            
            while( !sig_file.Eof() ){
                sig_array.Add(line);
                line = sig_file.GetNextLine();
            }

            
            // Remove the first two (R/S) lines, the Data Server Signature
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
                
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
                
            // Remove and save the next two R/S signature lines 
            wxArrayString sig_save_array;
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    sig_save_array.Add(line);
                    sig_save_array.Add(sig_array[i+1]);
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    sig_save_array.Add(line);
                    sig_save_array.Add(sig_array[i+1]);
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            //  Make one long string of the remainder of the file, to treat as a blob
            wxString pub_key_blob;
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                pub_key_blob += line;
                pub_key_blob += _T("\r\n");
            }                
            
            wxCharBuffer blob_buf = pub_key_blob.ToUTF8();

            //  Hash the blob
            SHA1Context sha1;
            uint8_t sha1sum[SHA1HashSize];
            SHA1Reset(&sha1);
            
            SHA1Input(&sha1, (uint8_t *)blob_buf.data(), strlen( blob_buf.data()) );
            SHA1Result(&sha1, sha1sum);
            
            mp_int hash, r, s;
            mp_init(&hash);
            mp_init( &r );
            mp_init( &s );
            mp_read_unsigned_bin(&hash, sha1sum, sizeof(sha1sum));
            
            
            //  Prepare the signature
            for(unsigned int i=0 ; i < sig_save_array.Count() ; i++){
                wxString line = sig_save_array[i];
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    if( (i+1) < sig_save_array.Count() ){
                        wxString sig_line = sig_save_array[i+1];
                        sig_line.Replace(_T(" "), _T("") );
                        wxCharBuffer lbuf = sig_line.ToUTF8();
                        mp_read_radix(&r, lbuf.data(), 16);
                    }
                }
                
                else if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    if( (i+1) < sig_save_array.Count() ){
                        wxString sig_line = sig_save_array[i+1];
                        sig_line.Replace(_T(" "), _T("") );
                        wxCharBuffer lbuf = sig_line.ToUTF8();
                        mp_read_radix(&s, lbuf.data(), 16);
                    }
                }
            }
                
            //  Verify the blob
            int val = _dsa_verify_hash(&r, &s, &hash, &(public_key.m_g), &(public_key.m_p), &(public_key.m_q), &(public_key.m_y) );            
            ret_val = (val == 1);
            
        }
    }
    
    return ret_val;
}


bool validate_enc_cell( const wxString &sig_file_name, const wxString &cell_file_name)
{
    bool ret_val = false;
    
    //  Open the signature file, and extract the first R/S strings
    wxArrayString sig_array;
    wxArrayString sig_save_array;
    if( wxFileName::FileExists(sig_file_name) ){
        wxTextFile sig_file( sig_file_name );
        if( sig_file.Open() ){
            
            wxString line = sig_file.GetFirstLine();
            
            while( !sig_file.Eof() ){
                sig_array.Add(line);
                line = sig_file.GetNextLine();
            }
            
            // Remove and save the first two R/S signature lines 
             for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    sig_save_array.Add(line);
                    sig_save_array.Add(sig_array[i+1]);
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    sig_save_array.Add(line);
                    sig_save_array.Add(sig_array[i+1]);
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            // Remove the next two (R/S) lines, the Data Server Signature
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            for(unsigned int i=0 ; i < sig_array.Count() ; i++){
                wxString line = sig_array[i];
                
                if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
                    sig_array.RemoveAt(i, 2);
                    break;
                }
            }                
            
            //  Make a public key from the leftover part of the line array
        }   // File Open
    }
            
    pub_key public_key;
    public_key.ReadKey( sig_array );

    if( !public_key.m_OK )
        return false;

    //  Hash the ENC
    SHA1Context sha1;
    uint8_t sha1sum[SHA1HashSize];
    SHA1Reset(&sha1);
    
#define S63BUFSIZ 512 * 1024    
    wxFileInputStream fin( cell_file_name );
    if( fin.IsOk() ) {
        unsigned char buf[S63BUFSIZ];
        for(;;){
            if( fin.Eof())
                break;
            fin.Read(buf, S63BUFSIZ);
            size_t n_read = fin.LastRead();
 
            SHA1Input(&sha1, (uint8_t *)buf, n_read );
        }
    }
        
    SHA1Result(&sha1, sha1sum);
        

    mp_int hash, r, s;
    mp_init(&hash);
    mp_init( &r );
    mp_init( &s );
    mp_read_unsigned_bin(&hash, sha1sum, sizeof(sha1sum));

    //  Prepare the signature
    for(unsigned int i=0 ; i < sig_save_array.Count() ; i++){
        wxString line = sig_save_array[i];
        if( line.Upper().Find(_T("PART R")) != wxNOT_FOUND ){
            if( (i+1) < sig_save_array.Count() ){
                wxString sig_line = sig_save_array[i+1];
                sig_line.Replace(_T(" "), _T("") );
        wxCharBuffer lbuf = sig_line.ToUTF8();
        mp_read_radix(&r, lbuf.data(), 16);
            }
        }
        
        else if( line.Upper().Find(_T("PART S")) != wxNOT_FOUND ){
            if( (i+1) < sig_save_array.Count() ){
                wxString sig_line = sig_save_array[i+1];
                sig_line.Replace(_T(" "), _T("") );
        wxCharBuffer lbuf = sig_line.ToUTF8();
        mp_read_radix(&s, lbuf.data(), 16);
            }
        }
    }
    
    //  Verify the blob
    int val = _dsa_verify_hash(&r, &s, &hash, &(public_key.m_g), &(public_key.m_p), &(public_key.m_q), &(public_key.m_y) );            
    ret_val = (val == 1);
    
    return ret_val;   
}

