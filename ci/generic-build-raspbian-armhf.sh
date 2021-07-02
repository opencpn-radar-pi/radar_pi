#!/usr/bin/env bash

#
# Build for Raspbian in a docker container
#
# Bugs: The buster build is real slow: https://forums.balena.io/t/85743

set -xe

# Create build script
#
if [ -n "$TRAVIS_BUILD_DIR" ]; then
    ci_source="$TRAVIS_BUILD_DIR"
elif [ -d ~/project ]; then
    ci_source=~/project
else
    ci_source="$(pwd)"
fi

cat > $ci_source/build.sh << "EOF"
curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt -q update

sudo apt install devscripts equivs
sudo mk-build-deps -ir /ci-source/build-deps/control-raspbian
sudo apt-get -q --allow-unauthenticated install -f

# Temporary fix until 3.19 is available as a pypi package
# 3.19 is needed: https://gitlab.kitware.com/cmake/cmake/-/issues/20568
url='https://dl.cloudsmith.io/public/alec-leamas/opencpn-plugins-stable/deb/debian'
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake-data_3.19.3-0.1_all.deb
wget $url/pool/${OCPN_TARGET/-*/}/main/c/cm/cmake_3.19.3-0.1_armhf.deb
sudo apt install ./cmake_3.19.3-0.1_armhf.deb ./cmake-data_3.19.3-0.1_all.deb

cd /ci-source
rm -rf build; mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=debug ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
EOF


# Run script in docker image
#
docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run --privileged -ti \
    -e "OCPN_TARGET=$OCPN_TARGET" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO=$OCPN_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    -e "TRAVIS_BUILD_NUMBER=$TRAVIS_BUILD_NUMBER" \
    -v "$ci_source:/ci-source:rw" \
    $DOCKER_IMAGE /bin/bash -xe /ci-source/build.sh
rm -f $ci_source/build.sh


# Install cloudsmith-cli (for upload) and cryptography (for git-push).
#
pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1 \
    > $HOME/.python-version
# Latest pip 21.0.0 is broken:
python3 -m pip install --force-reinstall pip==20.3.4
# https://github.com/pyca/cryptography/issues/5753 -> cryptography < 3.4
python3 -m pip install --user cloudsmith-cli 'cryptography<3.4'

# python install scripts in ~/.local/bin, teach upload.sh to use in it's PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
