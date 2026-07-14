/**
 * @file DigitalOutputPanel.h
 * @brief Pannello sinistro: comando delle uscite digitali D2..D9.
 *
 * VIEW (MVC). Per ogni uscita: un pulsante Toggle, un LED che riflette lo
 * stato REALE confermato dal firmware ("OK Dx=v"), un checkbox
 * "Temporizzato" e — quando questo è spuntato — i campi Tempo ON (s) e
 * Periodo (s). Con la temporizzazione attiva, la pressione di ON avvia il
 * ciclo (gestito da TimedOutputController via IUserActions); Periodo = "inf"
 * esegue un solo impulso: ON per il tempo impostato, poi OFF definitivo.
 */
#pragma once

#include <array>

#include <wx/panel.h>

#include "IUserActions.h"
#include "Version.h"

class wxCheckBox;
class wxStaticText;
class wxTextCtrl;
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

    /// Aggiorna SOLO l'aspetto del pulsante (toggle + etichetta ON/OFF),
    /// senza emettere azioni: usato dal Controller quando un ciclo one-shot
    /// termina da solo e il pulsante deve tornare su OFF.
    void setButtonState(int pin, bool on);

    /// Opzione "Avvio contemporaneo temporizzatori" (riquadro Opzioni,
    /// inoltrata da MainController): se attiva, il primo ON su un'uscita
    /// temporizzata avvia anche le altre righe con "Temporizzato" spuntato
    /// e campi validi (vedi startOtherTimedRows()).
    void setSimultaneousTimedStart(bool enabled);

private:
    struct Row {
        wxToggleButton* button = nullptr;
        LedIndicator* led = nullptr;
        wxCheckBox* timedCheck = nullptr;
        wxStaticText* onLabel = nullptr;
        wxTextCtrl* onField = nullptr;
        wxStaticText* periodLabel = nullptr;
        wxTextCtrl* periodField = nullptr;
        bool timedRunning = false;  ///< Ciclo temporizzato in corso per questa riga.
    };

    void onButtonToggled(int pin, bool on);
    void onTimedCheckToggled(int pin, bool checked);

    /// Avvia i cicli di TUTTE le altre righe temporizzate configurate
    /// (checkbox spuntato, ciclo non già in corso, campi validi), saltando
    /// excludePin (già avviato dal chiamante). Le righe con campi non validi
    /// vengono saltate in silenzio: l'unico feedback d'errore rumoroso
    /// spetta alla riga premuta direttamente dall'utente.
    void startOtherTimedRows(int excludePin);

    /// Legge e valida i campi della riga. @return false se non validi
    /// (Tempo ON <= 0, Periodo < Tempo ON, testo non numerico...).
    /// @param oneShot true se il campo Periodo contiene "inf".
    [[nodiscard]] bool parseTimedFields(const Row& row, double& onSeconds,
                                        double& periodSeconds, bool& oneShot) const;

    [[nodiscard]] static constexpr std::size_t index(int pin) noexcept
    {
        return static_cast<std::size_t>(pin - kFirstDigitalPin);
    }

    IUserActions& actions_;
    std::array<Row, kNumDigitalOutputs> rows_{};
    bool simultaneousStart_ = false;  ///< Opzione avvio contemporaneo (v1.2).
};

} // namespace am
