
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

static const NetworkAddress dataBR24 = {{IPV4_ADDR(236, 6, 7, 8)}, IPV4_PORT(6678)};
static const NetworkAddress reportBR24 = {{IPV4_ADDR(236, 6, 7, 9)}, IPV4_PORT(6679)};
static const NetworkAddress sendBR24 = {{IPV4_ADDR(236, 6, 7, 10)}, IPV4_PORT(6680)};

PLUGIN_END_NAMESPACE

#endif

// BR24 has 2048 spokes of exactly 512 bytes each
#if SPOKES_MAX < 2048
#undef SPOKES_MAX
#define SPOKES_MAX 2048
#endif
#if SPOKE_LEN_MAX < 512
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX 512
#endif

DEFINE_RADAR(RT_BR24,                                     /* Type */
             wxT("Navico BR24"),                          /* Name */
             2048,                                        /* Spokes */
             NavicoControlsDialog(RT_BR24),               /* Controls class */
             NavicoReceive(pi, ri, reportBR24, dataBR24), /* Receive class */
             NavicoControl(sendBR24)                      /* Send/Control class */
)
