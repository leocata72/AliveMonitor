/**
 * @file AcquisitionPanel.cpp
 * @brief Implementazione dei controlli di acquisizione.
 */
#include "view/AcquisitionPanel.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include "Version.h"
#include "i18n/Strings.h"

namespace am {

AcquisitionPanel::AcquisitionPanel(wxWindow* parent, IUserActions& actions)
    : wxPanel(parent)
    , actions_(actions)
    , debounceTimer_(this)
{
    Bind(wxEVT_TIMER, &AcquisitionPanel::onDebounceTimer, this);

    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::AqBoxTitle));
    wxWindow* boxWin = box->GetStaticBox();

    // --- Pulsanti Start / Pause / Stop --------------------------------------
    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    startButton_ = new wxButton(boxWin, wxID_ANY, tr(StringId::AqStart));
    pauseButton_ = new wxButton(boxWin, wxID_ANY, tr(StringId::AqPause));
    stopButton_  = new wxButton(boxWin, wxID_ANY, tr(StringId::AqStop));
    startButton_->Bind(wxEVT_BUTTON,
                       [this](wxCommandEvent&) { actions_.onAcquisitionStart(); });
    pauseButton_->Bind(wxEVT_BUTTON,
                       [this](wxCommandEvent&) { actions_.onAcquisitionPause(); });
    stopButton_->Bind(wxEVT_BUTTON,
                      [this](wxCommandEvent&) { actions_.onAcquisitionStop(); });
    buttons->Add(startButton_, 1, wxRIGHT, 4);
    buttons->Add(pauseButton_, 1, wxRIGHT, 4);
    buttons->Add(stopButton_, 1);
    box->Add(buttons, 0, wxEXPAND | wxALL, 8);

    // --- Checkbox di attivazione della registrazione CSV -----------------------
    csvLogCheck_ = new wxCheckBox(boxWin, wxID_ANY, tr(StringId::AqCsvCheck));
    csvLogCheck_->SetValue(true);  // default: comportamento invariato (chiede il file)
    csvLogCheck_->SetToolTip(tr(StringId::AqCsvTooltip));
    box->Add(csvLogCheck_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // --- Frequenza: SpinCtrl + Slider sincronizzati ---------------------------
    auto* freqRow = new wxBoxSizer(wxHORIZONTAL);
    freqRow->Add(new wxStaticText(boxWin, wxID_ANY, tr(StringId::AqFreqLabel)), 0,
                 wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    rateSpin_ = new wxSpinCtrl(boxWin, wxID_ANY, wxString(), wxDefaultPosition,
                               wxSize(80, -1), wxSP_ARROW_KEYS,
                               kMinSampleRateHz, kMaxSampleRateHz,
                               kDefaultSampleRateHz);
    freqRow->Add(rateSpin_, 0, wxALIGN_CENTER_VERTICAL);
    box->Add(freqRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    rateSlider_ = new wxSlider(boxWin, wxID_ANY, kDefaultSampleRateHz,
                               kMinSampleRateHz, kMaxSampleRateHz);
    box->Add(rateSlider_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    rateSpin_->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& e) {
        if (!syncing_) { onRateEdited(e.GetValue()); }
    });
    rateSlider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent& e) {
        if (!syncing_) { onRateEdited(e.GetInt()); }
    });

    // --- Etichette informative -------------------------------------------------
    setRateText_ = new wxStaticText(boxWin, wxID_ANY, wxString());
    measuredRateText_ = new wxStaticText(boxWin, wxID_ANY, tr(StringId::AqMeasuredRateNone));
    samplePeriodText_ = new wxStaticText(boxWin, wxID_ANY, tr(StringId::AqSamplePeriodNone));
    box->Add(setRateText_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    box->Add(measuredRateText_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    box->Add(samplePeriodText_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);
}

void AcquisitionPanel::onRateEdited(int rateHz)
{
    // Sincronizzazione bidirezionale spin <-> slider senza ricorsione.
    syncing_ = true;
    rateSpin_->SetValue(rateHz);
    rateSlider_->SetValue(rateHz);
    syncing_ = false;

    // Debounce: il comando parte 250 ms dopo l'ultima modifica.
    pendingRate_ = rateHz;
    debounceTimer_.StartOnce(250);
}

void AcquisitionPanel::onDebounceTimer(wxTimerEvent& WXUNUSED(event))
{
    actions_.onSampleRateChanged(pendingRate_);
}

bool AcquisitionPanel::csvLoggingEnabled() const
{
    return csvLogCheck_->GetValue();
}

void AcquisitionPanel::updateFrom(const BoardState::Snapshot& snapshot)
{
    startButton_->Enable(snapshot.acquisition != AcquisitionState::Running);
    pauseButton_->Enable(snapshot.acquisition == AcquisitionState::Running);
    stopButton_->Enable(snapshot.acquisition != AcquisitionState::Stopped);
    // Non ha senso cambiare idea a registrazione già decisa: la scelta vale
    // solo per il prossimo Start da Stopped (vedi MainController).
    csvLogCheck_->Enable(snapshot.acquisition == AcquisitionState::Stopped);

    setRateText_->SetLabel(
        wxString::Format(tr(StringId::AqSetRateFmt),
                         snapshot.requestedRateHz,
                         snapshot.confirmedRateHz > 0
                             ? wxString::Format("%d Hz", snapshot.confirmedRateHz)
                             : wxString("-")));

    measuredRateText_->SetLabel(
        snapshot.measuredRateHz > 0.0
            ? wxString::Format(tr(StringId::AqMeasuredRateFmt), snapshot.measuredRateHz)
            : tr(StringId::AqMeasuredRateNone));

    const int rate = snapshot.confirmedRateHz > 0 ? snapshot.confirmedRateHz
                                                  : snapshot.requestedRateHz;
    samplePeriodText_->SetLabel(
        rate > 0 ? wxString::Format(tr(StringId::AqSamplePeriodFmt), 1000.0 / rate)
                 : tr(StringId::AqSamplePeriodNone));
}

} // namespace am
