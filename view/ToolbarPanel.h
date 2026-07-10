/**
 * @file ToolbarPanel.h
 * @brief Barra superiore: stato connessione, porta, baud, LED, versione FW,
 *        pulsanti Connetti/Disconnetti.
 *
 * VIEW (MVC). Non contiene logica: mostra lo Snapshot del Model e inoltra
 * i click a IUserActions.
 */
#pragma once

#include <wx/panel.h>

#include "IUserActions.h"
#include "model/BoardState.h"

class wxButton;
class wxStaticText;

namespace am {

class LedIndicator;

class ToolbarPanel : public wxPanel {
public:
    ToolbarPanel(wxWindow* parent, IUserActions& actions);

    /// Aggiorna tutte le etichette dallo Snapshot del Model.
    /// @param message eventuale messaggio di stato (es. motivo disconnessione).
    void updateFrom(const BoardState::Snapshot& snapshot,
                    const wxString& message);

private:
    IUserActions& actions_;

    LedIndicator* led_ = nullptr;
    wxStaticText* statusText_ = nullptr;
    wxStaticText* portText_ = nullptr;
    wxStaticText* baudText_ = nullptr;
    wxStaticText* firmwareText_ = nullptr;
    wxStaticText* messageText_ = nullptr;
    wxButton* connectButton_ = nullptr;
    wxButton* disconnectButton_ = nullptr;
};

} // namespace am
