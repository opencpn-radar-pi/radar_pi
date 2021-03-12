#!/bin/bash
#
#
echo <<EOD
Build script for radar_pi
-------------------------

This script is intended for use by @canboat only, otherwise he forgets which VM to build
on such that it works on as many systems as possible.

Feel free to adapt this to your own situation.
EOD

set -e -u # Fail on first error

BUILDDIR=""
STATE="Unofficial"
TARGET="pkg"
MAKE_OPTIONS=""
host=$(hostname)
case $host in
  kees-mbp)
    BUILDDIR=rel-mac
    TARGET="create-pkg"
    PACKAGE="*.pkg"
    ;;

  openplotter)      # 'Raspian/Openplotter' on real RPi3 or Qemu
    BUILDDIR=rel-linux-armhf
    PACKAGE="*.deb *.rpm *.bz2"
    ;;

  pvlinux*)      # Parallels box with small RAM allocation
    export CMAKE_BUILD_PARALLEL_LEVEL=2
    ;;

  debian7)      # 'Debian 7' VirtualBox vm
    BUILDDIR=rel-linux-x86_64
    PACKAGE="*.deb *.rpm *.bz2"
    ;;

  kees-tp)      # Lenovo X1E2 8core
    export CMAKE_BUILD_PARALLEL_LEVEL=8
    BUILDDIR=rel-linux-x86_64
    PACKAGE="*.deb *.rpm *.bz2"
    ;;

  vlxkees)      # 'Debian 6' VirtualBox vm
    BUILDDIR=rel-linux-x86
    PACKAGE="*.deb *.rpm *.bz2"
    ;;
esac

if [ "$BUILDDIR" = "" ]
then
  echo "Building unofficial release, old style package."
  TARGET="pkg"
  STATE="unofficial"
  BUILDDIR="rel-`uname`-`uname -m`"
else
  STATE=`cat release-state`
  if [ "$STATE" = "" ]
  then
    echo "Please set release state of package"
    exit 1
  fi
  echo "-------------------------- BUILD $STATE -----------------------"
fi

[ $(uname) = "Darwin" ] && export MACOSX_DEPLOYMENT_TARGET=10.9
[ -d $BUILDDIR ] && rm -rf $BUILDDIR
mkdir -p $BUILDDIR
cd $BUILDDIR
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
echo "-------------------------- MAKE $MAKE_OPTIONS $TARGET -----------------------"
make $MAKE_OPTIONS $TARGET

# Check for proper file mods on .so file, case VBox mount is screwed.
if [ -s libradar_pi.so ]
then
  if ls -l libradar_pi.so | grep -- '^-rwxr-xr-x'
  then
    :
  else
    echo "ERROR: libradar_pi.so has incorrect permissions."
    exit 1
  fi
fi

