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
TARGET="package"
host=$(hostname)
case $host in
  kees-mbp)
    BUILDDIR=build-mac
    TARGET="create-pkg"
    PACKAGE="*.pkg"
    ;;

  openplotter)      # 'Raspian/Openplotter' on real RPi3 or Qemu
    BUILDDIR=build-linux-armhf
    PACKAGE="*.deb *.rpm *.bz2"
    ;;

  debian7|kees-tp)      # 'Debian 7' VirtualBox vm
    BUILDDIR=build-linux-x86_64
    PACKAGE="*.deb *.rpm *.bz2"
    ;;

  vlxkees)      # 'Debian 6' VirtualBox vm
    BUILDDIR=build-linux-x86
    PACKAGE="*.deb *.rpm *.bz2"
    ;;
esac

if [ "$BUILDDIR" = "" ]
then
  echo "Building unofficial release, without packages."
  TARGET=""
  STATE="unofficial"
  PACKAGE=""
  BUILDDIR="build-`uname`-`uname -m`"
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
cmake ..
echo "-------------------------- MAKE $TARGET -----------------------"
make $TARGET

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

