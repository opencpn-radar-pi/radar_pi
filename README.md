# shipdriver\_pi README

A plugin for OpenCPN. Almost a simulator or is it a game?

Icons made by Freepik(http://www.freepik.com) from Flaticon(https://www.flaticon.com/) and is licensed by Creative Commons BY 3.0 (http://creativecommons.org/licenses/by/3.0/)

The plugin uses a continous integration setup described in CI.md.

## Building

    $ rm -rf build && mkdir build && cd build
    $ cmake ..
    $ make
    $ make package
    $ make repack-tarball

*flatpak* builds does not follow this scheme, see flatpak/README.md

The build generates a tar.gz tarball which can be used by the new plugin
installer, available from OpenCPN 5.2.0. On most platforms it also builds
a platform-dependent legacy installer like an NSIS .exe on Windows, a Debian
.deb package on Linux and a .dmg image for MacOS.
