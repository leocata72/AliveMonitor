/**
 * @file CommandController.h
 * @brief Controller dei comandi verso Arduino (logica applicativa).
 *
 * CONTROLLER (MVC). Traduce le intenzioni dell'utente (accendi un'uscita,
 * cambia frequenza, avvia/ferma l'acquisizione) in:
 *  1. aggiornamenti dello stato desiderato nel Model;
 *  2. comandi di protocollo accodati al SerialController (non bloccante).
 *
 * Centralizza le regole di business (clamp della frequenza, semantica di
 * Start/Pause/Stop, azzeramento del buffer allo Stop), tenendole fuori
 * sia dalla View sia dal thread seriale.
 */
#pragma once

#include "controller/SerialController.h"
#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"
#include "model/DigitalOutputState.h"

namespace am {

class CommandController {
public:
    CommandController(SerialController& serial,
                      BoardState& state,
                      DigitalOutputState& outputs,
                      AnalogDataBuffer& buffer);

    CommandController(const CommandController&) = delete;
    CommandController& operator=(const CommandController&) = delete;

    /// Imposta un'uscita digitale D2..D9. Aggiorna lo stato desiderato e
    /// invia "SET Dx v"; il LED della GUI cambierà solo all'"OK Dx=v".
    void setDigitalOutput(int pin, bool on);

    /// Cambia la direzione del pin D2..D9 (v1.2): true = ingresso ("DIR Dx I",
    /// il firmware notificherà il livello con "IN Dx=v"), false = uscita
    /// ("DIR Dx O" seguito dal SET dello stato desiderato, perché il
    /// firmware riparte da LOW). Aggiorna la direzione desiderata, che viene
    /// riapplicata a ogni riconnessione.
    void setDigitalDirection(int pin, bool input);

    /// Cambia la frequenza di campionamento [Hz], anche durante
    /// l'acquisizione. Il valore viene limitato a [1..250].
    void setSampleRate(int rateHz);

    /// Avvia (o riprende da Pausa) lo streaming dei campioni.
    void startAcquisition();

    /// Sospende lo streaming conservando i dati e l'asse dei tempi.
    void pauseAcquisition();

    /// Ferma lo streaming e azzera il buffer dei campioni.
    void stopAcquisition();

private:
    SerialController& serial_;
    BoardState& state_;
    DigitalOutputState& outputs_;
    AnalogDataBuffer& buffer_;
};

} // namespace am
