
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo_B = {{IPV4_ADDR(236, 6, 7, 8)}, IPV4_PORT(6678)};
static const NetworkAddress reportHalo_B = {{IPV4_ADDR(236, 6, 7, 9)}, IPV4_PORT(6679)};
static const NetworkAddress sendHalo_B = {{IPV4_ADDR(236, 6, 7, 10)}, IPV4_PORT(6680)};

PLUGIN_END_NAMESPACE

#endif

// Halo has 2048 spokes of exactly 512 bytes each
#if SPOKES_MAX < 2048
#undef SPOKES_MAX
#define SPOKES_MAX 2048
#endif
#if SPOKE_LEN_MAX < 512
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX 512
#endif

DEFINE_RADAR(RT_HaloB,                                        /* Type */
             wxT("Navico Halo B"),                            /* Name */
             2048,                                            /* Spokes */
             512,                                             /* Spoke length (max) */
             NavicoControlsDialog(RT_HaloB),                  /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, reportHalo_B, dataHalo_B), /* Receive class constructor */
             NavicoControl(sendHalo_B)                        /* Send/Control class constructor */
)
