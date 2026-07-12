/**
 * @file SettingsDialog.cpp
 * @brief Implementazione del dialogo delle impostazioni.
 */
#include "view/SettingsDialog.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include "Version.h"
#include "i18n/Strings.h"

namespace am {

SettingsDialog::SettingsDialog(wxWindow* parent,
                               int renderFps,
                               double timeWindowSeconds,
                               bool continuousAutoscaleY)
    : wxDialog(parent, wxID_ANY, tr(StringId::SdTitle))
    , initialLanguage_(currentLanguage())
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* grid = new wxFlexGridSizer(2, 8, 12);
    grid->AddGrowableCol(1, 1);

    grid->Add(new wxStaticText(this, wxID_ANY, tr(StringId::SdFpsLabel)), 0,
              wxALIGN_CENTER_VERTICAL);
    fpsSpin_ = new wxSpinCtrl(this, wxID_ANY, wxString(), wxDefaultPosition,
                              wxSize(90, -1), wxSP_ARROW_KEYS,
                              kMinRenderFps, kMaxRenderFps, renderFps);
    grid->Add(fpsSpin_, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, tr(StringId::SdWindowLabel)), 0,
              wxALIGN_CENTER_VERTICAL);
    windowSpin_ = new wxSpinCtrl(this, wxID_ANY, wxString(), wxDefaultPosition,
                                 wxSize(90, -1), wxSP_ARROW_KEYS,
                                 5, 120,
                                 static_cast<int>(timeWindowSeconds));
    grid->Add(windowSpin_, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, tr(StringId::SdAutoYLabel)), 0,
              wxALIGN_CENTER_VERTICAL);
    autoYCheck_ = new wxCheckBox(this, wxID_ANY, wxString());
    autoYCheck_->SetValue(continuousAutoscaleY);
    grid->Add(autoYCheck_, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, tr(StringId::SdLanguageLabel)), 0,
              wxALIGN_CENTER_VERTICAL);
    languageChoice_ = new wxChoice(this, wxID_ANY);
    for (int i = 0; i < kLanguageCount; ++i) {
        languageChoice_->Append(nativeLanguageName(static_cast<Language>(i)));
    }
    languageChoice_->SetSelection(static_cast<int>(initialLanguage_));
    languageChoice_->Bind(wxEVT_CHOICE, &SettingsDialog::onLanguageChoice, this);
    grid->Add(languageChoice_, 0);

    sizer->Add(grid, 1, wxEXPAND | wxALL, 16);

    // Avviso di riavvio: nascosto finché la selezione non cambia davvero
    // rispetto a initialLanguage_ (vedi onLanguageChoice).
    restartNotice_ = new wxStaticText(this, wxID_ANY, tr(StringId::SdRestartNotice));
    restartNotice_->SetForegroundColour(wxColour(150, 90, 0));
    restartNotice_->Hide();
    sizer->Add(restartNotice_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);

    sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
               wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizerAndFit(sizer);
    CentreOnParent();
}

void SettingsDialog::onLanguageChoice(wxCommandEvent& WXUNUSED(event))
{
    const bool changed = selectedLanguage() != initialLanguage_;
    if (restartNotice_->IsShown() != changed) {
        restartNotice_->Show(changed);
        // Fit() (non solo Layout()): la riga dell'avviso comparendo/
        // sparendo cambia l'altezza minima del sizer, e senza ridimensionare
        // anche la finestra del dialogo il testo si sovrapporrebbe ai
        // pulsanti OK/Annulla sottostanti.
        Fit();
    }
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

Language SettingsDialog::selectedLanguage() const
{
    const int sel = languageChoice_->GetSelection();
    if (sel < 0 || sel >= kLanguageCount) {
        return initialLanguage_;  // wxNOT_FOUND o simile: non dovrebbe accadere
    }
    return static_cast<Language>(sel);
}

} // namespace am
