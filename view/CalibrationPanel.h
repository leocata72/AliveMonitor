/**
 * @file CalibrationPanel.h
 * @brief Griglia di calibrazione lineare per canale (G = a*V + b, unità).
 *
 * VIEW (MVC). Una wxGrid con una riga per canale analogico (A0..A5) e
 * cinque colonne (a, b, unità, descrizione, marker). Ogni modifica di cella
 * notifica immediatamente IUserActions (onCalibrationChanged per i parametri,
 * onChannelMarkerChanged per il marker); non tiene una copia propria dei dati
 * (la fonte di verità resta in MainController), la griglia È l'editor visuale
 * di quel dato. La descrizione è un'etichetta libera (es. "Temperatura")
 * usata da MainController come titolo della scheda dedicata al canale nel
 * grafico (GraphPanel). Sotto la griglia, i pulsanti Salva.../Carica...
 * delegano a MainController la persistenza su file di testo (v1.2, vedi
 * ChannelConfigFile).
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

    /// Riallinea tutte le celle ai valori passati SENZA emettere notifiche
    /// (SetCellValue programmatico non genera wxEVT_GRID_CELL_CHANGED):
    /// usata da MainController dopo il caricamento di un file di
    /// configurazione, quando la fonte di verità è cambiata fuori dalla griglia.
    void refreshFromCalibrations(const ChannelCalibrations& calibrations);

private:
    void onCellChanged(wxGridEvent& event);
    /// Legge la riga dalla griglia e notifica il Controller.
    void notifyChannel(int channel);
    /// Nome localizzato dello stile di marker con indice idx (0..8).
    [[nodiscard]] static wxString markerName(int idx);

    IUserActions& actions_;
    wxGrid* grid_ = nullptr;
};

} // namespace am
