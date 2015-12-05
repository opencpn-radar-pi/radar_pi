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

#include "br24Transmit.h"

br24Transmit::br24Transmit(wxString name, int radar)
{
    struct sockaddr_in m_addr;
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;

    static UINT8 radar_mcast_send_addr[2][4] =
            { { 236, 6, 7, 14 }
            , { 236, 6, 7, 10 }
            };

    static unsigned short radar_mcast_send_port[2] =
            { 6658
            , 6680
            };

    memcpy(&m_addr.sin_addr.s_addr, radar_mcast_send_addr[radar % 2], sizeof(m_addr.sin_addr.s_addr));
    m_addr.sin_port = htons(radar_mcast_send_port[radar % 2]);
    m_name = name;
    m_radar_socket = INVALID_SOCKET;
}

br24Transmit::~br24Transmit()
{
    if (m_radar_socket != INVALID_SOCKET) {
        closesocket(m_radar_socket);
        wxLogMessage(wxT("BR24radar_pi: %s transmit socket closed"), m_name);
    }
}

bool br24Transmit::Init( int verbose )
{
    int r;
    int one = 1;
    struct sockaddr_in adr;

    m_verbose = verbose;

    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(0);
    m_radar_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_radar_socket == INVALID_SOCKET) {
        r = -1;
    }
    else {
        r = setsockopt(m_radar_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &one, sizeof(one));
    }

    if (!r) {
        r = bind(m_radar_socket, (struct sockaddr *) &adr, sizeof(adr));
    }

    if (r) {
        wxLogError(wxT("BR24radar_pi: Unable to create UDP sending socket"));
        // Might as well give up now
        return false;
    }

    if (m_verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: %s transmit socket open"), m_name);
    }
    return true;
}


bool br24Transmit::TransmitCmd(UINT8 * msg, int size)
{
    if (sendto(m_radar_socket, (char *)msg, size, 0, (struct sockaddr *) &m_addr, sizeof(m_addr)) < size) {
        wxLogError(wxT("BR24radar_pi: Unable to transmit command to %s"), m_name);
        return false;
    }
    if (m_verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: %s transmit cmd len=%d"), m_name, size);
    }
    return true;
}

void br24Transmit::RadarTxOff()
{
    UINT8 pck[3] = {0x00, 0xc1, 0x01};

    if (m_verbose) {
        wxLogMessage(wxT("BR24radar_pi: %s transmit: turn Off"), m_name);
    }
    TransmitCmd(pck, sizeof(pck));
    pck[0] = 0x01;
    pck[2] = 0x00;
    TransmitCmd(pck, sizeof(pck));
}

void br24Transmit::RadarTxOn()
{
    UINT8 pck[3] = { 0x00, 0xc1, 0x01 };               // ON

    if (m_verbose) {
        wxLogMessage(wxT("BR24radar_pi: %s transmit: turn on"), m_name);
    }
    TransmitCmd(pck, sizeof(pck));
    pck[0] = 0x01;
    TransmitCmd(pck, sizeof(pck));
}

bool br24Transmit::RadarStayAlive()
{
    if (m_verbose) {
        wxLogMessage(wxT("BR24radar_pi: %s transmit: stay alive"), m_name);
    }

    UINT8 pck[] = {0xA0, 0xc1};
    TransmitCmd(pck, sizeof(pck));
    UINT8 pck2[] = { 0x03, 0xc2 };
    TransmitCmd(pck2, sizeof(pck2));
    UINT8 pck3[] = { 0x04, 0xc2 };
    TransmitCmd(pck3, sizeof(pck3));
    UINT8 pck4[] = { 0x05, 0xc2 };
    return TransmitCmd(pck4, sizeof(pck4));
}

bool br24Transmit::SetRange(int meters)
{
    if (meters >= 50 && meters <= 72704) {
        unsigned int decimeters = (unsigned int) meters * 10;
        UINT8 pck[] =
            { 0x03
            , 0xc1
            , (UINT8) ((decimeters >>  0) & 0XFFL)
            , (UINT8) ((decimeters >>  8) & 0XFFL)
            , (UINT8) ((decimeters >> 16) & 0XFFL)
            , (UINT8) ((decimeters >> 24) & 0XFFL)
            };
        if (m_verbose) {
            wxLogMessage(wxT("BR24radar_pi: %s transmit: range %d meters"), m_name, meters);
        }
        return TransmitCmd(pck, sizeof(pck));
    }
    return false;
}

