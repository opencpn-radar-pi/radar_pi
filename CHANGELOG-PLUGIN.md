# Changelog

All notable changes to this project will be documented in this file as of v5.5.0.

Note that 'Changelog.md' contains changes to the SD template, unfortunately this means we
cannot use this file yet.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

Sections can be: Added Changed Deprecated Removed Fixed Security.

## [Unreleased]

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
