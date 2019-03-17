#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo24_A(236, 6, 8, 35, 6515);
static const NetworkAddress reportHalo24_A(236, 6, 8, 35, 6517);
static const NetworkAddress sendHalo24_A(236, 6, 8, 36, 6516);

#define RANGE_METRIC_RT_Halo24A \
  { 50, 75, 100, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 12000, 16000, 24000, 36000, 48000 }
#define RANGE_MIXED_RT_Halo24A                                                                                               \
  {                                                                                                                          \
    50, 75, 100, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, 1852 * 6, \
        1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                                \
  }
#define RANGE_NAUTIC_RT_Halo24A                                                                                             \
  {                                                                                                                         \
    1852 / 32, 1852 / 16, 1852 / 8, 1852 / 4, 1852 / 2, 1852 * 3 / 4, 1852 * 1, 1852 * 3 / 2, 1852 * 2, 1852 * 3, 1852 * 4, \
        1852 * 6, 1852 * 8, 1852 * 12, 1852 * 16, 1852 * 24, 1852 * 36, 1852 * 48, 1852 * 64, 1852 * 72                     \
  }

PLUGIN_END_NAMESPACE

#endif

#include "NavicoCommon.h"

DEFINE_RADAR(RT_Halo24A,                                          /* Type */
             wxT("Navico Halo24 A"),                              /* Name */
             NAVICO_SPOKES,                                       /* Spokes */
             NAVICO_SPOKE_LEN,                                    /* Spoke length (max) */
             NavicoControlsDialog(RT_Halo24A),                    /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo24_A, dataHalo24_A), /* Receive class constructor */
             NavicoControl(sendHalo24_A)                          /* Send/Control class constructor */
)
