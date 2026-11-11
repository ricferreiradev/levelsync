# LevelSync

LevelSync is a Windows vocal-balance processor that smoothly adjusts vocal gain against a stereo instrumental sidechain. The project builds **VST3** and **Standalone** formats with C++17, JUCE, and CMake.

## Download

**[Download the latest LevelSync release](https://github.com/ricferreiradev/levelsync/releases/latest)**

The downloads page provides the Standalone application, a VST3 ZIP package, a Windows installer, and SHA-256 checksums. See [all releases](https://github.com/ricferreiradev/levelsync/releases) for previous versions.

> Project status: beta. Automated tests cover the DSP, processor, and interface, but they do not replace listening tests in a real DAW.

## Documentation

- [User Manual - English](output/pdf/LevelSync-User-Manual-EN.pdf)
- [Manual do Usuário - Português](output/pdf/LevelSync-Manual-do-Usuario-PT-BR.pdf)
- [Writing and reading DAW automation](Docs/Automation.md)
- [Automação na DAW - Português](Docs/pt-BR/Automation.md)
- [DAW installation and validation](Docs/DAWInstallationAndValidation.md)
- [Music Silence Protection](Docs/MusicSilenceProtection.md)
- [Performance methodology](Docs/PerformanceMethodology.md)
- [Comparison protocol](Docs/ComparisonProtocol.md)
- [Validation report](Docs/ValidationReport.md)
- [Release checklist](Docs/ReleaseChecklist.md)
- [Packaging and publishing](Packaging/README.md)

## Features

- Mono or stereo main input for vocals.
- Dedicated stereo instrumental sidechain.
- Automatic control with attack, release, deadband, strength, and gain limits.
- Learn Balance with a robust, persistent reference.
- Optional Perceptual Balance with 24-band ERB spectral analysis.
- Music Silence Protection that returns vocal gain to 0 dB during musical stops.
- Resizable interface with history, meters, and diagnostics.
- Real-time processing without mutexes or allocations after `prepareToPlay()`.
- VST3 and Standalone builds using the same processor and editor.
- Write/Read automation workflow through an editable `Rider Gain` DAW parameter.

## Requirements

- Windows 10 or later, x64.
- CMake 3.22 or later.
- Visual Studio 2022 or later with Desktop development with C++ and a Windows SDK.
- JUCE 9.0.5. By default, the project looks for JUCE at `C:\Dev\JUCE`.

JUCE is not vendored in this repository. Use the documented version for reproducible builds and review the obligations under [Licensing](#licensing).

## Build

Open PowerShell in the project root:

```powershell
cmake -S . -B build -A x64 `
  -DJUCE_DIR="C:/Dev/JUCE" `
  -DBUILD_TESTING=ON

cmake --build build --config Release `
  --target LevelSync_VST3 LevelSync_Standalone `
           LevelSyncDSPTests LevelSyncProcessorTests LevelSyncOfflineMetrics `
  --parallel 4

ctest --test-dir build -C Release --output-on-failure
```

Generated artifacts:

- VST3: `build/LevelSync_artefacts/Release/VST3/LevelSync.vst3`
- Standalone: `build/LevelSync_artefacts/Release/Standalone/LevelSync.exe`

## Release packages

The release workflow runs for tags matching `vMAJOR.MINOR.PATCH`. The tag version must match `CMakeLists.txt`.

```powershell
git tag -a v0.5.2 -m "LevelSync 0.5.2"
git push origin v0.5.2
```

The release contains:

- `LevelSync-0.5.2-Standalone-Windows-x64.zip`
- `LevelSync-0.5.2-VST3-Windows-x64.zip`
- `LevelSync-0.5.2-VST3-Setup-Windows-x64.exe`
- `SHA256SUMS.txt`

The ZIP packages include `LICENSE`, `NOTICE`, `README.md`, and `THIRD_PARTY_NOTICES.md`. The VST3 ZIP preserves the complete `LevelSync.vst3` bundle; do not extract only its internal binary.

To build packages locally:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/CreateReleasePackages.ps1 `
  -BuildDirectory ./build -OutputDirectory ./release -Version 0.5.2

powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/BuildInnoInstaller.ps1 `
  -BuildDirectory ./build -OutputDirectory ./release -Version 0.5.2 `
  -RepositoryUrl "https://github.com/ricferreiradev/levelsync"
```

The installer requires administrator privileges because it writes to the shared Windows VST3 directory. Local and CI builds are unsigned by default and may trigger a Microsoft Defender SmartScreen warning.

## DAW installation

1. Close existing LevelSync instances.
2. Copy the complete `LevelSync.vst3` folder to a VST3 directory scanned by your DAW. The standard Windows location is `C:\Program Files\Common Files\VST3`; if the DAW uses another configured directory, use it or add the standard path to its scanner.
3. Rescan VST3 plugins in the DAW.
4. Insert LevelSync on the vocal track.
5. Route the instrumental track or bus to the plugin's stereo sidechain.

The sidechain is read-only and is never mixed into the vocal output. See the [complete installation and validation guide](Docs/DAWInstallationAndValidation.md).

## Basic use

1. Play vocals and instrumental audio together for a few seconds.
2. Set `Vocal Threshold` high enough to reject the noise floor while still detecting useful vocals.
3. Set `Target Balance`, gain limits, attack, and release.
4. Optionally establish a suitable manual balance and run `Learn Balance`.
5. Keep Music Silence Protection enabled to preserve the original vocal level during instrumental stops.

## DAW automation

The header automation selector provides three modes:

- **Automation Off:** LevelSync calculates and applies gain internally, as in earlier versions.
- **Automation Write:** LevelSync applies its correction and publishes it to the host's automatable `Rider Gain` parameter at up to 25 updates per second. Put the `Rider Gain` lane in Write, Touch, or Latch mode in the DAW, play the song, then switch the host to Read.
- **Automation Read:** LevelSync stops using the automatic controller for output gain and follows the DAW's `Rider Gain` curve with a 20 ms click-free ramp.

The host owns the recorded automation, so points can be edited, simplified, copied, or deleted outside the plugin. Host support varies; see the [automation guide](Docs/Automation.md).

Displayed loudness values are 400 ms K-weighted estimates. They are not normative integrated or short-term LUFS measurements.

## Architecture

```text
Vocal input --> Loudness/VAD --> Balance Controller --> Gain Smoother --> Output
                     ^                 ^
Instrumental sidechain --------> Silence + Spectral Analysis
```

- `Source/DSP`: UI-independent DSP components.
- `Source/UI`: theme, meters, and graphs.
- `Source/PluginProcessor.*`: buses, parameters, state, and telemetry bridge.
- `Source/PluginEditor.*`: interface shared by VST3 and Standalone.
- `Tests`: DSP, processor integration, persistence, and rendering tests.

## Contributing and security

- [Contributing guide](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)
- [Changelog](CHANGELOG.md)

## Known limitations

- Only Windows x64 is currently configured.
- The plugin does not perform source separation; noise and bleed can affect detectors.
- Positive gain can exceed 0 dBFS. Preserve headroom in the processing chain.
- Host automation is read once per block and smoothed; it is not sample-accurate.
- Standalone has no separate instrumental sidechain, so automatic control remains neutral.
- Real compatibility must be validated with the user's DAW version and audio drivers.

## Licensing

Original LevelSync code is distributed under the **GNU Affero General Public License v3.0 or later** (`AGPL-3.0-or-later`). See [LICENSE](LICENSE).

The project uses JUCE 9.0.5, offered under AGPLv3 or a commercial license. This repository uses the AGPL option. Proprietary distribution, or distribution that does not comply with the AGPL, requires an appropriate JUCE commercial license and a review of the rest of the project's licensing.

Binary distributions must provide the corresponding source code, build scripts, license, and third-party notices through an appropriate equivalent channel. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Copyright © 2026 LevelSync contributors.

This software is provided without warranty. This summary is not a substitute for the license texts or legal advice.
