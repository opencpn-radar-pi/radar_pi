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

#include "br24Receive.h"

/*
 * This file not only contains the radar receive threads, it is also
 * the only unit that understands what the radar returned data looks like.
 * The rest of the plugin uses a (slightly) abstract definition of the radar.
 */

// A marker that uniquely identifies BR24 generation scanners, as opposed to 4G(eneration)
// Note that 3G scanners are BR24's with better power, so they are more BR23+ than 4G-.
// As far as we know they 3G's use exactly the same command set.

// If BR24MARK is found, we switch to BR24 mode, otherwise 4G.
static UINT8 BR24MARK[] = { 0x00, 0x44, 0x0d, 0x0e };

#pragma pack(push,1)

struct br24_header {
    UINT8 headerLen;       //1 bytes
    UINT8 status;          //1 bytes
    UINT8 scan_number[2];  //2 bytes, 0-4095
    UINT8 mark[4];         //4 bytes 0x00, 0x44, 0x0d, 0x0e
    UINT8 angle[2];        //2 bytes
    UINT8 heading[2];      //2 bytes heading with RI-10/11?
    UINT8 range[4];        //4 bytes
    UINT8 u01[2];          //2 bytes blank
    UINT8 u02[2];          //2 bytes
    UINT8 u03[4];          //4 bytes blank
}; /* total size = 24 */

struct br4g_header {
    UINT8 headerLen;       //1 bytes
    UINT8 status;          //1 bytes
    UINT8 scan_number[2];  //2 bytes, 0-4095
    UINT8 u00[2];          //Always 0x4400 (integer)
    UINT8 largerange[2];   //2 bytes or -1
    UINT8 angle[2];        //2 bytes
    UINT8 heading[2];      //2 bytes heading with RI-10/11 or -1
    UINT8 smallrange[2];   //2 bytes or -1
    UINT8 rotation[2];     //2 bytes, rotation/angle
    UINT8 u02[4];          //4 bytes signed integer, always -1
    UINT8 u03[4];          //4 bytes signed integer, mostly -1 (0x80 in last byte) or 0xa0 in last byte
}; /* total size = 24 */

struct radar_line {
    union {
        br24_header   br24;
        br4g_header   br4g;
    };
    UINT8 data[RETURNS_PER_LINE];
};

/* Normally the packets are have 32 spokes, or scan lines, but we assume nothing
 * so we take up to 120 spokes. This is the nearest round figure without going over
 * 64kB.
 */

struct radar_frame_pkt {
    UINT8      frame_hdr[8];
    radar_line line[120];    //  scan lines, or spokes
};
#pragma pack(pop)

// Ethernet packet stuff *************************************************************

br24Receive::~br24Receive()
{
    wxLogMessage(wxT("BR24radar_pi: %s thread is stopping"), m_ri->name);
}

void br24Receive::OnExit()
{
}

void br24Receive::logBinaryData( const wxString& what, const UINT8 * data, int size )
{
    wxString explain;
    int i = 0;

    explain.Alloc(size * 3 + 50);
    explain += wxT("BR24radar_pi: ") + m_ri->name + wxT(" ");
    explain += what;
    explain += wxString::Format(wxT(" %d bytes: "), size);
    for (i = 0; i < size; i++) {
        explain += wxString::Format(wxT(" %02X"), data[i]);
    }
    wxLogMessage(explain);
}

