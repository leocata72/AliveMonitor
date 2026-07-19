/**
 * @file MainFrame.h
 * @brief Finestra principale: compone barra superiore, pannello uscite,
 *        controlli di acquisizione, grafico e barra di stato.
 *
 * VIEW (MVC). Nessuna logica applicativa: costruisce il layout, espone i
 * pannelli figli al MainController e inoltra menu/chiusura a IUserActions.
 */
#pragma once

#include <memory>

#include <wx/frame.h>

#include "IUserActions.h"
#include "model/AnalogDataBuffer.h"
#include "model/BoardState.h"
#include "model/ChannelCalibration.h"
#include "model/DigitalOutputState.h"

namespace am {

class AcquisitionPanel;
class CalibrationPanel;
class DigitalOutputPanel;
class GraphPanel;
class OptionsPanel;
class StatusPanel;
class ToolbarPanel;

class MainFrame : public wxFrame {
public:
    MainFrame(IUserActions& actions,
              BoardState& state,
              DigitalOutputState& outputs,
              const AnalogDataBuffer& buffer,
              const ChannelCalibrations& calibrations);

    /// Definito nel .cpp: il distruttore implicito richiederebbe qui il tipo
    /// completo di StatusPanel (unique_ptr), che nell'header è solo forward.
    ~MainFrame() override;

    // Accessori per il MainController (puntatori non proprietari,
    // validi per tutta la vita del frame).
    [[nodiscard]] ToolbarPanel* toolbarPanel() const { return toolbar_; }
    [[nodiscard]] DigitalOutputPanel* digitalPanel() const { return digital_; }
    [[nodiscard]] AcquisitionPanel* acquisitionPanel() const { return acquisition_; }
    [[nodiscard]] GraphPanel* graphPanel() const { return graph_; }
    [[nodiscard]] CalibrationPanel* calibrationPanel() const { return calibration_; }
    [[nodiscard]] OptionsPanel* optionsPanel() const { return options_; }
    /// Adapter sulla wxStatusBar nativa del frame (v1.2): non è una finestra,
    /// è posseduto dal frame stesso (unique_ptr), la barra dal framework.
    [[nodiscard]] StatusPanel* statusPanel() const { return status_.get(); }

private:
    void buildMenuBar();
    void onClose(wxCloseEvent& event);

    IUserActions& actions_;

    ToolbarPanel* toolbar_ = nullptr;
    DigitalOutputPanel* digital_ = nullptr;
    AcquisitionPanel* acquisition_ = nullptr;
    CalibrationPanel* calibration_ = nullptr;
    OptionsPanel* options_ = nullptr;      ///< Riquadro "Opzioni" (v1.2).
    GraphPanel* graph_ = nullptr;
    std::unique_ptr<StatusPanel> status_;  ///< Adapter sulla wxStatusBar nativa.
};

} // namespace am
