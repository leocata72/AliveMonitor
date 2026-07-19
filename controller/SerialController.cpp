/**
 * @file SerialController.cpp
 * @brief Implementazione del thread seriale: scansione, handshake, streaming.
 */
#include "controller/SerialController.h"

#include <algorithm>
#include <cmath>

#include "AppEvents.h"
#include "controller/CsvLoggerController.h"
#include "i18n/Strings.h"
#include "model/CommunicationProtocol.h"

namespace am {
namespace {

/// Attesa dopo l'apertura della porta: Arduino Uno si resetta all'apertura
/// (DTR) e il bootloader impiega fino a ~2 s prima di cedere il controllo
/// allo sketch.
constexpr auto kBootloaderDelay = std::chrono::milliseconds(1800);

/// Timeout di attesa della risposta ARDUINO_UNO dopo un HELLO di handshake.
constexpr auto kHandshakeTimeout = std::chrono::milliseconds(1200);

/// Tentativi di HELLO per porta durante la scansione.
constexpr int kHandshakeAttempts = 2;

/// Silenzio oltre il quale viene inviato un HELLO di keep-alive.
constexpr auto kKeepAliveAfter = std::chrono::milliseconds(1500);

/// Intervallo minimo tra due keep-alive consecutivi.
constexpr auto kKeepAliveSpacing = std::chrono::milliseconds(1000);

/// Silenzio oltre il quale la connessione è considerata caduta.
constexpr auto kLinkTimeout = std::chrono::milliseconds(4000);

/// Pausa tra due cicli di scansione senza esito.
constexpr auto kScanRetryDelay = std::chrono::milliseconds(1000);

/// Timeout della singola read durante il servizio della connessione:
/// piccolo, così i comandi accodati partono con latenza minima.
constexpr auto kServiceReadTimeout = std::chrono::milliseconds(20);

/// Throttling degli eventi statistiche verso la GUI.
constexpr auto kStatsPostInterval = std::chrono::milliseconds(250);

// --- Correzione della deriva del contatore campioni --------------------------
// Il timestamp t = t0 + n/confirmedRate segue l'oscillatore di Arduino
// (risuonatore ceramico, tolleranza tipica ±0.5%), mentre l'asse "now" del
// grafico segue l'orologio del PC: senza correzione, su una sessione lunga la
// differenza si accumula (secondi dopo pochi minuti) e la curva si stacca dal
// bordo destro della finestra in inseguimento. Anche una perdita reale di N
// frame sposta indietro di N periodi tutti i timestamp successivi (n non
// avanza per i frame mai arrivati). computeSampleTimestamp() confronta quindi
// t con acquisitionElapsed() e corregge t0.

/// Fascia morta: sotto questa differenza |t - wall| non si corregge nulla.
/// Deve essere ben più ampia del ritardo di consegna USB (i frame arrivano a
/// raffiche in ritardo di ~20-50 ms rispetto alla generazione) e dei normali
/// "singhiozzi" di scheduling del thread, altrimenti si inseguirebbe il rumore.
constexpr double kSampleClockDeadbandS = 0.25;

/// Oltre questo ritardo (t molto indietro rispetto al PC: perdita massiccia,
/// resume da sospensione...) si riaggancia di colpo invece di trascinare.
/// Solo in avanti: un salto all'indietro violerebbe l'ordine cronologico dei
/// timestamp su cui contano la ricerca binaria di copyWindow() e il CSV.
/// La soglia (2 s) è scelta più ampia del massimo arretrato che la catena di
/// consegna può accumulare SENZA perdite (buffer RX del driver: 16 KiB, vedi
/// SetupComm in SerialPortWindows ~ 1.6 s di frame a 250 Hz): se t è più
/// indietro di così, non è ritardo di consegna, è perdita vera.
constexpr double kSampleClockHardResyncS = 2.0;

/// Correzione dolce: frazione di periodo recuperata a ogni campione quando
/// si è fuori dalla fascia morta. All'1% per campione la correzione vale
/// l'1% del rate (es. 10 ms/s a qualunque frequenza): più che sufficiente a
/// compensare il ±0.5% dell'oscillatore, e abbastanza piccola da mantenere i
/// timestamp strettamente crescenti (l'incremento minimo resta 0.99 periodi).
constexpr double kSampleClockSlewFraction = 0.01;

// --- Stima dei pacchetti persi -----------------------------------------------
/// Tolleranza sul numero di campioni attesi: l'1% assorbe la differenza di
/// clock PC/Arduino (±0.5% + margine) e il jitter degli estremi della
/// finestra di misura. Il rovescio della medaglia, documentato: perdite reali
/// sotto l'1% del rate non vengono conteggiate — limite intrinseco di una
/// stima temporale, eliminabile solo con un numero di sequenza nel protocollo.
constexpr double kLossRateTolerance = 0.01;

/// Credito massimo accumulabile quando arrivano PIÙ campioni degli attesi
/// (clock Arduino veloce): senza questo limite il credito crescerebbe senza
/// fine e maschererebbe indefinitamente le perdite reali future.
constexpr double kMaxLossCredit = 2.0;

/// Soglia di conteggio, in SECONDI di campioni (moltiplicata per il rate):
/// il debito viene convertito in "persi" solo quando supera l'equivalente di
/// 30 ms di flusso (minimo 3 campioni a basse frequenze). Sotto questa soglia
/// non si può distinguere una perdita vera dal jitter di bordo finestra: la
/// consegna USB a raffiche fa oscillare "ricevuti vs attesi" di ± una
/// raffica (~20 ms di campioni), e il battimento fra il periodo di
/// campionamento e quello delle raffiche può produrre SEQUENZE correlate di
/// oscillazioni positive che un semplice pavimento a -kMaxLossCredit non
/// assorbe (verificato in simulazione: a 250 Hz con clock -0.5% comparivano
/// sporadici falsi "persi" anche partendo dal pavimento). Rovescio della
/// medaglia, documentato: un burst di perdita più corto di ~30 ms non viene
/// conteggiato — è il limite di risoluzione intrinseco di questa stima.
constexpr double kLossCountThresholdS = 0.03;

} // namespace

SerialController::SerialController(BoardState& state,
                                   SerialModel& serialModel,
                                   DigitalOutputState& outputs,
                                   AnalogDataBuffer& buffer,
                                   CsvLoggerController& csvLogger)
    : state_(state)
    , serialModel_(serialModel)
    , outputs_(outputs)
    , buffer_(buffer)
    , csvLogger_(csvLogger)
    , port_(ISerialPort::create())
{
}

SerialController::~SerialController()
{
    stop();
}

void SerialController::setEventSink(wxEvtHandler* sink)
{
    sink_ = sink;
}

void SerialController::start()
{
    if (running_.exchange(true)) {
        return;  // già avviato
    }
    thread_ = std::thread(&SerialController::threadMain, this);
}

void SerialController::stop()
{
    if (!running_.exchange(false)) {
        return;
    }
    wakeThread();
    if (thread_.joinable()) {
        thread_.join();
    }
    port_->close();
}

void SerialController::enqueueCommand(std::string commandLine)
{
    {
        const std::lock_guard lock(queueMutex_);
        commandQueue_.push_back(std::move(commandLine));
    }
    wakeThread();
}

void SerialController::setAutoConnect(bool enabled)
{
    autoConnect_.store(enabled);
    wakeThread();
}

void SerialController::wakeThread()
{
    // L'incremento avviene SOTTO wakeMutex_, non fuori come nella versione
    // precedente: per la correttezza di una condition_variable, la modifica
    // dello stato osservato dal predicato deve avvenire sotto lo stesso mutex
    // della wait, altrimenti resta una finestra di lost-wakeup — se
    // l'incremento+notify cadono fra la valutazione del predicato (falso) e il
    // blocco atomico della wait_for, il risveglio va perso e l'attesa dura
    // fino al timeout (fino a ~1.8 s nell'attesa bootloader). Tenendo il mutex
    // qui, l'incremento non può cadere in quella finestra: o precede la
    // valutazione del predicato (che allora lo vede), o attende che il waiter
    // sia effettivamente bloccato (e il notify lo raggiunge). Il notify_all()
    // resta fuori dal lock: notificare a mutex rilasciato è legale ed evita
    // che il thread risvegliato si blocchi subito sul mutex ancora occupato.
    {
        const std::lock_guard lock(wakeMutex_);
        wakeGeneration_.fetch_add(1, std::memory_order_relaxed);
    }
    wakeCv_.notify_all();
}

// ---------------------------------------------------------------------------
// Ciclo principale
// ---------------------------------------------------------------------------

void SerialController::threadMain()
{
    while (running_.load()) {
        if (!autoConnect_.load()) {
            // Disconnessione manuale richiesta dall'utente.
            if (port_->isOpen() || state_.connection() != ConnectionState::Disconnected) {
                port_->close();
                clearCommandQueue();
                outputs_.clearActual();
                state_.setDisconnected();
                postEvent(events::EVT_CONNECTION_CHANGED, 0, 0,
                          tr(StringId::ScDisconnectedByUser));
            }
            interruptibleSleep(std::chrono::milliseconds(200));
            continue;
        }

        if (!port_->isOpen()) {
            scanAndConnect();
        } else {
            serviceConnection();
        }
    }
}

void SerialController::scanAndConnect()
{
    if (state_.connection() != ConnectionState::Scanning) {
        state_.setScanning();
        postEvent(events::EVT_CONNECTION_CHANGED, 0, 0, tr(StringId::ScSearching));
    }

    auto ports = ISerialPort::enumeratePorts();
    serialModel_.setAvailablePorts(ports);
    ports = serialModel_.prioritized(std::move(ports));

    const unsigned baud = serialModel_.baudRate();
    for (const auto& portName : ports) {
        if (!running_.load() || !autoConnect_.load()) {
            return;
        }
        if (tryHandshake(portName, baud)) {
            onConnectionEstablished();
            return;
        }
    }
    interruptibleSleep(kScanRetryDelay);
}

bool SerialController::tryHandshake(const std::string& portName, unsigned baud)
{
    if (!port_->open(portName, baud)) {
        return false;
    }

    // L'apertura ha appena resettato la scheda: attesa del bootloader
    // (interrompibile, per non ritardare la chiusura del programma).
    interruptibleSleep(kBootloaderDelay);
    if (!running_.load() || !autoConnect_.load()) {
        port_->close();
        return false;
    }

    port_->flushInput();
    rxAccumulator_.clear();

    for (int attempt = 0; attempt < kHandshakeAttempts; ++attempt) {
        if (!sendLine(CommunicationProtocol::buildHello())) {
            break;
        }

        const auto deadline = Clock::now() + kHandshakeTimeout;
        while (Clock::now() < deadline && running_.load()) {
            std::uint8_t buf[256];
            const auto n = port_->read(buf, sizeof(buf),
                                       std::chrono::milliseconds(50));
            if (!n) {
                break;  // errore I/O sulla porta
            }
            rxAccumulator_.append(reinterpret_cast<const char*>(buf), *n);

            // Cerca la risposta di handshake tra le righe ricevute.
            std::size_t pos = 0;
            while ((pos = rxAccumulator_.find('\n')) != std::string::npos) {
                const std::string line = rxAccumulator_.substr(0, pos);
                rxAccumulator_.erase(0, pos + 1);
                const auto parsed = CommunicationProtocol::parseLine(line);
                if (parsed && parsed->type == ResponseType::Handshake) {
                    return true;
                }
            }
        }
    }

    port_->close();
    rxAccumulator_.clear();
    return false;
}

void SerialController::onConnectionEstablished()
{
    const std::string portName = port_->portName();
    serialModel_.setPreferredPort(portName);

    state_.setConnected(portName, serialModel_.baudRate());
    state_.resetStatistics();
    resyncSampleClock();
    lastRxTime_ = Clock::now();
    lastPingTime_ = lastRxTime_;

    postEvent(events::EVT_CONNECTION_CHANGED, 1, 0, wxString(portName));

    // Sequenza di sincronizzazione post-connessione: si interrompe al primo
    // errore di scrittura (sendLine() ritorna false e ha già chiamato
    // handleDisconnection(), che chiude la porta) invece di continuare a
    // scrivere sulla porta ormai chiusa. Senza questo controllo, ogni
    // sendLine() successivo falliva a sua volta, ripetendo inutilmente
    // handleDisconnection()/l'evento EVT_CONNECTION_CHANGED e incrementando
    // più volte serialErrors per un solo guasto reale.
    // 1) versione firmware;
    if (!sendLine(CommunicationProtocol::buildVersionRequest())) {
        return;
    }
    // 2) ri-applicazione della configurazione desiderata dei pin digitali
    //    (fondamentale dopo una riconnessione: la scheda si è resettata e
    //    al boot torna con tutti i pin OUTPUT/LOW). Pin in ingresso: DIR I
    //    (il firmware risponde con l'OK e il livello corrente); pin in
    //    uscita: SET dello stato desiderato (DIR O è superfluo al boot).
    const auto desired = outputs_.desiredAll();
    const auto inputs = outputs_.desiredDirectionsAll();
    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        const int pin = kFirstDigitalPin + i;
        const auto idx = static_cast<std::size_t>(i);
        const bool ok = inputs[idx]
            ? sendLine(CommunicationProtocol::buildSetDirection(pin, true))
            : sendLine(CommunicationProtocol::buildSetOutput(pin, desired[idx]));
        if (!ok) {
            return;
        }
    }
    // 3) frequenza di campionamento corrente;
    if (!sendLine(CommunicationProtocol::buildRate(state_.requestedRate()))) {
        return;
    }
    // 4) se l'acquisizione era attiva, riprende automaticamente lo streaming.
    if (state_.acquisition() == AcquisitionState::Running) {
        sendLine(CommunicationProtocol::buildStream(true));
    }
}

