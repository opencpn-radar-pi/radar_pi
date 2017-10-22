
#ifdef INITIALIZE_RADAR

PLUGIN_BEGIN_NAMESPACE

PLUGIN_END_NAMESPACE

#endif

DEFINE_RADAR(RT_EMULATOR, wxT("Emulator"), EmulatorControlsDialog, EmulatorReceive(pi, ri), EmulatorControl)
