Shipdriver flatpak README
-------------------------

A simple packaging to be used together with  opencpn's flatpak package.
To build and install:

  - Install flatpak and flatpak-builder as described in https://flatpak.org/
  - Install the opencpn  flatpak package using latest official version on
    flathub:

          $ flatpak install --user flathub org.opencpn.OpenCPN

    Or use the provisionary beta repo at fedorapeople.org (mirrors master):

          $ flatpak install --user \
              https://leamas.fedorapeople.org/opencpn/opencpn.flatpakref

  - The shipdriver flatpak plugin can now be built and installed using

        $ cd build
        $ cmake ..
        $ make flatpak

Build produces a plugin tarball and metadata file, to be used with the new
plugin installer available from 5.2.0. See the README.md in top directory.

The actual version built depends on the source stanza in the end of the
yaml file. By default, the tip of the master branch is used.

NOTE: As of today building against the flathub version does not work 
unless the .yaml file is patched, see comments in file. The long story
is that the development headers are unavailable in the flathub version, 
but exists in the nightly builds.
