/**
 * @file SerialModel.h
 * @brief Parametri e conoscenza della connessione seriale.
 *
 * MODEL (MVC). Mantiene: baud rate configurato, elenco delle porte candidate
 * rilevate all'ultima scansione e la "porta preferita" (l'ultima con cui
 * l'handshake è riuscito), che viene ritentata per prima alla riconnessione
 * per minimizzare i tempi di ripristino.
 */
#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "Version.h"

namespace am {

class SerialModel {
public:
    SerialModel() = default;

    [[nodiscard]] unsigned baudRate() const;
    void setBaudRate(unsigned baud);

    /// Elenco porte dell'ultima scansione (solo informativo/diagnostico).
    void setAvailablePorts(std::vector<std::string> ports);
    [[nodiscard]] std::vector<std::string> availablePorts() const;

    /// Porta dell'ultima connessione riuscita (vuota se mai connesso).
    void setPreferredPort(const std::string& port);
    [[nodiscard]] std::string preferredPort() const;

    /// Ordina l'elenco candidati mettendo la porta preferita per prima.
    [[nodiscard]] std::vector<std::string>
    prioritized(std::vector<std::string> ports) const;

private:
    mutable std::mutex mutex_;
    unsigned baudRate_ = kDefaultBaudRate;
    std::vector<std::string> availablePorts_;
    std::string preferredPort_;
};

} // namespace am
