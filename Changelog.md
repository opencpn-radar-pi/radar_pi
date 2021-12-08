3.1.0 TBD
* Drop the special treatment of libjsoncpp. Plugins which depends
  depends in this library must include and use it explicitly.
* Use the script win\_deps.bat in both CI and "manual" builds.


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