// ProcessFrame
// ------------
// Process one radar frame packet, which can contain up to 32 'spokes' or lines extending outwards
// from the radar up to the range indicated in the packet.
//
void br24Receive::ProcessFrame( UINT8 * data, int len )
{
    wxLongLong nowMillis = wxGetLocalTimeMillis();
    time_t now = time(0);
    radar_frame_pkt * packet = (radar_frame_pkt *) data;

    m_radar_seen = true;
    m_radar_watchdog = now;
    m_data_seen = true;
    m_data_watchdog = now;

    if (m_pi->m_settings.verbose >= 5) {
        wxLogMessage(wxT("BR24radar_pi: %s ProcessFrame"), m_ri->name);
    }

    int spoke = 0;
    m_ri->statistics.packets++;
    if (len < (int) sizeof(packet->frame_hdr)) {
        // The packet is so small it contains no scan_lines, quit!
        m_ri->statistics.broken_packets++;
        return;
    }
    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(radar_line);
    if (scanlines_in_packet != 32) {
        m_ri->statistics.broken_packets++;
    }

    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        radar_line * line = &packet->line[scanline];

        // Validate the spoke
        spoke = line->br24.scan_number[0] | (line->br24.scan_number[1] << 8);
        m_ri->statistics.spokes++;
        if (line->br24.headerLen != 0x18) {
            if (m_pi->m_settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: strange header length %d"), line->br24.headerLen);
            }
            // Do not draw something with this...
            m_ri->statistics.missing_spokes++;
            m_next_spoke = (spoke + 1) % SPOKES;
            continue;
        }
        if (line->br24.status != 0x02 && line->br24.status != 0x12) {
            if (m_pi->m_settings.verbose) {
                wxLogMessage(wxT("BR24radar_pi: strange status %02x"), line->br24.status);
            }
            m_ri->statistics.broken_spokes++;
        }
        if (m_next_spoke >= 0 && spoke != m_next_spoke) {
            if (spoke > m_next_spoke) {
                m_ri->statistics.missing_spokes += spoke - m_next_spoke;
            } else {
                m_ri->statistics.missing_spokes += SPOKES + spoke - m_next_spoke;
            }
        }
        m_next_spoke = (spoke + 1) % SPOKES;

        int range_raw = 0;
        int angle_raw = 0;
        short int hdm_raw = 0;
        short int hdt_raw = 0;
        short int large_range = 0;
        short int small_range = 0;
        int range_meters = 0;

        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0) {
            // BR24 and 3G mode
            range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
            range_meters = (int) ((double)range_raw * 10.0 / sqrt(2.0));
            if (m_ri->radar_type != RT_BR24) {
                wxLogMessage(wxT("BR24radar_pi: %s is Navico type BR24 or 3G"), m_ri->name);
            }
            m_ri->radar_type = RT_BR24;
        } else {
            // 4G mode
            large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
            small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
            if (large_range == 0x80) {
                if (small_range == -1) {
                    range_raw = 0; // Invalid range received
                } else {
                    range_raw = small_range;
                }
            } else {
                range_raw = large_range * 256;
            }
            range_meters = range_raw / 4;
            if (m_ri->radar_type != RT_4G) {
                wxLogMessage(wxT("BR24radar_pi: %s is Navico type 4G"), m_ri->name);
            }
            m_ri->radar_type = RT_4G;
        }

        // Range change received from radar?
        if (range_meters != m_range_meters) {
            if (m_pi->m_settings.verbose >= 1) {
                if (range_meters == 0) {
                    wxLogMessage(wxT("BR24radar_pi: Invalid range received, keeping %d meters"), m_range_meters);
                }
                else {
                    wxLogMessage(wxT("BR24radar_pi: Radar now scanning with range %d meters (was %d meters)"), range_meters, m_range_meters);
                }
            }
            m_range_meters = range_meters;
            m_updated_range = true;
        }

        hdm_raw = (line->br4g.heading[1] << 8) | line->br4g.heading[0];
        if (hdm_raw != INT16_MIN && TIMER_NOT_ELAPSED(now, m_pi->m_var_watchdog) && m_ri->radar_type == RT_4G) {
            if (!m_pi->m_heading_on_radar) {
                wxLogMessage(wxT("BR24radar_pi: %s transmits heading, using that as best source of heading"), m_ri->name);
            }
            m_pi->m_heading_on_radar = true;                            // heading on radar
            hdt_raw = MOD_ROTATION(hdm_raw + SCALE_DEGREES_TO_RAW(m_pi->m_var));
            m_pi->m_hdt = MOD_DEGREES(SCALE_RAW_TO_DEGREES(hdt_raw));
        }
        else {                                // no heading on radar
            m_pi->m_heading_on_radar = false;
            hdt_raw = SCALE_DEGREES_TO_RAW(m_pi->m_hdt);
        }
        angle_raw += hdt_raw;
        angle_raw += SCALE_DEGREES_TO_RAW(270); // Compensate openGL rotation compared to North UP
        // until here all is based on 4096 (SPOKES) scanlines

        SpokeBearing b = MOD_ROTATION2048(angle_raw / 2);   // divide by 2 to map on 2048 scanlines

        m_ri->ProcessRadarSpoke(b, line->data, RETURNS_PER_LINE, range_meters, nowMillis);
    }

    //  all scanlines ready now, refresh section follows

    m_ri->ProcessRadarPacket(now);
}

