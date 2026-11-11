# DAW automation: Write and Read modes

LevelSync 0.5.2 exposes an automatable parameter named `Rider Gain`. The parameter range is -12 to +12 dB and its default is 0 dB.

## Modes

### Automation Off

The automatic controller calculates and applies gain internally. No Rider Gain points are published to the host. This preserves the behavior of previous LevelSync versions.

### Automation Write

LevelSync continues to calculate and apply gain internally while publishing the applied correction to `Rider Gain` through the host automation API.

- Updates are limited to the existing 25 Hz control/telemetry rate.
- A new point is sent only when gain differs by at least 0.05 dB from the last published value.
- Host notifications and automation gestures run on the message thread, never inside the real-time audio callback.
- The `Rider Gain` value written by the host is not fed back into output gain while Write is active, preventing a double-gain loop.

To record:

1. Expose the plugin's `Rider Gain` automation lane in the DAW.
2. Put that lane or the plugin automation mode into Write, Touch, or Latch, according to the host.
3. Select **Automation Write** in the LevelSync header.
4. Play the desired section from start to finish.
5. Stop playback and switch the DAW lane to Read.
6. Select **Automation Read** in LevelSync.

## Automation Read

In Read mode, the automatic balance engine continues measuring audio for meters and diagnostics, but its calculated gain is not used for output. The vocal output follows only the host-controlled `Rider Gain` value.

Each block-level parameter change is ramped over 20 ms to avoid clicks. The host curve can be edited, copied, simplified, or deleted with normal DAW automation tools.

## Important behavior

- Automate `Rider Gain`, not the track volume fader, when you want the curve to remain associated with LevelSync.
- Do not leave the DAW lane in Write after recording; it may overwrite manual edits.
- Use Automation Read for playback of an edited curve. Leaving LevelSync in Write would publish new values again.
- Plugin parameter automation is read once per processing block and is not sample-accurate.
- Standalone exposes the controls but has no DAW host to record automation.
- Host behavior varies. Recording initiated by a plugin must be validated in each DAW before production use.

## Safety and fallback

If a host does not record plugin-originated parameter changes, keep Automation Off and use LevelSync's internal processing, or automate `Rider Gain` manually in Read mode. The existing balance algorithm remains unchanged in Off mode.
