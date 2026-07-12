/**
 * @file CalibrationPanel.cpp
 * @brief Implementazione della griglia di calibrazione per canale.
 */
#include "view/CalibrationPanel.h"

#include <wx/generic/gridctrl.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

#include "i18n/Strings.h"

namespace am {
namespace {

constexpr int kColA = 0;
constexpr int kColB = 1;
constexpr int kColUnit = 2;
constexpr int kColLabel = 3;

} // namespace

CalibrationPanel::CalibrationPanel(wxWindow* parent, IUserActions& actions,
                                   const ChannelCalibrations& initial)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::CalBoxTitle));
    wxWindow* boxWin = box->GetStaticBox();

    grid_ = new wxGrid(boxWin, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    grid_->CreateGrid(kNumAnalogChannels, 4);
    // "a"/"b" sono lettere di variabile matematica (G = a·V + b), non testo
    // da tradurre.
    grid_->SetColLabelValue(kColA, "a");
    grid_->SetColLabelValue(kColB, "b");
    grid_->SetColLabelValue(kColUnit, tr(StringId::CalColUnit));
    grid_->SetColLabelValue(kColLabel, tr(StringId::CalColDescription));
    grid_->SetRowLabelSize(34);
    grid_->SetColSize(kColA, 52);
    grid_->SetColSize(kColB, 52);
    grid_->SetColSize(kColUnit, 52);
    grid_->SetColSize(kColLabel, 100);
    grid_->EnableDragRowSize(false);
    grid_->EnableDragColSize(false);
    grid_->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    grid_->DisableDragGridSize();

    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto& c = initial[static_cast<std::size_t>(ch)];
        grid_->SetRowLabelValue(ch, wxString::Format("A%d", ch));
        grid_->SetCellEditor(ch, kColA, new wxGridCellFloatEditor());
        grid_->SetCellEditor(ch, kColB, new wxGridCellFloatEditor());
        grid_->SetCellValue(ch, kColA, wxString::FromDouble(c.a));
        grid_->SetCellValue(ch, kColB, wxString::FromDouble(c.b));
        grid_->SetCellValue(ch, kColUnit, wxString(c.unit));
        grid_->SetCellValue(ch, kColLabel, wxString(c.label));
    }

    grid_->Bind(wxEVT_GRID_CELL_CHANGED, &CalibrationPanel::onCellChanged, this);

    box->Add(grid_, 0, wxALL, 8);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);
}

void CalibrationPanel::onCellChanged(wxGridEvent& event)
{
    notifyChannel(event.GetRow());
}

void CalibrationPanel::notifyChannel(int channel)
{
    if (channel < 0 || channel >= kNumAnalogChannels) {
        return;
    }
    double a = 1.0;
    double b = 0.0;
    // ToDouble lascia a/b invariati (default sopra) se la cella è vuota o
    // non numerica: la griglia non può mai produrre una calibrazione a
    // metà scritta che manda in errore chi la legge.
    grid_->GetCellValue(channel, kColA).ToDouble(&a);
    grid_->GetCellValue(channel, kColB).ToDouble(&b);
    wxString unit = grid_->GetCellValue(channel, kColUnit);
    if (unit.IsEmpty()) {
        unit = "V";
    }
    const wxString label = grid_->GetCellValue(channel, kColLabel);
    actions_.onCalibrationChanged(channel, a, b, unit, label);
}

} // namespace am
