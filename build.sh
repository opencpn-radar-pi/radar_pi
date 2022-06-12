#!/bin/bash
#
#
echo <<EOD
Build script for radar_pi
-------------------------

This script is intended for use by @canboat, @hakansv and @douwefokkema only.
We use it to set preconditions on our build machines to run in IDE.

Any others: use at own risk.

Feel free to adapt this to your own situation.
EOD

set -e -u # Fail on first error

BUILD_TYPE="${1:-cli}"
BUILDDIR="rel-`uname`-`uname -m`"
STATE="Unofficial"
TARGET="tarball"
CMAKE_OPTIONS=""
MAKE_OPTIONS=""
host=$(hostname)

echo "Adapting to $host-$BUILD_TYPE"
case $host-$BUILD_TYPE in

  kees-m*cli)
    BUILDDIR=rel-mac
    WX="${HOME}/src/wx315_opencpn50_macos1010"
    CMAKE_OPTIONS=" -DwxWidgets_CONFIG_EXECUTABLE=${WX}/bin/wx-config
                    -DwxWidgets_CONFIG_OPTIONS='--prefix=${WX}'
                    -DCMAKE_INSTALL_PREFIX=
                    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10
                    -DCMAKE_OSX_ARCHITECTURES=x86_64
                  "
    export CMAKE_BUILD_PARALLEL_LEVEL=8
    ;;

  kees-m*ide)
    BUILDDIR=rel-mac-xcode
    CMAKE_OPTIONS=" -DwxWidgets_CONFIG_EXECUTABLE=$HOME/src/wx315_opencpn50_macos1010/bin/wx-config
                    -DwxWidgets_CONFIG_OPTIONS='--prefix=$HOME/src/wx312_opencpn50_macos1010'
                    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 -G Xcode"
    export CMAKE_BUILD_PARALLEL_LEVEL=8
    ;;

  pvlinux*)      # Parallels box with small RAM allocation
    export CMAKE_BUILD_PARALLEL_LEVEL=2
    ;;

  kees-tp)      # Lenovo X1E2 8core
    export CMAKE_BUILD_PARALLEL_LEVEL=16
    ;;

esac

if [ "$BUILDDIR" = "" ]
then
  echo "Building unofficial release, old style package."
  TARGET="pkg"
  STATE="unofficial"
  BUILDDIR="rel-`uname`-`uname -m`"
fi

[ $(uname) = "Darwin" ] && export MACOSX_DEPLOYMENT_TARGET=10.9
[ -d $BUILDDIR ] && rm -rf $BUILDDIR
mkdir -p $BUILDDIR
set -x 
cd $BUILDDIR
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TYPE="${BUILD_TYPE}" ${CMAKE_OPTIONS} ..
echo "-------------------------- MAKE $MAKE_OPTIONS $TARGET -----------------------"
make $MAKE_OPTIONS $TARGET

