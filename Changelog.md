3.0.3 Feb 17, 2022
* No changes from 3.0.3-beta1

3.0.3-beta1 Feb 02, 2022
* Win build fixes (#413)
* ubuntu-armhf: Don't forget 'apt update' (#409)
* update-templates: Add missing private data if required (#405)
* win_deps.bat: Make PATH handling more consistent (#407)
* Add new Debian armhf builds, misc (#404)
* Build ubuntu armhf targets (#402)
* Drop xenial builds (#401)
* Fixes from o-charts_pi (#398)      

3.0.2 Jan 11, 2022
* Checklist completed (#392)

3.0.2-beta1 Jan 9, 2022
* update-templates: update and polish

3.0.1 Jan 5, 2022
* update-templates: Don't touch libs (#378)

3.0.1-beta4  Jan 4, 2022
* Major updates to update-templates to handle pristine plugins.
* Use a SVG main icon (#354).

3.0.1-beta3  Jan 3, 2022
* Broken release based on wrong branch

3.0.1-beta2  Jan 2, 2022
* Broken release based on wrong branch

3.0.1-beta1 Dec 22, 2021
* Actually use the tag as effective version (#349)
* update-templates: Don't forget to update buildwin (#133).
* update-templates: Remove unused files (#336).
* update-templates: Only import sd\* tags to client plugin
* ci: flatpak: Drop transition 18.08 builds
* ci: flatpak: Build aarch 64 from master branch after 5.6.0 release.

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