bool br24Transmit::SetControlValue(ControlType controlType, int value)
{                                                   // sends the command to the radar
    bool r = false;

    switch (controlType) {

        case CT_GAIN: {
            if (value < 0) {                // AUTO gain
                UINT8 cmd[] = {
                    0x06,
                    0xc1,
                    0, 0, 0, 0, 0x01,
                    0, 0, 0, 0xad     // changed from a1 to ad
                };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s Gain: Auto in setcontrolvalue"), m_name);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            } else {                        // Manual Gain
                int v = (value + 1) * 255 / 100;
                if (v > 255) {
                    v = 255;
                }
                UINT8 cmd[] = {
                    0x06,
                    0xc1,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    (UINT8) v
                };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s Gain: %d"), m_name, value);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            }
            break;
        }

        case CT_RAIN: {                       // Rain Clutter - Manual. Range is 0x01 to 0x50
            int v = (value + 1) * 255 / 100;
            if (v > 255) {
                v = 255;
            }
            UINT8 cmd[] = { 0x06, 0xc1, 0x04, 0, 0, 0, 0, 0, 0, 0, (UINT8) v };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Rain: %d"), m_name, value);
            }
            r =  TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_SEA: {
            if (value < 0) {                 // Sea Clutter - Auto
                UINT8 cmd[11] = { 0x06, 0xc1, 0x02, 0, 0, 0, 0x01, 0, 0, 0, 0xd3 };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s Sea: Auto"), m_name);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            } else {                       // Sea Clutter
                int v = (value + 1) * 255 / 100 ;
                if (v > 255) {
                    v = 255;
                }
                UINT8 cmd[] = {
                    0x06,
                    0xc1,
                    0x02,
                    0, 0, 0, 0, 0, 0, 0,
                    (UINT8) v
                };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s Sea: %d"), m_name, value);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            }
            break;
        }

        case CT_INTERFERENCE_REJECTION: {
            UINT8 cmd[] = {
                0x08,
                0xc1,
                (UINT8) value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Rejection: %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_TARGET_SEPARATION: {
            UINT8 cmd[] = {
                0x22,
                0xc1,
                (UINT8) value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Target separation: %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_NOISE_REJECTION: {
            UINT8 cmd[] = {
                0x21,
                0xc1,
                (UINT8) value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Noise rejection: %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_TARGET_BOOST: {
            UINT8 cmd[] = {
                0x0a,
                0xc1,
                (UINT8) value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Target boost: %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_SCAN_SPEED: {
            UINT8 cmd[] = {
                0x0f,
                0xc1,
                (UINT8) value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Scan speed: %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_ANTENNA_HEIGHT: {
            int v = value * 1000;
            int v1 = v / 256;
            int v2 = v - 256 * v1;
            UINT8 cmd[10] = { 0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0 };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Antenna height: %d"), m_name, v);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_BEARING_ALIGNMENT: {   // to be consistent with the local bearing alignment of the pi
                                       // this bearing alignment works opposite to the one an a Lowrance display
            if (value < 0) {
                value += 360;
            }
            int v = value * 10;
            int v1 = v / 256;
            int v2 = v - 256 * v1;
            UINT8 cmd[4] = { 0x05, 0xc1,
                (UINT8)v2, (UINT8)v1 };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s Bearing alignment: %d"), m_name, v);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        case CT_SIDE_LOBE_SUPPRESSION: {
            if (value < 0) {
                UINT8 cmd[] = {                 // SIDE_LOBE_SUPPRESSION auto
                    0x06, 0xc1, 0x05, 0, 0, 0, 0x01, 0, 0, 0, 0xc0 };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s command Tx CT_SIDE_LOBE_SUPPRESSION Auto"), m_name);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            }
            else{
                int v = (value + 1) * 255 / 100;
                if (v > 255) {
                    v = 255;
                }
                UINT8 cmd[] = {
                    0x6, 0xc1, 0x05, 0, 0, 0, 0, 0, 0, 0,
                    (UINT8)v
                };
                if (m_verbose) {
                    wxLogMessage(wxT("BR24radar_pi: %s command Tx CT_SIDE_LOBE_SUPPRESSION: %d"), m_name, value);
                }
                r = TransmitCmd(cmd, sizeof(cmd));
            }
            break;
        }

        case CT_LOCAL_INTERFERENCE_REJECTION: {
            if (value < 0) value = 0;
            if (value > 3) value = 3;
            UINT8 cmd[] = {
                0x0e,
                0xc1,
                (UINT8)value
            };
            if (m_verbose) {
                wxLogMessage(wxT("BR24radar_pi: %s local interference rejection %d"), m_name, value);
            }
            r = TransmitCmd(cmd, sizeof(cmd));
            break;
        }

        default: {
            r = false;
        }
    }

    return r;
}

// vim: sw=4:ts=8:
