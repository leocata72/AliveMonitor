/**
 * @file DigitalOutputPanel.cpp
 * @brief Implementazione del pannello delle uscite digitali.
 */
#include "view/DigitalOutputPanel.h"

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/tglbtn.h>

#include "i18n/Strings.h"
#include "model/DigitalOutputState.h"
#include "view/LedIndicator.h"

namespace am {

DigitalOutputPanel::DigitalOutputPanel(wxWindow* parent, IUserActions& actions)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::DoBoxTitle));
    auto* grid = new wxFlexGridSizer(3 /*colonne*/, 4 /*vgap*/, 8 /*hgap*/);
    grid->AddGrowableCol(1, 1);

    for (int i = 0; i < kNumDigitalOutputs; ++i) {
        const int pin = kFirstDigitalPin + i;
        const std::size_t idx = index(pin);

        auto* label = new wxStaticText(box->GetStaticBox(), wxID_ANY,
                                       wxString::Format("D%d", pin));
        grid->Add(label, 0, wxALIGN_CENTER_VERTICAL);

        auto* button = new wxToggleButton(box->GetStaticBox(), wxID_ANY, tr(StringId::DoOff));
        button->Bind(wxEVT_TOGGLEBUTTON, [this, pin, button](wxCommandEvent& e) {
            const bool on = e.IsChecked();
            button->SetLabel(on ? tr(StringId::DoOn) : tr(StringId::DoOff));
            actions_.onDigitalOutputToggled(pin, on);
        });
        rows_[idx].button = button;
        grid->Add(button, 1, wxEXPAND);

        auto* led = new LedIndicator(box->GetStaticBox(),
                                     wxColour(46, 204, 64),
                                     wxColour(110, 110, 110), 14);
        rows_[idx].led = led;
        grid->Add(led, 0, wxALIGN_CENTER_VERTICAL);
    }

    box->Add(grid, 1, wxEXPAND | wxALL, 8);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);

    setControlsEnabled(false);  // attivi solo a scheda connessa
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

} // namespace am
