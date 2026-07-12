# AliveMonitor

**Italiano** · [English](README.md) · [Français](readme_fr.md) · [Español](readme_es.md) · [Deutsch](readme_de.md)

Applicazione desktop (C++20, wxWidgets 3.2+, CMake, architettura MVC) per l'acquisizione in tempo reale delle tensioni analogiche A0–A5 e il controllo delle uscite digitali D2–D9 di una scheda **Arduino Uno** collegata via USB, con firmware dedicato incluso.
AVVISO: CARICARE PRIMA IL FIRMWARE SU ARDUINO!!
Caratteristiche principali: rilevamento automatico della porta seriale (handshake `HELLO`/`ARDUINO_UNO`) con riconnessione automatica; grafico in tempo reale organizzato in 7 schede — una combinata a sei curve (zoom, pan, autoscale, legenda, griglia) più una scheda per canale con asse Y dedicato nella grandezza fisica convertita, vedi sotto — con esportazione PNG; calibrazione lineare per canale (V → grandezza fisica, vedi sotto); registrazione CSV continua allo Start (produttore/consumatore, vedi sotto); frequenza di acquisizione regolabile da 1 a 250 Hz anche durante l'acquisizione; ring buffer thread-safe da 60 secondi per canale (dimensionato già per 500 Hz); rendering disaccoppiato dall'acquisizione (10–60 FPS configurabili); barra di stato con FPS, pacchetti ricevuti/persi, errori CRC/seriali, tempo di connessione e CPU; interfaccia disponibile in 5 lingue (Italiano, English, Français, Español, Deutsch), selezionabile da Impostazioni (richiede riavvio).

## Struttura del progetto

```
AliveMonitor/
├── CMakeLists.txt          Build system (Windows e Linux)
├── README.md               Questo file (inglese, versione di riferimento)
├── readme_it.md            Versione italiana
├── readme_fr.md            Version française
├── readme_es.md            Versión en español
├── readme_de.md            Deutsche Version
├── LICENSE                 Licenza MIT
├── include/                Header condivisi (Version, Language, eventi, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── i18n/                   Strings.h/.cpp: tabella di traduzione dell'interfaccia
├── model/                  MODEL: BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer,
│                           ChannelCalibration, AppSettings
├── view/                   VIEW: MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, CalibrationPanel, GraphPanel,
│                           StatusPanel, SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER: MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + implementazioni Win32 e POSIX
├── resources/              Risorse incorporate (icone, guida HTML in 5 lingue)
├── installer/              Script Inno Setup per il setup.exe Windows
├── docs/                   Architecture.md, Protocol.md, Translations.md, UML/
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

### Creare l'installer Windows (setup.exe)

Per distribuire AliveMonitor a un utente finale senza fargli installare compilatore o wxWidgets, si può generare un installer autonomo con [Inno Setup](https://jrsoftware.org/isinfo.php) (gratuito, va installato solo sulla macchina che *crea* il setup, non su quella che lo *riceve*):

```bat
scripts\setup-wxwidgets.bat
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-static -j8

powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

Il wrapper `package-windows.ps1` verifica che `build-static\AliveMonitor.exe` esista (serve la **build completamente statica**, senza DLL da cui dipendere: vedi sopra), ricava automaticamente la versione da `include/Version.h`, individua `ISCC.exe` (nel `PATH` o nelle cartelle di installazione predefinite di Inno Setup) e produce `dist\AliveMonitor-Setup-<versione>.exe` — icona, voce nel menu Avvio, collegamento sul desktop opzionale, licenza mostrata durante l'installazione, disinstallatore in "App e funzionalità". Oltre all'eseguibile, l'installer copia anche lo sketch `AliveMonitor.ino` (cartella `arduino\AliveMonitor\`, raggiungibile anche dal menu Avvio): chi installa da questo setup.exe, senza aver clonato il repository, deve comunque poter caricare il firmware sulla scheda.

Lo script Inno Setup (`installer\AliveMonitor.iss`) impacchetta l'**eseguibile già compilato**, non i sorgenti (quelli restano nel repository GitHub): il file `dist\AliveMonitor-Setup-*.exe` prodotto NON va committato — è un artefatto di build (cartella `dist/` già in `.gitignore`), da distribuire a parte, ad esempio come allegato di una GitHub Release.

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

### Calibrazione canali (V → grandezza fisica)

