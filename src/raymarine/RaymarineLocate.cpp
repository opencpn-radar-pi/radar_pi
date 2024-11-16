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
 *           Andrei Bankovs: Raymarine radars
 *           Martin Hassellov: testing the Raymarine radar
 *           Matt McShea: testing the Raymarine radar
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

#include "raymarine/RaymarineLocate.h"

#include "MessageBox.h"
#include "RadarInfo.h"

PLUGIN_BEGIN_NAMESPACE

//
// Raymarine E120 radars and compatible report their addresses here, (including the version?) #.
//
static const NetworkAddress reportRaymarineCommon(224, 0, 0, 1, 5800);
static const NetworkAddress reportRaymarineQuantumWiFi(232, 1, 1, 1, 5800);

#define SECONDS_PER_SELECT (1)
#define PERIOD_UNTIL_CARD_REFRESH (60)
#define PERIOD_UNTIL_WAKE_RADAR (30)

void RaymarineLocate::CleanupCards() {
  if (m_interface_addr) {
    delete[] m_interface_addr;
    m_interface_addr = 0;
  }
  if (m_socket) {
    for (size_t i = 0; i < m_interface_count; i++) {
      if (m_socket[i] != INVALID_SOCKET) {
        closesocket(m_socket[i]);
      }
    }
    delete[] m_socket;
    m_socket = 0;
  }
  m_interface_count = 0;
}

void RaymarineLocate::UpdateEthernetCards() {
  struct ifaddrs *addr_list;
  struct ifaddrs *addr;
  size_t i = 0;
  wxString error;

  CleanupCards();

  if (!getifaddrs(&addr_list)) {
    // Count the # of active IPv4 cards
    for (addr = addr_list; addr; addr = addr->ifa_next) {
      if (VALID_IPV4_ADDRESS(addr)) {
        m_interface_count++;
      }
    }

    // If there are any fill packed array (m_socket, m_interface_addr) with them.
    if (m_interface_count > 0) {
      m_socket = new SOCKET[m_interface_count * 2];
      m_interface_addr = new NetworkAddress[m_interface_count * 2];

      for (addr = addr_list; addr; addr = addr->ifa_next) {
        if (VALID_IPV4_ADDRESS(addr)) {
          struct sockaddr_in *sa = (struct sockaddr_in *)addr->ifa_addr;
          m_interface_addr[i].addr = sa->sin_addr;
          m_interface_addr[i].port = 0;
          wxLogError(wxT("Attempting to start receive socket for multicast %s"), reportRaymarineCommon.to_string());
          m_socket[i] = startUDPMulticastReceiveSocket(m_interface_addr[i], reportRaymarineCommon, error);
          if (m_socket[i] == INVALID_SOCKET) LOG_RECEIVE(wxT("Socket failing=%i"), i);
          LOG_INFO(wxT("RaymarineLocate scanning interface %s for radars"), m_interface_addr[i].FormatNetworkAddress());
          i++;

          m_interface_addr[i].addr = sa->sin_addr;
          m_interface_addr[i].port = 0;
          wxLogError(wxT("Attempting to start receive socket for multicast %s"), reportRaymarineQuantumWiFi.to_string());
          m_socket[i] = startUDPMulticastReceiveSocket(m_interface_addr[i], reportRaymarineQuantumWiFi, error);
          if (m_socket[i] == INVALID_SOCKET) LOG_RECEIVE(wxT("Socket failing=%i"), i);
          LOG_INFO(wxT("RaymarineLocate scanning interface %s for radars"), m_interface_addr[i].FormatNetworkAddress());
          i++;
        }
      }
    }

    freeifaddrs(addr_list);
  }

  // WakeRadar(); not  known for Raymarine
}

/*
 * Entry
 *
 * Called by wxThread when the new thread is running.
 * It should remain running until Shutdown is called.
 */
