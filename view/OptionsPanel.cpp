/**
 * @file OptionsPanel.cpp
 * @brief Implementazione del riquadro "Opzioni".
 */
#include "view/OptionsPanel.h"

#include <algorithm>
#include <cmath>

#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include "i18n/Strings.h"

namespace am {
namespace {

/// Limiti dello spin "campioni per le statistiche". Il minimo è 2 (con un
/// solo campione la deviazione standard non ha senso); il massimo copre
/// l'intero ring buffer.
constexpr int kMinStatsSamples = 2;
constexpr int kDefaultStatsSamples = 10;

// Colonne della griglia statistiche.
constexpr int kColMean = 0;
constexpr int kColStdDev = 1;
constexpr int kColRms = 2;
constexpr int kColMax = 3;
constexpr int kColMin = 4;
constexpr int kStatsCols = 5;

} // namespace

OptionsPanel::OptionsPanel(wxWindow* parent, IUserActions& actions,
                           const AnalogDataBuffer& buffer,
                           const ChannelCalibrations& calibrations)
    : wxPanel(parent)
    , actions_(actions)
    , buffer_(buffer)
    , calibrations_(calibrations)
{
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::OpBoxTitle));
    wxWindow* boxWin = box->GetStaticBox();

    simultaneousTimersCheck_ = new wxCheckBox(
        boxWin, wxID_ANY, tr(StringId::OpSimultaneousTimers));
    simultaneousTimersCheck_->SetToolTip(tr(StringId::OpSimultaneousTimersTooltip));
    simultaneousTimersCheck_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        actions_.onSimultaneousTimersChanged(e.IsChecked());
    });
    box->Add(simultaneousTimersCheck_, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    // --- Statistiche per canale sugli ultimi N campioni ----------------------
    auto* samplesRow = new wxBoxSizer(wxHORIZONTAL);
    samplesRow->Add(new wxStaticText(boxWin, wxID_ANY,
                                     tr(StringId::OpStatsSamplesLabel)),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    samplesSpin_ = new wxSpinCtrl(boxWin, wxID_ANY, wxString(),
                                  wxDefaultPosition, wxSize(80, -1),
                                  wxSP_ARROW_KEYS, kMinStatsSamples,
                                  static_cast<int>(kRingBufferCapacity),
                                  kDefaultStatsSamples);
    samplesSpin_->SetToolTip(tr(StringId::OpStatsSamplesTooltip));
    // Effetto immediato: il prossimo tick del timer GUI (max 500 ms dopo)
    // rilegge il valore; nessun ricalcolo esplicito necessario qui.
    samplesRow->Add(samplesSpin_, 0, wxALIGN_CENTER_VERTICAL);
    box->Add(samplesRow, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    // Griglia di sola visualizzazione: righe A0..A5 (con l'unità del canale
    // nell'etichetta di riga), colonne media / sigma / max / min. "media" è
    // testo tradotto; sigma, max e min sono notazione universale.
    statsGrid_ = new wxGrid(boxWin, wxID_ANY);
    statsGrid_->CreateGrid(kNumAnalogChannels, kStatsCols);
    statsGrid_->EnableEditing(false);
    statsGrid_->EnableDragRowSize(false);
    statsGrid_->EnableDragColSize(false);
    statsGrid_->DisableDragGridSize();
    statsGrid_->SetCellHighlightPenWidth(0);  // niente cursore cella: è un display
    statsGrid_->SetColLabelValue(kColMean, tr(StringId::OpStatsColMean));
    statsGrid_->SetColLabelValue(kColStdDev, wxString::FromUTF8("σ"));
    statsGrid_->SetColLabelValue(kColRms, "RMS");
    statsGrid_->SetColLabelValue(kColMax, "max");
    statsGrid_->SetColLabelValue(kColMin, "min");
    statsGrid_->SetRowLabelSize(64);  // spazio per "A0 [°C]"
    statsGrid_->SetColLabelSize(20);
    statsGrid_->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        statsGrid_->SetRowLabelValue(ch, wxString::Format("A%d", ch));
        statsGrid_->SetRowSize(ch, 20);
        for (int col = 0; col < kStatsCols; ++col) {
            statsGrid_->SetCellValue(ch, col, wxString::FromUTF8("—"));
        }
    }
    for (int col = 0; col < kStatsCols; ++col) {
        statsGrid_->SetColSize(col, 66);
    }
    box->Add(statsGrid_, 0, wxALL, 8);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);
}

void OptionsPanel::setCell(int row, int col, const wxString& text)
{
    if (statsGrid_->GetCellValue(row, col) != text) {
        statsGrid_->SetCellValue(row, col, text);
    }
}

void OptionsPanel::updateStats()
{
    const auto n = static_cast<std::size_t>(samplesSpin_->GetValue());
    const auto stats = buffer_.lastStats(n);
    const wxString dash = wxString::FromUTF8("—");

    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        const auto& s = stats[chIdx];
        const auto& cal = calibrations_[chIdx];

        // Etichetta di riga con l'unità corrente del canale (letta live:
        // cambia insieme alla calibrazione senza bisogno di notifiche).
        const wxString rowLabel = wxString::Format(
            "A%d [%s]", ch, wxString(cal.unit));
        if (statsGrid_->GetRowLabelValue(ch) != rowLabel) {
            statsGrid_->SetRowLabelValue(ch, rowLabel);
        }

        if (s.count == 0) {
            for (int col = 0; col < kStatsCols; ++col) {
                setCell(ch, col, dash);
            }
            continue;
        }

        // Conversione lineare esatta delle statistiche nella grandezza
        // fisica del canale: media_G = a*media_V + b, sigma_G = |a|*sigma_V;
        // min e max si convertono entrambi e si riordinano, perché un
        // coefficiente a negativo scambia gli estremi.
        const double mean = cal.convert(s.meanVolts);
        const double sigma = std::abs(cal.a) * s.stddevVolts;
        const double e1 = cal.convert(s.minVolts);
        const double e2 = cal.convert(s.maxVolts);
        // RMS totale (componente continua inclusa) dall'identità
        // RMS^2 = media^2 + sigma^2, valida per qualunque segnale e
        // preservata dalla conversione lineare: nessun terzo passaggio sul
        // buffer necessario. NB: la sola componente alternata (AC RMS) è
        // già la colonna sigma.
        const double rms = std::sqrt(mean * mean + sigma * sigma);

        setCell(ch, kColMean, wxString::Format("%.4f", mean));
        setCell(ch, kColStdDev, wxString::Format("%.4f", sigma));
        setCell(ch, kColRms, wxString::Format("%.4f", rms));
        setCell(ch, kColMax, wxString::Format("%.4f", std::max(e1, e2)));
        setCell(ch, kColMin, wxString::Format("%.4f", std::min(e1, e2)));
    }
}

} // namespace am
