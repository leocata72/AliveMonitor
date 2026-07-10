/**
 * @file SerialPortWindows.h
 * @brief Implementazione Windows (Win32 API) della porta seriale.
 */
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>

#include <string>

#include "serial/ISerialPort.h"

namespace am {

class SerialPortWindows final : public ISerialPort {
public:
    SerialPortWindows() = default;
    ~SerialPortWindows() override;

    // Non copiabile (possiede un HANDLE), non movibile per semplicità.
    SerialPortWindows(const SerialPortWindows&) = delete;
    SerialPortWindows& operator=(const SerialPortWindows&) = delete;

    bool open(const std::string& portName, unsigned baudRate) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] std::string portName() const override;

    [[nodiscard]] std::optional<std::size_t>
    read(std::uint8_t* buffer, std::size_t maxBytes,
         std::chrono::milliseconds timeout) override;

    bool write(const void* data, std::size_t size) override;
    void flushInput() override;

private:
    /// Aggiorna i COMMTIMEOUTS solo se il timeout richiesto è cambiato.
    bool applyTimeout(std::chrono::milliseconds timeout);

    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::string portName_;
    long long lastTimeoutMs_ = -1;
};

} // namespace am
