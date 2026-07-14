# AliveMonitor

[Italiano](readme_it.md) · [English](README.md) · [Français](readme_fr.md) · [Español](readme_es.md) · **Deutsch**

Desktop-Anwendung (C++20, wxWidgets 3.2+, CMake, MVC-Architektur) zur Echtzeiterfassung der analogen Spannungen A0–A5 und zur Steuerung der digitalen Ausgänge D2–D9 eines über USB angeschlossenen **Arduino Uno**-Boards, mit enthaltenem, dediziertem Firmware-Sketch.
HINWEIS: ZUERST DIE FIRMWARE AUF DEN ARDUINO LADEN!! Sie befindet sich im Installationsordner ..\AliveMonitor\arduino\AliveMonitor
Hauptmerkmale: automatische Erkennung der seriellen Schnittstelle (`HELLO`/`ARDUINO_UNO`-Handshake) mit automatischer Wiederverbindung; Echtzeitdiagramm mit 7 Registerkarten — eine kombinierte mit sechs Kurven (Zoom, Verschieben, Autoskalierung, Legende, Raster) plus eine Registerkarte pro Kanal mit eigener Y-Achse in der umgerechneten physikalischen Größe, siehe unten — mit PNG-Export; lineare Kalibrierung pro Kanal (V → physikalische Größe, siehe unten); fortlaufende CSV-Aufzeichnung ab Start (Erzeuger/Verbraucher, siehe unten); Erfassungsfrequenz von 1 bis 250 Hz einstellbar, auch während der laufenden Erfassung; thread-sicherer 60-Sekunden-Ringpuffer pro Kanal (bereits für 500 Hz dimensioniert); vom Erfassen entkoppeltes Rendering (10–60 konfigurierbare FPS); Statusleiste mit FPS, empfangenen/verlorenen Paketen, CRC-/seriellen Fehlern, Verbindungszeit und CPU-Auslastung; Oberfläche in 5 Sprachen verfügbar (Italiano, English, Français, Español, Deutsch), auswählbar in den Einstellungen (erfordert einen Neustart).
Bei dieser Gelegenheit danke ich Vincenzo Gentile für seine Anregungen und Ratschläge. Weitere sind jederzeit willkommen.

## Projektstruktur

```
AliveMonitor/
├── CMakeLists.txt          Build-System (Windows und Linux)
├── README.md               Diese Datei (Englisch, Referenzversion)
├── readme_it.md            Versione italiana
├── readme_fr.md            Version française
├── readme_es.md            Versión en español
├── readme_de.md            Deutsche Version
├── LICENSE                 MIT-Lizenz
├── include/                Gemeinsame Header (Version, Language, Ereignisse, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── i18n/                   Strings.h/.cpp: Übersetzungstabelle der Oberfläche
├── model/                  MODEL: BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer,
│                           ChannelCalibration, AppSettings
├── view/                   VIEW: MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, CalibrationPanel, GraphPanel,
│                           StatusPanel, SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER: MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + Win32- und POSIX-Implementierungen
├── resources/              Eingebettete Ressourcen (Icons, HTML-Anleitung in 5 Sprachen)
├── installer/              Inno-Setup-Skript für die Windows-setup.exe
├── docs/                   Architecture.md, Protocol.md, Translations.md, UML/
└── arduino/
    └── AliveMonitor/
        └── AliveMonitor.ino  Vollständige Firmware für Arduino Uno
```

## Kompilierung — Linux

Erfordert GCC 12+ (oder Clang 15+), CMake ≥ 3.21 und wxWidgets ≥ 3.2.

