# DAW installation and validation

This guide applies to Windows x64 DAWs that load VST3 plugins and support sidechain routing. Commands vary by host; consult your DAW documentation.

## Installation

1. Close LevelSync instances loaded in the DAW.
2. Copy the complete `LevelSync.vst3` bundle to a VST3 folder scanned by the DAW. The standard Windows location is `C:\Program Files\Common Files\VST3`; if the DAW uses another configured directory, use it or add the standard path to its scanner. Administrator privileges may be required.
3. Open the DAW, rescan VST3 plugins, and confirm that LevelSync is detected.

## Routing

1. Insert LevelSync on the vocal track. The main input accepts mono or stereo, and the output preserves that layout.
2. Send the instrumental track or bus to LevelSync's stereo auxiliary input. Do not route the vocal output back into the sidechain.
3. Confirm valid `Instrumental Loudness` and active `Vocal Loudness`. The plugin reports zero samples of latency.

DAWs expose auxiliary buses differently. A host without VST3 sidechain support cannot provide full automatic balancing.

## Manual validation

1. Test Enable and host bypass at neutral gain, checking transparent passage and no clicks.
2. Run Learn Balance. Confirm a smooth return to 0 dB in Preparing, paused progress without valid vocal/sidechain, and completion after 5 valid seconds.
3. Apply a sustained instrumental change. Observe Current/Target Balance, theoretical/applied gain, history, and Max Boost/Cut indicators.
4. Test short and long stops, Music Silence Protection, Vocal Hold, and unity returns. Confirm that noise is not chased.
5. Automate each parameter, save, close, and reopen the project. Confirm parameters and learned reference.
6. Repeat at 44.1, 48, and 96 kHz with small and large buffers. Compare offline rendering with real-time playback.
7. Close the GUI during playback and rendering; audio must continue normally.

## Validation status

Builds, headless tests, the VST3 manifest, and interface rendering are validated automatically. Scanner discovery, real sidechain exposure, host automation, session restoration, and online/offline equivalence remain manual, DAW-specific tests.

VST3 compatibility does not guarantee identical behavior in every DAW. Include host name/version, bus layout, sample rate, buffer, and driver in reports.
