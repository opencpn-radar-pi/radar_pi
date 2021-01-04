# Build Dependencies HOWTO

Before making a local build, build dependencies needs to be installed. 
This document tries to describe the procedures.

## Windows
Before build, run the `buildwin\win_deps.bat` file. The file requires a
working choco installation, see https://docs.chocolatey.org/en-us/choco/setup.

The first run will install various software using choco, and should be 
invoked with administrative privileges. 

Subsequent builds should still invoke the file, but it can be done with
regular permissions.

## Debian/Ubuntu
Install build dependencies using something like:

     $ sudo apt install devscripts equivs
     $ sudo mk-build-deps -ir ci/control
     $ sudo apt-get -q --allow-unauthenticated install -f

## Flatpak
Install runtime and opencpn as described in flatpak/README.md


## MacOS

Use the ci script, which just builds the artifacts used like

    $ cd build
    $ ../ci/circleci-build-macos local_build

After the initial run the build dependencies are in place and configured,
builds could be done using `make tarball` etc. Note that MacOS also
supports `make create-pkg` which creates a .pkg installer.