**Ubuntu 24.04+ / Debian 12+:**

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev
cd AliveMonitor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/AliveMonitor
```

**Distributionen ohne wxWidgets 3.2 in den offiziellen Repositories** (z. B. Ubuntu 22.04, das nur 3.0 hat; oder Slackware, das es überhaupt nicht anbietet — nur wxGTK3 3.0.5 über SlackBuilds.org): Statt wxWidgets von Hand zu kompilieren, kann das automatische Skript verwendet werden, das von keinem Paketmanager abhängt (es benötigt nur `cmake`, `gcc`, `g++`, `make`, `curl` — unter Slackware muss `cmake` ergänzt werden, der Rest ist in einer "full"-Installation enthalten):

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Lädt wxWidgets 3.2.11 herunter, kompiliert (gemeinsam genutzter Build) und installiert es in `third_party/wx-install`, automatisch von `CMakeLists.txt` erkannt — `-DCMAKE_PREFIX_PATH` muss nicht übergeben werden. Idempotent (`--force` zum Wiederholen), Optionen mit `--help`.

**Rechte für die serielle Schnittstelle:** den Benutzer der Gruppe `dialout` hinzufügen (`sudo usermod -aG dialout $USER`, danach neu anmelden).

### Die ausführbare Datei auf einen anderen Linux-Rechner verteilen (ohne wxGTK zu installieren)

Unter Linux ist ein wirklich "abhängigkeitsfreier" Build wie unter Windows
nicht realistisch (wxGTK stützt sich auf GTK, das zur Laufzeit ohnehin
Themes/Icons vom System lädt, selbst wenn statisch gelinkt wird). Die
praktische Alternative ist ein **portables Bundle**: ein Ordner mit der
ausführbaren Datei und all ihren nicht systemeigenen gemeinsam genutzten
Bibliotheken, bereit, auf einen anderen Rechner (gleiche Architektur)
kopiert und ausgeführt zu werden, ohne etwas zu installieren:

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
bash scripts/package-linux.sh
```

Erstellt `dist/AliveMonitor-linux-x64/` mit der ausführbaren Datei plus
einem `lib/`-Ordner mit wxGTK, GTK, glib, pango, cairo, X11 usw. (mit `ldd`
erkannt). Basisbibliotheken (glibc, libpthread, ld-linux...) werden
bewusst ausgeschlossen: Sie sind an den Kernel/die libc des Zielrechners
gebunden, sie zu bündeln würde die Portabilität beeinträchtigen statt sie
zu unterstützen. Das funktioniert dank eines relativen RPATH
(`$ORIGIN/lib`), der bereits von `CMakeLists.txt` in die ausführbare Datei
eingebettet wird: Es genügt, den gesamten Ordner
`dist/AliveMonitor-linux-x64/` zu kopieren und `./AliveMonitor`
auszuführen — kein `LD_LIBRARY_PATH` muss gesetzt werden (`run.sh` ist als
Sicherheitsnetz enthalten). Das Skript selbst prüft mit `ldd`, dass keine
Abhängigkeiten fehlen, bevor es sich für abgeschlossen erklärt.

Einschränkung: Der Zielrechner benötigt eine kompatible libc (gleiche
Distribution/Familie oder eine Version, die gleich oder neuer ist als die
des Build-Rechners) und dieselbe Architektur. Für volle
distributionsübergreifende Portabilität (auch glibc bündeln) wäre ein
Format wie AppImage nötig — hier nicht enthalten, aber das Bundle von
`package-linux.sh` deckt die meisten realen Fälle ab (gleiche Distribution
oder Familie, z. B. von Slackware zu Slackware, von Ubuntu zu Debian).

## Kompilierung — Windows

### Option 0 (empfohlen): automatisches Skript, statischer Build ohne DLLs

