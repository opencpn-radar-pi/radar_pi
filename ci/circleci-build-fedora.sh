#!/bin/sh  -xe

#
# Build the mingw artifacts inside the Fedora container
#
set -euo pipefail
set -x

su -c "dnf install -y sudo dnf-plugins-core"
sudo dnf builddep  -y ci/opencpn-fedora.spec
rm -rf build; mkdir build; cd build
cmake ..
make -j2
make package
find . -name '*.so' -exec ls -l {} \;

