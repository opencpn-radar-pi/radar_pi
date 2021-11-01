## INSTALL: Building Plugins generic README.

Install build dependencies as described in the 
[manual](https://opencpn-manuals.github.io/main/AlternativeWorkflow/Local-Build.html)

After cloning, enter this directory, setup the library submodule and
enter a fresh working directory:

    $ git submodule update --init opencpn-libs
    $ rm -rf build; mkdir build; cd build

A "normal" (not flatpak) tar.gz tarball which can be used by the new plugin
installer available from OpenCPN 5.2.0 is built using:

    $ cmake ..
    $ make tarball

To build the flatpak tarball:

    $ cmake ..
    $ make flatpak

Historically, it has been possible to build legacy packages like
an NSIS installer on Windows and .deb packages on Linux. This ability
has been removed in the 5.6.0 cycle.

#### Building for Android

Builds for android requires an ndk installation and an updated cmake,
see manual (above).

To build an android aarch64 tarball:

    $ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/android-aarch64-toolchain.cmake ..
    $ make

To build an android armhf tarball

    $ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/android-armhf-toolchain.cmake ..
    $ make

#### Building on windows (MSVC)
On windows, a different workflow is used:

    > ..\buildwin\win_deps.bat
    > cmake -T v141_xp -G "Visual Studio 15 2017" ^
           -DCMAKE_BUILD_TYPE=RelWithDebInfo  ..
    > cmake --build . --target tarball --config RelWithDebInfo
