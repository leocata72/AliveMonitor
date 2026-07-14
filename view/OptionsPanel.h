/**
 * @file OptionsPanel.h
 * @brief Riquadro "Opzioni" della colonna sinistra (ex segnaposto
 *        "Funzionalità future" della 1.2).
 *
 * VIEW (MVC). Contiene le opzioni di comportamento dell'applicazione che non
 * meritano il dialogo Impostazioni (sono legate all'uso corrente, non alla
 * configurazione persistente). Oggi: avvio contemporaneo delle uscite
 * temporizzate — se spuntato, il primo ON su un'uscita con "Temporizzato"
 * attivo avvia anche tutte le altre uscite temporizzate configurate.
 * Le opzioni notificano IUserActions; nessuno stato applicativo vive qui.
 */
#pragma once

#include <wx/panel.h>

#include "IUserActions.h"

class wxCheckBox;

namespace am {

class OptionsPanel : public wxPanel {
public:
    OptionsPanel(wxWindow* parent, IUserActions& actions);

private:
    IUserActions& actions_;
    wxCheckBox* simultaneousTimersCheck_ = nullptr;
};

} // namespace am
