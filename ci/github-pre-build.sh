#!/bin/sh
#
# This script is used by GitHub to install the dependencies
# before building wxWidgets but can also be run by hand if necessary (but
# currently it only works for Ubuntu versions used by the CI builds).

set -e -x

SUDO=sudo

case $(uname -s) in
    Linux)
        if [ -f /etc/apt/sources.list ]; then
            # Show information about the repositories and priorities used.
            echo 'APT sources used:'
            $SUDO grep --no-messages '^[^#]' /etc/apt/sources.list /etc/apt/sources.list.d/* || true
            echo '--- End of APT files dump ---'

            run_apt() {
                echo "-> Running apt-get $@"

                # Disable some (but not all) output.
                $SUDO apt-get -q -o=Dpkg::Use-Pty=0 "$@"

                rc=$?
                echo "-> Done with $rc"

                return $rc
            }

            run_apt update || echo 'Failed to update packages, but continuing nevertheless.'

            sudo apt-get -qq install devscripts equivs software-properties-common

            sudo mk-build-deps -ir build-deps/control
            sudo apt-get -q --allow-unauthenticated install -f
        fi
        ;;

    Darwin)
        # Install packaged dependencies
        here=$(cd "$(dirname "$0")"; pwd)
        for pkg in $(sed '/#/d' < $here/../build-deps/macos-deps);  do
            brew list --versions $pkg || brew install $pkg || brew install $pkg || :
            brew link --overwrite $pkg || brew install $pkg
        done

	pipx install cloudsmith-cli

        if [ ${USE_HOMEBREW:-0} -ne 1 ]; then
            # Install the pre-built wxWidgets package
            wget -q https://dl.cloudsmith.io/public/nohal/opencpn-plugins/raw/files/macos_deps_universal.tar.xz \
                -O /tmp/macos_deps_universal.tar.xz
            sudo tar -C /usr/local -xJf /tmp/macos_deps_universal.tar.xz
        else
            brew update
            brew install wxwidgets
        fi
        ;;
esac
