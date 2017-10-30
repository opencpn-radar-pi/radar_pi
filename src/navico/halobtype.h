
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo_B(236, 6, 7, 103, 6135);
static const NetworkAddress reportHalo_B(236, 6, 7, 105, 6137);
static const NetworkAddress sendHalo_B(236, 6, 7, 104, 6136);

PLUGIN_END_NAMESPACE

#endif

#include "NavicoCommon.h"

DEFINE_RADAR(RT_HaloB,                                        /* Type */
             wxT("Navico Halo B"),                            /* Name */
             NAVICO_SPOKES,                                   /* Spokes */
             NAVICO_SPOKE_LEN,                                /* Spoke length (max) */
             NavicoControlsDialog(RT_HaloB),                  /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo_B, dataHalo_B), /* Receive class constructor */
             NavicoControl(sendHalo_B)                        /* Send/Control class constructor */
)
