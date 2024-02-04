Fix plugin information
======================

Upcoming changes in main OpenCPN makes it important to provide correct
information in the plugin API. In this context, "correct" means using the
info defined in Plugin.cmake and hence in the plugin catalog.

In order to achieve this, first add these two lines to _config.h.in_:

    #define PKG_SUMMARY _("@PKG_SUMMARY@")
    #define PKG_DESCRIPTION _(R"""(@PKG_DESCRIPTION@)""")

In the next step redefine the API functions `GetCommonName()`, 
`GetShortDescription()` and `GetLongDescription()`. In the 
shipdriver case these lives in _src/Shipdriver_pi.cpp_ and becomes:

    wxString ShipDriver_pi::GetCommonName() { return PLUGIN_API_NAME; }
    wxString ShipDriver_pi::GetShortDescription() { return PKG_SUMMARY; }
    wxString ShipDriver_pi::GetLongDescription() { return PKG_DESCRIPTION; }

This is boilerplate code common for all plugins.
