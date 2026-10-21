; Inno Setup script: installs BigAppleSam.scr into System32 so it appears in Screen Saver Settings,
; then (optionally) opens Screen Saver Settings with Big Apple Sam selected.
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{26CA548E-5B30-4169-B30E-14B6DACF6851}
AppName=Big Apple Sam
AppVersion={#AppVersion}
AppVerName=Big Apple Sam screen saver
AppPublisher=Sam Ling
AppPublisherURL=https://samuelling.github.io/Big-Apple-Sam/
DefaultDirName={autopf}\Big Apple Sam
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=.
OutputBaseFilename=Big-Apple-Sam-Windows-Setup
SetupIconFile=app.ico
UninstallDisplayIcon={sys}\BigAppleSam.scr
UninstallDisplayName=Big Apple Sam screen saver
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[Files]
Source: "BigAppleSam.scr"; DestDir: "{sys}"; Flags: ignoreversion

[Run]
Filename: "{sys}\rundll32.exe"; Parameters: "desk.cpl,InstallScreenSaver ""{sys}\BigAppleSam.scr"""; \
  Description: "Open Screen Saver Settings with Big Apple Sam selected"; Flags: postinstall nowait runasoriginaluser

[UninstallDelete]
Type: filesandordirs; Name: "{localappdata}\Big Apple Sam\site"
