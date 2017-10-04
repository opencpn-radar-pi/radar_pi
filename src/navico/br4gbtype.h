
#ifdef INITIALIZE_RADAR
static UINT8 br4gbSendAddr[4] = {236, 6, 7, 14};
#endif

DEFINE_RADAR(RT_4GB, "Navico 4G B", NavicoControlsDialog, NavicoReceive, NavicoControl(br4gbSendAddr, 6658))
