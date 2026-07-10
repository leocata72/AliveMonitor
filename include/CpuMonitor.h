/**
 * @file CpuMonitor.h
 * @brief Misura dell'utilizzo CPU del processo corrente (se disponibile).
 *
 * Windows: GetProcessTimes(). Linux: /proc/self/stat.
 * Se la piattaforma non fornisce il dato, sample() restituisce std::nullopt
 * e la GUI mostra "n/d".
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace am {

class CpuMonitor {
public:
    CpuMonitor();

    /// Percentuale CPU del processo (0..100*nCore) dall'ultima chiamata.
    /// Restituisce nullopt se il dato non è disponibile o se l'intervallo
    /// dall'ultima misura è troppo breve per essere significativo (< 200 ms
    /// -> viene restituito l'ultimo valore valido).
    [[nodiscard]] std::optional<double> sample();

private:
    std::chrono::steady_clock::time_point lastWall_;
    std::uint64_t lastProcTicks_ = 0;   ///< Tempo CPU cumulativo (unità piattaforma).
    std::optional<double> lastValue_;   ///< Ultimo valore valido calcolato.
    bool valid_ = false;                ///< Prima lettura effettuata con successo.

    /// Tempo CPU cumulativo del processo in microsecondi (nullopt se non disponibile).
    [[nodiscard]] static std::optional<std::uint64_t> processCpuMicros();
};

} // namespace am
