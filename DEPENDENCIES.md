Plugin build dependencies
=========================

In order to build, the cmake setup requires build dependencies to be
installed. This document aimes to describe this for all platforms besides
flatpak, which is documented in flatpak/README.md

Windows
-------

Windows requires a working choco installation. How to set up this is outside
the scope of this document

Install choco deps (might require administrator privileges)

    > choco install wget
    > choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
    > refreshenv
    > choco install poedit
    > SET PATH="%PATH%;C:\Program Files (x86)\Poedit\Gettexttools\bin"

Define the base directory for wxWidgets, download and install:

    > SET wxWIN=C:\wxWidgets-3.1.2
    > SET wxWidgets_ROOT_DIR=%wxWIN%
    > SET wxWidgets_LIB_DIR=%wxWIN%\lib\vc_dll
    > wget https://download.opencpn.org/s/E2p4nLDzeqx4SdX/download ^
       -O wxWidgets-3.1.2.7z
    > 7z x wxWidgets-3.1.2.7z -o$wxWIN > null
    > SET PATH= "%PATH%;%wxWIN%;%wxWidgets_LIB_DIR%"

Based on ci/appveyor.yml, the ultimate source for windows builds.


Debian/Ubuntu
-------------

Install necessary packages using:

    $ sudo mk-build-deps -ir ../ci/control
    $ sudo apt-get -q --allow-unauthenticated install -f

MacOS
-----

MacOS needs a working brew installation. How to set up this is outside the
scope of this document.

Install required packages using

    $ for pkg in cairo cmake gettext libarchive libexif python wget; \
         do brew install $pkg; done
    $ brew install --cask packages

Download and unpack required pre-compiled wxWidgets:

    $ wget -q https://download.opencpn.org/s/rwoCNGzx6G34tbC/download \
            -O /tmp/wx312B_opencpn50_macos109.tar.xz
    $ tar -C /tmp -xJf /tmp/wx312B_opencpn50_macos109.tar.xz
