/**
 * @file LanguageSelectDialog.cpp
 * @brief Implementazione del dialogo di scelta lingua al primo avvio.
 */
#include "view/LanguageSelectDialog.h"

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "i18n/Strings.h"

namespace am {

LanguageSelectDialog::LanguageSelectDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "AliveMonitor")
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // La lingua non è ancora nota a questo punto (è proprio quello che
    // l'utente sta per scegliere): il prompt è mostrato per esteso in tutte
    // le 5 lingue supportate, non tradotto con tr().
    auto* prompt = new wxStaticText(this, wxID_ANY,
        wxString::FromUTF8(
            "Choose your interface language:\n"
            "Scegli la lingua dell'interfaccia:\n"
            "Choisissez la langue de l'interface :\n"
            "Elige el idioma de la interfaz:\n"
            "Wählen Sie die Sprache der Oberfläche:"));
    sizer->Add(prompt, 0, wxALL, 16);

    languageChoice_ = new wxChoice(this, wxID_ANY);
    for (int i = 0; i < kLanguageCount; ++i) {
        languageChoice_->Append(nativeLanguageName(static_cast<Language>(i)));
    }
    languageChoice_->SetSelection(static_cast<int>(kDefaultLanguage));
    sizer->Add(languageChoice_, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    // Solo OK: la scelta è obbligatoria, non ha senso un Annulla qui (vedi
    // selectedLanguage() nell'header per cosa succede chiudendo con [X]).
    sizer->Add(CreateStdDialogButtonSizer(wxOK), 0,
               wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizerAndFit(sizer);
    CentreOnScreen();
}

Language LanguageSelectDialog::selectedLanguage() const
{
    const int sel = languageChoice_->GetSelection();
    if (sel < 0 || sel >= kLanguageCount) {
        return kDefaultLanguage;  // wxNOT_FOUND o simile: non dovrebbe accadere
    }
    return static_cast<Language>(sel);
}

} // namespace am