/*
 * Called once a second. Emulate a radar return that is
 * at the current desired auto_range.
 * Speed is 24 images per minute, e.g. 1/2.5 of a full
 * image.
 */

void br24Receive::EmulateFakeBuffer(void)
{
    wxLongLong nowMillis = wxGetLocalTimeMillis();
    time_t now = time(0);
    UINT8 data[RETURNS_PER_LINE];

    m_ri->statistics.packets++;
    m_radar_seen = true;
    m_radar_watchdog = now;
    int scanlines_in_packet = 2048 * 24 / 60;
    int range_meters = 4000;
    int spots = 0;
    m_ri->radar_type = RT_BR24;
    if (range_meters != m_range_meters) {
        m_range_meters = range_meters;
    }
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++) {
        int angle_raw = m_next_spoke;
        m_next_spoke = (m_next_spoke + 1) % LINES_PER_ROTATION;
        m_ri->statistics.spokes++;

        // Invent a pattern. Outermost ring, then a square pattern
        for (size_t range = 0; range < sizeof(data); range++) {
            size_t bit = range >> 5;
            // use bit 'bit' of angle_raw
            UINT8 color = ((angle_raw >> 3) & (2 << bit)) > 0 ? 200 : 0;
            data[range] = color;
            if (color > 0) {
                spots++;
            }
        }

        m_ri->ProcessRadarSpoke(angle_raw, data, sizeof(data), range_meters, nowMillis);
    }

    //  all scanlines ready now, refresh section follows

    m_ri->ProcessRadarPacket(now);
    if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: %") wxTPRId64 wxT(" emulating %d spokes at range %d with %d spots"), nowMillis, scanlines_in_packet, range_meters, spots);
    }
}

SOCKET br24Receive::PickNextEthernetCard()
{
    SOCKET socket = INVALID_SOCKET;

    // Pick the next ethernet card
    // If set, we used this one last time. Go to the next card.
    if (m_interface) {
        m_interface = m_interface->ifa_next;
    }
    // Loop until card with a valid IPv4 address
    while (m_interface && !VALID_IPV4_ADDRESS(m_interface)) {
        m_interface = m_interface->ifa_next;
    }
    if (!m_interface) {
        if (m_interface_array) {
            freeifaddrs(m_interface_array);
            m_interface_array = 0;
        }
        if (!getifaddrs(&m_interface_array)) {
            m_interface = m_interface_array;
        }
        // Loop until card with a valid IPv4 address
        while (m_interface && !VALID_IPV4_ADDRESS(m_interface)) {
            m_interface = m_interface->ifa_next;
        }
    }
    if (VALID_IPV4_ADDRESS(m_interface)) {
        wxString error;
        socket = startUDPMulticastReceiveSocket((struct sockaddr_in *)m_interface->ifa_addr, 6679, "236.6.7.9", error);
        if (socket != INVALID_SOCKET) {
            wxString addr;
            UINT8 * a = (UINT8 *)&((struct sockaddr_in *)m_interface->ifa_addr)->sin_addr; // sin_addr is in network layout
            addr.Printf(wxT("%u.%u.%u.%u"), a[0], a[1], a[2], a[3]);
            if (m_pi->m_settings.verbose >= 1) {
                wxLogMessage(wxT("BR24radar_pi: Listening for %s reports on %s"), m_ri->name, addr.c_str());
            }
            m_pi->m_ip_address = addr;
            m_pi->m_update_address_control = true;
        } else {
            wxLogMessage(wxT("BR24radar_pi: Unable to listen to socket: %s"), error.c_str());
        }
    }

    return socket;
}


