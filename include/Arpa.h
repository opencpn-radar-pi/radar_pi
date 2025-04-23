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

#ifndef _RADAR_MARPA_H_
#define _RADAR_MARPA_H_

#include "Doppler.h"
#include "Kalman.h"
#include "Matrix.h"
#include "RadarInfo.h"
#include <bitset>
#include <deque>
#include <memory>

PLUGIN_BEGIN_NAMESPACE

//    Forward definitions
class KalmanFilter;

#define TARGET_SEARCH_RADIUS1                                                  \
    (5) // radius of target search area for pass 1 (on top of the size of the
        // blob)
#define TARGET_SEARCH_RADIUS2                                                  \
    (10) // radius of target search area for pass 2   // used to be 15 in open
         // version
#define SCAN_MARGIN                                                            \
    (200) // number of lines that a next scan of the target may have moved
#define SCAN_MARGIN2                                                           \
    (1000) // if target is refreshed after this time you will be shure it is the
// next sweep

#define MAX_CONTOUR_LENGTH                                                     \
    (2000) // defines maximal size of target contour in pixels
#define MAX_CONTOUR_LENGTH_USED (500);
#define MAX_TARGET_DIAMETER                                                    \
    (200) // target will be set lost if diameter in pixels is larger than this
          // value
#define MAX_LOST_COUNT                                                         \
    (12) // number of sweeps that target can be missed before it is set to lost

#define FOR_DELETION                                                           \
    (-2) // status of a duplicate target used to delete a target
#define LOST (-1)
#define ACQUIRE0 (0) // 0 under acquisition, first seen, no contour yet
#define ACQUIRE1 (1) // 1 under acquisition, contour found, first position FOUND
#define ACQUIRE2 (2) // 2 under acquisition, speed and course taken
#define ACQUIRE3                                                               \
    (3) // 3 under acquisition, speed and course verified, next time active
        //    >=4  active

#define Q_NUM (4) // status Q to OCPN at target status
#define T_NUM (6) // status T to OCPN at target status
#define TARGET_SPEED_DIV_SDEV 2.
#define STATUS_TO_OCPN (5) // First status to be send to OCPN
#define START_UP_SPEED                                                         \
    (0.5) // maximum allowed speed (m/sec) for new target, real format with .
#define DISTANCE_BETWEEN_TARGETS (30) // minimum separation between targets
#define STATUS_TO_OCPN (5) // First status to be send to OCPN

typedef int target_status;
enum OCPN_target_status {
    Q, // acquiring
    T, // active
    L // lost
};

struct DynamicTargetData {
    int target_id;
    Matrix<double, 4> P;
    ExtendedPosition position;
    int status;
};

enum RefresState { NOT_FOUND, OUT_OF_SCOPE, FOUND };

class ArpaTarget {
    friend class Arpa; // Allow Arpa access to private members

public:
    // ArpaTarget(radar_pi* pi, RadarInfo* ri);
    ArpaTarget(radar_pi* pi, Arpa* arpa, int uid);
    ~ArpaTarget();

