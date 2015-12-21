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
 *   Copyright (C) 2012-2013 by Kees Verruijt         canboat@verruijt.net *
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

#ifndef _BR24RECEIVE_H_
#define _BR24RECEIVE_H_

#include "pi_common.h"
#include "socketutil.h"
#include "RadarInfo.h"

class br24Receive : public wxThread {
 public:
  br24Receive(br24radar_pi *pi, volatile bool *quit, RadarInfo *ri)
      : wxThread(wxTHREAD_JOINABLE), m_pi(pi), m_ri(ri), m_quit(quit) {
    Create(1024 * 1024);  // Stack size, be liberal
    m_next_spoke = -1;
    m_mcast_addr = 0;
    m_radar_addr = 0;
    m_range_meters = 0;
    m_updated_range = false;

    if (m_pi->m_settings.verbose >= 2) {
      wxLogMessage(wxT("BR24radar_pi: br24Receive ctor"));
    }
  };

  ~br24Receive(void);
  void *Entry(void);
  void OnExit(void);

  sockaddr_in *m_mcast_addr;
  sockaddr_in *m_radar_addr;
  wxIPV4address m_ip_addr;
  bool m_new_ip_addr;

  int m_range_meters;    // Last received range in meters
  bool m_updated_range;  // m_range_meters has changed

 private:
  void logBinaryData(const wxString &what, const UINT8 *data, int size);

  void ProcessFrame(UINT8 *data, int len);
  bool ProcessReport(UINT8 *data, int len);

  void EmulateFakeBuffer(void);
  SOCKET PickNextEthernetCard();
  SOCKET GetNewDataSocket();
  SOCKET GetNewCommandSocket();

  br24radar_pi *m_pi;
  wxString m_ip;
  RadarInfo *m_ri;  // All transfer of data passes back through this.
  volatile bool *m_quit;

  struct ifaddrs *m_interface_array;
  struct ifaddrs *m_interface;

  int m_next_spoke;
};

#endif /* _BR24RECEIVE_H_ */

// vim: sw=4:ts=8:
