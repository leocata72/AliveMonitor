/**
 * @file SerialPortWindows.cpp
 * @brief Porta seriale Win32 + enumerazione COM1..COM256 via QueryDosDevice.
 *
 * Compilato solo su Windows (vedi CMakeLists.txt).
 */
#include "serial/SerialPortWindows.h"

namespace am {

SerialPortWindows::~SerialPortWindows()
{
    close();  // RAII: chiusura garantita
}

bool SerialPortWindows::open(const std::string& portName, unsigned baudRate)
{
    close();

    // Il prefisso "\\.\" è obbligatorio per COM10 e superiori;
    // è innocuo per COM1..COM9, quindi viene aggiunto sempre.
    const std::string fullName = "\\\\.\\" + portName;

    handle_ = ::CreateFileA(fullName.c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,                       // nessuna condivisione
                            nullptr,
                            OPEN_EXISTING,
                            0,                       // I/O sincrono
                            nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!::GetCommState(handle_, &dcb)) {
        close();
        return false;
    }
    dcb.BaudRate = static_cast<DWORD>(baudRate);
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX  = FALSE;
    // DTR asserito: provoca il reset automatico di Arduino Uno all'apertura,
    // esattamente come avviene su Linux. Comportamento uniforme tra piattaforme.
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!::SetCommState(handle_, &dcb)) {
        close();
        return false;
    }

    ::SetupComm(handle_, 1 << 14, 1 << 12);  // buffer driver RX 16 KiB, TX 4 KiB
    ::PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    lastTimeoutMs_ = -1;  // forza la riconfigurazione dei timeout alla prima read
    portName_ = portName;
    return true;
}

void SerialPortWindows::close()
{
    if (handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    portName_.clear();
}

bool SerialPortWindows::isOpen() const
{
    return handle_ != INVALID_HANDLE_VALUE;
}

std::string SerialPortWindows::portName() const
{
    return portName_;
}

bool SerialPortWindows::applyTimeout(std::chrono::milliseconds timeout)
{
    if (timeout.count() == lastTimeoutMs_) {
        return true;  // già configurato: evita una syscall per ogni read
    }

    // Configurazione "ritorna appena c'è almeno un byte, altrimenti attendi
    // fino al timeout": è il pattern documentato di ReadIntervalTimeout=MAXDWORD
    // con ReadTotalTimeoutMultiplier=MAXDWORD.
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout        = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = MAXDWORD;
    to.ReadTotalTimeoutConstant   = static_cast<DWORD>(timeout.count());
    to.WriteTotalTimeoutConstant  = 500;
    to.WriteTotalTimeoutMultiplier = 0;

    if (!::SetCommTimeouts(handle_, &to)) {
        return false;
    }
    lastTimeoutMs_ = timeout.count();
    return true;
}

std::optional<std::size_t> SerialPortWindows::read(std::uint8_t* buffer,
                                                   std::size_t maxBytes,
                                                   std::chrono::milliseconds timeout)
{
    if (handle_ == INVALID_HANDLE_VALUE || !applyTimeout(timeout)) {
        return std::nullopt;
    }

    DWORD bytesRead = 0;
    if (!::ReadFile(handle_, buffer, static_cast<DWORD>(maxBytes),
                    &bytesRead, nullptr)) {
        return std::nullopt;  // errore I/O: probabile scollegamento USB
    }
    return static_cast<std::size_t>(bytesRead);  // 0 = timeout, non errore
}

bool SerialPortWindows::write(const void* data, std::size_t size)
{
    if (handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    if (!::WriteFile(handle_, data, static_cast<DWORD>(size), &written, nullptr)) {
        return false;
    }
    return written == size;
}

void SerialPortWindows::flushInput()
{
    if (handle_ != INVALID_HANDLE_VALUE) {
        ::PurgeComm(handle_, PURGE_RXCLEAR);
    }
}

// ---------------------------------------------------------------------------
// Factory & enumerazione (implementazione Windows di ISerialPort)
// ---------------------------------------------------------------------------

std::unique_ptr<ISerialPort> ISerialPort::create()
{
    return std::make_unique<SerialPortWindows>();
}

std::vector<std::string> ISerialPort::enumeratePorts()
{
    std::vector<std::string> ports;
    char target[512];

    // QueryDosDevice risolve il nome solo se la porta esiste davvero:
    // metodo leggero e senza dipendenze da SetupAPI.
    for (int i = 1; i <= 256; ++i) {
        const std::string name = "COM" + std::to_string(i);
        if (::QueryDosDeviceA(name.c_str(), target, sizeof(target)) != 0) {
            ports.push_back(name);
        }
    }
    return ports;
}

} // namespace am
