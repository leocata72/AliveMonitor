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
 *
 * Il pannello è organizzato in 7 schede (wxNotebook): la prima ("Tutti")
 * è il grafico combinato di sempre, in Volt; le altre sei (A0..A5) mostrano
 * un solo canale ciascuna con un proprio asse Y nella grandezza fisica
 * convertita (G = a*V + b, vedi ChannelCalibration) — un modo semplice per
 * avere "assi multipli" senza dover mescolare unità diverse su un unico
 * grafico.
 */
#pragma once

#include <array>
#include <vector>

#include <wx/panel.h>

#include "IUserActions.h"
#include "Version.h"
#include "model/AnalogDataBuffer.h"
#include "model/ChannelCalibration.h"

class wxCheckBox;
class wxNotebook;
class wxBookCtrlEvent;
class wxStaticText;

namespace am {

class LedIndicator;  // forward: incluso solo nel .cpp

/**
 * @brief Superficie di disegno del grafico (double-buffered, interattiva).
 */
class PlotCanvas : public wxWindow {
public:
    /// @param soloChannel -1 = scheda combinata (tutti i canali, in Volt,
    ///        comportamento invariato); 0..5 = scheda dedicata a un solo
    ///        canale, che traccia e scala l'asse Y sul valore CONVERTITO
    ///        (calibrations[soloChannel].convert(volts)) invece dei Volt.
    PlotCanvas(wxWindow* parent, const AnalogDataBuffer& buffer,
               const ChannelCalibrations& calibrations,
               int soloChannel = -1);

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

    /// Valore da tracciare/scalare per il campione s del canale ch: il Volt
    /// grezzo in modalità combinata, il valore convertito in modalità
    /// canale singolo (soloChannel_ == ch, l'unico caso possibile qui).
    [[nodiscard]] double plottedValue(int ch, const AnalogSample& s) const;

    /// Unità da mostrare sull'asse Y: "V" in modalità combinata, l'unità
    /// calibrata del canale in modalità singola.
    [[nodiscard]] wxString yAxisUnit() const;

    // --- Dati ----------------------------------------------------------------
    const AnalogDataBuffer& buffer_;
    const ChannelCalibrations& calibrations_;  ///< Letta live a ogni frame (legenda + solo-canale).
    const int soloChannel_;  ///< -1 = combinato; 0..5 = un solo canale, valore convertito.
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
 * @brief Pannello completo: wxNotebook con 7 schede (grafico combinato +
 *        una scheda per canale), ciascuna con la propria barra di
 *        controlli e il proprio PlotCanvas.
 */
class GraphPanel : public wxPanel {
public:
    GraphPanel(wxWindow* parent, IUserActions& actions,
               const AnalogDataBuffer& buffer,
               const ChannelCalibrations& calibrations);

    /// Inoltrato SOLO al canvas della scheda attualmente attiva (evita di
    /// pagare 7 volte il costo di copyWindow() a ogni frame di rendering,
    /// dato che al massimo una scheda per volta è visibile). Il cambio di
    /// scheda innesca un refresh immediato (vedi onPageChanged) così la
    /// nuova scheda non resta con dati non aggiornati fino al frame
    /// successivo del GraphController.
    void refreshData(double now, bool following);

    /// Esporta la scheda attualmente attiva.
    [[nodiscard]] bool exportPng(const wxString& path) const;

    /// Applica/legge la finestra temporale su TUTTE le schede (impostazione
    /// globale dal SettingsDialog): il getter legge dalla scheda combinata.
    [[nodiscard]] double timeWindowSeconds() const;
    void setTimeWindowSeconds(double seconds);

    /// Autoscale Y continuo della sola scheda combinata (dal SettingsDialog):
    /// le schede a canale singolo hanno ciascuna il proprio Auto Y locale
    /// (attivo di default, dato che la scala convertita non è nota a
    /// priori), non toccato da questa impostazione globale.
    [[nodiscard]] bool continuousAutoscaleY() const;
    void setContinuousAutoscaleY(bool enabled);

    /// Aggiorna il titolo della scheda dedicata al canale (chiamato da
    /// MainController::onCalibrationChanged): usa label se non vuota,
    /// altrimenti torna al nome del canale ("A0".."A5"). Il titolo non è
    /// riletto a ogni frame come calibrations_ (è testo statico del
    /// wxNotebook), va quindi aggiornato esplicitamente a ogni modifica.
    void setChannelTabTitle(int channel, const wxString& label);

    /// Mostra il percorso completo del file CSV in scrittura e accende il LED
    /// (chiamato da MainController all'avvio di una nuova sessione). Presente
    /// solo nella scheda combinata: la registrazione non è per-canale.
    void setCsvPath(const wxString& path);

    /// Ripristina l'etichetta e spegne il LED quando la registrazione CSV
    /// termina (Stop).
    void clearCsvPath();

private:
    static constexpr int kTabCount = kNumAnalogChannels + 1;  ///< Combinata + 6 canali.

    /// Costruisce una scheda (barra controlli + PlotCanvas) e la aggiunge al
    /// notebook. @param soloChannel -1 per la scheda combinata (con
    /// checkbox canali + LED/percorso CSV), 0..5 per una scheda a canale
    /// singolo (controlli semplificati, nessuna checkbox).
    PlotCanvas* buildTab(const wxString& title, int soloChannel);

    void onPageChanged(wxBookCtrlEvent& event);

    IUserActions& actions_;
    wxNotebook* notebook_ = nullptr;
    const AnalogDataBuffer& buffer_;
    const ChannelCalibrations& calibrations_;

    /// [0] = scheda combinata, [1..6] = A0..A5.
    std::array<PlotCanvas*, kTabCount> canvases_{};
    std::array<wxCheckBox*, kTabCount> autoYChecks_{};
    std::array<wxCheckBox*, kNumAnalogChannels> channelChecks_{};  ///< Solo scheda combinata.

    LedIndicator* csvLed_ = nullptr;      ///< Verde = in scrittura, rosso = ferma.
    wxStaticText* csvPathText_ = nullptr;

    // Ultimo (now, following) ricevuto dal GraphController: riusato per il
    // refresh immediato al cambio scheda (vedi refreshData).
    double lastNow_ = 0.0;
    bool lastFollowing_ = false;
};

} // namespace am
