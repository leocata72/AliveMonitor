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
#include <cstdint>
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
    /// Timestamp del campione ADC corrente, derivato dal contatore di
    /// campioni (non dall'istante di arrivo/parsing: vedi sampleIndex_).
    double computeSampleTimestamp();
    /// Rifissa l'origine del contatore di campioni sull'istante corrente.
    /// Chiamata su OK RATE / OK STREAM (che sul firmware sono esattamente
    /// gli istanti in cui nextSampleAtUs viene riallineato, vedi
    /// AliveMonitor.ino) e alla connessione.
    void resyncSampleClock();
    /// Aggiorna la finestra di misura frequenza/perdite (~250 ms) con un
    /// nuovo campione ricevuto.
    void updateRateWindow();

    // --- Utilità ----------------------------------------------------------------
    bool sendLine(const std::string& command);
    void drainCommandQueue();
    void clearCommandQueue();
    /// Attesa interrompibile da stop() e dall'arrivo di comandi.
    void interruptibleSleep(std::chrono::milliseconds ms);
    /// Risveglia interruptibleSleep(): incrementa wakeGeneration_ sotto
    /// wakeMutex_ (obbligatorio per non perdere il risveglio, vedi il .cpp)
    /// e notifica. Chiamata da stop()/enqueueCommand()/setAutoConnect().
    void wakeThread();
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
    /// Incrementato da wakeThread() (sempre sotto wakeMutex_, mai fuori:
    /// vedi wakeThread() nel .cpp per il perché) ad ogni richiesta di
    /// risveglio da stop()/enqueueCommand()/setAutoConnect(): permette a
    /// interruptibleSleep() di distinguere un risveglio "vero" da uno
    /// spurio, dato che il predicato originale (solo !running_) ignorava
    /// questi notify (vedi interruptibleSleep()).
    std::atomic<std::uint64_t> wakeGeneration_{ 0 };

    // --- Stato interno del thread (acceduto SOLO dal thread seriale) ----------------
    std::string rxAccumulator_;      ///< Byte ricevuti in attesa di '\n'.
    Clock::time_point lastRxTime_{};
    Clock::time_point lastPingTime_{};
    Clock::time_point lastStatsPost_{};

    // --- Temporizzazione campioni: contatore, non istante di arrivo USB -------------
    // La seriale/USB consegna i frame a raffiche (più frame nello stesso
    // batch con dt di pochi microsecondi, poi un salto di alcuni ms): usare
    // l'istante di parsing come timestamp produce un asse dei tempi (e una
    // colonna tempo_s nel CSV) non uniforme. Il timestamp è invece derivato
    // da t0 + n/confirmedRate, risincronizzato su OK RATE/OK STREAM e
    // corretto in modo dolce contro la deriva clock Arduino/PC (vedi
    // computeSampleTimestamp() e le costanti kSampleClock* nel .cpp).
    std::uint64_t sampleIndex_ = 0;   ///< Campioni emessi dall'ultimo resync.
    double sampleEpochT0_ = 0.0;      ///< acquisitionElapsed() al resync.
    bool sampleClockSynced_ = false;  ///< false finché non c'è stato un resync.

    // --- Misura frequenza/perdite: finestra a tempo reale (~250 ms) -----------------
    // Non una EMA per-riga (che erediterebbe lo stesso problema dei "burst":
    // frequenze istantanee fittizie e falsi "pacchetti persi" sul gap fra
    // batch), ma un conteggio aggregato su finestre temporali confrontato
    // col numero di campioni atteso a confirmedRate.
    Clock::time_point rateWindowStart_{};
    std::uint64_t samplesInWindow_ = 0;
    /// Accumulatore frazionario "attesi - ricevuti" trascinato fra le
    /// finestre di misura: le perdite vengono contate solo quando il debito,
    /// CONFERMATO su due finestre consecutive (vedi prevWindowLossDebt_),
    /// supera la soglia; il resto resta qui (vedi updateRateWindow()).
    double lossDebt_ = 0.0;
    /// Debito alla chiusura della finestra precedente: il conteggio usa
    /// min(corrente, precedente) per non scambiare uno stallo di consegna
    /// (debito alto per UNA finestra, rimborsato dalla raffica di recupero
    /// nella successiva) per una perdita reale (debito alto persistente).
    double prevWindowLossDebt_ = 0.0;
};

} // namespace am
