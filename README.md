# AliveMonitor

Applicazione desktop professionale (C++20, wxWidgets 3.2+, CMake, architettura MVC) per l'acquisizione in tempo reale delle tensioni analogiche A0–A5 e il controllo delle uscite digitali D2–D9 di una scheda **Arduino Uno** collegata via USB, con firmware dedicato incluso.

Caratteristiche principali: rilevamento automatico della porta seriale (handshake `HELLO`/`ARDUINO_UNO`) con riconnessione automatica; grafico in tempo reale a sei curve con zoom, pan, autoscale, legenda, griglia, esportazione PNG; registrazione CSV continua allo Start (produttore/consumatore, vedi sotto); frequenza di acquisizione regolabile da 1 a 250 Hz anche durante l'acquisizione; ring buffer thread-safe da 60 secondi per canale (dimensionato già per 500 Hz); rendering disaccoppiato dall'acquisizione (10–60 FPS configurabili); barra di stato con FPS, pacchetti ricevuti/persi, errori CRC/seriali, tempo di connessione e CPU.

## Struttura del progetto

```
AliveMonitor/
├── CMakeLists.txt          Build system (Windows e Linux)
├── README.md               Questo file
├── include/                Header condivisi (Version, eventi, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── model/                  MODEL: BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer
├── view/                   VIEW: MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, GraphPanel, StatusPanel,
│                           SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER: MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + implementazioni Win32 e POSIX
├── resources/              Risorse (icone, ecc.)
├── docs/                   Architecture.md, Protocol.md, UML/
└── arduino/
    └── AliveMonitor/
        └── AliveMonitor.ino  Firmware completo per Arduino Uno
```

## Compilazione — Linux

Richiede GCC 12+ (o Clang 15+), CMake ≥ 3.21 e wxWidgets ≥ 3.2.

**Ubuntu 24.04+ / Debian 12+:**

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev
cd AliveMonitor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/AliveMonitor
```

**Distribuzioni senza wxWidgets 3.2 nei repository ufficiali** (es. Ubuntu 22.04, che ha solo la 3.0; o Slackware, che non la offre affatto — solo wxGTK3 3.0.5 via SlackBuilds.org): invece di compilare wxWidgets a mano si può usare lo script automatico, che non dipende da nessun package manager (serve solo `cmake`, `gcc`, `g++`, `make`, `curl` — su Slackware `cmake` va aggiunto, il resto è incluso in un'installazione "full"):

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Scarica, compila (build condivisa) e installa wxWidgets 3.2.11 in `third_party/wx-install`, rilevata automaticamente dal `CMakeLists.txt` — non serve passare `-DCMAKE_PREFIX_PATH`. Idempotente (`--force` per rifare), opzioni con `--help`.

**Permessi porta seriale:** aggiungere l'utente al gruppo `dialout` (`sudo usermod -aG dialout $USER`, poi rieseguire il login).

### Distribuire l'eseguibile su un'altra macchina Linux (senza installare wxGTK)

Su Linux una build davvero "zero dipendenze" come su Windows non è
realistica (wxGTK si appoggia a GTK, che a runtime carica temi/icone dal
sistema comunque anche se linkata staticamente). L'alternativa pratica è un
**bundle portabile**: una cartella con l'eseguibile e tutte le sue librerie
condivise non di sistema, pronta da copiare e lanciare su un'altra macchina
(stessa architettura) senza installare nulla:

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
bash scripts/package-linux.sh
```

Crea `dist/AliveMonitor-linux-x64/` con l'eseguibile + una cartella `lib/`
contenente wxGTK, GTK, glib, pango, cairo, X11 ecc. (rilevate con `ldd`).
Escluse deliberatamente le librerie di base (glibc, libpthread, ld-linux...):
sono legate al kernel/alla libc della macchina di destinazione, bundlarle
romperebbe la portabilità invece di aiutarla. Funziona grazie a un RPATH
relativo (`$ORIGIN/lib`) già incorporato nell'eseguibile dal
`CMakeLists.txt`: basta copiare l'intera cartella `dist/AliveMonitor-linux-x64/`
ed eseguire `./AliveMonitor` — nessun `LD_LIBRARY_PATH` da impostare
(`run.sh` incluso come rete di sicurezza). Lo script stesso verifica con
`ldd` che non manchino dipendenze prima di dichiararsi concluso.

Limite: la macchina di destinazione deve avere una libc compatibile (stessa
distro/famiglia, o versione uguale/più recente di quella di build) e la
stessa architettura. Per la piena portabilità cross-distro (bundle anche di
glibc) servirebbe un formato come AppImage — non incluso qui, ma il bundle
di `package-linux.sh` copre la maggior parte dei casi reali (stessa distro o
famiglia, es. da Slackware a Slackware, da Ubuntu a Debian).

