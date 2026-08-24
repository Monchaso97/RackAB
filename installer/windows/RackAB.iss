; ============================================================
;  RackAB - Windows installer (Inno Setup)
;  Welcome page: plugin image on the left + description on the right,
;  then a directory page (default VST3 path, changeable) with an Install button.
; ============================================================

#define MyAppName "RackAB"
#define MyAppVersion "1.0.0"
#define MyPublisher "SlideOne"
#define MyVstSource "..\..\build\RackAB_artefacts\Release\VST3\RackAB.vst3"

[Setup]
AppId={{7E2A9C40-4B1A-4E2E-9C6E-A1B2C3D4E5F6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyPublisher}
DefaultDirName={commoncf}\VST3
DisableDirPage=no
AllowNoIcons=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
DisableWelcomePage=no
UninstallDisplayName={#MyAppName} (VST3)
OutputDir=output
OutputBaseFilename=RackAB-{#MyAppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
WizardImageFile=welcome.bmp
WizardImageStretch=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
en.WelcomeLabel1=Install RackAB
en.WelcomeLabel2=RackAB is a VST3 rack that hosts your third-party VST3 plugins in a reorderable serial chain.%n%nActivate COMPARE mode and use the Up/Down arrow keys to solo one plugin at a time for fast, level-matched A/B auditions (e.g. compare 10 compressors).%n%nClick Next to choose where to install the plugin.
es.WelcomeLabel1=Instalar RackAB
es.WelcomeLabel2=RackAB es un rack VST3 que aloja tus plugins VST3 de terceros en una cadena serie reordenable.%n%nActiva el modo COMPARE y usa las flechas Arriba/Abajo para dejar en solo un plugin cada vez y comparar A/B de forma fluida e igualada (p.ej. 10 compresores).%n%nPulsa Siguiente para elegir donde instalar el plugin.

[Files]
Source: "{#MyVstSource}\*"; DestDir: "{app}\RackAB.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{app}\RackAB.vst3"
