/**
 * @file AcquisitionPanel.h
 * @brief Controlli di acquisizione: Start/Pause/Stop, frequenza (spin + slider).
 *
 * VIEW (MVC). SpinCtrl e Slider sono sincronizzati tra loro; la nuova
 * frequenza viene notificata a IUserActions con un piccolo debounce (250 ms)
 * per non inondare la seriale mentre l'utente trascina lo slider.
 * La frequenza è modificabile anche DURANTE l'acquisizione.
 *
 * Include anche la checkbox "Registra CSV": se spuntata (default) lo Start
 * da acquisizione ferma chiede il file di log (vedi MainController); se
 * smarcata lo Start parte subito, senza alcuna registrazione né dialogo.
 */
#pragma once

#include <wx/panel.h>
#include <wx/timer.h>

#include "IUserActions.h"
#include "model/BoardState.h"

class wxButton;
class wxCheckBox;
class wxSlider;
class wxSpinCtrl;
class wxStaticText;

namespace am {

class AcquisitionPanel : public wxPanel {
public:
    AcquisitionPanel(wxWindow* parent, IUserActions& actions);

    /// Aggiorna pulsanti ed etichette (freq. impostata/confermata/misurata, Ts).
    void updateFrom(const BoardState::Snapshot& snapshot);

    /// true se la checkbox "Registra CSV" è spuntata: MainController la legge
    /// in onAcquisitionStart() per decidere se chiedere il file di log.
    [[nodiscard]] bool csvLoggingEnabled() const;

private:
    void onRateEdited(int rateHz);   ///< Sincronizza i controlli e arma il debounce.
    void onDebounceTimer(wxTimerEvent& event);

    IUserActions& actions_;

    wxButton* startButton_ = nullptr;
    wxButton* pauseButton_ = nullptr;
    wxButton* stopButton_ = nullptr;
    wxCheckBox* csvLogCheck_ = nullptr;
    wxSpinCtrl* rateSpin_ = nullptr;
    wxSlider* rateSlider_ = nullptr;
    wxStaticText* setRateText_ = nullptr;
    wxStaticText* measuredRateText_ = nullptr;
    wxStaticText* samplePeriodText_ = nullptr;

    wxTimer debounceTimer_;
    int pendingRate_ = 0;
    bool syncing_ = false;  ///< Evita ricorsione spin <-> slider.
};

} // namespace am
