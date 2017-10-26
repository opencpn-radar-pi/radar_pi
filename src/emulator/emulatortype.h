
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

PLUGIN_END_NAMESPACE

#endif

// Emulator has 2048 spokes of exactly 512 bytes each
#if SPOKES_MAX < 2048
#undef SPOKES_MAX
#define SPOKES_MAX 2048
#endif
#if SPOKE_LEN_MAX < 512
#undef SPOKE_LEN_MAX
#define SPOKE_LEN_MAX 512
#endif

//           Type         Name             Spokes ControlDialog          Receive class            Send/Control class
DEFINE_RADAR(RT_EMULATOR, wxT("Emulator"), 2048, EmulatorControlsDialog, EmulatorReceive(pi, ri), EmulatorControl)
