/**
 * @file CommandController.cpp
 * @brief Implementazione della logica dei comandi utente.
 */
#include "controller/CommandController.h"

#include <algorithm>

#include "model/CommunicationProtocol.h"

namespace am {

CommandController::CommandController(SerialController& serial,
                                     BoardState& state,
                                     DigitalOutputState& outputs,
                                     AnalogDataBuffer& buffer)
    : serial_(serial)
    , state_(state)
    , outputs_(outputs)
    , buffer_(buffer)
{
}

void CommandController::setDigitalOutput(int pin, bool on)
{
    if (!DigitalOutputState::isValidPin(pin)) {
        return;
    }
    outputs_.setDesired(pin, on);
    serial_.enqueueCommand(CommunicationProtocol::buildSetOutput(pin, on));
}

void CommandController::setSampleRate(int rateHz)
{
    const int clamped = std::clamp(rateHz, kMinSampleRateHz, kMaxSampleRateHz);
    state_.setRequestedRate(clamped);
    // Inviato anche a acquisizione ferma: il firmware memorizza il periodo
    // e risponde "OK RATE=n", tenendo PC e scheda sempre allineati.
    serial_.enqueueCommand(CommunicationProtocol::buildRate(clamped));
}

void CommandController::startAcquisition()
{
    const auto current = state_.acquisition();
    if (current == AcquisitionState::Running) {
        return;
    }
    if (current == AcquisitionState::Stopped) {
        // Nuova sessione: origine dei tempi azzerata e buffer pulito.
        state_.restartAcquisitionClock();
        buffer_.clear();
        state_.resetStatistics();
    }
    // Da Paused si riprende senza toccare buffer né asse dei tempi:
    // nel grafico resterà visibile il "buco" della pausa.
    state_.setAcquisition(AcquisitionState::Running);
    serial_.enqueueCommand(CommunicationProtocol::buildRate(state_.requestedRate()));
    serial_.enqueueCommand(CommunicationProtocol::buildStream(true));
}

void CommandController::pauseAcquisition()
{
    if (state_.acquisition() != AcquisitionState::Running) {
        return;
    }
    state_.setAcquisition(AcquisitionState::Paused);
    serial_.enqueueCommand(CommunicationProtocol::buildStream(false));
}

void CommandController::stopAcquisition()
{
    if (state_.acquisition() == AcquisitionState::Stopped) {
        return;
    }
    state_.setAcquisition(AcquisitionState::Stopped);
    serial_.enqueueCommand(CommunicationProtocol::buildStream(false));
    buffer_.clear();
}

} // namespace am
