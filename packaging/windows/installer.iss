; OBS Polyemesis - Windows Installer Script (InnoSetup)
; Based on OBS Plugin Template best practices
;
; This script creates a Windows installer that installs the plugin
; into the OBS Studio plugins directory.
;
; Requirements:
;   - Inno Setup 6.x or later
;   - Built plugin files in build directory
;
; Build command:
;   iscc /DVERSION=0.9.10 packaging\windows\installer.iss
;
; Or with default version:
;   iscc packaging\windows\installer.iss

; Version can be overridden via command line: /DVERSION=x.y.z
#ifndef VERSION
  #define VERSION "0.9.12"
#endif

#define AppName "Restreamer Control for OBS"
#define AppPublisher "rainmanjam"
#define AppURL "https://github.com/rainmanjam/polyemesis"
#define AppExeName "obs-polyemesis.dll"

; Unique AppId - DO NOT CHANGE after first release (used for upgrade detection)
#define AppId "{{7A8F3D2E-9C1B-4E5A-B6D7-0F8E2C3A4B5D}"

[Setup]
; NOTE: AppId must remain constant across versions for upgrade detection
AppId={#AppId}
AppName={#AppName}
AppVersion={#VERSION}
AppVerName={#AppName} {#VERSION}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases

; Installation path - CRITICAL: Use {commonappdata} NOT {userappdata}
; {commonappdata} = C:\ProgramData (system-wide, recommended by OBS)
; {userappdata} = C:\Users\<user>\AppData\Roaming (per-user)
DefaultDirName={commonappdata}\obs-studio\plugins\obs-polyemesis

; Disable directory page since plugin MUST go to OBS plugins folder
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no

; Allow silent updates (no warning for existing directory)
DirExistsWarning=no
CreateUninstallRegKey=yes
UninstallDisplayIcon={app}\bin\64bit\{#AppExeName}
UninstallDisplayName={#AppName}

; Compression
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes

; Output configuration
OutputDir=..\..\build\installers
OutputBaseFilename=obs-polyemesis-{#VERSION}-windows-x64

; 64-bit only (OBS 28+ is 64-bit only)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; UI Settings
WizardStyle=modern
WizardSizePercent=100
ShowLanguageDialog=auto

; Version info embedded in installer
VersionInfoVersion={#VERSION}.0
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#VERSION}

; License
LicenseFile=..\..\LICENSE

; Privileges - user level is sufficient for ProgramData plugins folder
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "compact"; Description: "Plugin only (no locale files)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "plugin"; Description: "OBS Polyemesis Plugin (required)"; Types: full compact custom; Flags: fixed
Name: "locales"; Description: "Language Files (11 languages)"; Types: full

[Files]
; Plugin DLL - MUST be in bin\64bit\ subdirectory
Source: "..\..\build\Release\obs-polyemesis.dll"; DestDir: "{app}\bin\64bit"; Components: plugin; Flags: ignoreversion

; Debug symbols (optional, for crash reports)
Source: "..\..\build\Release\obs-polyemesis.pdb"; DestDir: "{app}\bin\64bit"; Components: plugin; Flags: ignoreversion skipifsourcedoesntexist

; Locale files
Source: "..\..\data\locale\*.ini"; DestDir: "{app}\data\locale"; Components: locales; Flags: ignoreversion

; License file
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"; Comment: "Uninstall {#AppName}"

[Registry]
; Add uninstall information
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Uninstall\{#AppId}_is1"; Flags: uninsdeletekey

[UninstallDelete]
Type: filesandordirs; Name: "{app}\bin"
Type: filesandordirs; Name: "{app}\data"
Type: files; Name: "{app}\LICENSE"

[Code]
var
  OBSPath: string;

function GetOBSInstallPath(): string;
var
  Path: string;
begin
  Result := '';

  // Try registry first (standard OBS installation)
  if RegQueryStringValue(HKLM, 'SOFTWARE\OBS Studio', '', Path) then
  begin
    Result := Path;
    Exit;
  end;

  if RegQueryStringValue(HKCU, 'SOFTWARE\OBS Studio', '', Path) then
  begin
    Result := Path;
    Exit;
  end;

  // Check common installation paths
  Path := ExpandConstant('{pf}\obs-studio');
  if FileExists(Path + '\bin\64bit\obs64.exe') then
  begin
    Result := Path;
    Exit;
  end;

  Path := ExpandConstant('{pf64}\obs-studio');
  if FileExists(Path + '\bin\64bit\obs64.exe') then
  begin
    Result := Path;
    Exit;
  end;

  Path := ExpandConstant('{localappdata}\Programs\obs-studio');
  if FileExists(Path + '\bin\64bit\obs64.exe') then
  begin
    Result := Path;
    Exit;
  end;

  // Check common ProgramData location (portable installs)
  Path := ExpandConstant('{commonpf}\obs-studio');
  if FileExists(Path + '\bin\64bit\obs64.exe') then
  begin
    Result := Path;
    Exit;
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  OBSPath := GetOBSInstallPath();

  if OBSPath = '' then
  begin
    if MsgBox('OBS Studio does not appear to be installed on this system.' + #13#10 + #13#10 +
              'The plugin requires OBS Studio 28.0 or later.' + #13#10 +
              'Tested with OBS Studio 32.0.2.' + #13#10 + #13#10 +
              'Do you want to continue with the installation anyway?',
              mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Log installation path for debugging
    Log('Plugin installed to: ' + ExpandConstant('{app}'));
    Log('Plugin DLL: ' + ExpandConstant('{app}\bin\64bit\obs-polyemesis.dll'));
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  NeedsRestart := False;
end;

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nThe plugin will be installed to:%n%nC:\ProgramData\obs-studio\plugins\obs-polyemesis%n%nThis is the recommended location for OBS plugins.

[CustomMessages]
english.FinishedHeadingLabel=Installation Complete
english.FinishedLabel=The plugin has been installed successfully.%n%nTo use the plugin:%n1. Launch OBS Studio%n2. Go to View > Docks > Restreamer Control%n3. Configure your Restreamer connection
