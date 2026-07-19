/**
 * @file DigitalOutputState.h
 * @brief Stato delle uscite digitali D2..D9 (desiderato e reale).
 *
 * MODEL (MVC). Distinzione fondamentale:
 *  - stato DESIDERATO: ciò che l'utente ha richiesto dalla GUI;
 *  - stato REALE:      ciò che il firmware ha confermato con "OK Dx=v".
 *
 * Il LED della GUI mostra SOLO lo stato reale. Lo stato desiderato viene
 * ri-applicato automaticamente a ogni (ri)connessione, così dopo un
 * distacco/riattacco della scheda le uscite tornano coerenti.
 */
#pragma once

#include <array>
#include <mutex>
#include <optional>

#include "Version.h"

namespace am {

class DigitalOutputState {
public:
    using PinArray = std::array<bool, kNumDigitalOutputs>;

    DigitalOutputState() = default;

    /// true se pin è nell'intervallo gestito [D2..D9].
    [[nodiscard]] static constexpr bool isValidPin(int pin) noexcept
    {
        return pin >= kFirstDigitalPin
            && pin < kFirstDigitalPin + kNumDigitalOutputs;
    }

    /// Imposta lo stato desiderato (richiesta utente). Ignora pin non validi.
    void setDesired(int pin, bool on);

    /// Imposta lo stato reale confermato dal firmware. Ignora pin non validi.
    void setActual(int pin, bool on);

    /// Azzera lo stato reale (alla disconnessione: non è più noto).
    void clearActual();

    [[nodiscard]] std::optional<bool> desired(int pin) const;
    [[nodiscard]] std::optional<bool> actual(int pin) const;

    /// Copie coerenti dell'intero array (per sincronizzazione alla connessione).
    [[nodiscard]] PinArray desiredAll() const;
    [[nodiscard]] PinArray actualAll() const;

    // --- Direzione dei pin (v1.2): false = uscita (default), true = ingresso.
    /// Direzione desiderata dall'utente (wxChoice IN/OUT); riapplicata a
    /// ogni (ri)connessione come lo stato delle uscite.
    void setDesiredDirection(int pin, bool input);
    /// Direzione confermata dal firmware ("OK DIR Dx=...").
    void setActualDirection(int pin, bool input);
    [[nodiscard]] std::optional<bool> desiredDirection(int pin) const;
    [[nodiscard]] PinArray desiredDirectionsAll() const;

private:
    [[nodiscard]] static constexpr std::size_t index(int pin) noexcept
    {
        return static_cast<std::size_t>(pin - kFirstDigitalPin);
    }

    mutable std::mutex mutex_;
    PinArray desired_{};        ///< Tutte false all'avvio.
    PinArray actual_{};
    PinArray desiredInput_{};   ///< false = OUT (default), true = IN. (v1.2)
    PinArray actualInput_{};    ///< Direzione confermata dal firmware.
};

} // namespace am
