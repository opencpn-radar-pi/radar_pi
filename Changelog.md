3.1.4-beta3 June 29, 2022
* More Flatpak FTBFS fixes for manifest comment lines.

3.1.4-beta2 June 29, 2022
* Fix a bug when configuring Flatpak manifests causing FTBFS errors.
* Flatpak manifest: Handle @include in comments when configuring.

3.1.4-beta1 June 13, 2022
* Fix bad bugs in Flatpak builds resulting in empty tarball (#453, 454).
* Drop focal-wx31 build.
* Add a debian-arm64 build (bullseye/11 only).
* Add @include, @app\_id and @plugin\_name support in flatpak .yaml mainfest.
* opencpn-libs: Several new libraries, wxcurl update.
* opencpn-libs: flatpak yaml snippets added for use via @include.
* Don't restore drone.yml in update-templates (#443).

3.1.3 May 11, 2022
* Add a build for new Ubuntu LTS Jammy (22.04)
* Fix correct target ubuntu (not ubuntu-gtk3) for 22.04+ versions.
* opencpn-libs: plugingl: Incorporate changes from o-charts\_pi affecting
  Android builds.

3.1.2 April 29, 2022
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
