
radar_pi - Radar Plugin for OpenCPN
===================================

This is a _plugin_ for [OpenCPN](https://opencpn.org), a free open source marine navigation program.

It supports the following yacht radars:

* Garmin HD
* Garmin xHD
* Navico Broadband Radar BR24, 3G and 4G
* Navico HALO (20, 20+, 24, 4, 6, 8)
* Various Raymarine radars
* A simple emulator

_WARNING_ Newer Garmin radars like xHD2 and Fantom are NOT supported. Furuno radars and all others not in the above list are not supported.

End Users
---------
Please see the documentation Wiki at https://github.com/opencpn-radar-pi/radar_pi/wiki

Compiling
---------
Building for external developers (not the plugin developers) is documented in INSTALL.md

License
-------
The plugin code is licensed under the terms of the GPL v2. See the file COPYING for details.

Acknowledgements
----------------

This would be a pretty useless plugin if OpenCPN did not exist. Hurrah for @bdbcat!

The plugin was started by @cowelld, who took @bdbcat's Garmin radar plugin and the reverse engineered data from http://www.roboat.at/technologie/radar/ and created the first working version in 2012.

@canboat started helping Dave. Soon he implemented most of the 4G specific functionality, refactored the code, rewrote the control dialogs, implemented the guard zones, target trails, PPI windows, EBL/VRM and mouse cursor functionality.

@nohal contributed the packaging installers, without which this would still be a set of sources instead of deliverable packages for various platforms.

@Hakansv did a lot of testing and contributed the idle timer, and coordinated work on the translations.

@douwefokkema also tested a lot and implemented the heading on radar functionality, optimised the OpenGL drawing, implemented the refresh rate code and optimised the target trails code. Douwe also made the MARPA and ARPA functionality.

@seandepagnier wrote initial shader support, challenged us to improve the code and wrote some optimizations.

[![Hosted By: Cloudsmith](https://img.shields.io/badge/OSS%20hosting%20by-cloudsmith-blue?logo=cloudsmith&style=for-the-badge)](https://cloudsmith.com)

Package repository hosting is graciously provided by  [Cloudsmith](https://cloudsmith.com).
Cloudsmith is doing this for free for open source projects, great stuff.
