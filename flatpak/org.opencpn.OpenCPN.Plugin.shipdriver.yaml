#
# Branches and runtime-version:
#   - master     Nigthly builds, 22.08 runtime
#   - beta       Flathub beta branch with 22.08 runtime.
#   - stable     Flathub main branch x86_64 with 20.08 runtime.
#
# This is a template used to create a complete manifest in the build
# directory. Doing so, it handles three tokens:
# @plugin_name
#    is replaced with PKG_NAME from Plugin.cmake
# @app_id
#    is  replaced with the last part of the initial id: line
#    after substituting possible @plugin_name.
# @include  <file>
#    Replaces the line with @include with the contents of <file>,
#
# See cmake/ConfigureManifest.cmake for details.
#
id: org.opencpn.OpenCPN.Plugin.@plugin_name

runtime: org.opencpn.OpenCPN
runtime-version: beta   # FIXME(alec) Revert to stable when updated to 22.08
sdk: org.freedesktop.Sdk//22.08
build-extension: true
separate-locales: false
appstream-compose: false
finish-args:
    - --socket=x11
    - --socket=pulseaudio
    - --filesystem=home
    - --device=all

modules:
    - @include opencpn-libs/flatpak/glu.yaml

    - name: @app_id
      no-autogen: true
      buildsystem: cmake
      config-opts:
          - -DCMAKE_INSTALL_PREFIX=/app/extensions/@app_id
          - -DCMAKE_BUILD_TYPE:STRING=Release
          - -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
          - -DBUILD_TYPE:STRING=tarball
          - -Uplugin_target
      build-options:
          cxxflags: -DFLATPAK -O3
          cflags: -DFLATPAK -O3
          # The flatpak-builder default CFLAGS adds -g
      sources:
          - type: git
            url: ..
            branch: HEAD
