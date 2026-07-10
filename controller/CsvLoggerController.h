/**
 * @file CsvLoggerController.h
 * @brief Logging asincrono dei campioni su CSV: produttore/consumatore.
 *
 * CONTROLLER (MVC). Il thread seriale (produttore) non deve mai attendere
 * l'I/O su disco: push() accoda il campione (breve lock, nessuna I/O) e
 * ritorna subito. Un thread dedicato (consumatore), avviato da start(),
 * consuma la coda e scrive sul file. stop() segnala la chiusura e ATTENDE
 * che il consumatore abbia scritto tutte le righe ancora in coda prima di
 * ritornare: nessun dato accodato viene perso alla chiusura o all'uscita
 * dall'applicazione.
 */
#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>

#include <wx/string.h>

#include "Version.h"
#include "model/ChannelCalibration.h"

namespace am {

class CsvLoggerController {
public:
    CsvLoggerController() = default;
    ~CsvLoggerController();

    CsvLoggerController(const CsvLoggerController&) = delete;
    CsvLoggerController& operator=(const CsvLoggerController&) = delete;

    /// Apre il file (sovrascrivendolo se già esiste, l'utente conferma
    /// l'overwrite nel wxFileDialog a monte) e avvia il thread consumatore.
    /// Se una registrazione era già attiva la chiude prima (stop()).
    /// @param calibrations copiata internamente (istantanea congelata
    ///        all'avvio): modifiche successive nella griglia non alterano
    ///        l'intestazione né le colonne già scritte in questa sessione.
    /// @return false se il file non può essere aperto in scrittura.
    bool start(const wxString& path, const ChannelCalibrations& calibrations);

    /// Segnala la fine della registrazione, ATTENDE che il consumatore
    /// scriva tutte le righe ancora in coda, chiude il file. Bloccante ma
    /// rapido nell'uso normale (il consumatore scrive più velocemente di
    /// quanto arrivino i campioni, la coda è quasi sempre vuota). Idempotente:
    /// chiamabile anche se non c'è una registrazione attiva.
    void stop();

    [[nodiscard]] bool isActive() const { return active_.load(); }
    [[nodiscard]] wxString currentPath() const { return currentPath_; }

    /// Accoda un campione (chiamato dal thread seriale, il produttore). Non
    /// bloccante (a parte la breve durata del lock) e no-op se non c'è una
    /// registrazione attiva: sicuro da chiamare sempre, anche a freddo.
    void push(double t, const std::array<std::uint16_t, kNumAnalogChannels>& values);

private:
    struct Row {
        double t;
        std::array<std::uint16_t, kNumAnalogChannels> raw;
    };

    void writerMain();
    void writeRow(const Row& row);

    std::ofstream file_;
    wxString currentPath_;
    ChannelCalibrations calibrations_;  ///< Istantanea congelata all'avvio di start().

    std::thread thread_;
    std::atomic<bool> active_{ false };
    bool stopping_ = false;  ///< Accesso protetto da queueMutex_.

    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::deque<Row> queue_;
};

} // namespace am