void *RaymarineLocate::Entry(void) {
  int r = 0;
  int rescan_network_cards = 0;
  bool success = false;

  union {
    sockaddr_storage addr;
    sockaddr_in ipv4;
  } rx_addr;
  socklen_t rx_len;

#define MAX_DATA 500
  uint8_t data[MAX_DATA];

  LOG_INFO(wxT("RaymarineLocate thread starting"));

  m_is_shutdown = false;

  UpdateEthernetCards();

  while (!success && !m_shutdown) {  // will run until the Raymarine radar location info has been found or shutdown
    // after that we stop the Raymarine locate, saves load and prevents that the serial nr gets overwritten
    struct timeval tv = {1, 0};
    fd_set fdin;
    FD_ZERO(&fdin);

    int maxFd = INVALID_SOCKET;
    for (size_t i = 0; i < m_interface_count * 2; i++) {
      if (m_socket[i] != INVALID_SOCKET) {
        FD_SET(m_socket[i], &fdin);
        maxFd = MAX(m_socket[i], maxFd);
      }
    }

    r = select(maxFd + 1, &fdin, 0, 0, &tv);
    if (r <= 0 && errno != 0) {
      UpdateEthernetCards();
      rescan_network_cards = 0;
    }
    if (r > 0) {
      for (size_t i = 0; i < m_interface_count * 2; i++) {
        if (m_socket[i] != INVALID_SOCKET && FD_ISSET(m_socket[i], &fdin)) {
          rx_len = sizeof(rx_addr);
          r = recvfrom(m_socket[i], (char *)data, sizeof(data), 0, (struct sockaddr *)&rx_addr, &rx_len);
          if (r > 2) {  // we are not interested in 2 byte messages
            if (r > MAX_DATA) wxLogError(wxT("Buffer overflow on reading Raymarine Locate"));
            NetworkAddress radar_address;
            radar_address.addr = rx_addr.ipv4.sin_addr;
            radar_address.port = rx_addr.ipv4.sin_port;
            if (ProcessReport(radar_address, m_interface_addr[i], data, (size_t)r)) {
              rescan_network_cards = -PERIOD_UNTIL_CARD_REFRESH;  // Give double time until we rescan
              success = true;
            }
          }
        }
      }
    } else {  // no data received -> select timeout
      if (++rescan_network_cards >= PERIOD_UNTIL_CARD_REFRESH) {
        UpdateEthernetCards();
        rescan_network_cards = 0;
      }
    }

  }  // endless loop until thread destroy

  CleanupCards();
  m_is_shutdown = true;
  if (success) {
    LOG_INFO(wxT("Raymarine locate stopped after success"));
  }
  return 0;
}

#pragma pack(push, 1)

struct LocationInfoBlock {
  uint32_t field1;      // 0
  uint32_t field2;      // 4
  uint8_t model_id;     // 0x28 byte 8
  uint8_t field3;       // byte 9
  uint16_t field4;      // byte 10
  uint32_t field5;      // 12
  uint32_t field6;      // 16
  uint32_t data_ip;     // 20
  uint32_t data_port;   // 24
  uint32_t radar_ip;    // 28
  uint32_t radar_port;  // 32
};
#pragma pack(pop)

bool RaymarineLocate::ProcessReport(const NetworkAddress &radar_address, const NetworkAddress &interface_address,
                                    const uint8_t *report, size_t len) {
  LocationInfoBlock *rRec = (LocationInfoBlock *)report;
  wxCriticalSectionLocker lock(m_exclusive);

  int raymarine_radar_code = 0;
  for (size_t r = 0; r < m_pi->m_settings.radar_count; r++) {
    if (m_pi->m_radar[r]->m_radar_type == RM_E120) {  // only one Raymarine radar allowed
      raymarine_radar_code = 01;
      break;
    }
    if (m_pi->m_radar[r]->m_radar_type == RM_QUANTUM) {
      raymarine_radar_code = 0x28;
      break;
    }
  }
  if (len == sizeof(LocationInfoBlock)){
    LOG_RECEIVE(wxT(" locate process len=%i, report model_id =%0x, raymarine_radar_code=%0x"), len, rRec->model_id,
             raymarine_radar_code);
    m_pi->logBinaryData(wxT("RaymarineLocate received RadarReport"), report, len);
    }
  if (len == sizeof(LocationInfoBlock) && rRec->model_id == raymarine_radar_code) {  // only length 36 is used

    RadarLocationInfo infoA;
    NetworkAddress radar_ipA = radar_address;
    if (rRec->data_ip == 0) {
      if (raymarine_radar_code != 0x28) {
        // Quantum WiFi is the only one that is all unicast
        return false;
      } else {
        radar_ipA.port = ntohs(rRec->radar_port);
        infoA.report_addr.addr.s_addr = ntohl(rRec->radar_ip);
        infoA.report_addr.port = ntohs(rRec->radar_port);
      }
    } else {
      radar_ipA.port = htons(RO_PRIMARY);
      infoA.report_addr.addr.s_addr = ntohl(rRec->data_ip);
      infoA.report_addr.port = ntohs(rRec->data_port);
    }

    infoA.send_command_addr.addr.s_addr = ntohl(rRec->radar_ip);
    infoA.send_command_addr.port = ntohs(rRec->radar_port);
    infoA.spoke_data_addr.addr.s_addr = ntohl(rRec->data_ip);  // Unused ???
    infoA.spoke_data_addr.port = ntohs(rRec->data_port);
    infoA.serialNr = wxT(" ");  // empty

    LOG_INFO(wxT("Located raymarine radar IP %s, interface %s [%s]"), radar_ipA.FormatNetworkAddressPort(),
             interface_address.FormatNetworkAddress(), infoA.to_string());
    m_report_count++;

    FoundRaymarineLocationInfo(radar_ipA, interface_address, infoA);
    return true;
  }

  // LOG_BINARY_RECEIVE(wxT("RaymarineLocate received unknown message"), report, len);
  return false;
}

