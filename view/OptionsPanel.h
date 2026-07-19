/**
 * @file OptionsPanel.h
 * @brief Riquadro "Opzioni" della colonna sinistra.
 *
 * VIEW (MVC). Contiene:
 *  - l'opzione "Avvio contemporaneo temporizzatori" (notificata via
 *    IUserActions, lo stato applicativo non vive qui);
 *  - le statistiche live per canale analogico (v1.2) in una wxGrid di sola
 *    lettura: media, deviazione standard, massimo e minimo degli ultimi N
 *    campioni del ring buffer, con N impostabile da wxSpinCtrl (default 10).
 *    Righe A0..A5 (l'etichetta di riga include l'unità del canale), valori
 *    nella grandezza CONVERTITA (G = a*V + b): per una trasformazione
 *    lineare la conversione delle statistiche è esatta (media_G = a*media_V
 *    + b, sigma_G = |a|*sigma_V, min/max convertiti e riordinati se a < 0),
 *    quindi si calcola in Volt nel Model e si converte qui.
 *    L'aggiornamento arriva dal timer GUI di MainController (2 Hz) via
 *    updateStats().
 */
#pragma once

#include <wx/panel.h>

#include "IUserActions.h"
#include "Version.h"
#include "model/AnalogDataBuffer.h"
#include "model/ChannelCalibration.h"

class wxCheckBox;
class wxGrid;
class wxSpinCtrl;

namespace am {

class OptionsPanel : public wxPanel {
public:
    OptionsPanel(wxWindow* parent, IUserActions& actions,
                 const AnalogDataBuffer& buffer,
                 const ChannelCalibrations& calibrations);

    /// Ricalcola e mostra media/sigma/max/min degli ultimi N campioni per
    /// canale. Chiamata dal timer GUI (2 Hz): il costo è N*6 letture sotto
    /// il mutex del buffer, trascurabile per gli N tipici di questo pannello.
    void updateStats();

private:
    /// Scrive la cella solo se il testo è cambiato (niente ridisegni inutili).
    void setCell(int row, int col, const wxString& text);

    IUserActions& actions_;
    const AnalogDataBuffer& buffer_;
    const ChannelCalibrations& calibrations_;  ///< Letta live (unità e a,b correnti).

    wxCheckBox* simultaneousTimersCheck_ = nullptr;
    wxSpinCtrl* samplesSpin_ = nullptr;
    wxGrid* statsGrid_ = nullptr;
};

} // namespace am
