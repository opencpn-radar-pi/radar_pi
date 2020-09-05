#!/usr/bin/env bash

#
# Build the flatpak artifacts. Uses docker to run Fedora on
# in full-fledged VM; the actual build is done in the Fedora
# container. 
#
# flatpak-builder can be run in a docker image. However, this
# must then be run in privileged mode, which means we need 
# a full-fledged VM to run it.
#

set -xe

# Give the apt update daemons a chance to leave the scene.
sudo systemctl stop apt-daily.service apt-daily.timer
sudo systemctl kill --kill-who=all apt-daily.service apt-daily-upgrade.service
sudo systemctl mask apt-daily.service apt-daily-upgrade.service
sudo systemctl daemon-reload


# Start up the docker instance
DOCKER_SOCK="unix:///var/run/docker.sock"
echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null

sudo systemctl restart docker.service
sudo docker pull fedora:31
docker run --privileged -d -ti -e "container=docker"  \
    -v /sys/fs/cgroup:/sys/fs/cgroup \
    -v "$(pwd):/project:rw" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    fedora:31   /bin/bash
DOCKER_CONTAINER_ID=$(docker ps | awk '/fedora/ {print $1}')
docker logs $DOCKER_CONTAINER_ID


cat > build.sh << "EOF"
su -c "dnf install -q -y sudo dnf-plugins-core"
sudo dnf -y copr enable leamas/opencpn-mingw
sudo dnf -q builddep  -y /project/mingw/fedora/opencpn-deps.spec
cd /project
rm -rf build; mkdir build; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../mingw/fedora/toolchain.cmake ..
make tarball
EOF

# Run the build in docker
docker exec -ti \
    $DOCKER_CONTAINER_ID /bin/bash -xec "bash -xe /project/build.sh"
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID
rm -f build.sh

# Wait for apt-daily to complete, apt-daily should not restart, it's masked.
# https://unix.stackexchange.com/questions/315502
echo -n "Waiting for apt_daily lock..."
sudo flock /var/lib/apt/daily_lock echo done

# Select latest available python, install cloudsmith required by upload script
pyenv local $(pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1)
pip3 install cloudsmith-cli