void SerialController::serviceConnection()
{
    // 1) Invio dei comandi accodati dalla GUI.
    drainCommandQueue();
    if (!port_->isOpen()) {
        return;  // drainCommandQueue può aver rilevato un errore di scrittura
    }

    // 2) Lettura con timeout breve (mantiene bassa la latenza dei comandi).
    std::uint8_t buf[512];
    const auto n = port_->read(buf, sizeof(buf), kServiceReadTimeout);
    if (!n) {
        state_.incSerialErrors();
        handleDisconnection(tr(StringId::ScIoError).ToStdString());
        return;
    }
    if (*n > 0) {
        rxAccumulator_.append(reinterpret_cast<const char*>(buf), *n);
        lastRxTime_ = Clock::now();
        extractLines();
    }

    // 3) Keep-alive: durante lo streaming il flusso ADC tiene viva la linea;
    //    a streaming fermo, un HELLO periodico verifica che la scheda ci sia.
    const auto now = Clock::now();
    if (now - lastRxTime_ > kKeepAliveAfter
        && now - lastPingTime_ > kKeepAliveSpacing) {
        lastPingTime_ = now;
        sendLine(CommunicationProtocol::buildHello());
    }
    if (now - lastRxTime_ > kLinkTimeout) {
        handleDisconnection(tr(StringId::ScTimeout).ToStdString());
        return;
    }

    // 4) Notifica statistiche alla GUI (throttled).
    maybePostStats();
}

