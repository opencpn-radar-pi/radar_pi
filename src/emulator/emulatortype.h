
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

DEFINE_RADAR(RT_EMULATOR,             /* Type */
             wxT("Emulator"),         /* Name */
             2048,                    /* Spokes */
             512,                     /* Spoke length */
             EmulatorControlsDialog,  /* Controls class */
             EmulatorReceive(pi, ri), /* Receive class */
             EmulatorControl          /* Send/Control class */
)
