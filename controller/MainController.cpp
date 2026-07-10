/**
 * @file MainController.cpp
 * @brief Implementazione del controller principale (composizione e coordinamento).
 */
#include "controller/MainController.h"

#include <wx/datetime.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/splash.h>

#include "AppEvents.h"
#include "Version.h"
#include "view/AcquisitionPanel.h"
#include "view/DigitalOutputPanel.h"
#include "view/GraphPanel.h"
#include "view/MainFrame.h"
#include "view/SettingsDialog.h"
#include "view/StatusPanel.h"
#include "view/ToolbarPanel.h"

namespace am {

MainController::MainController()
    : buffer_(kRingBufferCapacity)
    , serial_(state_, serialModel_, outputs_, buffer_, csvLogger_)
    , commands_(serial_, state_, outputs_, buffer_)
    , graph_(buffer_, state_)
    , uiTimer_(this)
{
}

MainController::~MainController()
{
    shutdown();
}

void MainController::initialize(wxSplashScreen* splash)
{
    // 0) Splash screen (opzionale, mostrata da main.cpp prima di questa
    //    chiamata): resta finché non arriva la prima connessione riuscita
    //    (onConnectionChanged) oppure scatta il suo timeout di sicurezza. In
    //    entrambi i casi wxEVT_DESTROY azzera splash_, quindi non teniamo
    //    mai un puntatore pendente.
    splash_ = splash;
    if (splash_ != nullptr) {
        splash_->Bind(wxEVT_DESTROY, &MainController::onSplashDestroyed, this);
    }

    // 1) Creazione della View. Il frame è posseduto da wxWidgets:
    //    verrà distrutto dal framework alla chiusura.
    frame_ = new MainFrame(*this, state_, outputs_, buffer_, calibrations_);

    // 2) Collegamento Controller <-> View.
    graph_.attachPanel(frame_->graphPanel());

    // 3) Eventi dal thread seriale verso questo handler (main thread).
    serial_.setEventSink(this);
    Bind(events::EVT_CONNECTION_CHANGED, &MainController::onConnectionChanged, this);
    Bind(events::EVT_STATS_UPDATED, &MainController::onStatsUpdated, this);
    Bind(events::EVT_OUTPUT_STATE_CHANGED, &MainController::onOutputStateChanged, this);
    Bind(events::EVT_FIRMWARE_INFO, &MainController::onFirmwareInfo, this);
    Bind(events::EVT_RATE_CONFIRMED, &MainController::onRateConfirmed, this);
    Bind(events::EVT_PROTOCOL_ERROR, &MainController::onProtocolError, this);
    Bind(wxEVT_TIMER, &MainController::onUiTimer, this);

    // 4) Avvio.
    frame_->Show(true);
    refreshStatusViews();
    serial_.start();          // thread seriale: scansione automatica immediata
    graph_.startRendering();  // timer di rendering indipendente
    uiTimer_.Start(500);      // aggiornamento etichette di stato a 2 Hz
}

void MainController::shutdown()
{
    if (shutdownDone_) {
        return;
    }
    shutdownDone_ = true;

    uiTimer_.Stop();
    graph_.stopRendering();
    serial_.stop();     // join del thread seriale: nessun altro push dopo questo punto
    csvLogger_.stop();  // drena la coda e attende che il consumatore finisca di scrivere
    frame_ = nullptr;
}

// ---------------------------------------------------------------------------
// IUserActions
// ---------------------------------------------------------------------------

void MainController::onConnectRequested()
{
    lastMessage_.clear();
    serial_.setAutoConnect(true);
}

void MainController::onDisconnectRequested()
{
    serial_.setAutoConnect(false);
}

void MainController::onDigitalOutputToggled(int pin, bool on)
{
    commands_.setDigitalOutput(pin, on);
}

void MainController::onAcquisitionStart()
{
    if (frame_ == nullptr) {
        return;
    }
    // Una NUOVA sessione (da Stopped) chiede prima dove salvare il CSV, ma
    // solo se la checkbox "Registra CSV" è spuntata: se l'utente l'ha
    // smarcata, Start parte subito senza dialogo e senza registrazione. Da
    // Paused si riprende invece lo streaming sullo stesso file già aperto
    // (nessun nuovo dialogo: il file resta lo stesso per l'intera sessione,
    // "buchi" di pausa inclusi, coerente con la semantica di startAcquisition
    // in CommandController).
    if (state_.acquisition() == AcquisitionState::Stopped &&
        frame_->acquisitionPanel()->csvLoggingEnabled()) {
        const wxString defaultName = wxString::Format(
            "acquisizione_%s.csv",
            wxDateTime::Now().Format("%Y-%m-%d_%H-%M-%S"));
        wxFileDialog dialog(frame_, "Salva registrazione CSV", wxString(),
                            defaultName, "File CSV (*.csv)|*.csv",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dialog.ShowModal() != wxID_OK) {
            return;  // annullato dall'utente: l'acquisizione non parte
        }
        if (!csvLogger_.start(dialog.GetPath(), calibrations_)) {
            wxMessageBox("Impossibile aprire il file CSV per la scrittura.",
                         kAppName, wxICON_ERROR | wxOK, frame_);
            return;
        }
        frame_->graphPanel()->setCsvPath(csvLogger_.currentPath());
    }
    commands_.startAcquisition();
    refreshStatusViews();
}

void MainController::onAcquisitionStop()
{
    commands_.stopAcquisition();
    csvLogger_.stop();  // attende che il consumatore finisca di scrivere il file
    if (frame_ != nullptr) {
        frame_->graphPanel()->clearCsvPath();
    }
    refreshStatusViews();
}

void MainController::onAcquisitionPause()
{
    commands_.pauseAcquisition();
    refreshStatusViews();
}

void MainController::onSampleRateChanged(int rateHz)
{
    commands_.setSampleRate(rateHz);
}

void MainController::onCalibrationChanged(int channel, double a, double b,
                                          const wxString& unit, const wxString& label)
{
    if (channel < 0 || channel >= kNumAnalogChannels) {
        return;
    }
    calibrations_[static_cast<std::size_t>(channel)] =
        ChannelCalibration{ a, b, unit.ToStdString(), label.ToStdString() };
    // Nessun refreshStatusViews(): la legenda del grafico legge calibrations_
    // direttamente (const&) ad ogni frame di rendering, già in corso a
    // 10..60 FPS indipendentemente da questa modifica. Il titolo della
    // scheda dedicata al canale, invece, NON è riletto a ogni frame (è
    // testo statico del wxNotebook): va aggiornato esplicitamente qui.
    if (frame_ != nullptr) {
        frame_->graphPanel()->setChannelTabTitle(channel, label);
    }
}

void MainController::onExportPngRequested()
{
    if (frame_ == nullptr) {
        return;
    }
    wxFileDialog dialog(frame_, "Esporta grafico come PNG", wxString(),
                        "grafico.png", "Immagine PNG (*.png)|*.png",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    if (!graph_.exportPng(dialog.GetPath())) {
        wxMessageBox("Esportazione PNG non riuscita.", kAppName,
                     wxICON_ERROR | wxOK, frame_);
    }
}

void MainController::onSettingsRequested()
{
    if (frame_ == nullptr) {
        return;
    }
    SettingsDialog dialog(frame_,
                          graph_.renderFps(),
                          frame_->graphPanel()->timeWindowSeconds(),
                          frame_->graphPanel()->continuousAutoscaleY());
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    graph_.setRenderFps(dialog.renderFps());
    frame_->graphPanel()->setTimeWindowSeconds(dialog.timeWindowSeconds());
    frame_->graphPanel()->setContinuousAutoscaleY(dialog.continuousAutoscaleY());
}

void MainController::onShutdown()
{
    shutdown();
}

// ---------------------------------------------------------------------------
// Eventi dal thread seriale (già nel main thread grazie a wxQueueEvent)
// ---------------------------------------------------------------------------

void MainController::onConnectionChanged(wxThreadEvent& event)
{
    lastMessage_ = event.GetString();
    const bool connected = (event.GetInt() == 1);
    if (frame_ != nullptr) {
        frame_->digitalPanel()->setControlsEnabled(connected);
        if (!connected) {
            // Lo stato reale delle uscite non è più noto.
            frame_->digitalPanel()->clearLeds();
        }
    }
    if (connected && splash_ != nullptr) {
        // Prima connessione riuscita: la splash ha assolto il suo scopo.
        // onSplashDestroyed azzererà splash_ quando la distruzione avviene
        // davvero (Close() la nasconde e la accoda per la distruzione).
        splash_->Close();
    }
    refreshStatusViews();
}

void MainController::onSplashDestroyed(wxWindowDestroyEvent& WXUNUSED(event))
{
    splash_ = nullptr;
}

void MainController::onStatsUpdated(wxThreadEvent& WXUNUSED(event))
{
    // Le etichette vengono aggiornate dal timer GUI a 2 Hz: qui non serve
    // fare nulla di costoso. L'evento resta utile come "hook" di estensione
    // (es. logging su file/database a ogni aggiornamento statistiche).
}

void MainController::onOutputStateChanged(wxThreadEvent& event)
{
    if (frame_ != nullptr) {
        frame_->digitalPanel()->setLed(event.GetInt(), event.GetExtraLong() != 0);
    }
}

void MainController::onFirmwareInfo(wxThreadEvent& WXUNUSED(event))
{
    refreshStatusViews();
}

void MainController::onRateConfirmed(wxThreadEvent& WXUNUSED(event))
{
    refreshStatusViews();
}

void MainController::onProtocolError(wxThreadEvent& event)
{
    lastMessage_ = "ERR: " + event.GetString();
    refreshStatusViews();
}

// ---------------------------------------------------------------------------
// Aggiornamento periodico delle View di stato
// ---------------------------------------------------------------------------

void MainController::onUiTimer(wxTimerEvent& WXUNUSED(event))
{
    refreshStatusViews();
}

void MainController::refreshStatusViews()
{
    if (frame_ == nullptr) {
        return;
    }
    const auto snap = state_.snapshot();
    frame_->toolbarPanel()->updateFrom(snap, lastMessage_);
    frame_->acquisitionPanel()->updateFrom(snap);
    frame_->statusPanel()->update(snap, graph_.measuredFps(),
                                  cpuMonitor_.sample());
}

} // namespace am
