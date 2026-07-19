/**
 * @file DigitalOutputState.cpp
 * @brief Implementazione dello stato thread-safe delle uscite digitali.
 */
#include "model/DigitalOutputState.h"

namespace am {

void DigitalOutputState::setDesired(int pin, bool on)
{
    if (!isValidPin(pin)) {
        return;
    }
    const std::lock_guard lock(mutex_);
    desired_[index(pin)] = on;
}

void DigitalOutputState::setActual(int pin, bool on)
{
    if (!isValidPin(pin)) {
        return;
    }
    const std::lock_guard lock(mutex_);
    actual_[index(pin)] = on;
}

void DigitalOutputState::clearActual()
{
    const std::lock_guard lock(mutex_);
    actual_.fill(false);
}

std::optional<bool> DigitalOutputState::desired(int pin) const
{
    if (!isValidPin(pin)) {
        return std::nullopt;
    }
    const std::lock_guard lock(mutex_);
    return desired_[index(pin)];
}

std::optional<bool> DigitalOutputState::actual(int pin) const
{
    if (!isValidPin(pin)) {
        return std::nullopt;
    }
    const std::lock_guard lock(mutex_);
    return actual_[index(pin)];
}

DigitalOutputState::PinArray DigitalOutputState::desiredAll() const
{
    const std::lock_guard lock(mutex_);
    return desired_;
}

DigitalOutputState::PinArray DigitalOutputState::actualAll() const
{
    const std::lock_guard lock(mutex_);
    return actual_;
}

void DigitalOutputState::setDesiredDirection(int pin, bool input)
{
    if (!isValidPin(pin)) {
        return;
    }
    const std::lock_guard lock(mutex_);
    desiredInput_[index(pin)] = input;
}

void DigitalOutputState::setActualDirection(int pin, bool input)
{
    if (!isValidPin(pin)) {
        return;
    }
    const std::lock_guard lock(mutex_);
    actualInput_[index(pin)] = input;
}

std::optional<bool> DigitalOutputState::desiredDirection(int pin) const
{
    if (!isValidPin(pin)) {
        return std::nullopt;
    }
    const std::lock_guard lock(mutex_);
    return desiredInput_[index(pin)];
}

DigitalOutputState::PinArray DigitalOutputState::desiredDirectionsAll() const
{
    const std::lock_guard lock(mutex_);
    return desiredInput_;
}

} // namespace am
