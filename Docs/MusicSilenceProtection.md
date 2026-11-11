# Music Silence Protection

Music Silence Protection prevents a previous vocal correction from remaining applied during an instrumental stop.

## Measurement

The detector uses unweighted energy from the stereo sidechain, separate from the K-weighted meter used by the balance controller. Its scale is:

`dBFS = 10 * log10((L² + R²) / 2)`

The dBFS threshold is never compared directly with a K-weighted value or treated as LUFS. A 5 ms energy envelope follows transitions. The state changes to silent only after `Silence Hold`. To return to active, the signal must remain 3 dB above the threshold for `Silence Release`.

## Behavior

- During the hold period, the last valid target is retained and no new target is calculated from decaying loudness.
- After silence is confirmed, the target becomes 0 dB and reaches 99.9% of unity within `Return to Unity`.
- The return does not depend on vocal presence and uses per-sample smoothing.
- Learn Balance pauses while the instrumental is invalid without replacing the previous reference.
- Perceptual correction is disabled and smoothly discharged while protection is active.
- A disconnected sidechain is handled separately and is not displayed as active protection.

Defaults: -55 dBFS threshold, 200 ms hold, 400 ms release, and 300 ms return.
