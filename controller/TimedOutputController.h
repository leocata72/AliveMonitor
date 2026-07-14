/**
 * @file TimedOutputController.h
 * @brief Cicli temporizzati delle uscite digitali (funzione "Temporizzato").
 *
 * CONTROLLER (MVC). Gestisce, con un unico wxTimer nel main thread, i cicli
 * ON/OFF delle uscite digitali configurate come temporizzate:
 *  - ciclo periodico: ON per onSeconds, OFF fino a periodSeconds, e da capo;
 *  - one-shot (campo Periodo = "inf" nella GUI): ON per onSeconds, poi OFF
 *    definitivo, con notifica alla GUI per riportare il pulsante su OFF.
 *
 * I comandi verso la scheda passano dal callback send (tipicamente
 * CommandController::setDigitalOutput): questo controller decide SOLO il
 * "quando", non conosce il protocollo né la seriale. Vengono inviati solo i
 * CAMBI di stato (non un comando a ogni tick). Risoluzione del ciclo: il
 * periodo del timer (50 ms) — più che adeguata per attuatori comandati in
 * secondi; non è un generatore di segnali di precisione.
 *
 * Tutto avviene nel main thread (wxTimer): nessuna sincronizzazione.
 */
#pragma once

#include <array>
#include <chrono>
#include <functional>

#include <wx/event.h>
#include <wx/timer.h>

#include "Version.h"

namespace am {

class TimedOutputController : public wxEvtHandler {
public:
    /// Invia lo stato desiderato di un pin (pin, on).
    using SendFn = std::function<void(int, bool)>;
    /// Ciclo one-shot completato: la GUI deve riportare il pulsante su OFF.
    using FinishedFn = std::function<void(int)>;

    TimedOutputController();

    TimedOutputController(const TimedOutputController&) = delete;
    TimedOutputController& operator=(const TimedOutputController&) = delete;

    /// Da chiamare una volta, prima di qualunque startCycle().
    void setCallbacks(SendFn send, FinishedFn finished);

    /// Avvia il ciclo del pin: accende subito, poi segue la temporizzazione.
    /// @param onSeconds     durata della fase ON (> 0).
    /// @param periodSeconds periodo del ciclo (>= onSeconds); ignorato se oneShot.
    /// @param oneShot       true = un solo impulso ON, poi OFF definitivo.
    void startCycle(int pin, double onSeconds, double periodSeconds, bool oneShot);

    /// Ferma il ciclo del pin e spegne subito l'uscita.
    void stopCycle(int pin);

    /// Ferma tutti i cicli attivi spegnendo le uscite (chiusura ordinata).
    void stopAll();

    /// true se il pin ha un ciclo temporizzato in corso.
    [[nodiscard]] bool isRunning(int pin) const;

private:
    using Clock = std::chrono::steady_clock;

    struct Cycle {
        bool active = false;
        bool oneShot = false;
        double onSeconds = 0.0;
        double periodSeconds = 0.0;
        Clock::time_point start{};   ///< Inizio del ciclo (fase ON).
        bool lastSent = false;       ///< Ultimo stato inviato alla scheda.
    };

    void onTimer(wxTimerEvent& event);
    /// Avvia/ferma il timer in base alla presenza di cicli attivi: il timer
    /// gira solo quando serve (zero tick a riposo).
    void updateTimerState();

    [[nodiscard]] static constexpr std::size_t index(int pin) noexcept
    {
        return static_cast<std::size_t>(pin - kFirstDigitalPin);
    }
    [[nodiscard]] static constexpr bool isValidPin(int pin) noexcept
    {
        return pin >= kFirstDigitalPin
            && pin < kFirstDigitalPin + kNumDigitalOutputs;
    }

    SendFn send_;
    FinishedFn finished_;
    wxTimer timer_;
    std::array<Cycle, kNumDigitalOutputs> cycles_{};
};

} // namespace am
