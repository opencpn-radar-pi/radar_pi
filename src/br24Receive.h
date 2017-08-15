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

#include "RadarInfo.h"
#include "pi_common.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

class br24Receive : public wxThread {
 public:
  br24Receive(br24radar_pi *pi, RadarInfo *ri) : wxThread(wxTHREAD_JOINABLE), m_pi(pi), m_ri(ri) {
    Create(1024 * 1024);  // Stack size, be liberal
    m_next_spoke = -1;
    m_mcast_addr = 0;
    m_radar_status = 0;
    m_new_ip_addr = false;
    m_next_rotation = 0;
    m_shutdown_time_requested = 0;
    m_is_shutdown = false;

    wxString mcast_address = m_pi->GetMcastIPAddress();

    if (mcast_address.length()) {
      int b[4];
      union {
        uint8_t b[4];
        uint32_t addr;
      } mcast;

      if (sscanf(mcast_address.c_str(), "%u.%u.%u.%u", &b[0], &b[1], &b[2], &b[3]) == 4) {
        mcast.b[0] = (uint8_t)b[0];
        mcast.b[1] = (uint8_t)b[1];
        mcast.b[2] = (uint8_t)b[2];
        mcast.b[3] = (uint8_t)b[3];

#ifdef __WXMAC__
        m_initial_mcast_addr.sin_len = sizeof(sockaddr_in);
#endif
        m_initial_mcast_addr.sin_addr.s_addr = mcast.addr;
        m_initial_mcast_addr.sin_port = 0;
        m_initial_mcast_addr.sin_family = AF_INET;
        m_mcast_addr = &m_initial_mcast_addr;
        LOG_VERBOSE(wxT("BR24radar_pi: assuming radar is still reachable via %s"), mcast_address.c_str());
      }
    }

    m_receive_socket = GetLocalhostServerTCPSocket();
    m_send_socket = GetLocalhostSendTCPSocket(m_receive_socket);

    LOG_RECEIVE(wxT("BR24radar_pi: %s receive thread created"), m_ri->m_name.c_str());
  };

  ~br24Receive() {}

  void *Entry(void);
  void Shutdown(void);

  sockaddr_in m_initial_mcast_addr;
  sockaddr_in *m_mcast_addr;
  wxIPV4address m_ip_addr;
  bool m_new_ip_addr;

  wxLongLong m_shutdown_time_requested;  // Main thread asks this thread to stop
  volatile bool m_is_shutdown;

 private:
  void logBinaryData(const wxString &what, const UINT8 *data, int size);

  void ProcessFrame(const UINT8 *data, int len);
  bool ProcessReport(const UINT8 *data, int len);
  void ProcessCommand(wxString &addr, const UINT8 *data, int len);

  void EmulateFakeBuffer(void);
  SOCKET PickNextEthernetCard();
  SOCKET GetNewReportSocket();
  SOCKET GetNewDataSocket();
  SOCKET GetNewCommandSocket();

  br24radar_pi *m_pi;
  wxString m_ip;
  RadarInfo *m_ri;  // All transfer of data passes back through this.

  SOCKET m_receive_socket;  // Where we listen for message from m_send_socket
  SOCKET m_send_socket;     // A message to this socket will interrupt select() and allow immediate shutdown

  struct ifaddrs *m_interface_array;
  struct ifaddrs *m_interface;

  int m_next_spoke;     // emulator next spoke
  int m_next_rotation;  // slowly rotate emulator
  char m_radar_status;
};

PLUGIN_END_NAMESPACE

#endif /* _BR24RECEIVE_H_ */
