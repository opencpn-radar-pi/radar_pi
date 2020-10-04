# shipdriver\_pi README

A plugin for OpenCPN. Almost a simulator or is it a game?

Icons made by Freepik(http://www.freepik.com) from Flaticon(https://www.flaticon.com/) and is licensed by Creative Commons BY 3.0 (http://creativecommons.org/licenses/by/3.0/)

This software is Copyright (c) Mike Rossiter 2017-2020. It is distributed under the terms of the Gnu Public License version 2 or, at your option, any later version. See the file COPYING for details.

The software contains open-source licensed files under different licenses including Expat and Libpng. See the source files for details.

The plugin uses a continous integration setup described in CI.md.

## Building

Start using  `rm -rf build; mkdir build; cd build`

#### Regular installer tarball

Generate a tar.gz tarball which can be used by the new plugin installer,
available from OpenCPN 5.2.0: 

    $ cmake ..
    $ make tarball           

#### flatpak tarball

Build a installer tarball to be used by the flatpak'ed opencpn plugin 
installer:

    $ cmake ..
    $ make flatpak

#### Legacy installer 

On most platforms besides flatpak: build a platform-dependent legacy
installer like a NSIS .exe on Windows, a Debian .deb package on Linux
and a .dmg image for MacOS:

    $ cmake ..
    $ make pkg

#### Building on windows (MSVC)
On windows, a somewhat different workflow is used:

    > cmake -T v141\_xp ..
    > cmake -G "Visual Studio 15 2017" --config RelWithDebInfo  ..
    > cmake --build . --target tarball --config RelWithDebInfo

This is to build the installer tarball. Use _--target pkg_ to build the
legacy NSIS installer. The build requires access to a specific wxWidgets
3.1.2 build, see the appveyour.yml file for details.