void SerialController::handleDisconnection(const std::string& reason)
{
    port_->close();
    rxAccumulator_.clear();
    clearCommandQueue();
    outputs_.clearActual();
    state_.setScanning();  // si torna subito in ricerca automatica
    postEvent(events::EVT_CONNECTION_CHANGED, 0, 0, wxString(reason));
}

// ---------------------------------------------------------------------------
// Parsing e aggiornamento del Model
// ---------------------------------------------------------------------------

void SerialController::extractLines()
{
    std::size_t pos = 0;
    while ((pos = rxAccumulator_.find('\n')) != std::string::npos) {
        const std::string line = rxAccumulator_.substr(0, pos);
        rxAccumulator_.erase(0, pos + 1);
        processLine(line);
    }
    // Protezione contro flussi senza terminatore (rumore sulla linea).
    if (rxAccumulator_.size() > 4096) {
        rxAccumulator_.clear();
        state_.incSerialErrors();
    }
}

void SerialController::processLine(const std::string& line)
{
    const auto parsed = CommunicationProtocol::parseLine(line);
    if (!parsed) {
        return;  // riga vuota
    }

    switch (parsed->type) {
    case ResponseType::Adc: {
        if (state_.acquisition() == AcquisitionState::Running) {
            const double t = computeSampleTimestamp();
            buffer_.push(t, parsed->adcValues);
            csvLogger_.push(t, parsed->adcValues);  // no-op se non si sta registrando
            updateRateWindow();
        }
        state_.incPacketsReceived();
        break;
    }
    case ResponseType::Handshake:
        // Risposta al keep-alive: nessuna azione, lastRxTime_ è già aggiornato.
        break;

    case ResponseType::Version:
        state_.setFirmwareVersion(parsed->version);
        postEvent(events::EVT_FIRMWARE_INFO, 0, 0, wxString(parsed->version));
        break;

    case ResponseType::OkOutput:
        outputs_.setActual(parsed->pin, parsed->pinState);
        postEvent(events::EVT_OUTPUT_STATE_CHANGED,
                  parsed->pin, parsed->pinState ? 1 : 0);
        break;

    case ResponseType::OkDirection:
        outputs_.setActualDirection(parsed->pin, parsed->isInput);
        break;

    case ResponseType::InputState:
        // Livello di un pin in ingresso: stesso canale eventi dello stato
        // delle uscite — per la GUI è comunque "accendi/spegni il LED del
        // pin", qualunque sia la direzione.
        postEvent(events::EVT_OUTPUT_STATE_CHANGED,
                  parsed->pin, parsed->pinState ? 1 : 0);
        break;

    case ResponseType::OkRate:
        state_.setConfirmedRate(parsed->rateHz);
        resyncSampleClock();  // evita falsi "pacchetti persi" al cambio rate
        postEvent(events::EVT_RATE_CONFIRMED, parsed->rateHz);
        break;

    case ResponseType::OkStream:
        resyncSampleClock();  // riparte il contatore campioni
        break;

    case ResponseType::Error:
        state_.incSerialErrors();
        postEvent(events::EVT_PROTOCOL_ERROR, 0, 0, wxString(parsed->message));
        break;

    case ResponseType::ChecksumError:
        state_.incCrcErrors();
        break;

    case ResponseType::Unknown:
    default:
        state_.incSerialErrors();
        break;
    }
}

