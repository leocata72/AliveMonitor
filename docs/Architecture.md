# Architettura software di AliveMonitor

## Panoramica

AliveMonitor è organizzato rigorosamente secondo il pattern **Model-View-Controller**, con una separazione aggiuntiva per il layer di accesso all'hardware (porta seriale). Il flusso dei dati è unidirezionale e disaccoppiato in ogni fase:

```
Arduino ──USB──▶ ISerialPort ──▶ SerialController (thread seriale)
                                      │ parsing (CommunicationProtocol)
                                      ├──────────────────────────────┐
                                      ▼                              ▼
                              MODEL thread-safe              CsvLoggerController
                    (AnalogDataBuffer, BoardState, ...)     (coda produttore/
                                      ▲            ▲           consumatore)
                     copyWindow 10–60 FPS      snapshot 2 Hz          │
                                      │            │            thread writer
                              GraphController   MainController ──▶ VIEW    │
                                (wxTimer)        (wxTimer + eventi)   file .csv
```

## I tre strati MVC

**MODEL** — stato puro, thread-safe, nessuna dipendenza dalla GUI (eccetto tipi base):
`BoardState` (connessione, acquisizione, frequenze, statistiche, con `Snapshot` atomico), `SerialModel` (baud, porte candidate, porta preferita), `AnalogDataBuffer` (sei `RingBuffer<AnalogSample>` sotto un unico mutex → snapshot coerenti tra canali), `DigitalOutputState` (stato *desiderato* vs *reale* delle uscite), `CommunicationProtocol` (sintassi del protocollo: classe stateless a metodi statici).

**VIEW** — solo presentazione, zero logica:
`MainFrame` (layout), `ToolbarPanel` (stato connessione, LED, Connetti/Disconnetti), `DigitalOutputPanel` (toggle + LED stato reale), `AcquisitionPanel` (Start/Pause/Stop, SpinCtrl+Slider sincronizzati con debounce), `GraphPanel`/`PlotCanvas` (renderer custom), `StatusPanel` (contatori), `SettingsDialog`, `LedIndicator`, `SplashScreen` (schermata di avvio decorativa, autonoma, si chiude da sola). Le View comunicano con i controller **solo** tramite l'interfaccia astratta `IUserActions`.

**CONTROLLER** — coordinamento e logica:
`MainController` (composition root: possiede Model e Controller, implementa `IUserActions`, riceve gli eventi del thread seriale), `SerialController` (possiede il thread seriale), `CommandController` (regole di business dei comandi), `GraphController` (timer di rendering, esportazione PNG), `CsvLoggerController` (registrazione CSV continua, produttore/consumatore su un terzo thread dedicato).

## Threading

Tre thread, con responsabilità nette:

| Thread | Fa | Non fa mai |
|---|---|---|
| **Seriale** | scansione porte, handshake, lettura continua, parsing, aggiornamento Model, `wxQueueEvent` verso la GUI, `push()` del campione verso `CsvLoggerController` | toccare widget, scrivere direttamente su file |
| **CSV writer** (`CsvLoggerController`) | preleva a batch dalla coda produttore/consumatore, formatta e scrive le righe su disco, drena tutta la coda prima di uscire | leggere la porta seriale, toccare widget |
| **Main (GUI)** | eventi utente, rendering, aggiornamento etichette | leggere/scrivere la porta, attendere il thread seriale o il writer CSV |

Meccanismi: i Model sono protetti da mutex a grana fine (lock tenuti per microsecondi); la GUI invia comandi tramite una **coda thread-safe non bloccante** (`enqueueCommand`); il thread seriale notifica la GUI con `wxThreadEvent` accodati nel main loop (throttling a 4 Hz per le statistiche); le attese del thread seriale sono interrompibili via `condition_variable`, quindi la chiusura dell'applicazione è immediata. Il logging CSV segue lo stesso principio produttore/consumatore: `push()` (thread seriale) accoda senza mai bloccare, `writerMain()` (terzo thread) attende su una `condition_variable` e scrive a batch; allo `stop()` la coda viene drenata **prima** di chiudere il file e joinare il thread, garantendo che nessun campione venga perso all'arresto o alla chiusura dell'app.

## Pipeline del grafico: acquisizione ≠ rendering

Requisito chiave: il grafico **non** si aggiorna a ogni campione.

1. **Acquisizione** (250 Hz): il thread seriale fa `push()` nel ring buffer — costo O(1), nessuna allocazione.
2. **Buffer**: `RingBuffer` pre-allocato per 60 s × 500 Hz = 30000 campioni/canale (≈ 2,9 MB totali). Nessun campione va perso: il rendering non tocca mai la cadenza di scrittura.
3. **Rendering** (10–60 FPS, configurabile): `GraphController` con un `wxTimer` chiama `copyWindow()` (copia solo la finestra visibile, riusando la capacità dei vettori) e `Refresh()`. Il paint usa **decimazione min/max per colonna di pixel**: con 15000 punti in finestra si disegnano al massimo ~2×larghezza segmenti, preservando picchi e forma d'onda.

