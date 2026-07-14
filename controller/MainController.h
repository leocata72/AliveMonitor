/**
 * @file MainController.h
 * @brief Controller principale: possiede il Model, coordina View e Controller.
 *
 * CONTROLLER (MVC). È il punto di composizione dell'applicazione:
 *  - possiede tutti i Model (BoardState, SerialModel, DigitalOutputState,
 *    AnalogDataBuffer) e gli altri Controller;
 *  - implementa IUserActions: riceve le azioni dell'utente dalle View;
 *  - riceve gli eventi del thread seriale (wxThreadEvent) e aggiorna le View;
 *  - gestisce il ciclo di vita (avvio thread/timer, chiusura ordinata).
 *
 * Le View non conoscono questa classe: vedono solo IUserActions e i Model
 * in sola lettura.
 */
#pragma once

#include <wx/event.h>
#include <wx/timer.h>

#include "CpuMonitor.h"
#include "IUserActions.h"
#include "controller/CommandController.h"
#include "controller/CsvLoggerController.h"
#include "controller/GraphController.h"
#include "controller/SerialController.h"
#include "controller/TimedOutputController.h"
#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"
#include "model/ChannelCalibration.h"
#include "model/DigitalOutputState.h"
#include "model/SerialModel.h"

class wxSplashScreen;  // forward (classe wx globale, non in namespace am)

namespace am {

class MainFrame;  // forward: incluso solo nel .cpp

class MainController : public wxEvtHandler, public IUserActions {
public:
    MainController();
    ~MainController() override;

    MainController(const MainController&) = delete;
    MainController& operator=(const MainController&) = delete;

    /// Imposta la lingua dell'interfaccia: dal file se già scelta in
    /// precedenza, altrimenti la chiede con LanguageSelectDialog. Va chiamata
    /// da main.cpp PRIMA di creare la splash screen (non da initialize()):
    /// la splash usa wxSTAY_ON_TOP, quindi un dialogo modale mostrato dopo
    /// la sua creazione resterebbe coperto e invisibile.
    void ensureLanguage();

    /// Crea la finestra principale, collega gli eventi e avvia thread e timer.
    /// @param splash Splash screen già mostrata da main.cpp (opzionale): resta
    ///        visibile finché non arriva la prima connessione riuscita al
    ///        firmware, poi viene chiusa da qui (vedi onConnectionChanged).
    void initialize(wxSplashScreen* splash = nullptr);

    /// Chiusura ordinata e idempotente: ferma timer e thread seriale.
    void shutdown();

    // --- IUserActions (azioni provenienti dalle View) -----------------------
    void onConnectRequested() override;
    void onDisconnectRequested() override;
    void onDigitalOutputToggled(int pin, bool on) override;
    void onTimedOutputStarted(int pin, double onSeconds,
                              double periodSeconds, bool oneShot) override;
    void onTimedOutputStopped(int pin) override;
    void onSimultaneousTimersChanged(bool enabled) override;
    void onAcquisitionStart() override;
    void onAcquisitionStop() override;
    void onAcquisitionPause() override;
    void onSampleRateChanged(int rateHz) override;
    void onCalibrationChanged(int channel, double a, double b,
                              const wxString& unit, const wxString& label) override;
    void onChannelMarkerChanged(int channel, int markerIndex) override;
    void onChannelConfigSaveRequested() override;
    void onChannelConfigLoadRequested() override;
    void onExportPngRequested() override;
    void onSettingsRequested() override;
    void onShutdown() override;

private:
    // --- Eventi dal thread seriale (eseguiti nel main thread) ----------------
    void onConnectionChanged(wxThreadEvent& event);
    void onStatsUpdated(wxThreadEvent& event);
    void onOutputStateChanged(wxThreadEvent& event);
    void onFirmwareInfo(wxThreadEvent& event);
    void onRateConfirmed(wxThreadEvent& event);
    void onProtocolError(wxThreadEvent& event);

    /// La splash screen si è distrutta (Close() proprio o timeout interno di
    /// sicurezza): azzera splash_ per non tenere un puntatore pendente.
    void onSplashDestroyed(wxWindowDestroyEvent& event);

    /// Timer GUI (2 Hz): aggiorna barra superiore, inferiore e pannelli.
    void onUiTimer(wxTimerEvent& event);

    /// Rilegge lo Snapshot del Model e aggiorna tutte le View di stato.
    void refreshStatusViews();

    // --- MODEL (posseduto qui: unico proprietario) -----------------------------
    BoardState state_;
    SerialModel serialModel_;
    DigitalOutputState outputs_;
    AnalogDataBuffer buffer_;
    ChannelCalibrations calibrations_;  ///< Identità (a=1,b=0,"V") finché non configurata.

    // --- CONTROLLER ------------------------------------------------------------
    // NOTA: dichiarati dopo i Model perché li ricevono per riferimento
    // (l'ordine di inizializzazione dei membri segue quello di dichiarazione).
    // csvLogger_ precede serial_ perché quest'ultimo la riceve per riferimento
    // (produttore: push() a ogni campione ADC ricevuto durante l'acquisizione).
    CsvLoggerController csvLogger_;
    SerialController serial_;
    CommandController commands_;
    GraphController graph_;
    TimedOutputController timedOutputs_;  ///< Cicli ON/OFF delle uscite (v1.2).

    // --- Servizi ------------------------------------------------------------------
    CpuMonitor cpuMonitor_;
    wxTimer uiTimer_;
    wxString lastMessage_;  ///< Ultimo messaggio di stato/errore da mostrare.

    // --- VIEW (posseduta dal framework wx: puntatore non proprietario) -------------
    MainFrame* frame_ = nullptr;
    wxSplashScreen* splash_ = nullptr;  ///< Chiusa alla prima connessione riuscita.

    bool shutdownDone_ = false;
};

} // namespace am
