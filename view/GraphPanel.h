/**
 * @file GraphPanel.h
 * @brief Pannello centrale: grafico in tempo reale delle sei tensioni A0..A5.
 *
 * VIEW (MVC). Renderer custom integrato (nessuna dipendenza esterna),
 * progettato per 250 Hz e oltre:
 *  - il rendering avviene SOLO su richiesta del GraphController (10..60 FPS),
 *    mai a ogni campione;
 *  - i dati vengono copiati dal ring buffer con copyWindow() riusando la
 *    capacità dei vettori (nessuna allocazione a regime);
 *  - decimazione min/max per colonna di pixel: la forma d'onda resta fedele
 *    anche con 15000 punti per canale in finestra.
 *
 * Funzionalità: 6 curve con colori diversi, legenda, griglia, asse dei tempi,
 * zoom (rotella; Ctrl+rotella per l'asse Y), pan (trascinamento), autoscale Y
 * (singolo o continuo), curve attivabili singolarmente, esportazione PNG,
 * finestra scorrevole degli ultimi 60 s (configurabile). Il percorso del CSV
 * in scrittura (registrazione continua, vedi CsvLoggerController) è mostrato
 * in un'etichetta nella barra dei controlli.
 */
#pragma once

#include <array>
#include <vector>

#include <wx/panel.h>

#include "IUserActions.h"
#include "Version.h"
#include "model/AnalogDataBuffer.h"

class wxCheckBox;
class wxStaticText;

namespace am {

class LedIndicator;  // forward: incluso solo nel .cpp

/**
 * @brief Superficie di disegno del grafico (double-buffered, interattiva).
 */
class PlotCanvas : public wxWindow {
public:
    PlotCanvas(wxWindow* parent, const AnalogDataBuffer& buffer);

    /// Aggiorna lo snapshot dei dati e ridisegna (chiamato dal GraphController).
    /// @param now       tempo corrente dell'asse di acquisizione [s]
    /// @param following true se l'acquisizione è in corso (finestra scorrevole)
    void refreshData(double now, bool following);

    void setChannelVisible(int channel, bool visible);

    void setTimeWindow(double seconds);
    [[nodiscard]] double timeWindow() const { return window_; }

    void setContinuousAutoscaleY(bool enabled);
    [[nodiscard]] bool continuousAutoscaleY() const { return autoY_; }

    /// Adatta una sola volta l'asse Y ai dati visibili.
    void autoscaleYOnce();

    /// Ripristina la vista predefinita (ultimi 60 s, 0..5 V, inseguimento).
    void resetView();

    /// Riattiva l'inseguimento della finestra scorrevole.
    void followNow();

    /// Salva il grafico corrente come PNG (risoluzione 1600x900).
    [[nodiscard]] bool exportPng(const wxString& path) const;

private:
    // --- Eventi -----------------------------------------------------------
    void onPaint(wxPaintEvent& event);
    void onMouseDown(wxMouseEvent& event);
    void onMouseUp(wxMouseEvent& event);
    void onMouseMove(wxMouseEvent& event);
    void onMouseWheel(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);

    // --- Rendering (const: usato anche dall'esportazione PNG) ---------------
    void render(wxDC& dc, const wxSize& size) const;
    void drawGrid(wxDC& dc, const wxRect& plot) const;
    void drawCurves(wxDC& dc, const wxRect& plot) const;
    void drawLegend(wxDC& dc, const wxRect& plot) const;

    /// Passo "gradevole" (1-2-5 * 10^k) per circa targetTicks divisioni.
    [[nodiscard]] static double niceStep(double range, int targetTicks);

    // --- Dati ----------------------------------------------------------------
    const AnalogDataBuffer& buffer_;
    std::array<std::vector<AnalogSample>, kNumAnalogChannels> snapshot_;
    std::array<bool, kNumAnalogChannels> visible_;
    std::array<wxColour, kNumAnalogChannels> colours_;

    // --- Vista -----------------------------------------------------------------
    double window_ = static_cast<double>(kBufferSeconds);  ///< Ampiezza X [s].
    double xMin_ = 0.0;
    double xMax_ = static_cast<double>(kBufferSeconds);
    double yMin_ = 0.0;
    double yMax_ = kAdcReferenceVolt;
    bool follow_ = true;   ///< Finestra agganciata al tempo corrente.
    bool autoY_ = false;   ///< Autoscale Y continuo a ogni refresh.

    // --- Interazione ---------------------------------------------------------------
    bool dragging_ = false;
    wxPoint lastDragPos_;

    // Vettore di punti riusato a ogni frame (nessuna allocazione a regime).
    mutable std::vector<wxPoint> scratchPoints_;
};

/**
 * @brief Pannello completo: barra dei controlli (visibilità curve, autoscale,
 *        reset, esportazioni) + PlotCanvas.
 */
class GraphPanel : public wxPanel {
public:
    GraphPanel(wxWindow* parent, IUserActions& actions,
               const AnalogDataBuffer& buffer);

    /// Inoltrato al canvas (chiamato dal GraphController).
    void refreshData(double now, bool following);

    [[nodiscard]] bool exportPng(const wxString& path) const;

    [[nodiscard]] double timeWindowSeconds() const;
    void setTimeWindowSeconds(double seconds);

    [[nodiscard]] bool continuousAutoscaleY() const;
    void setContinuousAutoscaleY(bool enabled);

    /// Mostra il percorso completo del file CSV in scrittura e accende il LED
    /// (chiamato da MainController all'avvio di una nuova sessione).
    void setCsvPath(const wxString& path);

    /// Ripristina l'etichetta e spegne il LED quando la registrazione CSV
    /// termina (Stop).
    void clearCsvPath();

private:
    IUserActions& actions_;
    PlotCanvas* canvas_ = nullptr;
    std::array<wxCheckBox*, kNumAnalogChannels> channelChecks_{};
    wxCheckBox* autoYCheck_ = nullptr;
    LedIndicator* csvLed_ = nullptr;      ///< Verde = in scrittura, rosso = ferma.
    wxStaticText* csvPathText_ = nullptr;
};

} // namespace am
