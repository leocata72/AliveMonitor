/**
 * @file GraphController.h
 * @brief Controller del rendering del grafico: disaccoppia acquisizione e video.
 *
 * CONTROLLER (MVC). L'acquisizione riempie il buffer a 250 Hz; questo
 * controller, con un wxTimer nel main thread, chiede al GraphPanel di
 * ridisegnarsi a una frequenza indipendente e selezionabile (10..60 FPS).
 *
 *      Acquisizione (thread seriale, 250 Hz)
 *              |
 *              v
 *      AnalogDataBuffer (ring buffer thread-safe)
 *              ^
 *              |  copyWindow() a 10..60 FPS
 *      GraphController::onRenderTimer  ->  GraphPanel::Refresh()
 *
 * Gestisce inoltre l'esportazione PNG e la misura degli FPS reali. Il
 * logging CSV continuo (produttore/consumatore) è invece gestito da
 * CsvLoggerController, alimentato direttamente dal thread seriale.
 */
#pragma once

#include <chrono>

#include <wx/event.h>
#include <wx/timer.h>

#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"

namespace am {

class GraphPanel;  // forward: il controller non richiede l'header completo

class GraphController : public wxEvtHandler {
public:
    GraphController(AnalogDataBuffer& buffer, BoardState& state);
    ~GraphController() override;

    GraphController(const GraphController&) = delete;
    GraphController& operator=(const GraphController&) = delete;

    /// Collega il pannello grafico (chiamato dopo la creazione della GUI).
    void attachPanel(GraphPanel* panel);

    /// Avvia/ferma il timer di rendering.
    void startRendering();
    void stopRendering();

    /// FPS di rendering desiderati (limitati a [kMinRenderFps..kMaxRenderFps]).
    void setRenderFps(int fps);
    [[nodiscard]] int renderFps() const { return fps_; }

    /// FPS effettivamente misurati nell'ultimo secondo.
    [[nodiscard]] double measuredFps() const { return measuredFps_; }

    /// Esporta il grafico corrente come immagine PNG.
    [[nodiscard]] bool exportPng(const wxString& path) const;

private:
    void onRenderTimer(wxTimerEvent& event);

    AnalogDataBuffer& buffer_;
    BoardState& state_;
    GraphPanel* panel_ = nullptr;

    wxTimer timer_;
    int fps_ = kDefaultRenderFps;

    // Misura degli FPS reali.
    std::chrono::steady_clock::time_point fpsWindowStart_{};
    int framesInWindow_ = 0;
    double measuredFps_ = 0.0;
};

} // namespace am
