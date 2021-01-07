#!/usr/bin/env bash

#
# Build the mingw artifacts. Uses docker to run Fedora
# in full-fledged VM; the actual build is done in the Fedora
# container (Fedora's mingw support is better).
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
sudo docker pull fedora:33
docker run --privileged -d -ti -e "container=docker"  \
    -v /sys/fs/cgroup:/sys/fs/cgroup \
    -v "$(pwd):/project:rw" \
    -e "CLOUDSMITH_STABLE_REPO=$CLOUDSMITH_STABLE_REPO" \
    -e "CLOUDSMITH_BETA_REPO=$CLOUDSMITH_BETA_REPO" \
    -e "CLOUDSMITH_UNSTABLE_REPO=$CLOUDSMITH_UNSTABLE_REPO" \
    -e "CIRCLE_BUILD_NUM=$CIRCLE_BUILD_NUM" \
    fedora:33   /bin/bash
DOCKER_CONTAINER_ID=$(docker ps | awk '/fedora/ {print $1}')
docker logs $DOCKER_CONTAINER_ID

# Create the  fedora build script
cat > build.sh << "EOF"
su -c "dnf install -q -y sudo dnf-plugins-core"
sudo dnf -y copr enable leamas/opencpn-mingw
sudo dnf -q builddep  -y /project/build-deps/opencpn-deps.spec
cd /project
rm -rf build; mkdir build; cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../mingw/fedora/toolchain.cmake \
    ..
make -j $(nproc) VERBOSE=1 tarball
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
pyenv versions | sed 's/*//' | awk '{print $1}' | tail -1 \
    > $HOME/.python-version
python3 -m pip install --user cloudsmith-cli

# Required by git-push
python3 -m pip install --user cryptography

# python install scripts in ~/.local/bin, teach upload.sh to use it in PATH:
echo 'export PATH=$PATH:$HOME/.local/bin' >> ~/.uploadrc
