/**
 * @file AnalogDataBuffer.h
 * @brief Contenitore thread-safe dei campioni analogici (A0..A5).
 *
 * MODEL (MVC). Un ring buffer per canale, protetti da un unico mutex per
 * garantire snapshot coerenti tra i sei canali. Il thread seriale scrive
 * (push), il thread GUI legge (copyWindow / exportCsv). Le copie riusano
 * la capacità dei vettori di destinazione: a regime nessuna allocazione.
 */
#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>
#include <ostream>
#include <vector>

#include "Version.h"
#include "model/RingBuffer.h"

namespace am {

/// Singolo campione di un canale analogico.
struct AnalogSample {
    double t = 0.0;          ///< Tempo [s] dall'inizio dell'acquisizione.
    std::uint16_t raw = 0;   ///< Valore ADC grezzo (0..1023).

    /// Conversione in tensione [V].
    [[nodiscard]] double volts() const noexcept
    {
        return static_cast<double>(raw) * kAdcReferenceVolt / kAdcMaxValue;
    }
};

class AnalogDataBuffer {
public:
    /// @param capacityPerChannel campioni per canale (default: 60 s a 500 Hz).
    explicit AnalogDataBuffer(std::size_t capacityPerChannel = kRingBufferCapacity);

    /// Inserisce un campione per ognuno dei sei canali (stesso timestamp).
    /// Chiamato dal thread seriale a ogni frame ADC ricevuto.
    void push(double t, const std::array<std::uint16_t, kNumAnalogChannels>& values);

    /// Copia nei vettori di destinazione tutti i campioni con t >= tMin.
    /// I vettori vengono svuotati ma NON deallocati (riuso della capacità).
    void copyWindow(double tMin,
                    std::array<std::vector<AnalogSample>, kNumAnalogChannels>& out) const;

    /// Timestamp del campione più recente (nullopt se vuoto).
    [[nodiscard]] std::optional<double> latestTime() const;

    /// Statistiche di un canale sugli ULTIMI n campioni (v1.2).
    struct ChannelStats {
        double meanVolts = 0.0;    ///< Media aritmetica [V].
        double stddevVolts = 0.0;  ///< Deviazione standard di popolazione [V].
        double minVolts = 0.0;     ///< Minimo [V].
        double maxVolts = 0.0;     ///< Massimo [V].
        std::size_t count = 0;     ///< Campioni effettivamente usati (<= n).
    };

    /// Media, deviazione standard, minimo e massimo degli ultimi n campioni
    /// di ogni canale, a ritroso dal più recente. Se i campioni disponibili
    /// sono meno di n si usano quelli che ci sono (count lo riporta);
    /// count==0 = buffer vuoto. Snapshot coerente fra i sei canali (unico lock).
    [[nodiscard]] std::array<ChannelStats, kNumAnalogChannels>
    lastStats(std::size_t n) const;

    /// Numero di campioni attualmente memorizzati per canale.
    [[nodiscard]] std::size_t sizePerChannel() const;

    /// Svuota tutti i canali (nessuna deallocazione).
    void clear();

    /// Esporta l'intero contenuto in CSV: header + una riga per campione
    /// (tempo + tensione dei sei canali). Ritorna false su errore di stream.
    bool exportCsv(std::ostream& os) const;

private:
    mutable std::mutex mutex_;
    std::array<RingBuffer<AnalogSample>, kNumAnalogChannels> channels_;
};

} // namespace am
