/**
 * @file GraphController.cpp
 * @brief Implementazione del controller di rendering ed esportazione.
 */
#include "controller/GraphController.h"

#include <algorithm>

#include <wx/string.h>

#include "view/GraphPanel.h"

namespace am {

GraphController::GraphController(AnalogDataBuffer& buffer, BoardState& state)
    : buffer_(buffer)
    , state_(state)
    , timer_(this)
{
    Bind(wxEVT_TIMER, &GraphController::onRenderTimer, this);
}

GraphController::~GraphController()
{
    stopRendering();
}

void GraphController::attachPanel(GraphPanel* panel)
{
    panel_ = panel;
}

void GraphController::startRendering()
{
    fpsWindowStart_ = std::chrono::steady_clock::now();
    framesInWindow_ = 0;
    timer_.Start(std::max(1, 1000 / fps_));
}

void GraphController::stopRendering()
{
    if (timer_.IsRunning()) {
        timer_.Stop();
    }
}

void GraphController::setRenderFps(int fps)
{
    fps_ = std::clamp(fps, kMinRenderFps, kMaxRenderFps);
    if (timer_.IsRunning()) {
        timer_.Start(std::max(1, 1000 / fps_));  // riavvia con il nuovo periodo
    }
}

void GraphController::onRenderTimer(wxTimerEvent& WXUNUSED(event))
{
    if (panel_ == nullptr) {
        return;
    }

    // "Adesso" per la finestra scorrevole: il tempo dell'asse di acquisizione.
    const bool following =
        state_.acquisition() == AcquisitionState::Running;
    panel_->refreshData(state_.acquisitionElapsed(), following);

    // Misura FPS reali su una finestra di 1 secondo.
    ++framesInWindow_;
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration<double>(now - fpsWindowStart_).count();
    if (elapsed >= 1.0) {
        measuredFps_ = framesInWindow_ / elapsed;
        framesInWindow_ = 0;
        fpsWindowStart_ = now;
    }
}

bool GraphController::exportPng(const wxString& path) const
{
    return panel_ != nullptr && panel_->exportPng(path);
}

} // namespace am