Risultato: a 250 Hz il carico CPU del rendering è indipendente dalla frequenza di acquisizione; a 500 Hz cambia solo la banda seriale (vedi `Protocol.md`), non l'architettura.

## Scelte progettuali (e perché)

- **Renderer custom invece di wxMathPlot/wxCharts**: zero dipendenze esterne (build riproducibile su Windows e Linux con la sola wxWidgets), pieno controllo su decimazione e frequenza di refresh — requisiti difficili da garantire con librerie generiche a 250 Hz.
- **`IUserActions`**: le View non conoscono i controller concreti → testabilità (mock) e nessuna dipendenza circolare.
- **Stato desiderato vs reale delle uscite**: il LED riflette solo l'`OK Dx=v` del firmware; alla riconnessione lo stato desiderato viene ri-applicato automaticamente (la scheda si resetta all'apertura della porta).
- **Protocollo testuale con checksum**: debuggabile con un monitor seriale, ma con integrità verificabile (XOR NMEA-style) per dare significato al contatore "Errori CRC".
- **Streaming lato firmware (`STREAM 1`) invece di polling `GET`**: dimezza il traffico e rende il periodo di campionamento preciso (temporizzato da `micros()` sulla scheda, non dal PC).
- **`Snapshot` del Model**: la GUI legge sempre una copia coerente, mai campi separati con lock multipli.
- **Nessuna variabile globale**: la composition root è `MainController`, posseduto dalla `wxApp` (RAII lungo tutta la catena: distruttore del controller → join del thread → chiusura porta).
- **`std::optional` per l'I/O seriale**: distingue esplicitamente timeout (0 byte, normale) da errore I/O (nullopt → riconnessione), evitando codici d'errore ambigui.
- **Stima dei pacchetti persi dai gap temporali**: il protocollo resta minimale (nessun numero di sequenza); un frame mancante a 250 Hz si manifesta come intervallo ≥ 2 periodi ed è conteggiato.

## Prestazioni a 250 Hz

- Thread seriale: read con timeout 20 ms, parsing `from_chars` senza allocazioni significative; CPU trascurabile.
- `push()`: un lock + 6 scritture O(1).
- Rendering a 30 FPS: copia della finestra (≤ 1,4 MB) + decimazione lineare; la GUI resta fluida perché il main thread non attende mai la seriale.
- Banda: 97,5 kbit/s su 115,2 disponibili.

## Come estendere il progetto

**Nuovi comandi (PWM, servo, ingressi digitali, EEPROM, I²C, SPI):** builder + parsing in `CommunicationProtocol`, metodo in `CommandController`, ramo in `handleCommand()` del firmware, widget nella View. Nessun'altra classe cambia.

**Frequenze fino a 500 Hz:** buffer già dimensionati; alzare `kMaxSampleRateHz`, portare il baud a 230400 (già mappato nel layer seriale, aggiornare anche il firmware) o introdurre frame binari `<len><payload><crc>` dietro la stessa interfaccia `CommunicationProtocol`.

**Altre schede (Mega, Due, GIGA, Portenta):** la risposta all'handshake identifica la scheda — estendere il parsing a `ARDUINO_MEGA` ecc. e parametrizzare numero di canali/pin in una struttura `BoardProfile` usata da Model e View.

**Più schede contemporaneamente:** istanziare N coppie `SerialController`+Model (l'architettura non ha stato condiviso globale); un `MultiBoardController` le aggrega e la View aggiunge una scheda per dispositivo.

**Logging su database:** il logging su file è già implementato così (`CsvLoggerController`, produttore/consumatore su un terzo thread, vedi sezione Threading); lo stesso schema si estende naturalmente a SQLite/PostgreSQL sostituendo `writeRow()` con una scrittura su database, mantenendo invariati coda, `push()` non bloccante e drenaggio garantito allo `stop()`.

**Controllo remoto TCP/IP o MQTT:** `IUserActions` è già l'API interna completa dell'applicazione: un `NetworkController` che riceve comandi di rete può invocarla esattamente come fa la GUI.

**Aggiornamento firmware via seriale:** aggiungere un comando `BOOT` che salta al bootloader (o usare avrdude esternamente sulla porta già individuata dal `SerialModel`).

**Encoder/interrupt:** lato firmware ISR con contatori `volatile`; lato protocollo una riga periodica `ENC,<count>,<rate>*CS` — il parsing rientra nello schema esistente.
