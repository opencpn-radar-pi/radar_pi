#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo_B(236, 6, 7, 103, 6135);
static const NetworkAddress reportHalo_B(236, 6, 7, 105, 6137);
static const NetworkAddress sendHalo_B(236, 6, 7, 104, 6136);

#define RANGE_METRIC_RT_HaloB \
  { 50, 75, 100, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 12000, 16000, 24000, 36000, 48000 }
#define RANGE_MIXED_RT_HaloB                                                                                                 \
  {                                                                                                                          \
    50, 75, 100, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, 1852 * 6, \
        1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                                \
  }
#define RANGE_NAUTIC_RT_HaloB                                                                                               \
  {                                                                                                                         \
    1852 / 32, 1852 / 16, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, \
        1852 * 6, 1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                     \
  }

PLUGIN_END_NAMESPACE

#endif

#include "NavicoCommon.h"

DEFINE_RADAR(RT_HaloB,                                        /* Type */
             wxT("Navico Halo B"),                            /* Name */
             NAVICO_SPOKES,                                   /* Spokes */
             NAVICO_SPOKE_LEN,                                /* Spoke length (max) */
             NavicoControlsDialog(RT_HaloB),                  /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo_B, dataHalo_B), /* Receive class constructor */
             NavicoControl(sendHalo_B),                       /* Send/Control class constructor */
             RO_SECONDARY                                     /* This type has two radars and this is the second */
)