SOCKET br24Receive::GetNewDataSocket()
{
    SOCKET socket;
    wxString error;

    if (!m_mcast_addr) {
        return INVALID_SOCKET;
    }

    if (m_ri->radar == 1) {
        socket = startUDPMulticastReceiveSocket(m_mcast_addr, 6657, "236.6.7.13", error);
    }
    else {
        socket = startUDPMulticastReceiveSocket(m_mcast_addr, 6678, "236.6.7.8", error);
    }
    if (socket != INVALID_SOCKET) {
        wxString addr;
        UINT8 * a = (UINT8 *) &m_mcast_addr->sin_addr; // sin_addr is in network layout
        addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);
        if (m_pi->m_settings.verbose) {
            wxLogMessage(wxT("BR24radar_pi: %s listening for data on %s"), m_ri->name, addr.c_str());
        }
    } else {
        wxLogMessage(wxT("BR24radar_pi: Unable to listen to socket: %s"), error.c_str());
    }
    return socket;
}

SOCKET br24Receive::GetNewCommandSocket()
{
    SOCKET socket;
    wxString error;

    if (!m_mcast_addr) {
        return INVALID_SOCKET;
    }

    if (m_ri->radar == 1) {
        socket = startUDPMulticastReceiveSocket(m_mcast_addr, 6658, "236.6.7.14", error);
    }
    else {
        socket = startUDPMulticastReceiveSocket(m_mcast_addr, 6680, "236.6.7.10", error);
    }
    if (socket != INVALID_SOCKET) {
        wxString addr;
        UINT8 * a = (UINT8 *) &m_mcast_addr->sin_addr; // sin_addr is in network layout
        addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);
        if (m_pi->m_settings.verbose) {
            wxLogMessage(wxT("BR24radar_pi: %s listening for command on %s"), m_ri->name, addr.c_str());
        }
    } else {
        wxLogMessage(wxT("BR24radar_pi: Unable to listen to socket: %s"), error.c_str());
    }
    return socket;
}

