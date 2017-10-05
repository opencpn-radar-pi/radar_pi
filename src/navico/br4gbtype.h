
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress data4G_B = {{IPV4_ADDR(236, 6, 7, 13)}, IPV4_PORT(6657)};
static const NetworkAddress send4G_B = {{IPV4_ADDR(236, 6, 7, 14)}, IPV4_PORT(6658)};
static const NetworkAddress report4G_B = {{IPV4_ADDR(236, 6, 7, 15)}, IPV4_PORT(6659)};

PLUGIN_END_NAMESPACE

#endif

DEFINE_RADAR(RT_4GB, "Navico 4G B", NavicoControlsDialog(RT_4GB), NavicoReceive(pi, ri, report4G_B, data4G_B),
             NavicoControl(send4G_B))
