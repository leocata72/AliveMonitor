/**
 * @file SerialPortPosix.cpp
 * @brief Porta seriale POSIX (termios) + enumerazione /dev/ttyACM*, /dev/ttyUSB*.
 *
 * Compilato solo su piattaforme non-Windows (vedi CMakeLists.txt).
 */
#include "serial/SerialPortPosix.h"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <filesystem>

namespace am {
namespace {

/// Mappa un baud numerico sulla costante termios corrispondente.
/// Ritorna 0 se non supportato.
[[nodiscard]] speed_t toSpeed(unsigned baud) noexcept
{
    switch (baud) {
        case 9600:    return B9600;
        case 19200:   return B19200;
        case 38400:   return B38400;
        case 57600:   return B57600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        default:      return 0;
    }
}

} // namespace

SerialPortPosix::~SerialPortPosix()
{
    close();  // RAII: chiusura garantita
}

bool SerialPortPosix::open(const std::string& portName, unsigned baudRate)
{
    close();

    const speed_t speed = toSpeed(baudRate);
    if (speed == 0) {
        return false;
    }

    // O_NOCTTY: la porta non diventa terminale di controllo del processo.
    // O_NONBLOCK: l'apertura non si blocca in attesa della linea DCD.
    fd_ = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        return false;
    }

    termios tio{};
    if (::tcgetattr(fd_, &tio) != 0) {
        close();
        return false;
    }

    ::cfmakeraw(&tio);            // modalità raw: nessuna elaborazione di linea
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~static_cast<tcflag_t>(CRTSCTS);  // no controllo di flusso HW
    tio.c_cc[VMIN] = 0;           // lettura non bloccante:
    tio.c_cc[VTIME] = 0;          // il timeout è gestito con select()

    if (::cfsetispeed(&tio, speed) != 0 || ::cfsetospeed(&tio, speed) != 0
        || ::tcsetattr(fd_, TCSANOW, &tio) != 0) {
        close();
        return false;
    }

    // Nota: l'apertura del device asserisce DTR -> Arduino Uno si resetta.
    // Il chiamante deve attendere il bootloader (~2 s) prima dell'handshake.
    ::tcflush(fd_, TCIOFLUSH);
    portName_ = portName;
    return true;
}

void SerialPortPosix::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    portName_.clear();
}

bool SerialPortPosix::isOpen() const
{
    return fd_ >= 0;
}

std::string SerialPortPosix::portName() const
{
    return portName_;
}

std::optional<std::size_t> SerialPortPosix::read(std::uint8_t* buffer,
                                                 std::size_t maxBytes,
                                                 std::chrono::milliseconds timeout)
{
    if (fd_ < 0) {
        return std::nullopt;
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd_, &readSet);

    timeval tv{};
    tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);

    const int sel = ::select(fd_ + 1, &readSet, nullptr, nullptr, &tv);
    if (sel < 0) {
        return (errno == EINTR) ? std::optional<std::size_t>(0) : std::nullopt;
    }
    if (sel == 0) {
        return 0;  // timeout: nessun dato, nessun errore
    }

    const ssize_t n = ::read(fd_, buffer, maxBytes);
    if (n < 0) {
        return (errno == EAGAIN || errno == EWOULDBLOCK)
            ? std::optional<std::size_t>(0)
            : std::nullopt;
    }
    if (n == 0) {
        // select segnalava dati ma read ne restituisce 0:
        // il dispositivo è stato scollegato.
        return std::nullopt;
    }
    return static_cast<std::size_t>(n);
}

bool SerialPortPosix::write(const void* data, std::size_t size)
{
    if (fd_ < 0) {
        return false;
    }
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t written = 0;
    while (written < size) {
        const ssize_t n = ::write(fd_, p + written, size - written);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return false;
        }
        written += static_cast<std::size_t>(n);
    }
    return true;
}

void SerialPortPosix::flushInput()
{
    if (fd_ >= 0) {
        ::tcflush(fd_, TCIFLUSH);
    }
}

// ---------------------------------------------------------------------------
// Factory & enumerazione (implementazione POSIX di ISerialPort)
// ---------------------------------------------------------------------------

std::unique_ptr<ISerialPort> ISerialPort::create()
{
    return std::make_unique<SerialPortPosix>();
}

std::vector<std::string> ISerialPort::enumeratePorts()
{
    std::vector<std::string> ports;
    namespace fs = std::filesystem;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/dev", ec)) {
        const std::string name = entry.path().filename().string();
        // ttyACM*: Arduino Uno (CDC/ACM). ttyUSB*: cloni con FTDI/CH340.
        if (name.rfind("ttyACM", 0) == 0 || name.rfind("ttyUSB", 0) == 0) {
            ports.push_back(entry.path().string());
        }
    }
    std::sort(ports.begin(), ports.end());
    return ports;
}

} // namespace am