Sotto il pannello di acquisizione è presente una griglia (`CalibrationPanel`)
con una riga per canale (A0..A5) e quattro colonne modificabili: coefficiente
`a`, offset `b`, unità di misura e descrizione. Per ogni trasduttore
collegato con legge lineare G = a·V + b (es. un sensore di temperatura 0–5 V
→ 0–100 °C avrebbe a=20, b=0, unità "°C"), inserire i valori nella riga del
canale corrispondente: l'effetto è immediato sulla legenda del grafico, che
accanto al nome del canale mostra anche il valore convertito in tempo reale
(es. "A0: 23.4 °C"). I canali non configurati restano all'identità (a=1,
b=0, unità "V"), cioè mostrano semplicemente la tensione. La descrizione è
un'etichetta libera (es. "Temperatura forno"): se compilata diventa il
titolo della scheda dedicata a quel canale nel grafico (vedi sotto),
altrimenti la scheda resta col nome del canale ("A0".."A5"). La calibrazione
vale solo per la sessione corrente (non viene salvata su disco); se attiva
una registrazione CSV, il file include — accanto alle sei colonne `A#_V` in
Volt — sei colonne aggiuntive `A#_conv[unità]` con il valore già convertito,
usando la calibrazione impostata al momento dello Start (una modifica fatta
a registrazione già avviata non altera l'intestazione né le righe già
scritte in quella sessione).

### Grafico a 7 schede (assi indipendenti per canale)

Il grafico è un `wxNotebook` con 7 schede. La prima, "Tutti", è il grafico
combinato di sempre: le sei curve condividono un unico asse Y in Volt, con
le checkbox per nascondere/mostrare i singoli canali e lo stato della
registrazione CSV (LED + percorso file). Le altre sei schede (A0..A5)
mostrano un solo canale ciascuna, con curva e asse Y nella grandezza *già
convertita* secondo la sua calibrazione (o in Volt se non calibrato) — un
modo semplice per avere in pratica "assi multipli" quando i canali misurano
grandezze di natura e scala diverse, senza dover sovrapporre unità
incompatibili sullo stesso asse. Il titolo di ciascuna di queste sei schede
è la descrizione impostata nella griglia di calibrazione (colonna
"descrizione"), o il nome del canale se non compilata; cambia in tempo
reale a ogni modifica della griglia, senza bisogno di riavviare. Ogni
scheda a canale singolo ha Auto Y
attivo di default (dato che il range della grandezza convertita non è noto
a priori) e i propri pulsanti Autoscala/Segui/Reset/PNG indipendenti dalle
altre schede; l'esportazione PNG salva sempre la scheda attualmente
visualizzata. La finestra temporale (impostazioni) si applica a tutte le
schede insieme, mentre l'Auto Y continuo del pannello Impostazioni riguarda
solo la scheda combinata — le sei schede per canale restano indipendenti.

### Lingua dell'interfaccia

Dal menu Strumenti &gt; Impostazioni è possibile scegliere la lingua
dell'interfaccia tra Italiano, English, Français, Español e Deutsch. La
scelta viene salvata in `%APPDATA%\AliveMonitor\settings.ini` (Windows) o
`~/.config/AliveMonitor/settings.ini` (Linux) e richiede il riavvio
dell'applicazione per avere effetto: i widget già costruiti non vengono
ritradotti a caldo. La tabella completa delle traduzioni è in
`docs/Translations.md`.

## Documentazione

La descrizione completa dell'architettura, delle scelte progettuali e dei suggerimenti di estensione è in `docs/Architecture.md`; la specifica del protocollo seriale in `docs/Protocol.md`; la tabella delle traduzioni dell'interfaccia in `docs/Translations.md`; i diagrammi UML (classi e sequenza, PlantUML + Mermaid) in `docs/UML/`; la guida passo-passo per compilare con CMake GUI (wxWidgets statica inclusa) in `docs/CMakeGUI.md`.

## Guida in-app

Dal menu Aiuto &gt; Guida (o F1) si apre nel browser predefinito una guida rapida all'uso dell'applicazione, nella lingua corrente dell'interfaccia, incorporata nell'eseguibile come le altre risorse (icona compresa): nessun file esterno da distribuire insieme al binario.

## Licenza

Distribuito sotto licenza **MIT**: uso, modifica e ridistribuzione liberi anche a scopo commerciale, fornito "così com'è" senza alcuna garanzia né responsabilità degli autori per eventuali danni derivanti dall'uso del software. Testo completo in [`LICENSE`](LICENSE).
