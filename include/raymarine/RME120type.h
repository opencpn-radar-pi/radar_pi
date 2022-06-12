#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Dummy addresses below to avoid "Unable to listen to socket" when no radar
static const NetworkAddress e120_report(236, 6, 7, 9, 1);
static const NetworkAddress e120_data(236, 6, 7, 9, 1);
static const NetworkAddress e120_send(0, 0, 0, 0, 0);

// Ranges below are indicative only, actual ranges are received from the radar
// ranges are filled in RMEReceive.cpp, from SRadarFeedback
// Received range values are divided by 2 before storing in the arrays below
// This is because Raymarine displays a range value that is half of the actual
// transmit range The actual range in the code is in m_range_meters, double the
// value of the arrays below
#define RANGE_METRIC_RM_E120                                                   \
    {                                                                          \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        \
    }
#define RANGE_MIXED_RM_E120                                                    \
    {                                                                          \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        \
    }
#define RANGE_NAUTIC_RM_E120                                                   \
    { 231, 463, 926, 1389, 2778, 5556, 11112, 22224, 44448, 88896,             \
        133344 }; // not needed ?

PLUGIN_END_NAMESPACE

#endif

#ifndef RM_E120_SPOKES
#define RM_E120_SPOKES 2048
#endif

#ifndef RM_E120_SPOKE_LEN

#define RM_E120_SPOKE_LEN                                                      \
    (1024) // BR radars generate 512 separate values per range, at 8 bits each
           // (according to original RMradar_pi)

#endif

#if SPOKES_MAX < RAYMARINE_SPOKES
#undef SPOKES_MAX
#define SPOKES_MAX RM_E120_SPOKES
#endif
#if SPOKE_LEN_MAX < RM_E120_SPOKE_LEN
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX RM_E120_SPOKE_LEN
#endif

// Raymarine E120 has 2048 spokes of exactly 1024 pixels of 4 bits each, packed
// in 512 bytes
DEFINE_RADAR(RM_E120, /* Type */
    wxT("Raymarine E120"), /* Name */
    RM_E120_SPOKES, /* Spokes */
    RM_E120_SPOKE_LEN, /* Spoke length (max) */
    RME120ControlsDialog(RM_E120), /* ControlsDialog class constructor */
    RaymarineReceive(pi, ri, e120_report, e120_data,
        e120_send), /* Receive class constructor */
    RME120Control(pi, ri), /* Send/Control class constructor */
    RO_SINGLE /* This type only has a single radar and does not need locating */
)
