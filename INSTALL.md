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
    $ git config --global protocol.file.allow always
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

The Android builds are governed by _build-conf.rc_ and can be disabled or
just work in dry-run mode without uploading anything. See the file for
details.

#### Building on windows (MSVC)
On Windows, build is performed in the _build_ directory using a CMD shell.

The first, initial step to install build depencies needs administrative
privileges and is run using:

    > set PATH=C:\ProgramData\chocolatey\bin;C:\Windows\system32;C:\Windows
    > ..\buildwin\win_deps.bat

The initial `set PATH=...` line strips down %PATH% to a very small path,
excluding most if not all otherwise available tools. In many cases this is
neither required nor convenient and can be excluded. However, doing it
represents a tested baseline.

Actual builds can (i. e., should) be performed without administrative
privileges by running 

    > ..\ci\appveyor.bat


#### Disabling Android builds

Plugins could either disable android builds completely, always run them or
just run them for example every tenth build. This is configured in the file
_build-conf.rc_, see comments in this file for details.

By default, these builds are enabled

#### Building for MacOS

The macos build uses a quite aggressive caching scheme. In case of problems
it might be necessary to invalidate the cache so new dependencies are
downloaded and built from source. This is done in the file
_build-deps/macos-cache-stamp_, see comments in that file.

Note that the initial MacOS build takes a long time. However, subsequent
builds runs at roughly the same time as other platforms.
