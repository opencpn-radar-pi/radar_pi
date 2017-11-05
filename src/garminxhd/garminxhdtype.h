
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

static const NetworkAddress gx_data(239, 254, 2, 0, 50102);
static const NetworkAddress gx_report(239, 254, 2, 0, 50100);
static const NetworkAddress gx_send(172, 16, 2, 0, 50101);

PLUGIN_END_NAMESPACE

#endif

// Garmin xHD has 1440 spokes of varying 547 - 733 bytes each
#define GARMIN_XHD_SPOKES 1440
#define GARMIN_XHD_MAX_SPOKE_LEN 733

#if SPOKES_MAX < GARMIN_XHD_SPOKES
#undef SPOKES_MAX
#define SPOKES_MAX GARMIN_XHD_SPOKES
#endif
#if SPOKE_LEN_MAX < GARMIN_XHD_MAX_SPOKE_LEN
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX GARMIN_XHD_MAX_SPOKE_LEN
#endif

DEFINE_RADAR(RT_GARMIN_XHD,                                /* Type */
             wxT("Garmin xHD"),                            /* Name */
             GARMIN_XHD_SPOKES,                            /* Spokes */
             GARMIN_XHD_MAX_SPOKE_LEN,                     /* Spoke length */
             GarminxHDControlsDialog,                      /* Controls class */
             GarminxHDReceive(pi, ri, gx_report, gx_data), /* Receive class */
             GarminxHDControl(gx_send)                     /* Send/Control class */
)