    int GetContour(Polar* p);
    bool FindNearestContour(Polar* pol, int dist);
    bool GetTarget(Polar expected_pol, Polar* target_pol, int dist);
    void RefreshTarget(int speed, int pass);
    void PassAIVDMtoOCPN(Polar* p);
    void PassTTMtoOCPN(Polar* p, OCPN_target_status s);
    void MakeAndTransmitTargetMessage();
    void MakeAndTransmitCoT();
    void SetStatusLost();
    void ResetPixels();
    void PixelCounter();
    void StateTransition(Polar* pol);
    bool Pix(int ang, int rad);
    //bool MultiPix(RadarInfo* ri, int ang, int rad, Doppler doppler);
    wxString EncodeAIVDM(
        int mmsi, double speed, double lon, double lat, double course);
    void TransferTargetToOtherRadar();
    void SendTargetToNearbyRadar(); 
    int m_status;
    int m_average_contour_length;
    bool m_small_fast; // For small and fast targets the Kalman filter will be overwritten for the initial positions
    RadarInfo* m_ri;
    Arpa* m_arpa;

private:
    radar_pi* m_pi;
    KalmanFilter m_kalman;
    int m_target_id;
    // radar position at time of last target fix, the polars in the contour
    // refer to this origin
    RefresState m_refreshed;
    //GeoPosition m_radar_pos;
    ExtendedPosition m_position; // holds actual position of target, after last SetMeasurement() // $$$ to do verify!! 
    // double m_speed_kn; // Average speed of target. TODO: Merge with
    //                    // m_position.speed?
    wxLongLong m_refresh_time; // time of last refresh
    double m_course;
    int m_stationary; // number of sweeps target was stationary
    int m_lost_count;
    Polar m_contour[MAX_CONTOUR_LENGTH
        + 1]; // contour of target, only valid immediately after finding it
    int m_contour_length;
    int m_previous_contour_length;
    Polar m_max_angle, m_min_angle, m_max_r, m_min_r,
        m_polar_pos; // charasterictics of contour
   // Polar m_expected_pol;   $$$
    bool m_automatic; // True for ARPA, false for MARPA.
    Doppler m_target_doppler; // ANY, NO_DOPPLER, APPROACHING, RECEDING,
                              // ANY_DOPPLER, NOT_APPROACHING, NOT_RECEDING
    
    uint32_t m_total_pix;
    uint32_t m_approaching_pix;
    uint32_t m_receding_pix;

    ExtendedPosition Polar2Pos(Polar pol, GeoPosition own_ship);
    Polar Pos2Polar(ExtendedPosition p, GeoPosition own_ship);
};

class Arpa {

    // Thread Analysis for locking purposes. All public functions are annotated
    // which thread calls them. THR(...) where it contains the following
    // letters:
    //          M = Main GUI thread
    //          R = RadarReceive
    //          I = InterRadar
    // LCK(...) where it indicates the following locks are already held:
    //          ri = RadarInfo exclusive lock
public:
    Arpa(radar_pi* pi); // THR(M)
    ~Arpa(); // THR(M)
    void DrawArpaTargetsOverlay(double scale, double arpa_rotate); // THR(M)
    void DrawArpaTargetsPanel(double scale, double arpa_rotate); // THR(M)
    void RefreshAllArpaTargets(); // THR(M LCK(ri))
    bool AcquireNewARPATarget(Polar pol, int status, Doppler doppler); // THR(M)
    void AcquireNewMARPATarget(RadarInfo* m_ri, ExtendedPosition pos); // THR(M)
    void DeleteTarget(const GeoPosition& pos); // THR(M)
    bool MultiPix(RadarInfo* ri, int ang, int rad, Doppler doppler); // THR(M)
    void DeleteAllTargets(); // THR(M)
    void RadarLost() // THR(M LCK(ri))
    {
        DeleteAllTargets(); // Let ARPA targets disappear
    }
    void InsertOrUpdateTargetFromOtherRadar(
        const DynamicTargetData* data, bool remote); // THR(M)
    void ClearContours(); // THR(R)
    int GetTargetCount()
    {
        return m_targets.size();
    } // THR(M), not sensitive to exact #
    void SearchDopplerTargets(RadarInfo* ri); // THR(M)
    void StoreRemoteTarget(DynamicTargetData* target); // THR(I)
    bool FindContourFromInside(RadarInfo* ri, Polar* p, Doppler doppler);

private:
    std::deque<std::unique_ptr<ArpaTarget>> m_targets;
    wxLongLong m_doppler_arpa_update_time[SPOKES_MAX];
    bool m_clear_contours;

    std::deque<GeoPosition> m_delete_target_position;

    std::deque<DynamicTargetData*> m_remote_target_queue;
    wxCriticalSection m_remote_target_lock;

    radar_pi* m_pi;
    

    void AcquireOrDeleteMarpaTarget(ExtendedPosition p, int status);
    void CalculateCentroid(ArpaTarget* t);
    void DrawContour(const ArpaTarget* t);
    bool Pix(RadarInfo* ri, int ang, int rad, Doppler doppler);
    bool IsAtLeastOneRadarTransmitting();
    void CleanUpLostTargets();
    DynamicTargetData* GetIncomingRemoteTarget();
};

PLUGIN_END_NAMESPACE

#endif
