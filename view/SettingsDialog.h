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

class wxCheckBox;
class wxSpinCtrl;

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

private:
    wxSpinCtrl* fpsSpin_ = nullptr;
    wxSpinCtrl* windowSpin_ = nullptr;
    wxCheckBox* autoYCheck_ = nullptr;
};

} // namespace am
