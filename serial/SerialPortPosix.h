/**
 * @file SerialPortPosix.h
 * @brief Implementazione POSIX (Linux) della porta seriale con termios.
 */
#pragma once

#include <string>

#include "serial/ISerialPort.h"

namespace am {

class SerialPortPosix final : public ISerialPort {
public:
    SerialPortPosix() = default;
    ~SerialPortPosix() override;

    // Non copiabile (possiede un file descriptor), non movibile per semplicità.
    SerialPortPosix(const SerialPortPosix&) = delete;
    SerialPortPosix& operator=(const SerialPortPosix&) = delete;

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
    int fd_ = -1;           ///< File descriptor (-1 = chiuso).
    std::string portName_;
};

} // namespace am
