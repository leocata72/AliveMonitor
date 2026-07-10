/**
 * @file SerialController.cpp
 * @brief Implementazione del thread seriale: scansione, handshake, streaming.
 */
#include "controller/SerialController.h"

#include <cmath>

#include "AppEvents.h"
#include "controller/CsvLoggerController.h"
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
    wakeCv_.notify_all();
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
    wakeCv_.notify_all();
}

void SerialController::setAutoConnect(bool enabled)
{
    autoConnect_.store(enabled);
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
                          "Disconnesso dall'utente");
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
        postEvent(events::EVT_CONNECTION_CHANGED, 0, 0, "Ricerca di Arduino...");
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
    lastSampleT_ = -1.0;
    rateEma_ = 0.0;
    lastRxTime_ = Clock::now();
    lastPingTime_ = lastRxTime_;

    postEvent(events::EVT_CONNECTION_CHANGED, 1, 0, wxString(portName));

    // Sequenza di sincronizzazione post-connessione:
    // 1) versione firmware;
    sendLine(CommunicationProtocol::buildVersionRequest());
    // 2) ri-applicazione dello stato desiderato delle uscite digitali
    //    (fondamentale dopo una riconnessione: la scheda si è resettata);
    const auto desired = outputs_.desiredAll();
    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        sendLine(CommunicationProtocol::buildSetOutput(
            kFirstDigitalPin + i, desired[static_cast<std::size_t>(i)]));
    }
    // 3) frequenza di campionamento corrente;
    sendLine(CommunicationProtocol::buildRate(state_.requestedRate()));
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
        handleDisconnection("Errore I/O sulla porta seriale");
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
        handleDisconnection("Nessuna risposta da Arduino (timeout)");
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
        const double t = state_.acquisitionElapsed();
        if (state_.acquisition() == AcquisitionState::Running) {
            buffer_.push(t, parsed->adcValues);
            csvLogger_.push(t, parsed->adcValues);  // no-op se non si sta registrando
            updateSampleTiming(t);
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

    case ResponseType::OkRate:
        state_.setConfirmedRate(parsed->rateHz);
        lastSampleT_ = -1.0;  // evita falsi "pacchetti persi" al cambio rate
        postEvent(events::EVT_RATE_CONFIRMED, parsed->rateHz);
        break;

    case ResponseType::OkStream:
        lastSampleT_ = -1.0;  // riparte la misura degli intervalli
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

void SerialController::updateSampleTiming(double t)
{
    if (lastSampleT_ >= 0.0) {
        const double dt = t - lastSampleT_;
        if (dt > 0.0) {
            // Frequenza misurata: media mobile esponenziale (reattiva ma stabile).
            const double inst = 1.0 / dt;
            rateEma_ = (rateEma_ <= 0.0) ? inst : (0.95 * rateEma_ + 0.05 * inst);
            state_.setMeasuredRate(rateEma_);

            // Stima dei pacchetti persi dai "buchi" temporali: se l'intervallo
            // supera nettamente il periodo nominale, dei frame sono andati persi.
            const int rate = state_.confirmedRate();
            if (rate > 0) {
                const double period = 1.0 / rate;
                if (dt > 1.75 * period) {
                    const auto missed =
                        static_cast<std::uint64_t>(std::llround(dt / period)) - 1U;
                    if (missed > 0) {
                        state_.addPacketsLost(missed);
                    }
                }
            }
        }
    }
    lastSampleT_ = t;
}

// ---------------------------------------------------------------------------
// Utilità
// ---------------------------------------------------------------------------

bool SerialController::sendLine(const std::string& command)
{
    const std::string wire = command + "\n";
    if (!port_->write(wire.data(), wire.size())) {
        state_.incSerialErrors();
        handleDisconnection("Errore di scrittura sulla porta seriale");
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
    std::unique_lock lock(wakeMutex_);
    wakeCv_.wait_for(lock, ms, [this] { return !running_.load(); });
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
