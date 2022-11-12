#!/bin/bash
#
# Perform the flatpak sandbox installation, called from within
# flatpak-builder.

here="$(dirname $(readlink -fn $0))"
prefix="/app/extensions/o-charts"

case "$(uname -m)" in
    x86_64)
        echo "Installing pre-built x86_64 binaries."
        cp $here/linux64/oexserverd  $prefix/bin
        cp $here/linux64/libsgllnx64-*.so $prefix/lib
        ;;
    aarch64)
        echo "Installing pre-built aarch64 binaries."
        cp $here/linuxarm64/oexserverd  $prefix/bin
        cp $here/linuxarm64/libsgllnx64-*.so $prefix/lib
        ;;
    armv7*)
        echo "Installing pre-built armhf binaries."
        cp $here/linuxarm/oexserverd  $prefix/bin
        cp $here/linuxarm/libsgllnx64-*.so $prefix/lib
        ;;
    *) 
        echo "$0: Unspported architecture: $(uname -m)"
        exit 1
        ;;
esac
