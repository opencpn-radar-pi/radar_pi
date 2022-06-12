#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Dummy addresses below to avoid "Unable to listen to socket" when no radar
static const NetworkAddress quantum_report(0, 0, 0, 0, 0);
static const NetworkAddress quantum_data(0, 0, 0, 0, 0);
static const NetworkAddress quantum_send(0, 0, 0, 0, 0);

// Ranges below are indicative only, actual ranges are received from the radar
// ranges are filled in RMEReceive.cpp, from SRadarFeedback
// Received range values are divided by 2 before storing in the arrays below
// This is because Raymarine displays a range value that is half of the actual
// transmit range The actual range in the code is in m_range_meters, double the
// value of the arrays below
#define RANGE_METRIC_RM_QUANTUM                                                \
    {                                                                          \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        \
    }
#define RANGE_MIXED_RM_QUANTUM                                                 \
    {                                                                          \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        \
    }
#define RANGE_NAUTIC_RM_QUANTUM { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

PLUGIN_END_NAMESPACE

#endif

#ifndef RM_QUANTUM_SPOKES
#define RM_QUANTUM_SPOKES 250
#endif

#ifndef RM_QUANTUM_SPOKE_LEN

#define RM_QUANTUM_SPOKE_LEN (250)

#endif

#if SPOKES_MAX < RM_QUANTUM_SPOKES
#undef SPOKES_MAX
#define SPOKES_MAX RM_QUANTUM_SPOKES
#endif
#if SPOKE_LEN_MAX < RM_QUANTUM_SPOKE_LEN
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX RM_E120_SPOKE_LEN
#endif

DEFINE_RADAR(RM_QUANTUM, /* Type */
    wxT("Raymarine Quantum"), /* Name */
    RM_QUANTUM_SPOKES, /* Spokes */
    RM_QUANTUM_SPOKE_LEN, /* Spoke length (max) */
    RMQuantumControlsDialog(RM_QUANTUM), /* ControlsDialog class constructor */
    RaymarineReceive(pi, ri, quantum_report, quantum_data,
        quantum_send), /* Receive class constructor */
    RMQuantumControl(pi, ri), /* Send/Control class constructor */
    RO_SINGLE /* This type only has a single radar and does not need locating */
)
