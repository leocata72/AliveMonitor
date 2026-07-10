/**
 * @file BoardState.h
 * @brief Stato applicativo della scheda Arduino e statistiche di sessione.
 *
 * MODEL (MVC). Classe thread-safe: il thread seriale scrive, la GUI legge.
 * La GUI ottiene sempre uno Snapshot atomico e coerente tramite snapshot(),
 * evitando letture "a metà" di campi correlati.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

#include "Version.h"

namespace am {

/// Stato della connessione seriale.
enum class ConnectionState {
    Disconnected,  ///< Nessuna porta aperta, ricerca sospesa dall'utente.
    Scanning,      ///< Ricerca automatica di Arduino in corso.
    Connected      ///< Handshake completato, comunicazione attiva.
};

/// Stato dell'acquisizione dati.
enum class AcquisitionState {
    Stopped,  ///< Ferma; il buffer viene azzerato allo Stop.
    Running,  ///< Streaming attivo alla frequenza impostata.
    Paused    ///< Streaming sospeso; buffer e asse dei tempi conservati.
};

/// Contatori diagnostici di sessione.
struct Statistics {
    std::uint64_t packetsReceived = 0;  ///< Frame ADC validi ricevuti.
    std::uint64_t packetsLost = 0;      ///< Stimati da buchi temporali.
    std::uint64_t crcErrors = 0;        ///< Checksum XOR non valido.
    std::uint64_t serialErrors = 0;     ///< Righe malformate / errori I/O / ERR.
};

class BoardState {
public:
    /// Copia atomica e coerente dell'intero stato, per la GUI.
    struct Snapshot {
        ConnectionState connection = ConnectionState::Scanning;
        std::string portName;
        unsigned baudRate = kDefaultBaudRate;
        std::string firmwareVersion;

        AcquisitionState acquisition = AcquisitionState::Stopped;
        int requestedRateHz = kDefaultSampleRateHz;  ///< Impostata dall'utente.
        int confirmedRateHz = 0;                     ///< Confermata dal firmware (OK RATE=).
        double measuredRateHz = 0.0;                 ///< Effettivamente misurata.

        Statistics stats;
        double connectedSeconds = 0.0;  ///< Durata connessione corrente (0 se non connesso).
    };

    BoardState() = default;

    // --- Connessione (thread seriale) ---------------------------------------
    void setScanning();
    void setConnected(const std::string& portName, unsigned baudRate);
    void setDisconnected();
    void setFirmwareVersion(const std::string& version);

    // --- Acquisizione ---------------------------------------------------------
    void setAcquisition(AcquisitionState s);
    /// Fissa l'origine dell'asse dei tempi (chiamata allo Start da Stopped).
    void restartAcquisitionClock();
    /// Secondi trascorsi dall'origine dell'asse dei tempi.
    [[nodiscard]] double acquisitionElapsed() const;

    // --- Frequenza --------------------------------------------------------------
    void setRequestedRate(int hz);
    void setConfirmedRate(int hz);
    void setMeasuredRate(double hz);
    [[nodiscard]] int requestedRate() const;
    [[nodiscard]] AcquisitionState acquisition() const;
    [[nodiscard]] ConnectionState connection() const;
    [[nodiscard]] int confirmedRate() const;

    // --- Statistiche (thread seriale) ---------------------------------------------
    void incPacketsReceived();
    void addPacketsLost(std::uint64_t n);
    void incCrcErrors();
    void incSerialErrors();
    void resetStatistics();

    /// Copia coerente per la GUI.
    [[nodiscard]] Snapshot snapshot() const;

private:
    using Clock = std::chrono::steady_clock;

    mutable std::mutex mutex_;

    ConnectionState connection_ = ConnectionState::Scanning;
    std::string portName_;
    unsigned baudRate_ = kDefaultBaudRate;
    std::string firmwareVersion_;
    Clock::time_point connectedSince_{};
    bool hasConnectedSince_ = false;

    AcquisitionState acquisition_ = AcquisitionState::Stopped;
    Clock::time_point acquisitionStart_{ Clock::now() };
    int requestedRateHz_ = kDefaultSampleRateHz;
    int confirmedRateHz_ = 0;
    double measuredRateHz_ = 0.0;

    Statistics stats_;
};

} // namespace am
