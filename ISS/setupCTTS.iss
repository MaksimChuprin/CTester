; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "StandTools"
#define MyAppVersion "1.0.1"


[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{95B28589-11E3-4EBE-987E-3630AD5DB84D}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName="{sd}\CTTS"
DefaultGroupName="CTTS\{#MyAppName}"
RestartIfNeededByRun=no
AllowNoIcons=yes
OutputBaseFilename=setup{#MyAppName}_{#MyAppVersion}
SetupIconFile=Resources\wrench.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "installdriver"; Description: "���������� ��������"; GroupDescription: "�������������� �����:"
Name: "instaltermite"; Description: "���������� �������� Termite"; GroupDescription: "�������������� �����:"
Name: "instalSTlink"; Description: "���������� �������� STM32 ST-Link"; GroupDescription: "�������������� �����:"

[InstallDelete]

[Dirs]

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; st-link
Source: "..\STLink\*"; DestDir: "{app}\drivers\STLink\"; Tasks: instalSTlink; Flags: recursesubdirs createallsubdirs ignoreversion deleteafterinstall

; Termite
Source: "..\Terminal\*"; DestDir: "{app}\drivers\Terminal\"; Tasks: instaltermite; Flags: recursesubdirs createallsubdirs ignoreversion deleteafterinstall

; drivers
Source: "..\ST_VCOM\*"; DestDir: "{app}\drivers\ST_VCOM\"; Tasks: installdriver; Flags: recursesubdirs createallsubdirs ignoreversion deleteafterinstall

; manual
Source: "..\Docs\CTTSManual.pdf"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]


[Run]
Filename: "{app}\drivers\ST_VCOM\vcom_install.bat"; Tasks: installdriver; StatusMsg: "Installing ST Driver..."; Flags: runminimized shellexec waituntilterminated
Filename: "{app}\drivers\Terminal\termite-3.4.exe"; Tasks: instaltermite; StatusMsg: "Installing Termite..."; Flags:  waituntilterminated
Filename: "{app}\drivers\STLink\STM32 ST-LINK Utility v4.1.0 setup.exe"; Tasks: instaltermite; StatusMsg: "Installing STM32 ST-Link..."; Flags:  waituntilterminated



[UninstallRun]