void *br24Receive::Entry(void)
{
    int r = 0;
    int no_data_timeout = 0;
    union {
        sockaddr_storage addr;
        sockaddr_in      ipv4;
    } rx_addr;
    socklen_t        rx_len;

    UINT8 data[sizeof(radar_frame_pkt)];
    m_interface_array = 0;
    m_interface = 0;
    static struct sockaddr_in mcastFoundAddr;
    static struct sockaddr_in radarFoundAddr;

    SOCKET dataSocket = INVALID_SOCKET;
    SOCKET commandSocket = INVALID_SOCKET;
    SOCKET reportSocket = INVALID_SOCKET;

    if (m_pi->m_settings.verbose) {
        wxLogMessage(wxT("BR24radar_pi: br24Receive thread %s starting"), m_ri->name);
    }
    // Do not draw something with this...

    while (!*m_quit) {

        if (m_pi->m_settings.emulator_on) {
            socketReady(INVALID_SOCKET, 1000); // sleep for 1s
            EmulateFakeBuffer();
            continue;
        }

        if (reportSocket == INVALID_SOCKET) {
            reportSocket = PickNextEthernetCard();
            if (reportSocket != INVALID_SOCKET) {
                no_data_timeout = 0;
            }
        }
        else {
            if (dataSocket == INVALID_SOCKET) {
                dataSocket = GetNewDataSocket();
            }
            else if (commandSocket == INVALID_SOCKET) {
                commandSocket = GetNewCommandSocket();
            }
        }

        struct timeval tv = { (long) 1, (long) 0 };

        fd_set fdin;
        FD_ZERO(&fdin);

        int maxFd = INVALID_SOCKET;
        if (reportSocket != INVALID_SOCKET) {
            FD_SET(reportSocket, &fdin);
            maxFd = MAX(reportSocket, maxFd);
        }
        if (commandSocket != INVALID_SOCKET) {
            FD_SET(commandSocket, &fdin);
            maxFd = MAX(commandSocket, maxFd);
        }
        if (dataSocket != INVALID_SOCKET) {
            FD_SET(dataSocket, &fdin);
            maxFd = MAX(dataSocket, maxFd);
        }

        if (m_pi->m_settings.verbose >= 4) {
            wxLogMessage(wxT("BR24radar_pi: %s receive %d waiting for socket %d,%d,%d"), m_ri->name, maxFd + 1, reportSocket, commandSocket, dataSocket);
        }

        r = select(maxFd + 1, &fdin, 0, 0, &tv);

        if (m_pi->m_settings.verbose >= 2) {
            wxLogMessage(wxT("BR24radar_pi: %s select r=%d reportFound=%d"), m_ri->name, r, FD_ISSET(reportSocket, &fdin));
        }

        if (r > 0) {
            if (dataSocket != INVALID_SOCKET && FD_ISSET(dataSocket, &fdin)) {
                rx_len = sizeof(rx_addr);
                r = recvfrom(dataSocket, (char *) data, sizeof(data), 0, (struct sockaddr *) &rx_addr, &rx_len);
                if (r > 0) {
                    //UINT8 * a = (UINT8 *) &rx_addr.ipv4.sin_addr; // sin_addr is in network layout
                    //wxLogMessage(wxT("%s at %u.%u.%u.%u received frame"), m_ri->name, a[0] , a[1] , a[2] , a[3]);
                    ProcessFrame(data, r);
                } else {
                    closesocket(dataSocket);
                    dataSocket = INVALID_SOCKET;
                    m_data_seen = false;
                }
            }

            if (commandSocket != INVALID_SOCKET && FD_ISSET(commandSocket, &fdin)) {
                rx_len = sizeof(rx_addr);
                r = recvfrom(commandSocket, (char *) data, sizeof(data), 0, (struct sockaddr *) &rx_addr, &rx_len);
                if (r > 0 && rx_addr.addr.ss_family == AF_INET && m_pi->m_settings.verbose) {
                    UINT8 * a = (UINT8 *) &rx_addr.ipv4.sin_addr; // sin_addr is in network layout
                    logBinaryData(wxString::Format(wxT("%s at %u.%u.%u.%u received command"), m_ri->name, a[0] , a[1] , a[2] , a[3]), data, r);
                } else {
                    closesocket(commandSocket);
                    commandSocket = INVALID_SOCKET;
                }
            }

            if (reportSocket != INVALID_SOCKET && FD_ISSET(reportSocket, &fdin)) {
                rx_len = sizeof(rx_addr);
                r = recvfrom(reportSocket, (char * ) data, sizeof(data), 0, (struct sockaddr *) &rx_addr, &rx_len);
                if (r > 0) {
                    if (ProcessReport(data, r)) {
                        memcpy(&mcastFoundAddr, m_interface->ifa_addr, sizeof(mcastFoundAddr));
                        m_mcast_addr = &mcastFoundAddr;
                        memcpy(&radarFoundAddr, &rx_addr, sizeof(radarFoundAddr));
                        m_radar_addr = &radarFoundAddr;
                        wxString addr;
                        UINT8 * a = (UINT8 *) &m_radar_addr->sin_addr; // sin_addr is in network layout
                        addr.Printf(wxT("%u.%u.%u.%u"), a[0] , a[1] , a[2] , a[3]);
                        m_pi->m_ip_address = addr;
                        m_pi->m_update_address_control = true;   //signals to RenderGLOverlay that the control box should be updated
                        if (!m_radar_seen) {
                            if (m_pi->m_settings.verbose) {
                                wxLogMessage(wxT("BR24radar_pi: %s detected at %s"), m_ri->name, addr.c_str());
                            }
                        }
                        m_radar_seen = true;
                        m_radar_watchdog = time(0);
                    }
                } else {
                    wxLogMessage(wxT("BR24radar_pi: %s report socket failed"), m_ri->name);
                    closesocket(reportSocket);
                    reportSocket = INVALID_SOCKET;
                    m_radar_seen = false;
                }
            }

        } else if (no_data_timeout >= 2) {
            no_data_timeout = 0;
            if (m_data_seen) {
                if (dataSocket != INVALID_SOCKET) {
                    closesocket(dataSocket);
                    dataSocket = INVALID_SOCKET;
                }
                if (commandSocket != INVALID_SOCKET) {
                    closesocket(commandSocket);
                    commandSocket = INVALID_SOCKET;
                }
            }
            else if (reportSocket != INVALID_SOCKET) {
                closesocket(reportSocket);
                reportSocket = INVALID_SOCKET;
                m_radar_seen = false;
                m_mcast_addr = 0;
                m_radar_addr = 0;
            }
        } else {
            no_data_timeout++;
        }
    }

    if (dataSocket != INVALID_SOCKET) {
        closesocket(dataSocket);
    }
    if (commandSocket != INVALID_SOCKET) {
        closesocket(commandSocket);
    }
    if (reportSocket != INVALID_SOCKET) {
        closesocket(reportSocket);
    }

    if (m_interface_array) {
        freeifaddrs(m_interface_array);
    }
    return 0;
}

