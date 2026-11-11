# Contributing to LevelSync

Thank you for your interest in improving LevelSync.

## Before you begin

- Read the [README](README.md), [license](LICENSE), and [third-party notices](THIRD_PARTY_NOTICES.md).
- By submitting a contribution, you agree that it may be distributed under `AGPL-3.0-or-later`.
- Do not submit code, presets, audio, images, or other material unless you may redistribute it under compatible terms.
- Changes that depend on a commercial JUCE license must not be mixed into this repository's AGPL build path.

## Recommended workflow

1. Open an issue for bugs or substantial changes.
2. Create a branch from the latest main branch.
3. Keep changes small and focused.
4. Add tests for changes to DSP, state, parameters, or layouts.
5. Build VST3 and Standalone in Release x64.
6. Run the complete CTest suite.
7. Update documentation and `CHANGELOG.md` when appropriate.

## Local verification

```powershell
cmake -S . -B build -A x64 -DJUCE_DIR="C:/Dev/JUCE" -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

## Real-time DSP rules

- Do not allocate memory or use mutexes on the audio thread.
- Do not access the GUI, files, or network from `processBlock()`.
- Keep the sidechain read-only.
- Sanitize NaN and infinity without propagating invalid values.
- Document scales clearly: dBFS, K-weighted estimates, and dB gain are not interchangeable.
- Test 44.1, 48, and 96 kHz with small, irregular, and large blocks.

## Pull requests

Explain the problem, solution, tests performed, and known risks. Do not claim DAW validation unless it was actually performed.
