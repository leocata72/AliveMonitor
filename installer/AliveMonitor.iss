; ============================================================================
;  AliveMonitor - script Inno Setup per il setup.exe Windows
; ============================================================================
; Impacchetta l'ESEGUIBILE GIA' COMPILATO (non i sorgenti: quelli restano su
; GitHub) in un installer con icona, voce nel menu Avvio, collegamento
; desktop opzionale e disinstallatore in "App e funzionalità".
;
; Presuppone una build COMPLETAMENTE STATICA (vedi README.md, sezione "Build
; completamente statica"): un solo file AliveMonitor.exe senza DLL di terze
; parti da cui dipendere. Oltre all'eseguibile, [Files] copia anche lo
; sketch del firmware Arduino: chi installa da questo setup.exe (senza aver
; clonato il repository sorgenti) deve comunque poterlo trovare, dato che è
; il primo passo obbligatorio della Guida in-app (menu Aiuto > Guida).
;
; Non va lanciato a mano con ISCC.exe: usa il wrapper
;     powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
; che verifica la build, ricava la versione da include/Version.h e passa i
; parametri qui sotto (MyAppVersion, SourceExe) con /D. I valori di default
; permettono comunque di lanciarlo anche a mano da dentro Inno Setup (GUI o
; ISCC diretto) per un rapido test.
;
; Prerequisito: Inno Setup (gratuito) installato sulla macchina che compila
; il setup — non sulla macchina dell'utente finale, che riceve solo il
; setup.exe risultante. https://jrsoftware.org/isinfo.php

#ifndef MyAppVersion
  #define MyAppVersion "1.1.0"
#endif
#ifndef SourceExe
  #define SourceExe "..\build-static\AliveMonitor.exe"
#endif

#define MyAppName "AliveMonitor"
#define MyAppPublisher "Leonardo Catalano"
#define MyAppURL "https://github.com/leocata72/AliveMonitor"

[Setup]
; GUID fisso e stabile fra le versioni: NON rigenerarlo ai rilasci futuri,
; è quello che permette a Inno Setup di riconoscere un aggiornamento in-place
; (stessa AppId) invece di un'installazione parallela.
AppId={{03797851-F908-4DC6-9B8F-F20C32884204}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=AliveMonitor-Setup-{#MyAppVersion}
SetupIconFile=..\resources\appicon.ico
UninstallDisplayIcon={app}\AliveMonitor.exe
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
; "x64compatible" richiede Inno Setup >= 6.3: con versioni più vecchie
; sostituire con "x64" (o rimuovere le due righe per un installer 32-bit
; universale, comunque compatibile con un AliveMonitor.exe a 64 bit).
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
; Lingue del wizard di installazione stesso (testi "Avanti"/"Annulla" ecc.,
; forniti da Inno Setup): allineate deliberatamente alle 5 lingue ora
; supportate dall'interfaccia di AliveMonitor (vedi i18n/Strings.h), anche
; se le due cose restano indipendenti — la lingua scelta qui non influenza
; quella dell'app, selezionabile separatamente da Impostazioni al primo
; avvio.
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[CustomMessages]
; Nome della voce di menu per la cartella del firmware Arduino, tradotto
; nelle stesse 5 lingue del wizard sopra (vedi [Languages]): senza questo
; resterebbe fissa in italiano anche scegliendo un'altra lingua per il
; wizard.
english.ArduinoFirmwareFolder=Arduino firmware folder
italian.ArduinoFirmwareFolder=Cartella firmware Arduino
french.ArduinoFirmwareFolder=Dossier firmware Arduino
spanish.ArduinoFirmwareFolder=Carpeta de firmware Arduino
german.ArduinoFirmwareFolder=Arduino-Firmware-Ordner

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceExe}"; DestDir: "{app}"; DestName: "AliveMonitor.exe"; Flags: ignoreversion
; README in tutte le 5 lingue supportate (README.md è la versione inglese
; di riferimento; le altre sono readme_<lingua>.md, vedi la nota in cima a
; ciascun file per lo switcher fra le versioni).
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\readme_it.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\readme_fr.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\readme_es.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\readme_de.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
; Firmware Arduino: senza questo, chi installa solo il setup.exe (senza
; clonare il repository) non avrebbe modo di caricare lo sketch sulla
; scheda, che è il primo passo richiesto prima di poter usare l'app.
Source: "..\arduino\AliveMonitor\AliveMonitor.ino"; DestDir: "{app}\arduino\AliveMonitor"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\AliveMonitor.exe"
Name: "{group}\{cm:ArduinoFirmwareFolder}"; Filename: "{app}\arduino\AliveMonitor"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\AliveMonitor.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\AliveMonitor.exe"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