//
// The following is the received radar state. It sends this regularly
// but especially after something sends it a state change.
//
#pragma pack(push,1)
struct radar_state02 {
    UINT8  what;     // 0   0x02
    UINT8  command;  // 1 0xC4
    UINT16 range;    //  2-3   0x06 0x09
    UINT32 field4;   // 4-7    0
    UINT32 field8;   // 8-11
    UINT8  gain;     // 12
    UINT8  field13;  // 13  ==1 for sea auto
    UINT8  field14;  // 14
    UINT16 field15;  // 15-16
    UINT32 sea;      // 17-20   sea state (17)
    UINT8  field21;  // 21
    UINT8  rain;     // 22   rain clutter
    UINT8  field23;  // 23
    UINT32 field24;  //24-27
    UINT32 field28;  //28-31
    UINT8  field32;  //32
    UINT8  field33;  //33
    UINT8  interference_rejection;  //34
    UINT8  field35;  //35
    UINT8  field36;  //36
    UINT8  field37;  //37
    UINT8  field38;  //38
    UINT8  field39;  //39
    UINT8  field40;  //40
    UINT8  field41;  //41
    UINT8  target_boost;  //42
    UINT16 field8a;
    UINT32 field8b;
    UINT32 field9;
    UINT32 field10;
    UINT32 field11;
    UINT32 field12;
    UINT32 field13x;
    UINT32 field14x;
};

struct radar_state04_66 {  // 04 C4 with length 66
    UINT8  what;     // 0   0x04
    UINT8  command;  // 1   0xC4
    UINT32 field2;   // 2-5
    UINT16 bearing_alignment;    // 6-7
    UINT16 field8;   // 8-9
    UINT16 antenna_height;       // 10-11
};

struct radar_state01_18 {  // 04 C4 with length 66
    UINT8  what;      // 0   0x01
    UINT8  command;   // 1   0xC4
    UINT8  radar_status;    // 2
    UINT8  field3;    // 3
    UINT8  field4;    // 4
    UINT8  field5;    // 5
    UINT16 field6;    // 6-7
    UINT16 field8;    // 8-9
    UINT16 field10;   // 10-11
};

struct radar_state08_18 {  //08 c4  length 18
    UINT8  what;        // 0  0x08
    UINT8  command;     // 1  0xC4
    UINT8  field2;      // 2
    UINT8  local_interference_rejection;   // 3
    UINT8  scan_speed;  // 4
    UINT8  sls_auto;    // 5 installation: sidelobe suppression auto
    UINT8  field6;      // 6
    UINT8  field7;      // 7
    UINT8  field8;      // 8
    UINT8  side_lobe_suppression;      // 9 installation: sidelobe suppression
    UINT16 field10;     // 10-11
    UINT8  noise_rejection;       // 12    noise rejection
    UINT8  target_sep;  // 13
};
#pragma pack(pop)

