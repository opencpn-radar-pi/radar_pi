/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
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

#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

int br24_inet_aton(const char *cp, struct in_addr *addr) {
  u_long val;
  u_int parts[4];
  u_int *pp = parts;

  char c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, isdigit=decimal.
     */
    if (!isdigit(c)) {
      return 0;
    }
    val = 0;
    int base = 10;
    if (c == '0') {
      c = *++cp;
      if (c == 'x' || c == 'X') {
        base = 16, c = *++cp;
      } else {
        base = 8;
      }
    }
    for (;;) {
      if (isascii(c) && isdigit(c)) {
        val = (val * base) + (c - '0');
        c = *++cp;
      } else if (base == 16 && isascii(c) && isxdigit(c)) {
        val = (val << 4) | (c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else {
        break;
      }
    }
    if (c == '.') {
      /*
       * Internet format:
       *    a.b.c.d
       *    a.b.c    (with c treated as 16 bits)
       *    a.b    (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
        return 0;
      }
      *pp++ = val;
      c = *++cp;
    } else {
      break;
    }
  }
  /*
   * Check for trailing characters.
   */
  if (c != '\0' && (!isascii(c) || !isspace(c))) {
    return 0;
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  int n = pp - parts + 1;
  switch (n) {
    case 0:
      return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
      break;

    case 2: /* a.b -- 8.24 bits */
      if (val > 0xffffff) {
        return 0;
      }
      val |= parts[0] << 24;
      break;

    case 3: /* a.b.c -- 8.8.16 bits */
      if (val > 0xffff) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16);
      break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
      if (val > 0xff) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
      break;
  }
  if (addr) {
    addr->s_addr = htonl(val);
  }
  return 1;
}

bool socketReady(SOCKET sockfd, int timeout) {
  int r = 0;
  fd_set fdin;
  struct timeval tv = {(long)timeout / MILLISECONDS_PER_SECOND,
                       (long)(timeout % MILLISECONDS_PER_SECOND) * MILLISECONDS_PER_SECOND};

  FD_ZERO(&fdin);
  if (sockfd != INVALID_SOCKET) {
    FD_SET(sockfd, &fdin);
    r = select(sockfd + 1, &fdin, 0, &fdin, &tv);
  } else {
#ifndef __WXMSW__
    // Common UNIX style sleep, unlike 'sleep' this causes no alarms
    // and has fewer threading issues.
    select(1, 0, 0, 0, &tv);
#else
    Sleep(timeout);
#endif
    r = 0;
  }

  return r > 0;
}

SOCKET startUDPMulticastReceiveSocket(struct sockaddr_in *addr, UINT16 port, const char *mcast_address, wxString &error_message) {
  SOCKET rx_socket;
  struct sockaddr_in adr;
  int one = 1;

  error_message = wxT("");

  if (!addr) {
    return INVALID_SOCKET;
  }

  UINT8 *a = (UINT8 *)&addr->sin_addr;  // sin_addr is in network layout
  wxString address;
  address.Printf(wxT(" %u.%u.%u.%u"), a[0], a[1], a[2], a[3]);

  CLEAR_STRUCT(adr);
  adr.sin_family = AF_INET;
  adr.sin_addr.s_addr = htonl(INADDR_ANY);  // I know, htonl is unnecessary here
  adr.sin_port = htons(port);
  rx_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (rx_socket == INVALID_SOCKET) {
    error_message << _("Cannot create UDP socket");
    goto fail;
  }
  if (setsockopt(rx_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one))) {
    error_message << _("Cannot set reuse address option on socket");
    goto fail;
  }

  if (bind(rx_socket, (struct sockaddr *)&adr, sizeof(adr))) {
    error_message << _("Cannot bind UDP socket to port ") << port;
    goto fail;
  }

  // Subscribe rx_socket to a multicast group
  struct ip_mreq mreq;
  mreq.imr_interface = addr->sin_addr;

  if (!br24_inet_aton(mcast_address, &mreq.imr_multiaddr)) {
    error_message << _("Invalid multicast address") << wxT(" ") << wxString::FromUTF8(mcast_address);
    goto fail;
  }

  if (setsockopt(rx_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq))) {
    error_message << _("Invalid IP address for UDP multicast");
    goto fail;
  }

  // Hurrah! Success!
  return rx_socket;

fail:
  error_message << wxT(" ") << address;
  if (rx_socket != INVALID_SOCKET) {
    closesocket(rx_socket);
  }
  return INVALID_SOCKET;
}

SOCKET GetLocalhostServerTCPSocket() {
  SOCKET server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in adr;

  CLEAR_STRUCT(adr);
  adr.sin_family = AF_INET;
  adr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // I know, htonl is unnecessary here
  adr.sin_port = htons(0);

  if (server == INVALID_SOCKET) {
    wxLogError(wxT("BR24radar_pi: cannot get socket"));
    return INVALID_SOCKET;
  }

  if (bind(server, (struct sockaddr *)&adr, sizeof(adr))) {
    wxLogError(wxT("BR24radar_pi: cannot bind socket to loopback address"));
    closesocket(server);
    return INVALID_SOCKET;
  }

  return server;
}

SOCKET GetLocalhostSendTCPSocket(SOCKET server) {
  SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in adr;
  socklen_t adrlen;

  CLEAR_STRUCT(adr);
  adrlen = sizeof(adr);

  if (client == INVALID_SOCKET) {
    wxLogError(wxT("BR24radar_pi: cannot get socket"));
    return INVALID_SOCKET;
  }

  if (getsockname(server, (struct sockaddr *)&adr, &adrlen)) {
    wxLogError(wxT("BR24radar_pi: cannot get sockname"));
    closesocket(client);
    return INVALID_SOCKET;
  }

  if (connect(client, (struct sockaddr *)&adr, adrlen)) {
    wxLogError(wxT("BR24radar_pi: cannot connect socket"));
    closesocket(client);
    return INVALID_SOCKET;
  }
  return client;
}

#ifdef __WXMSW__

int getifaddrs(struct ifaddrs **ifap) {
  char buf[2048];
  DWORD bytesReturned;

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    wxLogError(wxT("BR24radar_pi: Cannot get socket"));
    return -1;
  }

  if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, buf, sizeof(buf), &bytesReturned, 0, 0) < 0) {
    wxLogError(wxT("BR24radar_pi: Cannot get interface list"));
    closesocket(sock);
    return -1;
  }

  /* guess return structure from size */
  unsigned iilen;
  INTERFACE_INFO *ii;
  INTERFACE_INFO_EX *iix;

  if (0 == bytesReturned % sizeof(INTERFACE_INFO)) {
    iilen = bytesReturned / sizeof(INTERFACE_INFO);
    ii = (INTERFACE_INFO *)buf;
    iix = NULL;
  } else {
    iilen = bytesReturned / sizeof(INTERFACE_INFO_EX);
    ii = NULL;
    iix = (INTERFACE_INFO_EX *)buf;
  }

  /* alloc a contiguous block for entire list */
  unsigned n = iilen, k = 0;
  struct ifaddrs_storage *ifa = (struct ifaddrs_storage *)calloc(n, sizeof(struct ifaddrs_storage));

  /* foreach interface */
  struct ifaddrs_storage *ift = ifa;

  for (unsigned i = 0; i < iilen; i++) {
    ift->ifa.ifa_addr = (sockaddr *)&ift->addr;
    if (ii) {
      memcpy(ift->ifa.ifa_addr, &ii[i].iiAddress.AddressIn, sizeof(struct sockaddr_in));
      ift->ifa.ifa_flags = ii[i].iiFlags;
    } else {
      memcpy(ift->ifa.ifa_addr, iix[i].iiAddress.lpSockaddr, iix[i].iiAddress.iSockaddrLength);
      ift->ifa.ifa_flags = iix[i].iiFlags;
    }

    k++;
    if (k < n) {
      ift->ifa.ifa_next = (struct ifaddrs *)(ift + 1);
      ift = (struct ifaddrs_storage *)(ift->ifa.ifa_next);
    }
  }

  *ifap = (struct ifaddrs *)ifa;
  closesocket(sock);
  return 0;
}

void freeifaddrs(struct ifaddrs *ifa) { free(ifa); }

#endif

PLUGIN_END_NAMESPACE
