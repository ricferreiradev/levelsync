# Packaging and publishing

## Artifacts

The pipeline produces three Windows x64 distribution formats:

| File | Contents |
| --- | --- |
| `LevelSync-VERSION-Standalone-Windows-x64.zip` | Standalone application and legal documents |
| `LevelSync-VERSION-VST3-Windows-x64.zip` | Complete `LevelSync.vst3` bundle and legal documents |
| `LevelSync-VERSION-VST3-Setup-Windows-x64.exe` | Administrative VST3 installer |

Inno Setup generates `.exe` installers, not Windows Installer `.msi` packages.

## ZIP packages

Build the Release targets first, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/CreateReleasePackages.ps1 `
  -BuildDirectory ./build `
  -OutputDirectory ./release `
  -Version 0.5.1
```

The script fails if binaries or required legal documents are missing. The VST3 remains a bundle containing `Contents/Resources` and `Contents/x86_64-win` inside `LevelSync.vst3`.

## Inno Setup installer

Install Inno Setup, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File Packaging/BuildInnoInstaller.ps1 `
  -BuildDirectory ./build `
  -OutputDirectory ./release `
  -Version 0.5.1 `
  -RepositoryUrl "https://github.com/ricferreiradev/levelsync"
```

The installer:

- requires Windows x64 and administrator privileges;
- displays the AGPL and third-party notices;
- installs the bundle to `{Common Files}\VST3\LevelSync.vst3`;
- adds a Windows uninstall entry;
- removes installed files when uninstalled.

The installer is not signed automatically. For professional public distribution, configure code signing in a protected environment and never store the private key or password in the repository.

## GitHub Release

`.github/workflows/release.yml` runs for `vX.Y.Z` tags, validates the CMake version, builds and tests the project, creates all three packages, generates SHA-256 hashes, and publishes the release through the GitHub CLI.

GitHub automatically publishes the tagged source. Because the build uses JUCE under AGPLv3, the release must also identify and keep the corresponding JUCE 9.0.5 source available.
