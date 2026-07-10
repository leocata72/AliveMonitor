/**
 * @file SettingsDialog.cpp
 * @brief Implementazione del dialogo delle impostazioni.
 */
#include "view/SettingsDialog.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include "Version.h"

namespace am {

SettingsDialog::SettingsDialog(wxWindow* parent,
                               int renderFps,
                               double timeWindowSeconds,
                               bool continuousAutoscaleY)
    : wxDialog(parent, wxID_ANY, "Impostazioni")
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* grid = new wxFlexGridSizer(2, 8, 12);
    grid->AddGrowableCol(1, 1);

    grid->Add(new wxStaticText(this, wxID_ANY, "FPS del grafico:"), 0,
              wxALIGN_CENTER_VERTICAL);
    fpsSpin_ = new wxSpinCtrl(this, wxID_ANY, wxString(), wxDefaultPosition,
                              wxSize(90, -1), wxSP_ARROW_KEYS,
                              kMinRenderFps, kMaxRenderFps, renderFps);
    grid->Add(fpsSpin_, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, "Finestra temporale [s]:"), 0,
              wxALIGN_CENTER_VERTICAL);
    windowSpin_ = new wxSpinCtrl(this, wxID_ANY, wxString(), wxDefaultPosition,
                                 wxSize(90, -1), wxSP_ARROW_KEYS,
                                 5, 120,
                                 static_cast<int>(timeWindowSeconds));
    grid->Add(windowSpin_, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, "Autoscale Y continuo:"), 0,
              wxALIGN_CENTER_VERTICAL);
    autoYCheck_ = new wxCheckBox(this, wxID_ANY, wxString());
    autoYCheck_->SetValue(continuousAutoscaleY);
    grid->Add(autoYCheck_, 0);

    sizer->Add(grid, 1, wxEXPAND | wxALL, 16);
    sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
               wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizerAndFit(sizer);
    CentreOnParent();
}

int SettingsDialog::renderFps() const
{
    return fpsSpin_->GetValue();
}

double SettingsDialog::timeWindowSeconds() const
{
    return static_cast<double>(windowSpin_->GetValue());
}

bool SettingsDialog::continuousAutoscaleY() const
{
    return autoYCheck_->GetValue();
}

} // namespace am
