; ArSonComp Windows x64 Inno Setup installer
; Build: ISCC.exe /DStageDir="C:\stage" /DAppVersion="0.2.0" packaging\windows\ArSonComp.iss
#ifndef StageDir
  #error StageDir must point to the built stage (VST3, Standalone, LICENSE, START-HERE.md)
#endif
#ifndef AppVersion
  #define AppVersion "0.2.0"
#endif
[Setup]
AppId=ArSonComp-ArSonAudio
AppName=ArSonComp
AppVersion={#AppVersion}
AppPublisher=ArSon Audio
AppPublisherURL=https://github.com/masarray/arson-comp
AppSupportURL=https://github.com/masarray/arson-comp/issues
AppUpdatesURL=https://github.com/masarray/arson-comp/releases
DefaultDirName={autopf}\ArSon Audio\ArSonComp
DefaultGroupName=ArSon Audio
OutputBaseFilename=ArSonComp-{#AppVersion}-Windows-x64-Setup
OutputDir=.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
UsePreviousPrivileges=yes
DisableDirPage=yes
DisableProgramGroupPage=yes
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=ArSonComp
CloseApplications=yes
RestartApplications=no
LicenseFile={#StageDir}\LICENSE
SetupLogging=yes
MinVersion=10.0
[Types]
Name: "full"; Description: "Recommended: VST3 plug-in + Standalone"
Name: "plugin"; Description: "VST3 plug-in only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom
[Components]
Name: "vst3"; Description: "VST3 plug-in for DAWs and mixers"; Types: full plugin custom
Name: "standalone"; Description: "Standalone audio application"; Types: full custom
[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Shortcuts:"; Components: standalone; Flags: unchecked
[Files]
Source: "{#StageDir}\VST3\ArSonComp.vst3\*"; DestDir: "{code:GetPluginFolder}\ArSonComp.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\Standalone\ArSonComp.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#StageDir}\START-HERE.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{autoprograms}\ArSonComp"; Filename: "{app}\ArSonComp.exe"; Components: standalone
Name: "{autodesktop}\ArSonComp"; Filename: "{app}\ArSonComp.exe"; Components: standalone; Tasks: desktopicon
[Run]
Filename: "{app}\ArSonComp.exe"; Description: "Launch ArSonComp"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
[Code]
function GetPluginFolder(Param: String): String;
begin
  if IsAdminInstallMode then
    Result := ExpandConstant('{commoncf}\VST3')
  else
    Result := ExpandConstant('{localappdata}\Programs\Common\VST3');
end;
procedure CurStepChanged(CurStep: TSetupStep);
var
  OtherPlugin: String;
begin
  if (CurStep <> ssPostInstall) or not WizardIsComponentSelected('vst3') then
    Exit;
  if IsAdminInstallMode then
    OtherPlugin := ExpandConstant('{localappdata}\Programs\Common\VST3\ArSonComp.vst3')
  else
    OtherPlugin := ExpandConstant('{commoncf}\VST3\ArSonComp.vst3');
  if DirExists(OtherPlugin) then
    MsgBox('Another ArSonComp VST3 also exists in:' + #13#10 + OtherPlugin +
      #13#10#13#10 + 'Some DAWs may load that copy first. Remove the older copy.' +
      #13#10 + 'Restart or rescan your DAW after installing.',
      mbInformation, MB_OK);
end;
