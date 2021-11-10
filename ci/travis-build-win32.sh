#!/bin/bash
#
# Build tarball and metadata on Windows using git bash.

function choco_install() {
    if net session >/dev/null 2>&1; then
        choco install -y $1
    else
	echo "Cannot install $1, missing permissions."
    fi
}

set -x

python --version  >/dev/null 2>&1 ||  choco_install python3
[ -z "$TRAVIS" ] || choco_install python3
last_pythonpath=$(ls -d /C/Python3* | tail -1)
export PATH=$last_pythonpath:$last_pythonpath/Scripts:$PATH
python --version
python -m pip install -U --force-reinstall pip

cmake --version >/dev/null 2>&1 || choco_install cmake
export PATH="/c/Program Files/cmake/bin:$PATH"
cmake --version

wget --version >/dev/null 2>&1 || choco_install wget

7z i >/dev/null  2>&1 || choco_install 7zip

if [ ! -d 'C:\Program Files (x86)\Poedit' ]; then
  choco_install poedit
fi

python -m ensurepip
python -m pip install --upgrade pip
python -m pip install -q setuptools wheel
python -m pip install -q cloudsmith-cli
python -m pip install cryptography

here=$(readlink -fn $(dirname $0))
cache=$here/../cache
test -d $cache || mkdir $cache
export WXWIN=$cache/wxWidgets-3.1.2
export wxWidgets_ROOT_DIR=$WXWIN
export wxWidgets_LIB_DIR=$WXWIN/lib/vc_dll
export PATH="$PATH:$WXWIN:$wxWidgets_LIB_DIR"

if [ ! -d $WXWIN ]; then
    wget -q https://download.opencpn.org/s/E2p4nLDzeqx4SdX/download \
        -O wxWidgets-3.1.2.7z
    7z x wxWidgets-3.1.2.7z -o$WXWIN
fi

export PATH="$PATH:/C/Program Files (x86)/Poedit/Gettexttools/bin"
export PATH="$PATH:/C/Program Files (x86)/Poedit"

test -d build-windows || mkdir build-windows
cd build-windows
rm -f CMakeCache.txt
if [ -f Makefile ]; then make clean; fi


cmake -T v141_xp -G "Visual Studio 15 2017" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --target tarball --config RelWithDebInfo

echo "export PATH=$last_pythonpath:$last_pythonpath/Scripts:\$PATH" >> ~/.uploadrc
echo "export PYTHONPATH=$last_pythonpath/lib/site-packages:" >>  ~/.uploadrc
echo "export PYTHONPATH=\$PYTHONPATH:$last_pythonpath/lib/" >>  ~/.uploadrc

python -m site
