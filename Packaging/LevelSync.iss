#ifndef MyAppVersion
  #define MyAppVersion "0.5.0"
#endif
#ifndef BuildRoot
  #define BuildRoot "..\build"
#endif
#ifndef ProjectRoot
  #define ProjectRoot ".."
#endif
#ifndef OutputDir
  #define OutputDir "..\release"
#endif
#ifndef RepositoryUrl
  #define RepositoryUrl "https://github.com/"
#endif

#define VstBundle BuildRoot + "\LevelSync_artefacts\Release\VST3\LevelSync.vst3"

[Setup]
AppId={{A7B35145-849A-4EE7-91DD-1D86AC412E6B}
AppName=LevelSync VST3
AppVersion={#MyAppVersion}
AppVerName=LevelSync VST3 {#MyAppVersion}
AppPublisher=LevelSync contributors
AppPublisherURL={#RepositoryUrl}
AppSupportURL={#RepositoryUrl}
AppUpdatesURL={#RepositoryUrl}
DefaultDirName={autopf}\LevelSync
DefaultGroupName=LevelSync
DisableProgramGroupPage=yes
LicenseFile={#ProjectRoot}\LICENSE
InfoBeforeFile={#ProjectRoot}\THIRD_PARTY_NOTICES.md
OutputDir={#OutputDir}
OutputBaseFilename=LevelSync-{#MyAppVersion}-VST3-Setup-Windows-x64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.0
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
UninstallDisplayName=LevelSync VST3 {#MyAppVersion}
VersionInfoVersion={#MyAppVersion}
VersionInfoProductName=LevelSync VST3
VersionInfoProductVersion={#MyAppVersion}
VersionInfoCompany=LevelSync contributors
VersionInfoDescription=LevelSync VST3 installer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Files]
Source: "{#VstBundle}\*"; DestDir: "{commoncf64}\VST3\LevelSync.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ProjectRoot}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProjectRoot}\NOTICE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProjectRoot}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProjectRoot}\README.md"; DestDir: "{app}"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\LevelSync.vst3"
