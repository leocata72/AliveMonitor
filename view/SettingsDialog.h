/**
 * @file SettingsDialog.h
 * @brief Dialogo delle impostazioni: FPS di rendering, finestra temporale,
 *        autoscale Y continuo.
 *
 * VIEW (MVC). Dialogo modale: il MainController lo costruisce con i valori
 * correnti e, se l'utente conferma, legge i nuovi valori dai getter.
 */
#pragma once

#include <wx/dialog.h>

#include "Language.h"

class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
class wxStaticText;

namespace am {

class SettingsDialog : public wxDialog {
public:
    SettingsDialog(wxWindow* parent,
                   int renderFps,
                   double timeWindowSeconds,
                   bool continuousAutoscaleY);

    [[nodiscard]] int renderFps() const;
    [[nodiscard]] double timeWindowSeconds() const;
    [[nodiscard]] bool continuousAutoscaleY() const;

    /// Lingua selezionata nel wxChoice. Il cambio lingua richiede il
    /// riavvio dell'applicazione (nessuna ritraduzione a caldo dei widget
    /// già costruiti): il chiamante confronta con currentLanguage() e, se
    /// diversa, la salva soltanto su AppSettings senza applicarla subito.
    [[nodiscard]] Language selectedLanguage() const;

private:
    /// Mostra/nasconde l'avviso di riavvio quando la selezione cambia
    /// rispetto alla lingua attiva all'apertura del dialogo.
    void onLanguageChoice(wxCommandEvent& event);

    wxSpinCtrl* fpsSpin_ = nullptr;
    wxSpinCtrl* windowSpin_ = nullptr;
    wxCheckBox* autoYCheck_ = nullptr;
    wxChoice* languageChoice_ = nullptr;
    wxStaticText* restartNotice_ = nullptr;
    Language initialLanguage_ = kDefaultLanguage;
};

} // namespace am
