/**
 * @file SerialController.h
 * @brief Controller della comunicazione seriale: possiede il thread seriale.
 *
 * CONTROLLER (MVC). Responsabilità:
 *  - scansione automatica delle porte e handshake HELLO/ARDUINO_UNO;
 *  - lettura continua, ricomposizione delle righe, parsing (via
 *    CommunicationProtocol) e aggiornamento del Model;
 *  - invio dei comandi accodati dalla GUI (coda thread-safe);
 *  - rilevazione dello scollegamento e riconnessione automatica;
 *  - notifica alla GUI tramite wxThreadEvent (mai accesso diretto ai widget).
 *
 * Il thread GUI NON attende mai la seriale: interagisce solo con
 * enqueueCommand() (non bloccante) e con i Model thread-safe.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <wx/event.h>

#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"
#include "model/DigitalOutputState.h"
#include "model/SerialModel.h"
#include "serial/ISerialPort.h"

namespace am {

class CsvLoggerController;  // forward: incluso solo nel .cpp

class SerialController {
public:
    SerialController(BoardState& state,
                     SerialModel& serialModel,
                     DigitalOutputState& outputs,
                     AnalogDataBuffer& buffer,
                     CsvLoggerController& csvLogger);

    /// Il distruttore ferma e attende il thread (RAII).
    ~SerialController();

    SerialController(const SerialController&) = delete;
    SerialController& operator=(const SerialController&) = delete;

    /// Destinatario degli eventi (MainController). Da impostare prima di start().
    void setEventSink(wxEvtHandler* sink);

    /// Avvia il thread seriale.
    void start();

    /// Ferma il thread e chiude la porta. Bloccante ma rapido (< ~100 ms).
    void stop();

    /// Accoda una riga di comando (senza '\n'). Non bloccante, thread-safe.
    void enqueueCommand(std::string commandLine);

    /// true  -> ricerca/connessione automatica attiva (pulsante "Connetti");
    /// false -> disconnessione e ricerca sospesa (pulsante "Disconnetti").
    void setAutoConnect(bool enabled);

private:
    using Clock = std::chrono::steady_clock;

    // --- Ciclo principale del thread ----------------------------------------
    void threadMain();
    void scanAndConnect();
    bool tryHandshake(const std::string& portName, unsigned baud);
    void onConnectionEstablished();
    void serviceConnection();
    void handleDisconnection(const std::string& reason);

    // --- Parsing e Model ------------------------------------------------------
    void extractLines();
    void processLine(const std::string& line);
    void updateSampleTiming(double t);

    // --- Utilità ----------------------------------------------------------------
    bool sendLine(const std::string& command);
    void drainCommandQueue();
    void clearCommandQueue();
    /// Attesa interrompibile da stop() e dall'arrivo di comandi.
    void interruptibleSleep(std::chrono::milliseconds ms);
    void postEvent(const wxEventTypeTag<wxThreadEvent>& type,
                   int intPayload = 0, long longPayload = 0,
                   const wxString& strPayload = wxString());
    void maybePostStats();

    // --- Riferimenti al Model (posseduti da MainController) --------------------
    BoardState& state_;
    SerialModel& serialModel_;
    DigitalOutputState& outputs_;
    AnalogDataBuffer& buffer_;
    CsvLoggerController& csvLogger_;  ///< Produttore: push() per ogni campione ADC.

    // --- Porta e thread -----------------------------------------------------------
    std::unique_ptr<ISerialPort> port_;
    std::thread thread_;
    std::atomic<bool> running_{ false };
    std::atomic<bool> autoConnect_{ true };
    wxEvtHandler* sink_ = nullptr;  ///< Impostato prima di start(), poi immutato.

    // --- Coda comandi (GUI -> thread seriale) ---------------------------------------
    std::mutex queueMutex_;
    std::deque<std::string> commandQueue_;

    // --- Sveglia del thread -------------------------------------------------------
    std::mutex wakeMutex_;
    std::condition_variable wakeCv_;

    // --- Stato interno del thread (acceduto SOLO dal thread seriale) ----------------
    std::string rxAccumulator_;      ///< Byte ricevuti in attesa di '\n'.
    Clock::time_point lastRxTime_{};
    Clock::time_point lastPingTime_{};
    Clock::time_point lastStatsPost_{};
    double lastSampleT_ = -1.0;      ///< Timestamp ultimo campione (per stima perdite).
    double rateEma_ = 0.0;           ///< Media mobile esponenziale della frequenza.
};

} // namespace am
