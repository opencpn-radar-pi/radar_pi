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

#ifndef _NAVICORECEIVE_H_
#define _NAVICORECEIVE_H_

#include "NavicoCommon.h"
#include "RadarReceive.h"
#include "navico/NavicoLocate.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

extern SOCKET g_HaloInfoSocket;

//
// An intermediary class that implements the common parts of any Navico radar.
//

class NavicoReceive : public RadarReceive {
public:
    NavicoReceive(radar_pi* pi, RadarInfo* ri, NetworkAddress reportAddr,
        NetworkAddress dataAddr, NetworkAddress sendAddr)
        : RadarReceive(pi, ri)
    {
        m_info.serialNr = wxT(" ");
        m_info.spoke_data_addr = dataAddr;
        m_info.report_addr = reportAddr;
        m_info.send_command_addr = sendAddr;
        m_next_scan = 0;
        m_radar_status = 0;
        m_shutdown_time_requested = 0;
        m_is_shutdown = false;
        m_first_receive = true;
        m_interface_addr = m_ri->GetRadarInterfaceAddress();
        m_halo_received_info = wxGetUTCTimeMillis();
        m_halo_sent_heading = m_halo_received_info;
        m_halo_sent_mystery = m_halo_received_info;

        m_receive_socket = GetLocalhostServerTCPSocket();
        m_send_socket = GetLocalhostSendTCPSocket(m_receive_socket);
        SetInfoStatus(wxString::Format(
            wxT("%s: %s"), m_ri->m_name.c_str(), _("Initializing")));
        SetPriority(70); // Priority of receive thread should be lower than prio
                         // of Navicolocate
        LOG_INFO(wxT("%s receive thread created, prio= %i"),
            m_ri->m_name.c_str(), GetPriority());
        InitializeLookupData();

        RadarLocationInfo info = m_ri->GetRadarLocationInfo();
        if (info.report_addr.IsNull() && !m_info.report_addr.IsNull()) {
            // BR24, 3G, 4G initial setup, when ini file doesn't contain
            // multicast addresses yet In this case m_info.spoke_data_addr etc.
            // are correct, these don't really change in the wild according to
            // our data, so write them into the RadarLocationInfo object.
            m_ri->SetRadarLocationInfo(m_info);
            LOG_INFO(wxT("%s info van constuctor RadarReceive "
                         "SetRadarLocationInfo m_info= %s "),
                m_ri->m_name, m_info.to_string());
        } else if (!info.report_addr.IsNull() && ri->m_radar_type != RT_BR24) {
            // Restart, when ini file contains multicast addresses, that are
            // hopefully still correct. This will also overwrite the initial
            // addresses for 3G and 4G with those from the ini file If not we
            // will time-out and then NavicoLocate will find the radar.
            m_info = m_ri->GetRadarLocationInfo();
            LOG_INFO(wxT("radar addresses from ini file loaded"));
        }
        LOG_INFO(wxT("%s navico receive using addresses: %s"), m_ri->m_name,
            m_info.to_string());
        m_ri->SetRadarLocationInfo(
            m_info); //  in case the initial value from constuctor are used,
                     //  write these to radar_pi
    };

    ~NavicoReceive() {};

    void InitializeLookupData();

    void* Entry(void);
    void Shutdown(void);
    wxString GetInfoStatus();

    NetworkAddress m_interface_addr;
    RadarLocationInfo m_info;

    wxLongLong
        m_shutdown_time_requested; // Main thread asks this thread to stop
    volatile bool m_is_shutdown;

private:
    SOCKET GetNewDataSocket();
    SOCKET GetNewInfoSocket();
    SOCKET GetNewReportSocket();
    SOCKET PickNextEthernetCard();
    bool ProcessReport(const uint8_t* data, size_t len);
    void DetectedRadar(NetworkAddress& radar_address);
    void ProcessFrame(const uint8_t* data, size_t len);
    void ReleaseInfoSocket();
    void SendHeadingPacket();
    void SendMysteryPacket();
    void SetRadarType(RadarType t);

    SOCKET m_receive_socket; // Where we listen for message from m_send_socket
    SOCKET m_send_socket; // A message to this socket will interrupt select()
                          // and allow immediate shutdown

    struct ifaddrs* m_interface_array;
    struct ifaddrs* m_interface;

    uint8_t m_next_scan;
    char m_radar_status;
    bool m_first_receive;

    wxLongLong m_halo_received_info; // When some mfd sent info
    wxLongLong m_halo_sent_heading; // When we send it, every 100 ms
    wxLongLong m_halo_sent_mystery; // When we send it, every 250 ms

    wxCriticalSection m_lock; // Protects m_status
    wxString m_status; // Userfriendly string
    wxString m_firmware; // Userfriendly string #2

    void SetInfoStatus(wxString status)
    {
        wxCriticalSectionLocker lock(m_lock);
        m_status = status;
    }
    void SetFirmware(wxString s)
    {
        wxCriticalSectionLocker lock(m_lock);
        m_firmware = s;
    }
};

PLUGIN_END_NAMESPACE

#endif /* _NAVICORECEIVE_H_ */
