/**
 * @file DigitalOutputPanel.cpp
 * @brief Implementazione del pannello delle uscite digitali.
 */
#include "view/DigitalOutputPanel.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>

#include "i18n/Strings.h"
#include "model/DigitalOutputState.h"
#include "view/LedIndicator.h"

namespace am {

DigitalOutputPanel::DigitalOutputPanel(wxWindow* parent, IUserActions& actions)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::DoBoxTitle));
    wxWindow* boxWin = box->GetStaticBox();
    // 8 colonne: etichetta, pulsante, LED, checkbox "Temporizzato",
    // etichetta+campo Tempo ON, etichetta+campo Periodo. I campi della
    // temporizzazione sono nascosti finché il checkbox non è spuntato.
    auto* grid = new wxFlexGridSizer(8 /*colonne*/, 4 /*vgap*/, 6 /*hgap*/);
    grid->AddGrowableCol(1, 1);

    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        const int pin = kFirstDigitalPin + i;
        const std::size_t idx = index(pin);
        Row& row = rows_[idx];

        auto* label = new wxStaticText(boxWin, wxID_ANY,
                                       wxString::Format("D%d", pin));
        grid->Add(label, 0, wxALIGN_CENTER_VERTICAL);

        auto* button = new wxToggleButton(boxWin, wxID_ANY, tr(StringId::DoOff));
        button->Bind(wxEVT_TOGGLEBUTTON, [this, pin](wxCommandEvent& e) {
            onButtonToggled(pin, e.IsChecked());
        });
        row.button = button;
        grid->Add(button, 1, wxEXPAND);

        auto* led = new LedIndicator(boxWin,
                                     wxColour(46, 204, 64),
                                     wxColour(110, 110, 110), 14);
        row.led = led;
        grid->Add(led, 0, wxALIGN_CENTER_VERTICAL);

        // --- Temporizzazione (v1.2) --------------------------------------
        auto* timedCheck = new wxCheckBox(boxWin, wxID_ANY,
                                          tr(StringId::DoTimedCheck));
        timedCheck->SetToolTip(tr(StringId::DoTimedTooltip));
        timedCheck->Bind(wxEVT_CHECKBOX, [this, pin](wxCommandEvent& e) {
            onTimedCheckToggled(pin, e.IsChecked());
        });
        row.timedCheck = timedCheck;
        grid->Add(timedCheck, 0, wxALIGN_CENTER_VERTICAL);

        row.onLabel = new wxStaticText(boxWin, wxID_ANY,
                                       tr(StringId::DoTimeOnLabel));
        grid->Add(row.onLabel, 0, wxALIGN_CENTER_VERTICAL);
        row.onField = new wxTextCtrl(boxWin, wxID_ANY, wxString(),
                                     wxDefaultPosition, wxSize(52, -1));
        grid->Add(row.onField, 0, wxALIGN_CENTER_VERTICAL);

        row.periodLabel = new wxStaticText(boxWin, wxID_ANY,
                                           tr(StringId::DoPeriodLabel));
        grid->Add(row.periodLabel, 0, wxALIGN_CENTER_VERTICAL);
        row.periodField = new wxTextCtrl(boxWin, wxID_ANY, wxString(),
                                         wxDefaultPosition, wxSize(52, -1));
        row.periodField->SetToolTip(tr(StringId::DoTimedTooltip));
        grid->Add(row.periodField, 0, wxALIGN_CENTER_VERTICAL);

        // Nascosti finché "Temporizzato" non è spuntato.
        row.onLabel->Hide();
        row.onField->Hide();
        row.periodLabel->Hide();
        row.periodField->Hide();
    }

    box->Add(grid, 1, wxEXPAND | wxALL, 8);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);

    setControlsEnabled(false);  // attivi solo a scheda connessa
}

