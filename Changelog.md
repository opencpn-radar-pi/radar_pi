3.3.0 Oct 10, 2023
* Fix wrong upload directory for bookworm plugins (#492).
* Fix handling of wxWidgets 3.2 build deps in update-templates (#490).
* Use urllib3 < 2.0.0 (#520).
* Update opencpn-libs,
    - Provide a compatility target ocpn::api on api-18.
    - Fix bug in nmea0183 lib, see
      https://github.com/leamas/opencpn-libs/issues/15
    - Add new marnav library.
    - Add new N2k library required to receive and parse n2k messages.
* Fix expected hash in AndroidLibs.cmake to align with master.zip.
* Fix move/resize for Android builds

3.2.1  Dec 18, 2022
* New release afrter some release problems of the 3.2.0 tag

3.2.0  Dec 18, 2022
* Drop all ubuntu builds according to OpenCPN#2502.
* Update builds according to OpenCPN#2797:
  + Update windows build to use wxwidgets 3.2, new ABI msvc-wx32
  + Update macos to build for 10.10 on 13.4.1 using wxwidgets3.2,
    new ABI darwin-wx32
  + Update flatpak to runtime 22.08 and wxwidgets 3.2
  + New debian wxwidgets 3.2 builds for bookworm and bullseye, new
    ABI debian-wx32.
* Updates to opencpn-libs, notably a new api-18 exposing new
  communication interfaces.
* Windows build refactored with a ci/appveyor.bat script which can
  be run in local builds.
* Remove still more wxWidgets 3.1.5 builds (#444)
* update-templates: Remove .drone.yml (#443)
* Drop the __OCPN__ANDROID__ symbol, use __ANDROID__ instead.
* Fix bad bug when copying results in Flatpak resulting in empty
  Flatpak plugin tarballs (#453, 454).
* Drop the buggy and superfluous symbols ANDROID and ARMHF; use
  __ANDROID__, __arm__ and __aarch64__ instead  (#451).
* AndroidLibs: Update downloaded library hash (#446).
* update-templates: Don't access non-existing Flatpak/wx31.patch
* opencpn-libs: fixes to mute some warnings in plugin\_gl
* Handle updated ubuntu repository keys (#436).
* Remove too early ubuntu-wx315 which fails in validation (#437).

3.1.0 Mar 18, 2022

3.1.0-beta2 Mar 16, 2022
* Install this Changelog in client plugins (#430).
* Clean up the flatpak-wx315 build (#429).

3.1.0-beta1 Feb 20, 2022
* The drone.io builder is not longer used (#217).
* The Xenial builds are removed (#399)
* Raspbian armhf builds are replaced with Ubuntu (#380).
* The Flatpak runtime 18.08 compatibility builds are removed.
* New Debian 10/11 builds are added (#381).
* Add a git submodule with libraries. This affects how the plugin
  is cloned and initiated, see INSTALL.md (#338).
* Drop the special treatment of libjsoncpp. Plugins which depends
  on this library must include and use it explicitly in the same way
  as other plugins
* Clean up the handling of SVG cpp symbols and includes (#354).
* Use the script win\_deps.bat in both CI and "manual" builds.
* Actually use the tag as effective version (#349)
* update-templates: Don't forget to update buildwin (#133).
* update-templates: Remove unused files (#336).
* update-templates: Fixes for installing templates first time.
* Streamline Flatpak CI builds using released OpenCPN 5.6.0


3.0.0 Nov 27, 2021

* Update shipdriver main code to use correct config file section
  [PlugIns] instead of [Settings].
* Fix broken build due to missing, unused libjsoncpp lib (#324).
* Documentation updates and bugfixes.

3.0.0-beta2   Nov 21, 2021

* Code added to ShipDrivergui_impl.cpp/.h to allow mouse drag
  events for Android.
* libs/plugingl files work with Android builds.
* Added support for local builds including Cloudsmith uploads and
  metadata git push.
* Big rewrite of update-templates to allow a simplified history.
* Numerous bugfixes.

3.0.0-beta1   Nov 1, 2021

* Copyright and licensing has been clarified.
* Added Android builds
* Cleaned up OpenGL headers, always using platform's headers when
  available
* Refactored CMakeLists.txt, moving all plugin-dependent parts to
  Plugin.cmake making CMakeLists.txt a generic template.
* Cleaned up caching to use directories named 'cache'.
* MacOS deployment target updated to 10.9
* Flatpak runtime updated to 20.08. Transitional support for old
  18.08 is available, will be removed in next release.
* git-push key files default location is moved from ci/ to build-deps/.
* A new script update-templates supporting templates update is added.
