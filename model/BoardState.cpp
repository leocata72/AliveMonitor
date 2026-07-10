/**
 * @file BoardState.cpp
 * @brief Implementazione dello stato thread-safe della scheda Arduino.
 */
#include "model/BoardState.h"

#include <algorithm>

namespace am {

// ---------------------------------------------------------------------------
// Connessione
// ---------------------------------------------------------------------------

void BoardState::setScanning()
{
    const std::lock_guard lock(mutex_);
    connection_ = ConnectionState::Scanning;
    portName_.clear();
    firmwareVersion_.clear();
    hasConnectedSince_ = false;
}

void BoardState::setConnected(const std::string& portName, unsigned baudRate)
{
    const std::lock_guard lock(mutex_);
    connection_ = ConnectionState::Connected;
    portName_ = portName;
    baudRate_ = baudRate;
    connectedSince_ = Clock::now();
    hasConnectedSince_ = true;
}

void BoardState::setDisconnected()
{
    const std::lock_guard lock(mutex_);
    connection_ = ConnectionState::Disconnected;
    portName_.clear();
    firmwareVersion_.clear();
    hasConnectedSince_ = false;
}

void BoardState::setFirmwareVersion(const std::string& version)
{
    const std::lock_guard lock(mutex_);
    firmwareVersion_ = version;
}

// ---------------------------------------------------------------------------
// Acquisizione
// ---------------------------------------------------------------------------

void BoardState::setAcquisition(AcquisitionState s)
{
    const std::lock_guard lock(mutex_);
    acquisition_ = s;
    if (s != AcquisitionState::Running) {
        measuredRateHz_ = 0.0;
    }
}

void BoardState::restartAcquisitionClock()
{
    const std::lock_guard lock(mutex_);
    acquisitionStart_ = Clock::now();
}

double BoardState::acquisitionElapsed() const
{
    const std::lock_guard lock(mutex_);
    return std::chrono::duration<double>(Clock::now() - acquisitionStart_).count();
}

// ---------------------------------------------------------------------------
// Frequenza
// ---------------------------------------------------------------------------

void BoardState::setRequestedRate(int hz)
{
    const std::lock_guard lock(mutex_);
    requestedRateHz_ = std::clamp(hz, kMinSampleRateHz, kMaxSampleRateHz);
}

void BoardState::setConfirmedRate(int hz)
{
    const std::lock_guard lock(mutex_);
    confirmedRateHz_ = hz;
}

void BoardState::setMeasuredRate(double hz)
{
    const std::lock_guard lock(mutex_);
    measuredRateHz_ = hz;
}

int BoardState::requestedRate() const
{
    const std::lock_guard lock(mutex_);
    return requestedRateHz_;
}

int BoardState::confirmedRate() const
{
    const std::lock_guard lock(mutex_);
    return confirmedRateHz_;
}

AcquisitionState BoardState::acquisition() const
{
    const std::lock_guard lock(mutex_);
    return acquisition_;
}

ConnectionState BoardState::connection() const
{
    const std::lock_guard lock(mutex_);
    return connection_;
}

// ---------------------------------------------------------------------------
// Statistiche
// ---------------------------------------------------------------------------

void BoardState::incPacketsReceived()
{
    const std::lock_guard lock(mutex_);
    ++stats_.packetsReceived;
}

void BoardState::addPacketsLost(std::uint64_t n)
{
    const std::lock_guard lock(mutex_);
    stats_.packetsLost += n;
}

void BoardState::incCrcErrors()
{
    const std::lock_guard lock(mutex_);
    ++stats_.crcErrors;
}

void BoardState::incSerialErrors()
{
    const std::lock_guard lock(mutex_);
    ++stats_.serialErrors;
}

void BoardState::resetStatistics()
{
    const std::lock_guard lock(mutex_);
    stats_ = Statistics{};
    measuredRateHz_ = 0.0;
}

// ---------------------------------------------------------------------------
// Snapshot
// ---------------------------------------------------------------------------

BoardState::Snapshot BoardState::snapshot() const
{
    const std::lock_guard lock(mutex_);
    Snapshot s;
    s.connection = connection_;
    s.portName = portName_;
    s.baudRate = baudRate_;
    s.firmwareVersion = firmwareVersion_;
    s.acquisition = acquisition_;
    s.requestedRateHz = requestedRateHz_;
    s.confirmedRateHz = confirmedRateHz_;
    s.measuredRateHz = measuredRateHz_;
    s.stats = stats_;
    s.connectedSeconds = hasConnectedSince_
        ? std::chrono::duration<double>(Clock::now() - connectedSince_).count()
        : 0.0;
    return s;
}

} // namespace am
