/**
 * @file CalibrationPanel.h
 * @brief Griglia di calibrazione lineare per canale (G = a*V + b, unità).
 *
 * VIEW (MVC). Una wxGrid con una riga per canale analogico (A0..A5) e
 * quattro colonne (a, b, unità, descrizione). Ogni modifica di cella
 * notifica immediatamente IUserActions::onCalibrationChanged(); non tiene
 * una copia propria dei dati (la fonte di verità resta in MainController),
 * la griglia È l'editor visuale di quel dato. La descrizione è un'etichetta
 * libera (es. "Temperatura") usata da MainController come titolo della
 * scheda dedicata al canale nel grafico (GraphPanel).
 */
#pragma once

#include <wx/panel.h>

#include "IUserActions.h"
#include "model/ChannelCalibration.h"

class wxGrid;
class wxGridEvent;

namespace am {

class CalibrationPanel : public wxPanel {
public:
    /// @param initial valori iniziali con cui popolare la griglia (di norma
    ///        l'identità a=1,b=0,unit="V" per ogni canale, non ancora configurato).
    CalibrationPanel(wxWindow* parent, IUserActions& actions,
                      const ChannelCalibrations& initial);

private:
    void onCellChanged(wxGridEvent& event);
    /// Legge la riga dalla griglia e notifica il Controller.
    void notifyChannel(int channel);

    IUserActions& actions_;
    wxGrid* grid_ = nullptr;
};

} // namespace am
