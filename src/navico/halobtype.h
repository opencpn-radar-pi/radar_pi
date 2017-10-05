
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

// Note that the order of the ports is different on A and B. I guess someone
// in Navico just didn't realize this. Or it is just a bit of obfuscation.
static const NetworkAddress dataHalo_B = {{IPV4_ADDR(236, 6, 7, 8)}, IPV4_PORT(6678)};
static const NetworkAddress reportHalo_B = {{IPV4_ADDR(236, 6, 7, 9)}, IPV4_PORT(6679)};
static const NetworkAddress sendHalo_B = {{IPV4_ADDR(236, 6, 7, 10)}, IPV4_PORT(6680)};

PLUGIN_END_NAMESPACE

#endif

DEFINE_RADAR(RT_HaloB, "Navico Halo B", NavicoControlsDialog(RT_HaloB), NavicoReceive(pi, ri, reportHalo_B, dataHalo_B),
             NavicoControl(sendHalo_B))
