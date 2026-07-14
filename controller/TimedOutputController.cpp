/**
 * @file TimedOutputController.cpp
 * @brief Implementazione dei cicli temporizzati delle uscite digitali.
 */
#include "controller/TimedOutputController.h"

#include <cmath>

namespace am {
namespace {

/// Periodo del timer di servizio: risoluzione dei cicli ON/OFF.
constexpr int kTickMs = 50;

} // namespace

TimedOutputController::TimedOutputController()
    : timer_(this)
{
    Bind(wxEVT_TIMER, &TimedOutputController::onTimer, this);
}

void TimedOutputController::setCallbacks(SendFn send, FinishedFn finished)
{
    send_ = std::move(send);
    finished_ = std::move(finished);
}

void TimedOutputController::startCycle(int pin, double onSeconds,
                                       double periodSeconds, bool oneShot)
{
    if (!isValidPin(pin) || !send_ || !(onSeconds > 0.0)) {
        return;
    }
    if (!oneShot && !(periodSeconds >= onSeconds)) {
        return;  // periodo più corto della fase ON: configurazione non valida
    }

    auto& c = cycles_[index(pin)];
    c.active = true;
    c.oneShot = oneShot;
    c.onSeconds = onSeconds;
    c.periodSeconds = periodSeconds;
    c.start = Clock::now();
    c.lastSent = true;
    send_(pin, true);  // la fase ON parte subito, alla pressione del pulsante

    updateTimerState();
}

void TimedOutputController::stopCycle(int pin)
{
    if (!isValidPin(pin)) {
        return;
    }
    auto& c = cycles_[index(pin)];
    if (!c.active) {
        return;
    }
    c.active = false;
    if (send_) {
        send_(pin, false);  // spegnimento immediato, qualunque fosse la fase
    }
    updateTimerState();
}

void TimedOutputController::stopAll()
{
    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        stopCycle(kFirstDigitalPin + i);
    }
}

bool TimedOutputController::isRunning(int pin) const
{
    return isValidPin(pin) && cycles_[index(pin)].active;
}

void TimedOutputController::onTimer(wxTimerEvent& WXUNUSED(event))
{
    const auto now = Clock::now();

    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        auto& c = cycles_[static_cast<std::size_t>(i)];
        if (!c.active) {
            continue;
        }
        const int pin = kFirstDigitalPin + i;
        const double elapsed =
            std::chrono::duration<double>(now - c.start).count();

        bool desired = false;
        if (c.oneShot) {
            desired = elapsed < c.onSeconds;
            if (!desired) {
                // Impulso completato: OFF definitivo e notifica alla GUI
                // (il pulsante deve tornare visivamente su OFF).
                c.active = false;
                send_(pin, false);
                if (finished_) {
                    finished_(pin);
                }
                continue;
            }
        } else {
            // Fase all'interno del periodo corrente: [0, onSeconds) = ON,
            // [onSeconds, period) = OFF. fmod è sufficiente: elapsed e
            // period sono positivi per costruzione (vedi startCycle).
            const double phase = std::fmod(elapsed, c.periodSeconds);
            desired = phase < c.onSeconds;
        }

        // Solo i CAMBI di stato viaggiano verso la scheda: niente comando
        // ridondante a ogni tick (la seriale ringrazia, e il LED riflette
        // sempre una conferma reale "OK Dx=v" per ogni transizione).
        if (desired != c.lastSent) {
            c.lastSent = desired;
            send_(pin, desired);
        }
    }

    updateTimerState();
}

void TimedOutputController::updateTimerState()
{
    bool anyActive = false;
    for (const auto& c : cycles_) {
        anyActive = anyActive || c.active;
    }
    if (anyActive && !timer_.IsRunning()) {
        timer_.Start(kTickMs);
    } else if (!anyActive && timer_.IsRunning()) {
        timer_.Stop();
    }
}

} // namespace am