Zunächst wird ein installierter **MinGW-w64**-Compiler benötigt (z. B. über
[MSYS2](https://www.msys2.org/) oder [WinLibs](https://winlibs.com/)).
Danach, ohne Zwischenschritte:

```bat
cd AliveMonitor
scripts\setup-wxwidgets.bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
build\AliveMonitor.exe
```

Das Skript lädt ein statisches wxWidgets 3.2.11 herunter, kompiliert und
installiert es in `%LOCALAPPDATA%\AliveMonitor\wxWidgets` — bewusst
**außerhalb** des Projekts: Enthält der Repository-Pfad für reguläre
Ausdrücke besondere Zeichen (z. B. ein Ordner namens `c++`), wird die von
wxWidgets generierte Datei `wxWidgetsConfig.cmake` von CMake nicht korrekt
gefunden. `%LOCALAPPDATA%` vermeidet das Problem unabhängig davon, wo sich
das Projekt befindet, und wird ohnehin automatisch von `CMakeLists.txt`
erkannt. Es sucht selbstständig nach dem MinGW-Compiler (im `PATH`, dann
in den gängigsten Installationsordnern): Findet es ihn an genau einer
Stelle, fährt es fort, ohne etwas zu fragen. Findet es ihn nicht oder mehr
als einen installiert, hält es an und bittet um explizite Angabe:

```bat
scripts\setup-wxwidgets.bat -MinGwBin C:\Pfad\zu\mingw64\bin
```

`setup-wxwidgets.bat` ist ein Wrapper, der `setup-wxwidgets.ps1` aufruft
(und alle Argumente weiterleitet); wer möchte, kann das PowerShell-Skript
auch direkt aufrufen:
`powershell -ExecutionPolicy Bypass -File scripts\setup-wxwidgets.ps1 [-MinGwBin ...]`.
Das Skript ist idempotent (überspringt die Neukompilierung, falls schon
erfolgt; `-Force` zum Wiederholen).
Die resultierende ausführbare Datei hängt von keiner Dritthersteller-DLL
ab (überprüft mit `objdump -p AliveMonitor.exe`), nur von
Windows-System-DLLs: Sie kann allein kopiert und verteilt werden.

**Aufräumen:** Das kompilierte wxWidgets (Quellcode + Build + Installation,
einige hundert MB) verbleibt in `%LOCALAPPDATA%\AliveMonitor`, gemeinsam
genutzt von eventuellen Klonen des Projekts. Um Platz freizugeben oder eine
vollständige Neukompilierung zu erzwingen, löschen mit:

```bat
rmdir /s /q "%LOCALAPPDATA%\AliveMonitor"
```

(betrifft weder das Projekt noch den `build\`-Ordner von AliveMonitor: nur
wxWidgets muss bei Bedarf mit `scripts\setup-wxwidgets.bat` neu kompiliert
werden.)

Die Optionen A/B/C unten bleiben gültig für alle, die wxWidgets lieber
manuell verwalten möchten (z. B. gemeinsam genutzter Build/DLLs, oder ein
bereits anderswo installiertes wxWidgets).

### Option A: wxWidgets aus dem Quellcode mit MinGW kompiliert (CONFIG-Modus)

Wurde wxWidgets mit CMake aus dem Quellcode kompiliert, zum Beispiel:

```bat
cd C:\library\wxWidgets
mkdir build_gcc && cd build_gcc
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=ON -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
```

muss es anschließend **installiert werden** (in wx 3.2.x funktioniert die Datei `wxWidgetsConfig.cmake` nur vom Installationspräfix aus, nicht vom Build-Baum):

```bat
cmake --install C:\library\wxWidgets\build_gcc --prefix C:\library\wx-install
```

danach AliveMonitor unter Angabe des Präfixes kompilieren:

```bat
cd AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install
cmake --build build -j8
build\AliveMonitor.exe
```

Hinweise zu diesem Modus:
- bei `wxBUILD_SHARED=ON` werden die wxWidgets-DLLs beim Build **automatisch neben die ausführbare Datei kopiert** (deaktivierbar mit `-DAM_COPY_WX_DLLS=OFF`, für statische Builds zu verwenden);
- zur Laufzeit werden auch die MinGW-DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`) benötigt: Das Projekt linkt sie **standardmäßig statisch** (`AM_STATIC_RUNTIME=ON`, siehe unten), sodass sie normalerweise weder kopiert noch der `PATH` angepasst werden muss;
- **denselben MinGW-Compiler verwenden**, mit dem wxWidgets kompiliert wurde (dieselbe Toolchain im `PATH`) — eine nicht übereinstimmende Toolchain ist die häufigste Ursache des Fehlers *"Einstiegspunkt nicht gefunden"* beim Start.

### Vollständig statischer Build (keine zu verteilenden DLLs)

Um eine `AliveMonitor.exe` zu erhalten, die von keiner Dritthersteller-DLL abhängt (nur von Windows-System-DLLs), muss neben dem Compiler-Runtime auch **wxWidgets im statischen Modus** kompiliert werden:

```bat
cd C:\library\wxWidgets
mkdir build_gcc_static && cd build_gcc_static
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=OFF -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
cmake --install . --prefix C:\library\wx-install-static

cd AliveMonitor
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install-static
cmake --build build-static -j8
build-static\AliveMonitor.exe
```

- `AM_STATIC_RUNTIME` (Standard **ON**) fügt `-static-libgcc -static-libstdc++ -static` hinzu und bettet libstdc++/libgcc/winpthread in die exe ein. Deaktivierbar mit `-DAM_STATIC_RUNTIME=OFF`, falls diese DLLs lieber separat verteilt werden sollen.
- Bei statischem wx (`wxBUILD_SHARED=OFF`) deaktiviert sich der DLL-Kopierschritt (`AM_COPY_WX_DLLS`) automatisch: Es gibt nichts zu kopieren.
- Unter MSVC wird dasselbe Ziel mit `AM_STATIC_RUNTIME=ON` (CRT `/MT`, automatisch) plus einem mit statischem Triplet kompilierten wxWidgets/vcpkg erreicht, z. B. `vcpkg install wxwidgets:x64-windows-static`.
- Die statische ausführbare Datei ist größer (Runtime und wx sind eingebettet), lässt sich aber als einzelne Datei kopieren und verteilen, ohne Risiko einer Versionsinkompatibilität zwischen den DLLs.

### Option B: MSYS2/MinGW mit vorkompilierten Paketen

1. [MSYS2](https://www.msys2.org/) installieren, das Terminal *MSYS2 MINGW64* öffnen und ausführen:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-wxwidgets3.2-msw
cd /c/Pfad/zu/AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AliveMonitor.exe
```

### Option C: Visual Studio 2022 + vcpkg

```bat
vcpkg install wxwidgets:x64-windows
cd AliveMonitor
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
build\Release\AliveMonitor.exe
```

Der COM-Port-Treiber des Arduino Uno wird automatisch von Windows 10/11 installiert (bei Klonen mit CH340 den Treiber des Herstellers installieren).

### Den Windows-Installer erstellen (setup.exe)

Um AliveMonitor an einen Endbenutzer zu verteilen, ohne ihn einen Compiler oder wxWidgets installieren zu lassen, kann mit [Inno Setup](https://jrsoftware.org/isinfo.php) ein eigenständiger Installer erzeugt werden (kostenlos, muss nur auf dem Rechner installiert werden, der das Setup *erstellt*, nicht auf dem, der es *erhält*):

```bat
scripts\setup-wxwidgets.bat
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-static -j8

powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

Der Wrapper `package-windows.ps1` prüft, ob `build-static\AliveMonitor.exe` existiert (der **vollständig statische Build** wird benötigt, ohne abhängige DLLs: siehe oben), ermittelt automatisch die Version aus `include/Version.h`, findet `ISCC.exe` (im `PATH` oder in den Standard-Installationsordnern von Inno Setup) und erzeugt `dist\AliveMonitor-Setup-<Version>.exe` — Icon, Eintrag im Startmenü, optionale Desktop-Verknüpfung, während der Installation angezeigte Lizenz, Deinstallationsprogramm unter "Apps & Features". Neben der ausführbaren Datei kopiert der Installer auch den Sketch `AliveMonitor.ino` (Ordner `arduino\AliveMonitor\`, ebenfalls über das Startmenü erreichbar): Wer von diesem setup.exe installiert, ohne das Repository geklont zu haben, muss trotzdem in der Lage sein, die Firmware auf das Board zu laden.

Das Inno-Setup-Skript (`installer\AliveMonitor.iss`) verpackt die **bereits kompilierte ausführbare Datei**, nicht den Quellcode (der im GitHub-Repository verbleibt): Die erzeugte Datei `dist\AliveMonitor-Setup-*.exe` darf NICHT committet werden — sie ist ein Build-Artefakt (der Ordner `dist/` steht bereits in `.gitignore`), separat zu verteilen, z. B. als Anhang eines GitHub-Release.

## Arduino-Firmware

1. `arduino/AliveMonitor/AliveMonitor.ino` mit der Arduino IDE (≥ 1.8) oder `arduino-cli` öffnen.
2. Das Board **Arduino Uno** und den richtigen Port auswählen.
3. Den Sketch hochladen. Fertig: Die PC-Anwendung findet das Board von selbst.

Mit `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino
```

## Verwendung

Beim Start durchsucht das Programm automatisch die seriellen Schnittstellen und verbindet sich mit dem Board, das mit `ARDUINO_UNO` antwortet (grüne LED oben links). *Start* beginnt das Streaming mit der über SpinCtrl oder Schieberegler eingestellten Frequenz (1–250 Hz, auch während des Betriebs änderbar); *Pause* unterbricht es, wobei die Daten erhalten bleiben; *Stopp* beendet es und leert den Puffer. Die Schaltflächen D2–D9 steuern die Ausgänge: Die LED neben jeder Schaltfläche zeigt den vom Firmware **tatsächlich bestätigten** Zustand. Im Diagramm: Mausrad = Zoom auf der Zeitachse, Strg+Mausrad = Zoom bei der Spannung, Ziehen = Verschieben, *Folgen* koppelt das scrollende Fenster wieder an, *Zurücksetzen* stellt die Standardansicht wieder her (letzte 60 s, 0–5 V). Wird das Board getrennt, meldet die Anwendung dies und setzt die Suche automatisch fort, wobei bei der Wiederverbindung Ausgänge und Streaming wiederhergestellt werden.

### Fortlaufende CSV-Aufzeichnung

Wird *Start* aus dem gestoppten Zustand gedrückt (nicht aus *Pause*),
erscheint ein Fenster zur Auswahl des Speicherorts der Datei, mit einem
bereits vorgeschlagenen Namen
(`acquisizione_JJJJ-MM-TT_HH-MM-SS.csv`); wird das Fenster abgebrochen,
wird auch der Start der Erfassung abgebrochen. Ein Kontrollkästchen
"CSV aufzeichnen" unter Start/Pause/Stopp (standardmäßig aktiviert)
erlaubt es, den Dialog und die Aufzeichnung ganz zu überspringen: Ist es
deaktiviert, beginnt Start sofort, ohne Nachfrage und ohne eine Datei zu
schreiben. Bei aktiver Aufzeichnung wird jeder empfangene ADC-Messwert in
Echtzeit in die Datei geschrieben, ohne den seriellen Thread jemals zu
blockieren: Dies ist mit einem **Erzeuger/Verbraucher**-Muster
(`CsvLoggerController`) implementiert — der serielle Thread reiht die
Messwerte in eine thread-sichere Warteschlange ein (`push()`, nicht
blockierend), ein dritter, dedizierter Thread entnimmt sie und schreibt
sie auf die Festplatte. Eine LED (grün = schreibend, rot = gestoppt) und
eine Beschriftung mit dem vollständigen Dateipfad, zentriert in der
Werkzeugleiste des Diagramms, zeigen den Status der laufenden
Aufzeichnung. *Pause* unterbricht die Datei nicht (sie bleibt dieselbe,
mit möglichen "Lücken" während der Pausenzeiten); *Stopp* schließt die
Aufzeichnung, wobei gewartet wird, bis der Verbraucher das Schreiben aller
bereits in der Warteschlange befindlichen Messwerte beendet hat, sodass
weder beim manuellen Stoppen noch beim Schließen der Anwendung Daten
verloren gehen.

### Kanalkalibrierung (V → physikalische Größe)

Unterhalb des Erfassungsbereichs befindet sich eine Tabelle
(`CalibrationPanel`) mit einer Zeile pro Kanal (A0..A5) und vier
editierbaren Spalten: Koeffizient `a`, Offset `b`, Maßeinheit und
Beschreibung. Für jeden mit linearem Zusammenhang G = a·V + b
angeschlossenen Messumformer (z. B. hätte ein Temperatursensor 0–5 V →
0–100 °C a=20, b=0, Einheit "°C") die Werte in der Zeile des
entsprechenden Kanals eingeben: Die Auswirkung zeigt sich sofort in der
Legende des Diagramms, die neben dem Kanalnamen auch den in Echtzeit
umgerechneten Wert anzeigt (z. B. "A0: 23.4 °C"). Nicht konfigurierte
Kanäle bleiben bei der Identität (a=1, b=0, Einheit "V"), zeigen also
einfach die Spannung. Die Beschreibung ist eine freie Bezeichnung (z. B.
"Ofentemperatur"): Ist sie ausgefüllt, wird sie zum Titel der diesem Kanal
gewidmeten Registerkarte im Diagramm (siehe unten), andernfalls behält die
Registerkarte den Kanalnamen ("A0".."A5"). Die Kalibrierung gilt nur für
die aktuelle Sitzung (sie wird nicht auf der Festplatte gespeichert); ist
eine CSV-Aufzeichnung aktiv, enthält die Datei — neben den sechs Spalten
`A#_V` in Volt — sechs zusätzliche Spalten `A#_conv[Einheit]` mit dem
bereits umgerechneten Wert, unter Verwendung der zum Zeitpunkt des
Drückens von Start eingestellten Kalibrierung (eine Änderung bei bereits
laufender Aufzeichnung ändert weder die Kopfzeile noch die in dieser
Sitzung bereits geschriebenen Zeilen).

### Diagramm mit 7 Registerkarten (unabhängige Achsen pro Kanal)

Das Diagramm ist ein `wxNotebook` mit 7 Registerkarten. Die erste, "Alle",
ist das gewohnte kombinierte Diagramm: Die sechs Kurven teilen sich eine
einzige Y-Achse in Volt, mit Kontrollkästchen zum Aus-/Einblenden
einzelner Kanäle und dem Status der CSV-Aufzeichnung (LED + Dateipfad).
Die anderen sechs Registerkarten (A0..A5) zeigen jeweils einen einzelnen
Kanal, mit Kurve und Y-Achse in der gemäß ihrer Kalibrierung *bereits
umgerechneten* Größe (oder in Volt, falls unkalibriert) — eine einfache
Möglichkeit, praktisch "mehrere Achsen" zu haben, wenn die Kanäle Größen
unterschiedlicher Art und Skala messen, ohne inkompatible Einheiten auf
derselben Achse überlagern zu müssen. Der Titel jeder dieser sechs
Registerkarten ist die in der Kalibrierungstabelle festgelegte
Beschreibung (Spalte "Beschreibung"), oder der Kanalname, falls nicht
ausgefüllt; er ändert sich in Echtzeit bei jeder Änderung der Tabelle,
ohne dass ein Neustart nötig ist. Jede Einzelkanal-Registerkarte hat
standardmäßig Auto Y aktiviert (da der Bereich der umgerechneten Größe im
Voraus nicht bekannt ist) und eigene, von den anderen Registerkarten
unabhängige Schaltflächen Autoskalierung/Folgen/Zurücksetzen/PNG; der
PNG-Export speichert immer die aktuell angezeigte Registerkarte. Das
Zeitfenster (Einstellungen) gilt für alle 7 Registerkarten gemeinsam,
während die fortlaufende Y-Autoskalierung im Einstellungsbereich nur die
kombinierte Registerkarte betrifft — die sechs Registerkarten pro Kanal
bleiben unabhängig.

### Oberflächensprache

Über das Menü Extras &gt; Einstellungen kann die Oberflächensprache
zwischen Italiano, English, Français, Español und Deutsch gewählt werden.
Die Wahl wird in `%APPDATA%\AliveMonitor\settings.ini` (Windows) bzw.
`~/.config/AliveMonitor/settings.ini` (Linux) gespeichert und erfordert
einen Neustart der Anwendung, um wirksam zu werden: Bereits erstellte
Widgets werden nicht zur Laufzeit neu übersetzt. Die vollständige
Übersetzungstabelle befindet sich in `docs/Translations.md`.

## Dokumentation

Die vollständige Beschreibung der Architektur, der Designentscheidungen und der Erweiterungsvorschläge befindet sich in `docs/Architecture.md`; die Spezifikation des seriellen Protokolls in `docs/Protocol.md`; die Übersetzungstabelle der Oberfläche in `docs/Translations.md`; die UML-Diagramme (Klassen- und Sequenzdiagramme, PlantUML + Mermaid) in `docs/UML/`; die Schritt-für-Schritt-Anleitung zum Kompilieren mit der CMake-GUI (statisches wxWidgets eingeschlossen) in `docs/CMakeGUI.md`.

## Integrierte Anleitung

Über das Menü Hilfe &gt; Anleitung (oder F1) öffnet sich im Standardbrowser eine Kurzanleitung zur Nutzung der Anwendung, in der aktuellen Sprache der Oberfläche, eingebettet in die ausführbare Datei wie die anderen Ressourcen (einschließlich des Icons): keine externe Datei, die zusammen mit der Binärdatei verteilt werden müsste.

## Lizenz

Vertrieben unter der **MIT**-Lizenz: freie Nutzung, Änderung und Weitergabe, auch kommerziell, bereitgestellt "wie besehen" ohne jegliche Gewährleistung oder Haftung der Autoren für eventuelle Schäden, die aus der Nutzung der Software entstehen. Vollständiger Text in [`LICENSE`](LICENSE).
