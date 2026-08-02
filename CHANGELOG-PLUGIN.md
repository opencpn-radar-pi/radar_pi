# Changelog

All notable changes to this project will be documented in this file as of v5.5.0.

Note that 'Changelog.md' contains changes to the SD template, unfortunately this means we
cannot use this file yet.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

Sections can be: Added Changed Deprecated Removed Fixed Security.

## [Unreleased]

## [5.7.2]

### Fixed

- Windows (msvc) builds silently failed to upload to cloudsmith and never reached the plugin catalog

## [5.7.1]

### Fixed

- Garmin xHD in dual range mode: the spokes of the second (B) virtual radar are no longer mixed into the radar image
- Navico and Raymarine radar locator threads failed to start on some Linux systems (64 KB stack too small for glibc's static TLS); use a 1 MB stack like the receive threads (#298)

## [5.7.0]

### Added

- Multiple radar overlays on a each canvas.
The overlay with the shortest range, will be in the center. The second overlay will start at the range where the first overlay ends. This way two (or more) radar are integrated into a single radar image with a higher resolution than the individual radars. This feature is particularly interesting for owners of radars that contain more logical radars such as the Navico Halo range. But the feature can b used also with physically separate radars.

- Improved ARPA
Target tracking has been improved. Additional filters remove many spurious targets. In the target recognition process a linear filter is used besides the Kalman filter. This enables the recognition of faster moving targets. Even low flying airplanes can be picked up. Earlier the number of targets was limited to 100. Now there is no limit anymore (besides memory restrictions). Target tracking performance is better also. The system will easily follow 800 simultaneous targets.
In previous releases, ARPA targets were maintained per radar. Each logical radar had its own set of targets. A target would get lost when it moved outside the range of its radar and might exist with multiple radars. In the new release targets and radars are disconnected, a target will be handled by the radar with the highest resolution for the target and move smoothly to the region of another radar.

- Adjustable transparency of the guardzone shading
The shading of the guardzone often tended to be too heavy or too light. Now the user can adjust the transparency of the shading in Preferences in the "Guard Zone" box.

## [5.6.1]

### Fixed

- #297: Do not call any OpenGL code before OpenCPN initialisation is complete
- #295: Icon does not toggle in toolbar when status changed
- #239: Menu buttons text are disturbed when O's Dark/dusk color scheme have been used
- #129: Radar windows don't follow O's color scheme, dusk , night
- Bump opencpn-libs for running against O 5.12
- Raymarine documentation
- Allow override of HeadingTimeout in config file; normal is 5s.

## [5.5.5]

### Fixed
 
- Fix colors not showing in Preferences dialog on Linux
- Allow setting of loglevel, fixed heading and position in Preferences dialog
- Fixed a crash in weather mode
- Small Raymarine fixes
- Small Navico fixes
- Do not overlay radar on chart without heading source
- Updated translations

## [5.5.4]

### Fixed

- Compatibility with OpenCPN 5.10.0 (macOS universal build)
- #248: Stay Alive message for Halo radars not in line with latest Simrad MFD (#248)
- #244: OCPN crash when AIS targets on PPI window are shown (#244)
- #243: PPI: AIS target relative to center, not to ownship (#243)
- Show radar control from menu bar

## [5.5.0]

This release is mainly focused on compatibility with OpenCPN 5.8.0 which uses a new UI library
(wxWidgets v3.2 instead of v3.0) requiring re-compilation. Functionally the only major changes
are full HALO compatibility.

### Fixed

- #202: Hide socket errors on info window for radars once they are located
- #204: Add loglevel 64 for reports
- No ARPA without heading
- Various compilation/coding improvements
- Update to SD templates 3.2.3
- Update to wxWidgets 3.2 on all three major platforms; 3.0 still supported on some variants

### Added

- #201: HALO Bird mode/threshold improvements
- #179: HALO accent light support
- #190: Show RPM on info window
- #206: HALO sea clutter and sea state support
- #207: HALO target expansion
- #208: HALO fast scan and other settings
- #217: HALO no-transmit zones
- HALO: Add support for HALO 4 no-transmit zones
- Navico: add doppler speed threshold


## [5.3.4]

This release was made before this changelog was introduced.