void RaymarineLocate::FoundRaymarineLocationInfo(const NetworkAddress &addr, const NetworkAddress &interface_addr,
                                                 const RadarLocationInfo &info) {
  wxCriticalSectionLocker lock(m_exclusive);

  // Check if the info is OK
  if (info.report_addr.IsNull() || info.send_command_addr.IsNull()) {
    LOG_INFO(wxT("RaymarineLocate::FoundRaymarineLocationInfo something is null"));
    return;
  }

  // Find the number of physical Raymarine radars; only needed in case of more than one Raymarine radar
  size_t raymarines = 0;  // number of hard Raymarine radars

  int ray_nr = -1;
  for (size_t r = 0; r < m_pi->m_settings.radar_count; r++) {
    if (m_pi->m_radar[r]->m_radar_type == RM_E120 || m_pi->m_radar[r]->m_radar_type == RM_QUANTUM) {
      ray_nr = r;
      raymarines++;  // later more Raymarine radars may be covered
      break;         // RM_120 found, there should only be one
    }
  }
  if (ray_nr == -1) {  // no raymarine radar found
    LOG_INFO(wxT("No raymarine radar found"));
    return;
  }
  // more then 2 Raymarine radars: associate the info found with the right type of radar
  if (raymarines > 1) {
    LOG_INFO(wxT("Software does not yet allow more than one Raymarine radar type"));
  }

  NetworkAddress int_face_addr = interface_addr;
  NetworkAddress radar_addr = addr;

  // First, check if we already know this serial#

  m_pi->m_radar[ray_nr]->SetRadarLocationInfo(info);
  m_pi->m_radar[ray_nr]->SetRadarInterfaceAddress(int_face_addr, radar_addr);
  return;
}

PLUGIN_END_NAMESPACE

/*

//https :  // learn.microsoft.com/en-us/dotnet/api/system.net.sockets.multicastoption?view=net-8.0


         using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

// This is the listener example that shows how to use the MulticastOption class.
// In particular, it shows how to use the MulticastOption(IPAddress, IPAddress)
// constructor, which you need to use if you have a host with more than one
// network card.
// The first parameter specifies the multicast group address, and the second
// specifies the local address of the network card you want to use for the data
// exchange.
// You must run this program in conjunction with the sender program as
// follows:
// Open a console window and run the listener from the command line.
// In another console window run the sender. In both cases you must specify
// the local IPAddress to use. To obtain this address run the ipconfig command
// from the command line.
//
namespace Mssc.TransportProtocols.Utilities {
public class TestMulticastOption {
 private
  static IPAddress s_mcastAddress;
 private
  static int s_mcastPort;
 private
  static Socket s_mcastSocket;
 private
  static MulticastOption s_mcastOption;

 private
  static void MulticastOptionProperties() {
    Console.WriteLine("Current multicast group is: " + s_mcastOption.Group);
    Console.WriteLine("Current multicast local address is: " + s_mcastOption.LocalAddress);
  }

 private
  static void StartMulticast() {
    try {
      s_mcastSocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);

      Console.Write("Enter the local IP address: ");

      IPAddress localIPAddr = IPAddress.Parse(Console.ReadLine());

      // IPAddress localIP = IPAddress.Any;
      EndPoint localEP = (EndPoint) new IPEndPoint(localIPAddr, s_mcastPort);

      s_mcastSocket.Bind(localEP);

      // Define a MulticastOption object specifying the multicast group
      // address and the local IPAddress.
      // The multicast group address is the same as the address used by the server.
      s_mcastOption = new MulticastOption(s_mcastAddress, localIPAddr);

      s_mcastSocket.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.AddMembership, s_mcastOption);
    }

    catch (Exception e) {
      Console.WriteLine(e.ToString());
    }
  }

 private
  static void ReceiveBroadcastMessages() {
    bool done = false;
    byte[] bytes = new byte[100];
    IPEndPoint groupEP = new (s_mcastAddress, s_mcastPort);
    EndPoint remoteEP = (EndPoint) new IPEndPoint(IPAddress.Any, 0);

    try {
      while (!done) {
        Console.WriteLine("Waiting for multicast packets.......");
        Console.WriteLine("Enter ^C to terminate.");

        s_mcastSocket.ReceiveFrom(bytes, ref remoteEP);

        Console.WriteLine("Received broadcast from {0} :\n {1}\n", groupEP.ToString(),
                          Encoding.ASCII.GetString(bytes, 0, bytes.Length));
      }

      s_mcastSocket.Close();
    }

    catch (Exception e) {
      Console.WriteLine(e.ToString());
    }
  }

 public
  static void Main(string[] args) {
    // Initialize the multicast address group and multicast port.
    // Both address and port are selected from the allowed sets as
    // defined in the related RFC documents. These are the same
    // as the values used by the sender.
    s_mcastAddress = IPAddress.Parse("224.168.100.2");
    s_mcastPort = 11000;

    // Start a multicast group.
    StartMulticast();

    // Display MulticastOption properties.
    MulticastOptionProperties();

    // Receive broadcast messages.
    ReceiveBroadcastMessages();
  }
}
}  // namespace Mssc.TransportProtocols. Utilities

*/