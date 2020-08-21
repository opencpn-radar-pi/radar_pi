Shipdriver flatpak README
-------------------------

This is a simple packaging to be used together with  opencpn's flatpak
package. To build and install:

  - Install flatpak and flatpak-builder as described in https://flatpak.org/
  - Install the opencpn  flatpak package using latest official version on
    flathub:

          $ flatpak install --user flathub org.opencpn.OpenCPN

    Or use the provisionary beta repo at fedorapeople.org (mirrors master):

          $ flatpak install --user \
              https://leamas.fedorapeople.org/opencpn/opencpn.flatpakref

  - The shipdriver flatpak plugin can now be built and installed using

      $ cd build
      $ cmake .. -DOCPN_FLATPAK:BOOL=ON
      $ make flatpak-build
      $ make flatpak-pkg

The actual version built depends on the source stanza in the end of the
yaml file. By default, the tip of the master branch is used.

BUGS: As of today, building against the flathub version does not work since
the headers are stripped. Until fixed, build and install the opencpn
flatpak version locally as documented in flatpak/README.md
