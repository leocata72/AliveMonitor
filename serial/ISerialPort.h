/**
 * @file ISerialPort.h
 * @brief Interfaccia astratta della porta seriale (RAII, multipiattaforma).
 *
 * Le implementazioni concrete sono SerialPortWindows (Win32 API) e
 * SerialPortPosix (termios). Il resto del programma dipende SOLO da questa
 * interfaccia: la factory statica create() restituisce l'implementazione
 * corretta per la piattaforma corrente (una sola viene compilata, selezionata
 * da CMake).
 *
 * RAII: la distruzione dell'oggetto chiude sempre la porta.
 */
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace am {

class ISerialPort {
public:
    virtual ~ISerialPort() = default;

    /// Apre la porta a 8N1 con il baud indicato, DTR asserito
    /// (su Arduino Uno l'asserzione di DTR provoca il reset automatico).
    /// @return true se l'apertura e la configurazione riescono.
    virtual bool open(const std::string& portName, unsigned baudRate) = 0;

    /// Chiude la porta (idempotente).
    virtual void close() = 0;

    [[nodiscard]] virtual bool isOpen() const = 0;

    /// Nome della porta attualmente aperta (vuoto se chiusa).
    [[nodiscard]] virtual std::string portName() const = 0;

    /// Legge fino a maxBytes. Ritorna:
    ///  - nullopt          -> errore I/O (porta scollegata, ecc.)
    ///  - 0                -> timeout scaduto senza dati (non è un errore)
    ///  - n > 0            -> byte effettivamente letti
    [[nodiscard]] virtual std::optional<std::size_t>
    read(std::uint8_t* buffer, std::size_t maxBytes,
         std::chrono::milliseconds timeout) = 0;

    /// Scrive tutti i byte richiesti. false su errore I/O.
    virtual bool write(const void* data, std::size_t size) = 0;

    /// Scarta i dati in ingresso non ancora letti.
    virtual void flushInput() = 0;

    /// Factory: crea l'implementazione della piattaforma corrente.
    [[nodiscard]] static std::unique_ptr<ISerialPort> create();

    /// Elenca le porte seriali presenti nel sistema
    /// (Windows: COMx; Linux: /dev/ttyACMx, /dev/ttyUSBx).
    [[nodiscard]] static std::vector<std::string> enumeratePorts();
};

} // namespace am
