/**
 * @file DigitalOutputPanel.h
 * @brief Pannello sinistro: comando delle uscite digitali D2..D9.
 *
 * VIEW (MVC). Per ogni uscita: un pulsante Toggle e un LED che riflette lo
 * stato REALE confermato dal firmware ("OK Dx=v"), non quello richiesto.
 */
#pragma once

#include <array>

#include <wx/panel.h>

#include "IUserActions.h"
#include "Version.h"

class wxToggleButton;

namespace am {

class LedIndicator;

class DigitalOutputPanel : public wxPanel {
public:
    DigitalOutputPanel(wxWindow* parent, IUserActions& actions);

    /// Imposta il LED dello stato reale del pin (chiamato all'"OK Dx=v").
    void setLed(int pin, bool on);

    /// Spegne tutti i LED (alla disconnessione lo stato reale non è noto).
    void clearLeds();

    /// Abilita/disabilita i pulsanti (attivi solo a scheda connessa).
    void setControlsEnabled(bool enabled);

private:
    struct Row {
        wxToggleButton* button = nullptr;
        LedIndicator* led = nullptr;
    };

    [[nodiscard]] static constexpr std::size_t index(int pin) noexcept
    {
        return static_cast<std::size_t>(pin - kFirstDigitalPin);
    }

    IUserActions& actions_;
    std::array<Row, kNumDigitalOutputs> rows_{};
};

} // namespace am
