# Offline metrics methodology

`LevelSyncOfflineMetrics` processes deterministic synthetic signals at 48 kHz. A 997 Hz sine vocal remains active while the instrumental undergoes a sustained level change at eight seconds. The engine receives sample-by-sample audio and emits an analysis frame every 40 ms, matching the UI's 25 Hz telemetry.

Metrics use only frames with valid vocal measurement, valid sidechain, and vocal activity. Error is defined as:

```text
balanceErrorDb = targetBalanceDb - ((voicePreDb + appliedGainDb) - musicDb)
```

- **Mean Absolute Error:** mean `abs(balanceErrorDb)` in the stable region beginning six seconds after the change.
- **Maximum/Minimum Gain:** extrema of applied gain in valid post-change frames.
- **Gain Movement:** sum of absolute differences between consecutive gain values.
- **Peak Gain Speed:** largest inter-frame difference divided by 0.04 s.
- **Convergence Time:** after the change moves error outside ±0.5 dB, time until five consecutive frames return to that range.

The test prints measured values and verifies only objective properties: finite values, enough frames, nonzero movement, observed convergence, and compliance with ±6 dB limits. Synthetic signals do not represent the full variation of speech and music; these numbers are regression references, not real-material performance claims.
