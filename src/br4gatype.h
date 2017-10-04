
#ifdef INITIALIZE_RADAR
static UINT8 br4gaSendAddr[4] = {236, 6, 7, 10};
#endif

DEFINE_RADAR(RT_4GA, "Navico 4G A", NavicoControlsDialog, NavicoReceive, NavicoControl(br4gaSendAddr, 6680))