bool br24Receive::ProcessReport( UINT8 * report, int len )
{
    static char prevStatus = 0;

    if (m_pi->m_settings.verbose) {
        logBinaryData(wxT("ProcessReport"), report, len);
    }

    if (report[1] == 0xC4) {
        // Looks like a radar report. Is it a known one?
        switch ((len << 8) + report[0]) {
            case (18 << 8) + 0x01: { //  length 18, 01 C4
                radar_state01_18 * s = (radar_state01_18 *)report;
                // Radar status in byte 2
                if (s->radar_status != prevStatus) {
                    prevStatus = report[2];
                    m_ri->radar_type = RT_4G;   // only 4G Tx on channel B
                }
                break;
            }

            case (99 << 8) + 0x02: {  // length 99, 08 C4
                radar_state02 * s = (radar_state02 *)report;
                if (s->field8 == 1) {   // 1 for auto
                    m_ri->gain.Update(-1);  // auto gain
                }
                else {
                    m_ri->gain.Update(s->gain * 100 / 255);
                }
                m_ri->rain.Update(s->rain * 100 / 255);
                if (s->field13 == 0x01) {
                    m_ri->sea.Update(-1); // auto sea
                }
                else{
                    m_ri->sea.Update(s->sea * 100 / 255);
                }
                m_ri->target_boost.Update(s->target_boost);
                m_ri->interference_rejection.Update(s->interference_rejection);

                if (m_pi->m_settings.verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s state range=%u gain=%u sea=%u rain=%u interference_rejection=%u target_boost=%u ")
                    , m_ri->name
                    , s->range
                    , s->gain
                    , s->sea
                    , s->rain
                    , s->interference_rejection
                    , s->target_boost
                    );
                }
                break;
            }

            case (564 << 8) + 0x05: {    // length 564, 05 C4
                // Content unknown, but we know that BR24 radomes send this
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received familiar BR24 report"), report, len);
                }
                m_ri->radar_type = RT_BR24;
                break;
            }

            case (18 << 8) + 0x08: {  // length 18, 08 C4
                // contains scan speed, noise rejection and target_separation and sidelobe suppression
                radar_state08_18 * s08 = (radar_state08_18 *)report;

                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxString::Format(wxT("scanspeed= %d, noise = %u target_sep %u")
                                          , s08->scan_speed, s08->noise_rejection, s08->target_sep
                                          ), report, len);
                }
                m_ri->scan_speed.Update(s08->scan_speed);
                m_ri->noise_rejection.Update(s08->noise_rejection);
                m_ri->target_separation.Update(s08->target_sep);
                if (s08->sls_auto == 1) {
                    m_ri->side_lobe_suppression.Update(-1);
                }
                else{
                    m_ri->side_lobe_suppression.Update(s08->side_lobe_suppression * 100 / 255);
                }
                m_ri->local_interference_rejection.Update(s08->local_interference_rejection);

                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received report_08"), report, len);
                }
                break;
            }

            case (66 << 8) + 0x04: {     // 66 bytes starting with 04 C4
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received report_04"), report, len);
                }
                radar_state04_66 * s04_66 = (radar_state04_66 *)report;

                // bearing alignment
                int ba = (int)s04_66->bearing_alignment / 10;
                if (ba > 180) {
                    ba = ba - 360;
                }
                m_ri->bearing_alignment.Update(ba);

                // antenna height
                m_ri->antenna_height.Update(s04_66->antenna_height / 1000);
                break;
            }

            default: {
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received unknown report"), report, len);
                }
                break;
            }

        }
        return true;
    }
    else if (report[1] == 0xF5) {
        // Looks like a radar report. Is it a known one?
        switch ((len << 8) + report[0]) {
            case (16 << 8) + 0x0f:
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received BR24 report"), report, len);
                }
                m_ri->radar_type = RT_BR24;
                break;

            case (8 << 8) + 0x10:
            case (10 << 8) + 0x12:
            case (46 << 8) + 0x13:
                // Content unknown, but we know that BR24 radomes send this
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received familiar report"), report, len);
                }
                break;

            default:
                if (m_pi->m_settings.verbose) {
                    logBinaryData(wxT("received unknown report"), report, len);
                }
                break;

        }
        return true;
    }

    if (m_pi->m_settings.verbose) {
        logBinaryData(wxT("received unknown message"), report, len);
    }
    return false;
}

// vim: sw=4:ts=8:
