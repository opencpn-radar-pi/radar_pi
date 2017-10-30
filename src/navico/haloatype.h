
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo_A(236, 6, 7, 100, 6132);
static const NetworkAddress reportHalo_A(236, 6, 7, 102, 6134);
static const NetworkAddress sendHalo_A(236, 6, 7, 101, 6133);

PLUGIN_END_NAMESPACE

#endif

#include "NavicoCommon.h"

DEFINE_RADAR(RT_HaloA,                                        /* Type */
             wxT("Navico Halo A"),                            /* Name */
             NAVICO_SPOKES,                                   /* Spokes */
             NAVICO_SPOKE_LEN,                                /* Spoke length (max) */
             NavicoControlsDialog(RT_HaloA),                  /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo_A, dataHalo_A), /* Receive class constructor */
             NavicoControl(sendHalo_A)                        /* Send/Control class constructor */
)