double SerialController::computeSampleTimestamp()
{
    if (!sampleClockSynced_) {
        // Difesa contro un frame ADC ricevuto prima di qualunque resync (non
        // dovrebbe succedere: lo streaming parte sempre dopo OK RATE/OK
        // STREAM, vedi onConnectionEstablished/processLine): avvia comunque
        // il contatore da qui invece di propagare un timestamp negativo/nullo.
        resyncSampleClock();
    }
    const int rate = state_.confirmedRate();
    if (rate <= 0) {
        ++sampleIndex_;
        return state_.acquisitionElapsed();  // fallback: rate non ancora confermato
    }

    double t = sampleEpochT0_
        + static_cast<double>(sampleIndex_) / static_cast<double>(rate);

    // Correzione della deriva rispetto all'orologio del PC (vedi le costanti
    // kSampleClock* in testa al file per il perché serve). drift < 0: t è
    // indietro (oscillatore Arduino lento, o frame realmente persi che non
    // hanno fatto avanzare sampleIndex_); drift > 0: t è avanti (oscillatore
    // veloce). Il ritardo di consegna USB rende il drift "a riposo"
    // leggermente negativo: rientra nella fascia morta.
    const double wall = state_.acquisitionElapsed();
    const double drift = t - wall;
    if (drift < -kSampleClockHardResyncS) {
        // Molto indietro: riaggancio immediato IN AVANTI (monotono per
        // costruzione). Il salto sull'asse dei tempi è il male minore
        // rispetto a minuti di recupero al passo dell'1%.
        sampleEpochT0_ = wall;
        sampleIndex_ = 0;
        t = wall;
    } else if (std::abs(drift) > kSampleClockDeadbandS) {
        // Correzione dolce: t0 scivola di una frazione di periodo per
        // campione nella direzione che riduce |drift|. Mai un salto: i
        // timestamp restano strettamente crescenti (incremento minimo
        // (1 - kSampleClockSlewFraction) periodi per campione).
        sampleEpochT0_ += std::copysign(
            kSampleClockSlewFraction / static_cast<double>(rate), -drift);
    }

    ++sampleIndex_;
    return t;
}

