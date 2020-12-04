#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

static const NetworkAddress report(0, 0, 0, 0, 0);
static const NetworkAddress data(0, 0, 0, 0, 0);
static const NetworkAddress send(0, 0, 0, 0, 0);

// Ranges below are indicative only, actual ranges are received from the radar
// ranges are filled in RMEReceive.cpp, from SRadarFeedback
// Received range values are divided by 2 before storing in the arrays below
// This is because Raymarine displays a range value that is half of the actual transmit range
// The actual range in the code is in m_range_meters, double the value of the arrays below
#define RANGE_METRIC_RM_E120 \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#define RANGE_MIXED_RM_E120                                                                                                    \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#define RANGE_NAUTIC_RM_E120 \
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

PLUGIN_END_NAMESPACE

#endif

#ifndef RM_E120_SPOKES
#define RM_E120_SPOKES 2048
#endif

#ifndef RM_E120_SPOKE_LEN
#define RM_E120_SPOKE_LEN (512)  // BR radars generate 512 separate values per range, at 8 bits each (according to original RMradar_pi)
#define RETURNS_PER_LINE (512)  // BR radars generate 512 separate values per range, at 8 bits each
#endif

#if SPOKES_MAX < RAYMARINE_SPOKES
#undef SPOKES_MAX
#define SPOKES_MAX RM_E120_SPOKES
#endif
#if SPOKE_LEN_MAX < RM_E120_SPOKE_LEN
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX RM_E120_SPOKE_LEN
#endif

// Raymarine E120 has 2048 spokes of exactly 1024 pixels of 4 bits each, packed in 512 bytes
DEFINE_RADAR(RM_E120,                                                     /* Type */
             wxT("Raymarine E120"),                                       /* Name */
             RM_E120_SPOKES,                                              /* Spokes */
             RM_E120_SPOKE_LEN,                                           /* Spoke length (max) */             
             RME120ControlsDialog(RM_E120),                                 /* ControlsDialog class constructor */
             RME120Receive(pi, ri, report, data, send),                                       /* Receive class constructor */
             RME120Control(pi, ri),                     /* Send/Control class constructor */
             RO_SINGLE /* This type only has a single radar and does not need locating */
)