## Compilazione — Windows

### Opzione 0 (consigliata): script automatico, build statica senza DLL

Serve prima un compilatore **MinGW-w64** installato (es. tramite
[MSYS2](https://www.msys2.org/) o [WinLibs](https://winlibs.com/)). Poi,
senza passi intermedi:

```bat
cd AliveMonitor
scripts\setup-wxwidgets.bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
build\AliveMonitor.exe
```

Lo script scarica, compila e installa una wxWidgets 3.2.11 statica in
`%LOCALAPPDATA%\AliveMonitor\wxWidgets` — **fuori** dal progetto,
deliberatamente: se il percorso del repository contiene caratteri speciali
per le espressioni regolari (es. una cartella chiamata `c++`), il file
`wxWidgetsConfig.cmake` generato da wxWidgets non viene trovato
correttamente da CMake. `%LOCALAPPDATA%` evita il problema a prescindere da
dove si trovi il progetto, ed è comunque rilevata in automatico dal
`CMakeLists.txt`. Cerca da sé il compilatore MinGW (nel `PATH`, poi nelle
cartelle di installazione più comuni): se lo trova in un solo posto procede
senza chiedere nulla. Se non lo trova, o se ne trova più di uno installato,
si ferma e chiede di indicarlo esplicitamente:

```bat
scripts\setup-wxwidgets.bat -MinGwBin C:\percorso\mingw64\bin
```

`setup-wxwidgets.bat` è un wrapper che richiama `setup-wxwidgets.ps1`
(inoltra tutti gli argomenti); se preferisci puoi chiamare direttamente lo
script PowerShell: `powershell -ExecutionPolicy Bypass -File scripts\setup-wxwidgets.ps1 [-MinGwBin ...]`.
Lo script è idempotente (salta la ricompilazione se già fatta; `-Force` per
rifarla).
L'eseguibile risultante non dipende da nessuna DLL di terze parti (verificato
con `objdump -p AliveMonitor.exe`), solo dalle DLL di sistema Windows: si
può copiare e distribuire da solo.

**Pulizia:** wxWidgets compilata (sorgente + build + installazione, qualche
centinaio di MB) resta in `%LOCALAPPDATA%\AliveMonitor`, condivisa fra
eventuali cloni del progetto. Per liberare spazio o forzare una ricompilazione
completa, cancellala con:

```bat
rmdir /s /q "%LOCALAPPDATA%\AliveMonitor"
```

(non tocca il progetto né la cartella `build\` di AliveMonitor: solo
wxWidgets andrà ricompilata, con `scripts\setup-wxwidgets.bat`, se necessario.)

Le opzioni A/B/C sotto restano valide per chi preferisce gestire wxWidgets
manualmente (es. build condivisa/DLL, o una wxWidgets già installata altrove).

### Opzione A: wxWidgets compilata dai sorgenti con MinGW (CONFIG mode)

Se wxWidgets è stata compilata con CMake dai sorgenti, ad esempio:

```bat
cd C:\library\wxWidgets
mkdir build_gcc && cd build_gcc
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=ON -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
```

è poi **necessario installarla** (in wx 3.2.x il file `wxWidgetsConfig.cmake` funziona solo dal prefisso di installazione, non dall'albero di build):

```bat
cmake --install C:\library\wxWidgets\build_gcc --prefix C:\library\wx-install
```

quindi compilare AliveMonitor indicando il prefisso:

```bat
cd AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install
cmake --build build -j8
build\AliveMonitor.exe
```

Note per questa modalità:
- con `wxBUILD_SHARED=ON` le DLL di wxWidgets vengono **copiate automaticamente** accanto all'eseguibile in fase di build (disattivabile con `-DAM_COPY_WX_DLLS=OFF`, da usare per build statiche);
- servono a runtime anche le DLL di MinGW (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`): il progetto le linka **staticamente per default** (`AM_STATIC_RUNTIME=ON`, vedi sotto), quindi normalmente non serve copiarle né toccare il `PATH`;
- **usare lo stesso compilatore MinGW** con cui è stata compilata wxWidgets (stessa toolchain nel `PATH`) — un mismatch tra toolchain è la causa più comune dell'errore *"impossibile trovare il punto di ingresso"* all'avvio.

### Build completamente statica (nessuna DLL da distribuire)

Per ottenere un `AliveMonitor.exe` che non dipende da nessuna DLL di terze parti (solo dalle DLL di sistema di Windows), oltre al runtime del compilatore serve compilare anche **wxWidgets in modalità statica**:

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

- `AM_STATIC_RUNTIME` (default **ON**) aggiunge `-static-libgcc -static-libstdc++ -static`, incorporando nell'exe libstdc++/libgcc/winpthread. Disattivabile con `-DAM_STATIC_RUNTIME=OFF` se si preferisce distribuire quelle DLL a parte.
- Con wx statica (`wxBUILD_SHARED=OFF`) il passo di copia delle DLL (`AM_COPY_WX_DLLS`) si disattiva automaticamente: non c'è nulla da copiare.
- Su MSVC lo stesso obiettivo si ottiene con `AM_STATIC_RUNTIME=ON` (CRT `/MT`, automatico) più una wxWidgets/vcpkg compilata con triplet statico, es. `vcpkg install wxwidgets:x64-windows-static`.
- L'eseguibile statico è più grande (il runtime e wx sono incorporati) ma si copia e distribuisce come singolo file, senza rischio di mismatch di versione tra le DLL.

### Opzione B: MSYS2/MinGW con pacchetti precompilati

1. Installare [MSYS2](https://www.msys2.org/), aprire il terminale *MSYS2 MINGW64* ed eseguire:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-wxwidgets3.2-msw
cd /c/percorso/AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AliveMonitor.exe
```

### Opzione C: Visual Studio 2022 + vcpkg

```bat
vcpkg install wxwidgets:x64-windows
cd AliveMonitor
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
build\Release\AliveMonitor.exe
```

Il driver della porta COM di Arduino Uno viene installato automaticamente da Windows 10/11 (per i cloni con CH340 installare il driver del produttore).

## Firmware Arduino

1. Aprire `arduino/AliveMonitor/AliveMonitor.ino` con l'Arduino IDE (≥ 1.8) o `arduino-cli`.
2. Selezionare scheda **Arduino Uno** e la porta corretta.
3. Caricare lo sketch. Fatto: l'applicazione PC troverà la scheda da sola.

Con `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino
```

## Utilizzo

All'avvio il programma scandisce automaticamente le porte seriali e si connette alla scheda che risponde `ARDUINO_UNO` (LED verde in alto a sinistra). *Start* avvia lo streaming alla frequenza impostata con lo SpinCtrl o lo slider (1–250 Hz, modificabile al volo); *Pause* sospende conservando i dati; *Stop* ferma e azzera il buffer. I pulsanti D2–D9 comandano le uscite: il LED accanto a ogni pulsante riflette lo **stato reale** confermato dal firmware. Nel grafico: rotella = zoom sul tempo, Ctrl+rotella = zoom in tensione, trascinamento = pan, *Segui* riaggancia la finestra scorrevole, *Reset* ripristina la vista (ultimi 60 s, 0–5 V). Se la scheda viene scollegata l'applicazione lo segnala e riprende automaticamente la ricerca, ripristinando alla riconnessione le uscite e lo streaming.

### Registrazione CSV continua

Premendo *Start* da acquisizione ferma (non da *Pause*) compare una finestra
per scegliere dove salvare il file, con un nome proposto già pronto
(`acquisizione_AAAA-MM-GG_HH-MM-SS.csv`); annullare la finestra annulla anche
l'avvio dell'acquisizione. Una checkbox "Registra CSV" sotto Start/Pause/Stop
(spuntata di default) permette di saltare del tutto il dialogo e la
registrazione: se smarcata, Start parte subito senza chiedere né scrivere
alcun file. Con la registrazione attiva, ogni campione ADC ricevuto viene
scritto su file in tempo reale, senza bloccare mai il thread seriale: è
implementato con un pattern **produttore/consumatore** (`CsvLoggerController`)
— il thread seriale accoda i campioni in una coda thread-safe (`push()`,
non bloccante), un terzo thread dedicato li preleva e li scrive su disco.
Un LED (verde = in scrittura, rosso = fermo) e un'etichetta con il percorso
completo del file, centrati nella barra del grafico, mostrano lo stato della
registrazione in corso. *Pause* non interrompe il file (resta lo stesso, con
eventuali "buchi" nei tempi di pausa); *Stop* chiude la registrazione
attendendo che il consumatore abbia terminato di scrivere tutti i campioni
già in coda, così nessun dato viene perso né all'arresto manuale né alla
chiusura dell'applicazione.

## Documentazione

La descrizione completa dell'architettura, delle scelte progettuali e dei suggerimenti di estensione è in `docs/Architecture.md`; la specifica del protocollo seriale in `docs/Protocol.md`; i diagrammi UML (classi e sequenza, PlantUML + Mermaid) in `docs/UML/`; la guida passo-passo per compilare con CMake GUI (wxWidgets statica inclusa) in `docs/CMakeGUI.md`.

## Licenza

Codice fornito come base progettuale riutilizzabile; definire la licenza secondo le esigenze del progetto.
