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

#ifndef _NAVICOCONTROL_H_
#define _NAVICOCONTROL_H_

#include "RadarInfo.h"
#include "pi_common.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

class NavicoControl : public RadarControl {
public:
    NavicoControl(radar_pi* pi, RadarInfo* ri)
    {
        m_pi = pi;
        m_ri = ri;
        m_send_address = NetworkAddress();
        m_radar_socket = INVALID_SOCKET;
        m_name = ri->m_name;
    }

    ~NavicoControl()
    {
        if (m_radar_socket != INVALID_SOCKET) {
            closesocket(m_radar_socket);
            LOG_TRANSMIT(wxT("%s transmit socket closed"), m_name.c_str());
        }
    }

    bool Init(radar_pi* pi, RadarInfo* ri, NetworkAddress& interfaceAddress,
        NetworkAddress& radarAddress);
    void RadarTxOff();
    void RadarTxOn();
    bool RadarStayAlive();
    bool SetRange(int meters);
    bool SetControlValue(ControlType controlType, RadarControlItem& item,
        RadarControlButton* button);

    /*
     * Update the send address where to send data; this is variable
     * on Navico and RME radars, we are told where it is using a message.
     */
    void SetSendAddress(NetworkAddress sendSendAddress)
    {
        m_send_address = sendSendAddress;
    }

    bool TransmitCmd(
        const NetworkAddress& sendAddress, const uint8_t* msg, int size);

private:
    radar_pi* m_pi;
    RadarInfo* m_ri;
    SOCKET m_radar_socket;
    wxString m_name;
    NetworkAddress m_send_address;

    bool TransmitCmd(const uint8_t* msg, int size);
    void logBinaryData(const wxString& what, const uint8_t* data, int size);
};

PLUGIN_END_NAMESPACE

#endif /* _NAVICOCONTROL_H_ */
