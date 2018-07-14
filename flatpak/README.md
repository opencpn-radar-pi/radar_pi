Shipdriver flatpak README
-------------------------

This is a simple packaging to be use the shipdriver plugin when using
the opencpn's flatpak package. To build and install:

  - Install flatpak and flatpak-builder as described in https://flatpak.org/
  - Install the opencpn  flatpak package.

      + Using latest official version on flathub:

          $ flatpak install --user flathub org.opencpn.OpenCPN

      + Using the provisionary beta repo at fedorapeople.org do:

          $ flatpak install --user \
              https://leamas.fedorapeople.org/opencpn/opencpn.flatpakref

  - The shipdriver plugin can now be built and installed using

      $ cd build; make -f ../flatpak/Makefile build
      $ cd build; make -f ../flatpak/Makefile install

The actual version built depends on the tag: stanza in the yaml file; update to
other versions as preferred.
