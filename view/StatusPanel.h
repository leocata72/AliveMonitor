/**
 * @file StatusPanel.h
 * @brief Barra inferiore: FPS, frequenza, pacchetti, errori, tempo, CPU.
 *
 * VIEW (MVC). Sola visualizzazione: riceve lo Snapshot del Model e i valori
 * misurati (FPS del grafico, CPU) dal MainController.
 */
#pragma once

#include <optional>

#include <wx/panel.h>

#include "model/BoardState.h"

class wxStaticText;

namespace am {

class StatusPanel : public wxPanel {
public:
    explicit StatusPanel(wxWindow* parent);

    /// Aggiorna tutti i campi.
    /// @param graphFps FPS del grafico misurati dal GraphController
    /// @param cpuUsage percentuale CPU del processo (nullopt = non disponibile)
    void update(const BoardState::Snapshot& snapshot,
                double graphFps,
                std::optional<double> cpuUsage);

private:
    /// Crea un campo etichettato con separatore.
    wxStaticText* addField(wxSizer* sizer, const wxString& initial);

    wxStaticText* fpsText_ = nullptr;
    wxStaticText* rateText_ = nullptr;
    wxStaticText* receivedText_ = nullptr;
    wxStaticText* lostText_ = nullptr;
    wxStaticText* crcText_ = nullptr;
    wxStaticText* serialErrText_ = nullptr;
    wxStaticText* connTimeText_ = nullptr;
    wxStaticText* cpuText_ = nullptr;
};

} // namespace am
