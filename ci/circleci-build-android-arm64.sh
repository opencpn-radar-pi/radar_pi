#!/bin/sh  -xe

#
# Build the Android artifacts inside the circleci linux container
#


set -xe

pwd

#not needed for CI built, the workflow starts up already in "project" directory.
# but required for local build.
#cd project

ls -la


sudo apt-get -q update
sudo apt-get -y install git cmake gettext unzip

# Get the OCPN Android build support package.
#NOT REQUIRED FOR LOCAL BUILD
echo "CIRCLECI_LOCAL: $CIRCLECI_LOCAL"
if [ -z "$CIRCLECI_LOCAL" ]; then
   wget https://github.com/bdbcat/OCPNAndroidCommon/archive/master.zip
fi
unzip -qq -o master.zip

pwd
ls -la

#change this for local build, so as not to overwrite any other generic build.
#mkdir -p build_android_64_ci
#cd build_android_64_ci
mkdir -p build
cd build

rm -f CMakeCache.txt
COMPDIR=$(find /opt/android -iname "android-ndk*")

cmake  \
  -D_wx_selected_config=androideabi-qt-arm64 \
  -DwxQt_Build=build_android_release_64_static_O3 \
  -DQt_Build=build_arm64/qtbase \
  -DCMAKE_AR=$COMPDIR/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-ar \
  -DCMAKE_CXX_COMPILER=$COMPDIR/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang++ \
  -DCMAKE_C_COMPILER=$COMPDIR/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang \
  -DOCPN_Android_Common=OCPNAndroidCommon-master \
  -DCMAKE_INSTALL_PREFIX=/ \
  ..

# Get number of processors and use this on make to speed up build
if type nproc &> /dev/null
then
    make_cmd="make -j"$(nproc)
else
    make_cmd="make"
fi
eval $make_cmd
make package

#  All below for local docker build
#ls -l

#xml=$(ls *.xml)
#tarball=$(ls *.tar.gz)
#tarball_basename=${tarball##*/}

#echo $xml
#echo $tarball
#echo $tarball_basename
#sudo sed -i -e "s|@filename@|$tarball_basename|" $xml


#tmpdir=repack.$$
#sudo rm -rf $tmpdir && sudo mkdir $tmpdir
#sudo tar -C $tmpdir -xf $tarball_basename
#sudo cp oesenc-plugin-android-arm64-16.xml metadata.xml
#sudo cp metadata.xml $tmpdir
#sudo tar -C $tmpdir -czf $tarball_basename .
#sudo rm -rf $tmpdir
