# Comparison protocol: LevelSync and Waves Vocal Rider

## Objective

Compare balance stability, balance error, gain movement, and transient behavior under reproducible conditions. The plugins may use different detectors, windows, and control strategies; this protocol measures outcomes and does not assume internal equivalence or superiority.

## DAW session preparation

1. Use a copy of the session and disable automatic normalization, other riders, compressors, and volume automation that would affect the test.
2. Choose one vocal track and one instrumental stem that remain unchanged between comparisons. Record sample rate, buffer, pan law, and fader levels.
3. Create two identical vocal routes, one for each plugin. Keep the same insert position and monitoring bus. Never play both simultaneously during listening evaluation.
4. Feed LevelSync from the same instrumental stem used for playback. Configure Vocal Rider according to its documentation for equivalent conditions. Record any routing difference that cannot be eliminated.
5. Match initial gain with both plugins neutral. Do not compensate later with different fader settings.

## Test regions

Mark regions containing:

- sustained vocal with stable instrumental;
- sustained ±6 dB instrumental change;
- short words and consonant transients;
- a short pause between phrases;
- prolonged vocal silence;
- bleed or background noise;
- quiet and loud vocal passages;
- sidechain connection and removal, for LevelSync only.

Use identical time boundaries for every render. Include at least five stable seconds after each change to separate transient and steady-state behavior.

## Plugin configuration

Run Learn Balance in LevelSync at the chosen manual balance and record Target Balance, Strength, Attack, Release, Deadband, Vocal Hold, Silence Return, Max Boost, and Max Cut. Record every Vocal Rider control. Match maximum movement ranges where possible without assuming similarly named controls have identical responses.

## Rendering

1. Render each route offline as float32 WAV with identical duration, sample rate, and start point.
2. Also render the unridden vocal and reference instrumental.
3. Blind the filenames for listening evaluation while keeping a private mapping table.
4. Confirm sample alignment. LevelSync reports zero latency; verify any compensation applied by the host to the other plugin.

## Metrics

Use the same detector and windows for every file. In regions with active vocals and valid instrumental audio, compare:

- balance error: target minus the post-processed vocal/instrumental difference;
- mean absolute error in stable regions;
- maximum and minimum gain;
- cumulative movement and peak gain speed;
- time to enter and remain within ±0.5 dB after a sustained change;
- overshoot and target crossings;
- noise level during vocal pauses.

Report transient and stable regions separately. A global average can hide pumping, slow reaction, or noise amplification.

## Listening evaluation

Perform a blind, output-level-matched comparison without clipping. Evaluate intelligibility, naturalness, dynamic preservation, pumping, consonant response, breathing, and between-phrase noise. Record observations by region before revealing plugin identities.

## Report

Include plugin versions, DAW name/version, hardware, sample rate, buffer, online/offline rendering, parameters, routing, analyzed files, and tools/scripts. Results apply only to the tested material and settings. Do not generalize a win without repeating the protocol on varied material.
