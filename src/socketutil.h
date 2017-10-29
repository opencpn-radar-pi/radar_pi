/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Hakan Svensson
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

#ifndef _SOCKETUTIL_H_
#define _SOCKETUTIL_H_

#include "pi_common.h"

PLUGIN_BEGIN_NAMESPACE

#define VALID_IPV4_ADDRESS(i)                                                                                                    \
  (i && i->ifa_addr && i->ifa_addr->sa_family == AF_INET && (i->ifa_flags & IFF_UP) > 0 && (i->ifa_flags & IFF_LOOPBACK) == 0 && \
   (i->ifa_flags & IFF_MULTICAST) > 0)

// easy define of mcast addresses. Note that these are in network order already.
#define IPV4_ADDR(a, b, c, d) ((uint32_t)(((a)&0xff) << 24) | (((b)&0xff) << 16) | (((c)&0xff) << 8) | ((d)&0xff))

#define IPV4_PORT(p) (htons(p))

class NetworkAddress {
public:
  NetworkAddress() {
    addr.s_addr = 0;
    port = 0;
  }
  NetworkAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t p)
  {
    union {
      uint8_t byte[4];
      uint32_t word;
    } u;

    u.byte[0] = a;
    u.byte[1] = b;
    u.byte[2] = c;
    u.byte[3] = d;

    addr.s_addr = u.word;
    port = htons(p);
  }
  struct in_addr addr;
  uint16_t port;
};

extern wxString FormatNetworkAddress(NetworkAddress &addr);
extern wxString FormatNetworkAddressPort(NetworkAddress &addr);

extern bool socketReady(SOCKET sockfd, int timeout);

extern int radar_inet_aton(const char *cp, struct in_addr *addr);
extern SOCKET startUDPMulticastReceiveSocket(NetworkAddress &addr, NetworkAddress &mcast_address, wxString &error_message);
extern SOCKET GetLocalhostServerTCPSocket();
extern SOCKET GetLocalhostSendTCPSocket(SOCKET receive_socket);

#ifndef __WXMSW__

// Mac and Linux have ifaddrs.
#include <ifaddrs.h>
#include <net/if.h>

#else

// Emulate (just enough of) ifaddrs on Windows
// Thanks to
// https://code.google.com/p/openpgm/source/browse/trunk/openpgm/pgm/getifaddrs.c?r=496&spec=svn496
// Although that file has interesting new APIs the old ioctl works fine with XP and W7, and does
// enough
// for what we want to do.

struct ifaddrs {
  struct ifaddrs *ifa_next;
  struct sockaddr *ifa_addr;
  ULONG ifa_flags;
};

struct ifaddrs_storage {
  struct ifaddrs ifa;
  struct sockaddr_storage addr;
};

extern int getifaddrs(struct ifaddrs **ifap);
extern void freeifaddrs(struct ifaddrs *ifa);

#endif

PLUGIN_END_NAMESPACE

#endif
