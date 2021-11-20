3.0.0-beta2   TBD

* Added support for device context for Android builds (#318)
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
