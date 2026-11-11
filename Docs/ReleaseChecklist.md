# Release checklist

## Code and tests

- [ ] Update the version in `CMakeLists.txt` and `CHANGELOG.md`.
- [ ] Configure with JUCE 9.0.5 and Release x64.
- [ ] Build `LevelSync_VST3` and `LevelSync_Standalone`.
- [ ] Run CTest with zero failures.
- [ ] Open Standalone and inspect the interface at 100% and high DPI scaling.
- [ ] Validate VST3 in at least one DAW with a real sidechain.
- [ ] Test 44.1/48/96 kHz and the buffer sizes used for manual validation.
- [ ] Record, edit, and replay `Rider Gain` automation in at least one DAW; confirm that Write and Read do not fight each other.

## AGPL distribution

- [ ] Include `LICENSE`, `NOTICE`, and `THIRD_PARTY_NOTICES.md`.
- [ ] Publish the exact source corresponding to the binary.
- [ ] Identify the exact JUCE version and provide access to its corresponding source.
- [ ] Include CMake scripts and enough information to reproduce the build.
- [ ] Exclude unauthorized proprietary files, DAW projects, and audio.
- [ ] Publish artifact checksums.
- [ ] Confirm ZIP packages contain legal files and the correct binary structure.
- [ ] Build the Inno Setup installer and review its log without performing a destructive installation.
- [ ] Digitally sign the installer if a release signing certificate is configured.

## GitHub

- [ ] Create a signed `vX.Y.Z` tag.
- [ ] Attach the complete compressed `LevelSync.vst3` bundle.
- [ ] Attach Standalone separately.
- [ ] Attach the Inno Setup VST3 installer.
- [ ] Attach `SHA256SUMS.txt` and verify published hashes.
- [ ] Copy relevant changelog notes into the release.
- [ ] State clearly that automated tests are not validation in every DAW.
