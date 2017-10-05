
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

static const NetworkAddress dataBR24 = {{IPV4_ADDR(236, 6, 7, 8)}, IPV4_PORT(6678)};
static const NetworkAddress reportBR24 = {{IPV4_ADDR(236, 6, 7, 9)}, IPV4_PORT(6679)};
static const NetworkAddress sendBR24 = {{IPV4_ADDR(236, 6, 7, 10)}, IPV4_PORT(6680)};

PLUGIN_END_NAMESPACE

#endif

DEFINE_RADAR(RT_BR24, "Navico BR24", NavicoControlsDialog(RT_BR24), NavicoReceive(pi, ri, reportBR24, dataBR24),
             NavicoControl(sendBR24))
