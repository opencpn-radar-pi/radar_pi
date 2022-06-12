/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
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

#ifndef _RAYMARINERECEIVE_H_
#define _RAYMARINERECEIVE_H_

#include "RadarFactory.h"
#include "RadarReceive.h"
#include "raymarine/RaymarineLocate.h"
#include "socketutil.h"

PLUGIN_BEGIN_NAMESPACE

//
// An intermediary class that implements the common parts of some radars.
//

class RaymarineReceive : public RadarReceive {
public:
    RaymarineReceive(radar_pi* pi, RadarInfo* ri, NetworkAddress reportAddr,
        NetworkAddress dataAddr, NetworkAddress sendAddr)
        : RadarReceive(pi, ri)
    {
        m_info.serialNr = wxT(" ");
        m_info.spoke_data_addr = dataAddr;
        m_info.report_addr = reportAddr;
        m_info.send_command_addr = sendAddr;
        m_next_spoke = -1;
        m_range_meters = 0;
        m_target_expansion = false;
        m_comm_socket = INVALID_SOCKET;
        // However radar is leading for range_units, will be overwritten with
        // value from the radar
        m_shutdown_time_requested = 0;
        m_is_shutdown = false;
        m_first_receive = true;
        m_interface_addr = m_ri->GetRadarInterfaceAddress();
        wxString addr1 = m_interface_addr.FormatNetworkAddress();
        m_receive_socket = GetLocalhostServerTCPSocket();
        m_send_socket = GetLocalhostSendTCPSocket(m_receive_socket);
        SetInfoStatus(wxString::Format(
            wxT("%s: %s"), m_ri->m_name.c_str(), _("Initializing")));
        SetPriority(70);
        LOG_INFO(wxT("%s receive thread created, prio= %i"),
            m_ri->m_name.c_str(), GetPriority());

        // InitializeLookupData();

        RadarLocationInfo info = m_ri->GetRadarLocationInfo();
        if (info.report_addr.IsNull() && !m_info.report_addr.IsNull()) {
            // BR24, 3G, 4G initial setup, when ini file doesn't contain
            // multicast addresses yet In this case m_info.spoke_data_addr etc.
            // are correct, these don't really change in the wild according to
            // our data, so write them into the RadarLocationInfo object.
            m_ri->SetRadarLocationInfo(m_info);
            LOG_INFO(
                wxT("%s  RaymarineReceive SetRadarLocationInfo m_info= %s "),
                m_ri->m_name, m_info.to_string());
        } else if (!info.report_addr.IsNull() && ri->m_radar_type != RT_BR24) {
            // Restart, when ini file contains multicast addresses, that are
            // hopefully still correct. This will also overwrite the initial
            // addresses for 3G and 4G with those from the ini file If not we
            // will time-out and then NavicoLocate will find the radar.
            m_info = m_ri->GetRadarLocationInfo();
            LOG_INFO(wxT("radar addresses from ini file loaded"));
        }
        LOG_INFO(
            wxT(" %s using addresses: %s"), m_ri->m_name, m_info.to_string());
        m_ri->SetRadarLocationInfo(
            m_info); //  in case the initial values from constuctor are used,
                     //  write these to radar_pi

        m_range_meters = 1; // this will be considered an invalid value.
        m_previous_angle = 0;
    };

    ~RaymarineReceive() {};

    void* Entry(void);
    void Shutdown(void);
    wxString GetInfoStatus();
    SOCKET GetCommSocket() { return m_comm_socket; }

    NetworkAddress m_interface_addr;
    RadarLocationInfo m_info;

    wxLongLong
        m_shutdown_time_requested; // Main thread asks this thread to stop
    volatile bool m_is_shutdown;

private:
    void ProcessFrame(const uint8_t* data, size_t len);

    SOCKET PickNextEthernetCard();
    SOCKET GetNewReportSocket();

    SOCKET m_receive_socket; // Where we listen for message from m_send_socket
    SOCKET m_send_socket; // A message to this socket will interrupt select()
                          // and allow immediate shutdown
    SOCKET m_comm_socket; // Radar communication socket

    struct ifaddrs* m_interface_array;
    struct ifaddrs* m_interface;

    int m_next_spoke;
    bool m_first_receive;
    SpokeBearing m_previous_angle;

    wxCriticalSection m_lock; // Protects m_status
    wxString m_status; // Userfriendly string
    wxString m_firmware; // Userfriendly string #2

    void ProcessRMReport(const UINT8* data, int len);
    int m_range_meters, m_updated_range;
    bool m_target_expansion;
    void ProcessFixedReport(const UINT8* data, int len);
    void ProcessScanData(const UINT8* data, int len);
    void ProcessQuantumScanData(const UINT8* data, int len);
    void ProcessQuantumReport(const UINT8* data, int len);

    void SetFirmware(wxString s);
    void UpdateSendCommand();

    void SetInfoStatus(wxString status)
    {
        wxCriticalSectionLocker lock(m_lock);
        m_status = status;
    }
};

PLUGIN_END_NAMESPACE
#endif /* _RAYMARINERECEIVE_H_ */