void DigitalOutputPanel::onButtonToggled(int pin, bool on)
{
    Row& row = rows_[index(pin)];

    if (on && row.timedCheck->IsChecked()) {
        double onSeconds = 0.0;
        double periodSeconds = 0.0;
        bool oneShot = false;
        if (!parseTimedFields(row, onSeconds, periodSeconds, oneShot)) {
            // Campi non validi: il pulsante torna su OFF senza fare nulla.
            // wxBell() come feedback minimo (nessun dialogo: l'errore è
            // evidente e correggibile al volo nei campi accanto).
            row.button->SetValue(false);
            wxBell();
            return;
        }
        row.button->SetLabel(tr(StringId::DoOn));
        row.timedRunning = true;
        actions_.onTimedOutputStarted(pin, onSeconds, periodSeconds, oneShot);
        if (simultaneousStart_) {
            // Opzione "Avvio contemporaneo": il primo ON trascina con sé
            // tutte le altre uscite temporizzate configurate, ciascuna con i
            // PROPRI tempi (partenza sincronizzata, non parametri condivisi).
            startOtherTimedRows(pin);
        }
        return;
    }

    row.button->SetLabel(on ? tr(StringId::DoOn) : tr(StringId::DoOff));

    if (!on && row.timedRunning) {
        // Interruzione manuale di un ciclo in corso: il controller spegne
        // subito l'uscita, qualunque fosse la fase.
        row.timedRunning = false;
        actions_.onTimedOutputStopped(pin);
        return;
    }

    actions_.onDigitalOutputToggled(pin, on);
}

void DigitalOutputPanel::startOtherTimedRows(int excludePin)
{
    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        const int pin = kFirstDigitalPin + i;
        if (pin == excludePin) {
            continue;
        }
        Row& row = rows_[index(pin)];
        if (!row.timedCheck->IsChecked() || row.timedRunning) {
            continue;  // non temporizzata, o ciclo già in corso: non si tocca
        }
        double onSeconds = 0.0;
        double periodSeconds = 0.0;
        bool oneShot = false;
        if (!parseTimedFields(row, onSeconds, periodSeconds, oneShot)) {
            continue;  // campi non validi: saltata in silenzio (vedi header)
        }
        row.button->SetValue(true);
        row.button->SetLabel(tr(StringId::DoOn));
        row.timedRunning = true;
        actions_.onTimedOutputStarted(pin, onSeconds, periodSeconds, oneShot);
    }
}

void DigitalOutputPanel::setSimultaneousTimedStart(bool enabled)
{
    simultaneousStart_ = enabled;
}

void DigitalOutputPanel::onTimedCheckToggled(int pin, bool checked)
{
    Row& row = rows_[index(pin)];
    row.onLabel->Show(checked);
    row.onField->Show(checked);
    row.periodLabel->Show(checked);
    row.periodField->Show(checked);

    if (!checked && row.timedRunning) {
        // Spuntare via il checkbox con un ciclo in corso lo interrompe
        // (comportamento meno sorprendente di un ciclo che continua
        // "invisibile" con i campi ormai nascosti).
        row.timedRunning = false;
        row.button->SetValue(false);
        row.button->SetLabel(tr(StringId::DoOff));
        actions_.onTimedOutputStopped(pin);
    }
    Layout();
    // Il pannello può dover crescere: il genitore rifà il layout della colonna.
    if (GetParent() != nullptr) {
        GetParent()->Layout();
    }
}

bool DigitalOutputPanel::parseTimedFields(const Row& row, double& onSeconds,
                                          double& periodSeconds, bool& oneShot) const
{
    // ToCDouble: accetta il punto decimale indipendentemente dalla locale
    // (coerente con CalibrationPanel; vedi il commento lì per il perché).
    if (!row.onField->GetValue().ToCDouble(&onSeconds) || !(onSeconds > 0.0)) {
        return false;
    }

    wxString periodText = row.periodField->GetValue();
    periodText.Trim(true).Trim(false);
    if (periodText.Lower() == "inf") {
        oneShot = true;
        periodSeconds = 0.0;  // ignorato in one-shot
        return true;
    }
    oneShot = false;
    if (!periodText.ToCDouble(&periodSeconds)) {
        return false;
    }
    // Il periodo deve contenere per intero la fase ON.
    return periodSeconds > 0.0 && periodSeconds >= onSeconds;
}

void DigitalOutputPanel::setLed(int pin, bool on)
{
    if (!DigitalOutputState::isValidPin(pin)) {
        return;
    }
    rows_[index(pin)].led->setOn(on);
}

void DigitalOutputPanel::clearLeds()
{
    for (auto& row : rows_) {
        row.led->setOn(false);
    }
}

void DigitalOutputPanel::setControlsEnabled(bool enabled)
{
    for (auto& row : rows_) {
        row.button->Enable(enabled);
    }
}

void DigitalOutputPanel::setButtonState(int pin, bool on)
{
    if (!DigitalOutputState::isValidPin(pin)) {
        return;
    }
    Row& row = rows_[index(pin)];
    row.timedRunning = false;
    row.button->SetValue(on);
    row.button->SetLabel(on ? tr(StringId::DoOn) : tr(StringId::DoOff));
}

} // namespace am
