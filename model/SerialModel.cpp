/**
 * @file SerialModel.cpp
 * @brief Implementazione del modello dei parametri seriali.
 */
#include "model/SerialModel.h"

#include <algorithm>

namespace am {

unsigned SerialModel::baudRate() const
{
    const std::lock_guard lock(mutex_);
    return baudRate_;
}

void SerialModel::setBaudRate(unsigned baud)
{
    const std::lock_guard lock(mutex_);
    baudRate_ = baud;
}

void SerialModel::setAvailablePorts(std::vector<std::string> ports)
{
    const std::lock_guard lock(mutex_);
    availablePorts_ = std::move(ports);
}

std::vector<std::string> SerialModel::availablePorts() const
{
    const std::lock_guard lock(mutex_);
    return availablePorts_;
}

void SerialModel::setPreferredPort(const std::string& port)
{
    const std::lock_guard lock(mutex_);
    preferredPort_ = port;
}

std::string SerialModel::preferredPort() const
{
    const std::lock_guard lock(mutex_);
    return preferredPort_;
}

std::vector<std::string> SerialModel::prioritized(std::vector<std::string> ports) const
{
    const std::string preferred = preferredPort();
    if (!preferred.empty()) {
        const auto it = std::find(ports.begin(), ports.end(), preferred);
        if (it != ports.end() && it != ports.begin()) {
            std::rotate(ports.begin(), it, it + 1);
        }
    }
    return ports;
}

} // namespace am
