/**
 * @file OptionsPanel.cpp
 * @brief Implementazione del riquadro "Opzioni".
 */
#include "view/OptionsPanel.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

#include "i18n/Strings.h"

namespace am {

OptionsPanel::OptionsPanel(wxWindow* parent, IUserActions& actions)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(StringId::OpBoxTitle));
    wxWindow* boxWin = box->GetStaticBox();

    simultaneousTimersCheck_ = new wxCheckBox(
        boxWin, wxID_ANY, tr(StringId::OpSimultaneousTimers));
    simultaneousTimersCheck_->SetToolTip(tr(StringId::OpSimultaneousTimersTooltip));
    simultaneousTimersCheck_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        actions_.onSimultaneousTimersChanged(e.IsChecked());
    });
    box->Add(simultaneousTimersCheck_, 0, wxALL, 8);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(box, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);
}

} // namespace am
