# Validation report - LevelSync 0.4.0

Date: 2026-11-03. Validated project: `C:\projetos\LevelSync`. Local JUCE: 9.0.5 at `C:\Dev\JUCE`. Compiler: MSVC 19.52.37025.0, Release x64.

## Passed

- CMake configuration with JUCE through `add_subdirectory()`.
- Builds for `LevelSync_VST3`, `LevelSyncDSPTests`, `LevelSyncProcessorTests`, and `LevelSyncOfflineMetrics`.
- VST3 manifest generated for LevelSync 0.4.0 / VST SDK 3.8.0.
- CTest: 3 of 3 executables passed; 27 named scenarios, zero failures, 12.47 s.
- DSP: 44.1/48/96 kHz, varied blocks, NaN/Inf, silence, missing/disconnected sidechain, Learn, limits, hold, neutral return, long execution, and no prepared-path allocation.
- SPSC queue: fixed capacity, FIFO, non-blocking overflow, and no allocation during operations.
- Processor: mono/stereo layouts, read-only sidechain, versioned state, automation, Enable/bypass, closed GUI, empty buffer, and repeated editor open/resize/close.
- Interface rendered at 1180×820 and visually inspected; no controls were clipped in the standard capture used at that time.

## Measured synthetic metrics

The scenario and definitions are documented in `PerformanceMethodology.md`.

| Metric | Result |
| --- | ---: |
| Stable Mean Absolute Error | 0.0105566 dB |
| Maximum Gain | +6.0000 dB |
| Minimum Gain | -1.9485 dB |
| Cumulative Gain Movement | 7.9485 dB |
| Peak Gain Speed | 7.47678 dB/s |
| Convergence Time | 2.4 s |
| Valid stable frames | 150 |

These values belong to the deterministic test signal and serve as a regression reference. They do not measure general performance on real speech and music.

## Failed

No automated test failed in the final run.

## Not executed in that environment

- Discovery and loading by a VST3-compatible DAW scanner.
- Real sidechain exposure and routing in a DAW.
- Host-recorded and replayed automation.
- Comparison between host playback and offline rendering.
- State restoration after closing and reopening a real DAW session.
- Comparison with Waves Vocal Rider on musical material.
- Listening evaluation, intersample clipping, and CPU use in a real session.

## Known limitations

- Loudness is a smoothed K-weighted estimate, not a certified normative LUFS meter.
- Energy-based vocal detection can respond to bleed, noise, and other sounds.
- There is no limiter or true-peak protection; positive gain requires downstream headroom.
- Automation is read once per block, without per-sample accuracy.
- Version 0.5.2 automation writing requires separate real-host validation; the historical results above predate that feature.
- Visual history exists while the editor is open. If the GUI does not consume a full queue, new frames are discarded without affecting DSP.
- Telemetry and metrics are observational and do not participate in audio control.
