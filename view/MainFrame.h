/**
 * @file MainFrame.h
 * @brief Finestra principale: compone barra superiore, pannello uscite,
 *        controlli di acquisizione, grafico e barra di stato.
 *
 * VIEW (MVC). Nessuna logica applicativa: costruisce il layout, espone i
 * pannelli figli al MainController e inoltra menu/chiusura a IUserActions.
 */
#pragma once

#include <wx/frame.h>

#include "IUserActions.h"
#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"
#include "model/DigitalOutputState.h"

namespace am {

class AcquisitionPanel;
class DigitalOutputPanel;
class GraphPanel;
class StatusPanel;
class ToolbarPanel;

class MainFrame : public wxFrame {
public:
    MainFrame(IUserActions& actions,
              BoardState& state,
              DigitalOutputState& outputs,
              const AnalogDataBuffer& buffer);

    // Accessori per il MainController (puntatori non proprietari,
    // validi per tutta la vita del frame).
    [[nodiscard]] ToolbarPanel* toolbarPanel() const { return toolbar_; }
    [[nodiscard]] DigitalOutputPanel* digitalPanel() const { return digital_; }
    [[nodiscard]] AcquisitionPanel* acquisitionPanel() const { return acquisition_; }
    [[nodiscard]] GraphPanel* graphPanel() const { return graph_; }
    [[nodiscard]] StatusPanel* statusPanel() const { return status_; }

private:
    void buildMenuBar();
    void onClose(wxCloseEvent& event);

    IUserActions& actions_;

    ToolbarPanel* toolbar_ = nullptr;
    DigitalOutputPanel* digital_ = nullptr;
    AcquisitionPanel* acquisition_ = nullptr;
    GraphPanel* graph_ = nullptr;
    StatusPanel* status_ = nullptr;
};

} // namespace am