void SerialController::resyncSampleClock()
{
    sampleEpochT0_ = state_.acquisitionElapsed();
    sampleIndex_ = 0;
    sampleClockSynced_ = true;
    samplesInWindow_ = 0;
    // Il debito riparte dal PAVIMENTO di credito, non da zero: partendo da 0,
    // nel transitorio iniziale (prima che lo sconto kLossRateTolerance abbia
    // avuto il tempo di spingere il debito sul pavimento, dove il rumore non
    // può più raggiungere la soglia) il jitter di bordo finestra può
    // accumulare qualche falso "perso". Verificato in simulazione: partendo
    // da 0 si contavano 1-2 falsi positivi nei primi istanti di streaming,
    // partendo dal pavimento zero.
    lossDebt_ = -kMaxLossCredit;
    prevWindowLossDebt_ = -kMaxLossCredit;
    // Origine della finestra di misura ANCORA QUI, non al prossimo campione
    // (vedi updateRateWindow() per il perché questo evita un bias sistematico).
    rateWindowStart_ = Clock::now();
}

void SerialController::updateRateWindow()
{
    ++samplesInWindow_;

    const auto now = Clock::now();
    const auto elapsed = now - rateWindowStart_;
    if (elapsed < kStatsPostInterval) {
        return;  // finestra non ancora chiusa: aggregazione prosegue
    }

    const double elapsedS = std::chrono::duration<double>(elapsed).count();
    if (elapsedS > 0.0) {
        state_.setMeasuredRate(static_cast<double>(samplesInWindow_) / elapsedS);

        // Stima dei pacchetti persi: confronto fra i campioni attesi a
        // confirmedRate nella finestra e quelli effettivamente ricevuti.
        // A differenza della vecchia EMA per-riga, il salto temporale fra due
        // frame dello stesso batch USB (dt ~ pochi microsecondi) o fra due
        // batch successivi (dt ~ periodo firmware, per via del buffering
        // seriale) non genera più falsi positivi: qui si guarda solo il
        // totale su una finestra di ~250 ms, non l'intervallo istantaneo.
        //
        // Raffinamenti rispetto al semplice llround(expected) - received:
        //  1) accumulatore frazionario (lossDebt_) che si trascina il resto
        //     fra una finestra e l'altra: il rumore di arrotondamento ai
        //     bordi (che una soglia per-finestra conterebbe solo quando è
        //     positivo, gonfiando il totale) si compensa da solo;
        //  2) attesi scontati di kLossRateTolerance: il clock di Arduino non
        //     è quello del PC (vedi la costante);
        //  3) conteggio solo oltre kLossCountThresholdS e solo se CONFERMATO
        //     su due finestre consecutive (vedi sotto).
        const int rate = state_.confirmedRate();
        if (rate > 0) {
            lossDebt_ += elapsedS * static_cast<double>(rate)
                             * (1.0 - kLossRateTolerance)
                       - static_cast<double>(samplesInWindow_);
            if (lossDebt_ < -kMaxLossCredit) {
                // Un clock Arduino veloce consegna PIÙ campioni degli attesi:
                // senza pavimento, il credito crescerebbe indefinitamente e
                // assorbirebbe qualunque perdita reale futura.
                lossDebt_ = -kMaxLossCredit;
            }
            // Conteggio DIFFERITO di una finestra: si converte in "persi"
            // solo il debito confermato da due finestre consecutive (il
            // minimo fra la corrente e la precedente). Motivo: uno stallo di
            // consegna (GUI/SO congelati per un attimo) fa chiudere una
            // finestra lunga con pochissimi campioni (debito enorme), ma la
            // raffica di recupero arriva nella finestra SUCCESSIVA: contare
            // subito trasformerebbe lo stallo in centinaia di falsi "persi",
            // perché il rimborso della raffica arriverebbe a conteggio già
            // fatto (e finirebbe pure troncato dal pavimento di credito). Una
            // perdita VERA, invece, lascia il debito alto anche nella
            // finestra successiva (nessuna raffica lo rimborsa): il minimo su
            // due finestre distingue esattamente i due casi, al prezzo di
            // ~250 ms di ritardo nell'aggiornamento del contatore.
            const double confirmed = std::min(lossDebt_, prevWindowLossDebt_);
            prevWindowLossDebt_ = lossDebt_;
            const double countThreshold = std::max(
                3.0, static_cast<double>(rate) * kLossCountThresholdS);
            if (confirmed >= countThreshold) {
                const auto lost = static_cast<std::uint64_t>(confirmed);
                state_.addPacketsLost(lost);
                lossDebt_ -= static_cast<double>(lost);
                prevWindowLossDebt_ = lossDebt_;
            }
        }
    }

    samplesInWindow_ = 0;
    // La finestra successiva riparte da QUESTO istante (l'arrivo del
    // campione che ha appena chiuso la finestra corrente), non da quello del
    // prossimo campione: se l'anchor fosse invece impostata al PRIMO
    // campione della nuova finestra (come nella versione precedente), N
    // campioni contati coprirebbero solo N-1 intervalli di tempo reali (il
    // primo campione della finestra non ha un intervallo "prima di sé"
    // all'interno della finestra), facendo sistematicamente misurare una
    // frequenza più alta di quella vera per un fattore N/(N-1) — piccolo ma
    // non trascurabile a basse frequenze (es. +4% a 100 Hz con finestre da
    // 250 ms). Ancorando qui invece, ogni finestra copre esattamente il
    // numero di campioni attesi nel suo intervallo di tempo, senza bias.
    rateWindowStart_ = now;
}

