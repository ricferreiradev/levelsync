# Changelog

All notable changes to this project are documented in this file. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project intends to follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- English user manual while retaining the Brazilian Portuguese edition.

### Changed

- GitHub-facing documentation is now written in English.

## [0.5.1] - 2026-11-11

### Changed

- Compacted the interface while preserving every control.
- Reorganized monitoring, Learn Balance, and control panels for better vertical space usage.

### Fixed

- Corrected spacing and clipping in the lower Music Silence Protection controls.

### Added

- GitHub documentation and project governance structure.
- AGPL-3.0-or-later licensing and third-party notices.
- Standalone and VST3 ZIP packaging.
- Windows VST3 installer built with Inno Setup.
- Automated release publishing from version tags.

## [0.5.0] - 2026-11-11

### Added

- Perceptual Balance with 24-band ERB spectral analysis.
- Configurable Music Silence Protection.
- Standalone format in addition to VST3.
- Professional interface, diagnostics, and telemetry.
- Learn Balance with a persistent spectral reference.

### Fixed

- Vocal return to unity during prolonged instrumental stops.
- Diagnostics overlay drawing order.

## [0.1.0] - 2026-11-03

### Added

- Initial JUCE/CMake VST3 project with passthrough processing.
