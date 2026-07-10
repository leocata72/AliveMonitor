/**
 * @file CpuMonitor.cpp
 * @brief Implementazione multipiattaforma della misura CPU del processo.
 */
#include "CpuMonitor.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fstream>
    #include <sstream>
    #include <string>
#endif

namespace am {

CpuMonitor::CpuMonitor()
    : lastWall_(std::chrono::steady_clock::now())
{
    if (const auto t = processCpuMicros()) {
        lastProcTicks_ = *t;
        valid_ = true;
    }
}

std::optional<std::uint64_t> CpuMonitor::processCpuMicros()
{
#ifdef _WIN32
    FILETIME creation{}, exitTime{}, kernel{}, user{};
    if (!::GetProcessTimes(::GetCurrentProcess(), &creation, &exitTime, &kernel, &user)) {
        return std::nullopt;
    }
    const auto toU64 = [](const FILETIME& ft) {
        return (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };
    // FILETIME è in unità da 100 ns -> /10 per ottenere microsecondi.
    return (toU64(kernel) + toU64(user)) / 10ULL;
#else
    // /proc/self/stat: i campi 14 (utime) e 15 (stime) sono in clock tick.
    std::ifstream stat("/proc/self/stat");
    if (!stat.is_open()) {
        return std::nullopt;
    }
    std::string content;
    std::getline(stat, content);
    // Il nome del processo (campo 2) può contenere spazi ed è tra parentesi:
    // si riparte dall'ultima ')' per un parsing robusto.
    const auto pos = content.rfind(')');
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    std::istringstream iss(content.substr(pos + 1));
    std::string field;
    // Dopo ')' seguono: state(3), poi i campi 4..13, quindi utime è il 12° token.
    unsigned long long utime = 0, stime = 0;
    for (int i = 3; i <= 15 && (iss >> field); ++i) {
        if (i == 14) { utime = std::stoull(field); }
        if (i == 15) { stime = std::stoull(field); }
    }
    const long ticksPerSec = ::sysconf(_SC_CLK_TCK);
    if (ticksPerSec <= 0) {
        return std::nullopt;
    }
    const long double micros =
        static_cast<long double>(utime + stime) * 1'000'000.0L / ticksPerSec;
    return static_cast<std::uint64_t>(micros);
#endif
}

std::optional<double> CpuMonitor::sample()
{
    const auto now = std::chrono::steady_clock::now();
    const auto wallUs =
        std::chrono::duration_cast<std::chrono::microseconds>(now - lastWall_).count();

    if (wallUs < 200'000) {
        // Intervallo troppo breve: restituisce l'ultimo valore calcolato.
        return lastValue_;
    }

    const auto proc = processCpuMicros();
    if (!proc || !valid_) {
        if (proc) { // prima lettura valida: inizializza la baseline
            lastProcTicks_ = *proc;
            lastWall_ = now;
            valid_ = true;
        }
        return std::nullopt;
    }

    const auto procDelta = static_cast<double>(*proc - lastProcTicks_);
    lastProcTicks_ = *proc;
    lastWall_ = now;

    lastValue_ = 100.0 * procDelta / static_cast<double>(wallUs);
    return lastValue_;
}

} // namespace am
