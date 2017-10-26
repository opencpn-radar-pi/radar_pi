
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress data4G_A = {{IPV4_ADDR(236, 6, 7, 8)}, IPV4_PORT(6678)};
static const NetworkAddress report4G_A = {{IPV4_ADDR(236, 6, 7, 9)}, IPV4_PORT(6679)};
static const NetworkAddress send4G_A = {{IPV4_ADDR(236, 6, 7, 10)}, IPV4_PORT(6680)};

PLUGIN_END_NAMESPACE

#endif

// 4G has 2048 spokes of exactly 512 bytes each
#if SPOKES_MAX < 2048
#undef SPOKES_MAX
#define SPOKES_MAX 2048
#endif
#if SPOKE_LEN_MAX < 512
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX 512
#endif

DEFINE_RADAR(RT_4GA,                                      /* Type */
             wxT("Navico 4G A"),                          /* Name */
             2048,                                        /* Spokes */
             512,                                         /* Spoke length (max) */
             NavicoControlsDialog(RT_4GA),                /* ControlsDialog class constructor */
             NavicoReceive(pi, ri, report4G_A, data4G_A), /* Receive class constructor */
             NavicoControl(send4G_A)                      /* Send/Control class constructor */
)
