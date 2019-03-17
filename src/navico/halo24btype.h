#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo24_B(236, 6, 8, 37, 6518);
static const NetworkAddress reportHalo24_B(236, 6, 8, 37, 6520);
static const NetworkAddress sendHalo24_B(236, 6, 8, 38, 6519);

#define RANGE_METRIC_RT_Halo24B \
  { 50, 75, 100, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 12000, 16000, 24000, 36000, 48000 }
#define RANGE_MIXED_RT_Halo24B                                                                                               \
  {                                                                                                                          \
    50, 75, 100, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, 1852 * 6, \
        1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                                \
  }
#define RANGE_NAUTIC_RT_Halo24B                                                                                             \
  {                                                                                                                         \
    1852 / 32, 1852 / 16, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, \
        1852 * 6, 1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                     \
  }

PLUGIN_END_NAMESPACE

#endif

#include "NavicoCommon.h"

DEFINE_RADAR(RT_Halo24B,                                          /* Type */
             wxT("Navico Halo24 B"),                              /* Name */
             NAVICO_SPOKES,                                       /* Spokes */
             NAVICO_SPOKE_LEN,                                    /* Spoke length (max) */
             NavicoControlsDialog(RT_Halo24B),                    /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo24_B, dataHalo24_B), /* Receive class constructor */
             NavicoControl(sendHalo24_B)                          /* Send/Control class constructor */
)
