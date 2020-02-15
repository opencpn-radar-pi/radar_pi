radar plugin flatpak README
---------------------------

This is a simple packaging to use the radar plugin when using the opencpn's
flatpak package. To build and install:

  - Install flatpak and flatpak-builder as described in https://flatpak.org/
  - Install the opencpn flatpak package. Using the provisionary repo at
    fedorapeople.org do:

      $ flatpak install --user \
          https://opencpn.duckdns.org/opencpn/opencpn.flatpakref

  - The radar plugin can now be built and installed using

      $ make
      $ make install

The actual version built depends on the *tag:* stanza in the yaml file;
update to other versions as preferred.
