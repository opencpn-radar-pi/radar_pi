#!/usr/bin/env bash

#
#

# bailout on errors and echo commands.
set -euo pipefail
set -x

sudo apt-get -qq update

DOCKER_SOCK="unix:///var/run/docker.sock"

echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null
sudo service docker restart;
sleep 5;

docker run --rm --privileged multiarch/qemu-user-static:register --reset

docker run --privileged -d -ti -e "container=docker" \
      -v ~/source_top:/source_top \
      -v $(pwd):/ci-source:rw \
      $DOCKER_IMAGE /bin/bash

DOCKER_CONTAINER_ID=$(sudo docker ps | grep raspbian | awk '{print $1}')

#echo $DOCKER_CONTAINER_ID 

docker exec -ti $DOCKER_CONTAINER_ID apt-get update || \
docker exec -ti $DOCKER_CONTAINER_ID apt-get update
docker exec -ti $DOCKER_CONTAINER_ID echo "------\nEND apt-get update\n" 

docker exec -ti $DOCKER_CONTAINER_ID apt-get -y install git cmake build-essential cmake gettext wx-common libwxgtk3.0-dev libbz2-dev libcurl4-openssl-dev libexpat1-dev libcairo2-dev libarchive-dev liblzma-dev libexif-dev lsb-release || \
docker exec -ti $DOCKER_CONTAINER_ID apt-get -y install git cmake build-essential cmake gettext wx-common libwxgtk3.0-dev libbz2-dev libcurl4-openssl-dev libexpat1-dev libcairo2-dev libarchive-dev liblzma-dev libexif-dev lsb-release

docker exec -ti $DOCKER_CONTAINER_ID /bin/bash -c \
    'set -x; mkdir ci-source/build; cd ci-source/build; cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; make; make package'

echo "Stopping"
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID

sudo apt-get install python3-pip python3-setuptools || :

#  Upload to cloudsmith

STABLE_REPO=${OCPN_STABLE_REPO}
UNSTABLE_REPO=${OCPN_UNSTABLE_REPO}

#UNSTABLE_REPO=${CLOUDSMITH_UNSTABLE_REPO:-'kees-verruijt/ocpn-plugins-unstable'}
#STABLE_REPO=${CLOUDSMITH_STABLE_REPO:-'kees-verruijt/ocpn-plugins-stable'}

echo "Check 0.5"
echo $STABLE_REPO
echo $UNSTABLE_REPO

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

set -xe

#python -m ensurepip

python3 -m pip install -q setuptools
python3 -m pip install -q cloudsmith-cli

BUILD_ID=${APPVEYOR_BUILD_NUMBER:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
tag=$(git tag --contains HEAD)

echo "Check 1"
echo $tag
echo $commit
echo $OCPN_BRANCH

#  shift to the build directory linked from docker execution
echo "Check 1.5"
pwd
ls

cd build

xml=$(ls *.xml)
tarball=$(ls *.tar.gz)
tarball_basename=${tarball##*/}

# extract the project name for a filename.  e.g. oernc-pi... sets PROJECT to  "oernc"
PROJECT=$(ls *.xml | awk '{split($0,a,"-"); print a[1]}')

echo "Check 2"
echo $PROJECT
echo $xml
echo $tarball_basename
echo $tarball

source ../build/pkg_version.sh
test -n "$tag" && VERSION="$tag" || VERSION="${VERSION}.${commit}"
test -n "$tag" && REPO="$STABLE_REPO" || REPO="$UNSTABLE_REPO"
tarball_name=${PKG_UPLOAD_NAME}-tarball

echo "Check 3"

sudo sed -i -e "s|@pkg_repo@|$REPO|"  $xml
sudo sed -i -e "s|@name@|$tarball_name|" $xml
sudo sed -i -e "s|@version@|$VERSION|" $xml
sudo sed -i -e "s|@filename@|$tarball_basename|" $xml

echo "Check 4"
#echo $PKG_TARGET
#raspbian
#echo $PKG_TARGET_VERSION
#10

cat $xml

sudo tar xf $tarball
tar_dir=${tarball%%.tar.gz}
ls -la
sudo cp $xml $tar_dir/metadata.xml
tar_dir_here=${tar_dir##*/}
sudo tar czf $tarball $tar_dir_here

ls -l "$tarball" "$xml"

_cloudsmith_options="push raw --republish --verbose"

cloudsmith ${_cloudsmith_options}                                               \
    --name ${PKG_UPLOAD_NAME}-metadata                                          \
    --version ${VERSION}                                                        \
    --summary "radar opencpn plugin metadata for automatic installation"        \
    $REPO $xml

cloudsmith ${_cloudsmith_options}                                               \
    --name $tarball_name                                                        \
    --version ${VERSION}                                                        \
    --summary "radar opencpn plugin tarball for automatic installation"         \
    $REPO $tarball