// ---------------------------------------------------------------------------
// Utilità
// ---------------------------------------------------------------------------

bool SerialController::sendLine(const std::string& command)
{
    const std::string wire = command + "\n";
    if (!port_->write(wire.data(), wire.size())) {
        state_.incSerialErrors();
        handleDisconnection(tr(StringId::ScWriteError).ToStdString());
        return false;
    }
    return true;
}

void SerialController::drainCommandQueue()
{
    std::deque<std::string> pending;
    {
        const std::lock_guard lock(queueMutex_);
        pending.swap(commandQueue_);
    }
    for (const auto& cmd : pending) {
        if (!port_->isOpen() || !sendLine(cmd)) {
            return;
        }
    }
}

void SerialController::clearCommandQueue()
{
    const std::lock_guard lock(queueMutex_);
    commandQueue_.clear();
}

void SerialController::interruptibleSleep(std::chrono::milliseconds ms)
{
    // Il predicato originale controllava solo !running_: i notify_all() di
    // enqueueCommand()/setAutoConnect() risvegliavano wait_for(), ma con la
    // condizione ancora falsa il predicato la faceva riaddormentare subito
    // (wait_for() reinterpreta un "risveglio spurio" tornando ad aspettare),
    // rendendo l'attesa NON interrompibile come documentato: es. "Disconnetti"
    // durante i ~1.8 s di attesa bootloader restava senza effetto fino alla
    // scadenza del timeout. wakeGeneration_ (incrementato da wakeThread(),
    // SOTTO wakeMutex_: vedi lì per la finestra di lost-wakeup che questo
    // chiude) rende il predicato vero anche per un risveglio "di comando",
    // non solo per lo stop. Lo snapshot è preso PRIMA di acquisire il lock:
    // così anche un wake arrivato fra lo snapshot e l'ingresso in wait_for
    // viene onorato (predicato subito vero), mentre i wake precedenti allo
    // snapshot sono deliberatamente ignorati (stato già visto dal chiamante).
    const std::uint64_t generationAtStart =
        wakeGeneration_.load(std::memory_order_relaxed);
    std::unique_lock lock(wakeMutex_);
    wakeCv_.wait_for(lock, ms, [this, generationAtStart] {
        return !running_.load()
            || wakeGeneration_.load(std::memory_order_relaxed) != generationAtStart;
    });
}

void SerialController::postEvent(const wxEventTypeTag<wxThreadEvent>& type,
                                 int intPayload, long longPayload,
                                 const wxString& strPayload)
{
    if (sink_ == nullptr) {
        return;
    }
    auto* evt = new wxThreadEvent(type);
    evt->SetInt(intPayload);
    evt->SetExtraLong(longPayload);
    evt->SetString(strPayload);
    wxQueueEvent(sink_, evt);  // thread-safe: accoda nel main loop della GUI
}

void SerialController::maybePostStats()
{
    const auto now = Clock::now();
    if (now - lastStatsPost_ >= kStatsPostInterval) {
        lastStatsPost_ = now;
        postEvent(events::EVT_STATS_UPDATED);
    }
}

} // namespace am
