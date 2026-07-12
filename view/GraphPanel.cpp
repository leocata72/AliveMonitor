/**
 * @file GraphPanel.cpp
 * @brief Implementazione del grafico in tempo reale con renderer custom.
 */
#include "view/GraphPanel.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "i18n/Strings.h"
#include "view/LedIndicator.h"

namespace am {
namespace {

constexpr int kMarginLeft = 56;
constexpr int kMarginRight = 12;
constexpr int kMarginTop = 10;
constexpr int kMarginBottom = 32;

constexpr int kExportWidth = 1600;
constexpr int kExportHeight = 900;

/// Limiti di zoom per evitare viste degeneri.
constexpr double kMinSpanX = 0.05;     // 50 ms
constexpr double kMaxSpanX = 3600.0;   // 1 ora
constexpr double kMinSpanY = 0.05;     // 50 mV
constexpr double kMaxSpanY = 50.0;     // 50 V

} // namespace

// ===========================================================================
// PlotCanvas
// ===========================================================================

PlotCanvas::PlotCanvas(wxWindow* parent, const AnalogDataBuffer& buffer,
                       const ChannelCalibrations& calibrations,
                       int soloChannel)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
               wxFULL_REPAINT_ON_RESIZE)
    , buffer_(buffer)
    , calibrations_(calibrations)
    , soloChannel_(soloChannel)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(400, 300));

    if (soloChannel_ >= 0) {
        // Scheda a canale singolo: solo quel canale è "visibile" (nessuna
        // checkbox in questa modalità, il canale è fisso per tutta la vita
        // della scheda).
        visible_.fill(false);
        visible_[static_cast<std::size_t>(soloChannel_)] = true;
    } else {
        visible_.fill(true);
    }
    // Palette a contrasto elevato, distinguibile anche a colori sovrapposti.
    colours_ = { wxColour(31, 119, 180),   // A0 blu
                 wxColour(255, 127, 14),   // A1 arancio
                 wxColour(44, 160, 44),    // A2 verde
                 wxColour(214, 39, 40),    // A3 rosso
                 wxColour(148, 103, 189),  // A4 viola
                 wxColour(140, 86, 75) };  // A5 marrone

    // Pre-allocazione dello snapshot per la finestra massima a 500 Hz.
    for (auto& ch : snapshot_) {
        ch.reserve(kRingBufferCapacity);
    }
    scratchPoints_.reserve(8192);

    Bind(wxEVT_PAINT, &PlotCanvas::onPaint, this);
    Bind(wxEVT_LEFT_DOWN, &PlotCanvas::onMouseDown, this);
    Bind(wxEVT_LEFT_UP, &PlotCanvas::onMouseUp, this);
    Bind(wxEVT_MOTION, &PlotCanvas::onMouseMove, this);
    Bind(wxEVT_MOUSEWHEEL, &PlotCanvas::onMouseWheel, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, &PlotCanvas::onMouseCaptureLost, this);
}

void PlotCanvas::refreshData(double now, bool following)
{
    if (follow_ && following) {
        // Finestra scorrevole agganciata al tempo corrente di acquisizione.
        xMax_ = std::max(now, window_);
        xMin_ = xMax_ - window_;
    }
    buffer_.copyWindow(xMin_, snapshot_);
    if (autoY_) {
        autoscaleYOnce();
    }
    Refresh(false);
}

void PlotCanvas::setChannelVisible(int channel, bool visible)
{
    if (channel >= 0 && channel < kNumAnalogChannels) {
        visible_[static_cast<std::size_t>(channel)] = visible;
        Refresh(false);
    }
}

void PlotCanvas::setTimeWindow(double seconds)
{
    window_ = std::clamp(seconds, kMinSpanX, kMaxSpanX);
    if (follow_) {
        xMin_ = xMax_ - window_;
    }
    Refresh(false);
}

void PlotCanvas::setContinuousAutoscaleY(bool enabled)
{
    autoY_ = enabled;
    if (enabled) {
        autoscaleYOnce();
    }
    Refresh(false);
}

void PlotCanvas::autoscaleYOnce()
{
    double lo = std::numeric_limits<double>::max();
    double hi = std::numeric_limits<double>::lowest();

    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        if (!visible_[static_cast<std::size_t>(ch)]) {
            continue;
        }
        for (const auto& s : snapshot_[static_cast<std::size_t>(ch)]) {
            if (s.t < xMin_ || s.t > xMax_) {
                continue;
            }
            const double v = plottedValue(ch, s);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    }

    if (lo > hi) {
        // Nessun dato visibile: intervallo predefinito (identità se il
        // canale non è calibrato, altrimenti 0..5V passati attraverso la
        // calibrazione). NB: std::minmax() qui creerebbe reference pendenti
        // sui temporanei restituiti da convert() (-Wdangling-reference);
        // con variabili concrete non c'è ambiguità sulla lifetime.
        const double defA = soloChannel_ >= 0
            ? calibrations_[static_cast<std::size_t>(soloChannel_)].convert(0.0)
            : 0.0;
        const double defB = soloChannel_ >= 0
            ? calibrations_[static_cast<std::size_t>(soloChannel_)].convert(kAdcReferenceVolt)
            : kAdcReferenceVolt;
        yMin_ = std::min(defA, defB);
        yMax_ = std::max(defA, defB);
        return;
    }
    double pad = (hi - lo) * 0.08;
    if (pad < 0.02) {
        pad = 0.02;  // segnale costante: margine minimo per non degenerare
    }
    yMin_ = lo - pad;
    yMax_ = hi + pad;
    Refresh(false);
}

void PlotCanvas::resetView()
{
    window_ = static_cast<double>(kBufferSeconds);
    if (soloChannel_ >= 0) {
        // Range di default 0..5V mappato attraverso la calibrazione (min/max
        // perché a può essere negativo, invertendo l'ordine). Variabili
        // concrete, non std::minmax(): evita reference pendenti sui
        // temporanei restituiti da convert() (-Wdangling-reference).
        const auto& cal = calibrations_[static_cast<std::size_t>(soloChannel_)];
        const double a = cal.convert(0.0);
        const double b = cal.convert(kAdcReferenceVolt);
        yMin_ = std::min(a, b);
        yMax_ = std::max(a, b);
    } else {
        yMin_ = 0.0;
        yMax_ = kAdcReferenceVolt;
    }
    follow_ = true;
    xMin_ = xMax_ - window_;
    Refresh(false);
}

void PlotCanvas::followNow()
{
    follow_ = true;
    Refresh(false);
}

// ---------------------------------------------------------------------------
// Interazione
// ---------------------------------------------------------------------------

void PlotCanvas::onMouseDown(wxMouseEvent& event)
{
    dragging_ = true;
    lastDragPos_ = event.GetPosition();
    CaptureMouse();
    SetCursor(wxCursor(wxCURSOR_HAND));
}

void PlotCanvas::onMouseUp(wxMouseEvent& WXUNUSED(event))
{
    if (dragging_) {
        dragging_ = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        SetCursor(wxNullCursor);
    }
}

void PlotCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
    dragging_ = false;
    SetCursor(wxNullCursor);
}

void PlotCanvas::onMouseMove(wxMouseEvent& event)
{
    if (!dragging_) {
        return;
    }
    const wxPoint pos = event.GetPosition();
    const wxSize size = GetClientSize();
    const int plotW = std::max(1, size.x - kMarginLeft - kMarginRight);
    const int plotH = std::max(1, size.y - kMarginTop - kMarginBottom);

    // Conversione dello spostamento in pixel in unità dati (pan).
    const double dx = (lastDragPos_.x - pos.x)
                      * (xMax_ - xMin_) / static_cast<double>(plotW);
    const double dy = (pos.y - lastDragPos_.y)
                      * (yMax_ - yMin_) / static_cast<double>(plotH);
    lastDragPos_ = pos;

    if (dx != 0.0) {
        follow_ = false;  // il pan orizzontale sgancia l'inseguimento
        xMin_ += dx;
        xMax_ += dx;
        buffer_.copyWindow(xMin_, snapshot_);
    }
    yMin_ += dy;
    yMax_ += dy;
    Refresh(false);
}

void PlotCanvas::onMouseWheel(wxMouseEvent& event)
{
    const double factor = event.GetWheelRotation() > 0 ? (1.0 / 1.2) : 1.2;
    const wxSize size = GetClientSize();
    const int plotW = std::max(1, size.x - kMarginLeft - kMarginRight);
    const int plotH = std::max(1, size.y - kMarginTop - kMarginBottom);

    if (event.ControlDown()) {
        // Zoom asse Y, ancorato alla posizione verticale del mouse.
        const double anchor = yMax_
            - (event.GetY() - kMarginTop) * (yMax_ - yMin_) / plotH;
        double span = (yMax_ - yMin_) * factor;
        span = std::clamp(span, kMinSpanY, kMaxSpanY);
        const double frac = (yMax_ - anchor) / (yMax_ - yMin_);
        yMax_ = anchor + frac * span;
        yMin_ = yMax_ - span;
    } else {
        // Zoom asse X, ancorato alla posizione orizzontale del mouse.
        const double anchor = xMin_
            + (event.GetX() - kMarginLeft) * (xMax_ - xMin_) / plotW;
        double span = (xMax_ - xMin_) * factor;
        span = std::clamp(span, kMinSpanX, kMaxSpanX);
        const double frac = (anchor - xMin_) / (xMax_ - xMin_);
        xMin_ = anchor - frac * span;
        xMax_ = xMin_ + span;
        window_ = span;   // lo zoom ridefinisce l'ampiezza della finestra
        follow_ = false;  // e sgancia l'inseguimento
        buffer_.copyWindow(xMin_, snapshot_);
    }
    Refresh(false);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void PlotCanvas::onPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    render(dc, GetClientSize());
}

double PlotCanvas::niceStep(double range, int targetTicks)
{
    if (range <= 0.0 || targetTicks <= 0) {
        return 1.0;
    }
    const double raw = range / targetTicks;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double norm = raw / mag;  // in [1, 10)
    double step = 10.0;
    if (norm <= 1.5) {
        step = 1.0;
    } else if (norm <= 3.5) {
        step = 2.0;
    } else if (norm <= 7.5) {
        step = 5.0;
    }
    return step * mag;
}

double PlotCanvas::plottedValue(int ch, const AnalogSample& s) const
{
    if (soloChannel_ >= 0) {
        // Nelle schede a canale singolo l'unico canale visibile è
        // soloChannel_ stesso: si traccia sempre il valore convertito.
        return calibrations_[static_cast<std::size_t>(ch)].convert(s.volts());
    }
    return s.volts();  // scheda combinata: sempre Volt, canali con unità diverse
}

wxString PlotCanvas::yAxisUnit() const
{
    if (soloChannel_ >= 0) {
        return wxString(calibrations_[static_cast<std::size_t>(soloChannel_)].unit);
    }
    return "V";
}

void PlotCanvas::render(wxDC& dc, const wxSize& size) const
{
    // Sfondo generale e area di tracciamento.
    dc.SetBackground(wxBrush(wxColour(250, 250, 250)));
    dc.Clear();

    const wxRect plot(kMarginLeft, kMarginTop,
                      std::max(1, size.x - kMarginLeft - kMarginRight),
                      std::max(1, size.y - kMarginTop - kMarginBottom));

    dc.SetBrush(*wxWHITE_BRUSH);
    dc.SetPen(wxPen(wxColour(180, 180, 180)));
    dc.DrawRectangle(plot);

    drawGrid(dc, plot);

    dc.SetClippingRegion(plot);
    drawCurves(dc, plot);
    dc.DestroyClippingRegion();

    drawLegend(dc, plot);
}

void PlotCanvas::drawGrid(wxDC& dc, const wxRect& plot) const
{
    // NB: non chiamare la variabile "small": su Windows è una macro di sistema.
    wxFont smallFont(wxFontInfo(8));
    dc.SetFont(smallFont);
    dc.SetTextForeground(wxColour(90, 90, 90));
    dc.SetPen(wxPen(wxColour(228, 228, 228)));

    // --- Griglia e etichette asse X (tempo [s]) -----------------------------
    const double spanX = xMax_ - xMin_;
    const double stepX = niceStep(spanX, 8);
    const int decimalsX = stepX < 0.1 ? 2 : (stepX < 1.0 ? 1 : 0);
    for (double t = std::ceil(xMin_ / stepX) * stepX; t <= xMax_ + 1e-9;
         t += stepX) {
        const int x = plot.x
            + static_cast<int>((t - xMin_) / spanX * plot.width);
        if (x <= plot.x || x >= plot.GetRight()) {
            continue;
        }
        dc.DrawLine(x, plot.y + 1, x, plot.GetBottom() - 1);
        const wxString label = wxString::Format("%.*f", decimalsX, t);
        const wxSize ext = dc.GetTextExtent(label);
        dc.DrawText(label, x - ext.x / 2, plot.GetBottom() + 4);
    }

    // --- Griglia e etichette asse Y (Volt, o grandezza convertita nelle
    //     schede a canale singolo) ------------------------------------------
    const wxString unit = yAxisUnit();
    const double spanY = yMax_ - yMin_;
    const double stepY = niceStep(spanY, 6);
    const int decimalsY = stepY < 0.1 ? 2 : (stepY < 1.0 ? 1 : 0);
    for (double v = std::ceil(yMin_ / stepY) * stepY; v <= yMax_ + 1e-9;
         v += stepY) {
        const int y = plot.y + plot.height
            - static_cast<int>((v - yMin_) / spanY * plot.height);
        if (y <= plot.y || y >= plot.GetBottom()) {
            continue;
        }
        dc.DrawLine(plot.x + 1, y, plot.GetRight() - 1, y);
        const wxString label = wxString::Format("%.*f %s", decimalsY, v, unit);
        const wxSize ext = dc.GetTextExtent(label);
        dc.DrawText(label, plot.x - ext.x - 6, y - ext.y / 2);
    }

    // Titolo asse X.
    const wxString xTitle = tr(StringId::GpAxisTime);
    const wxSize ext = dc.GetTextExtent(xTitle);
    dc.DrawText(xTitle, plot.x + (plot.width - ext.x) / 2,
                plot.GetBottom() + 16);
}

void PlotCanvas::drawCurves(wxDC& dc, const wxRect& plot) const
{
    const double spanX = xMax_ - xMin_;
    const double spanY = yMax_ - yMin_;
    if (spanX <= 0.0 || spanY <= 0.0) {
        return;
    }
    const double sx = plot.width / spanX;
    const double sy = plot.height / spanY;

    const auto toX = [&](double t) {
        return plot.x + static_cast<int>((t - xMin_) * sx);
    };
    const auto toY = [&](double v) {
        return plot.y + plot.height - static_cast<int>((v - yMin_) * sy);
    };

    // Soglia minima di "buco" in secondi anche ad alta frequenza (250 Hz):
    // evita falsi positivi da normale jitter tra campioni consecutivi.
    constexpr double kMinGapSeconds = 0.05;

    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        if (!visible_[chIdx]) {
            continue;
        }
        const auto& data = snapshot_[chIdx];
        if (data.size() < 2) {
            continue;
        }

        dc.SetPen(wxPen(colours_[chIdx], 1));
        scratchPoints_.clear();

        // Stato per il rilevamento dei "buchi" temporali (Pausa, o più
        // raramente qualche pacchetto perso): la stima del periodo tipico
        // (emaDt) è una media mobile aggiornata solo sugli intervalli
        // "normali", così un cambio di frequenza dal vivo o una pausa non la
        // corrompono — anzi, un buco vero fa adottare subito il nuovo
        // periodo come riferimento, autocorreggendosi al campione successivo.
        bool haveLast = false;
        double lastT = 0.0;
        double emaDt = -1.0;

        // Disegna il segmento accumulato finora e prepara il prossimo:
        // interrompere qui la polilinea è esattamente il "buco" visivo.
        const auto flushSegment = [&]() {
            if (scratchPoints_.size() >= 2) {
                dc.DrawLines(static_cast<int>(scratchPoints_.size()),
                             scratchPoints_.data());
            }
            scratchPoints_.clear();
        };

        // true se il campione a tempo t apre un nuovo segmento (buco rilevato
        // rispetto all'ultimo campione processato); aggiorna anche lo stato.
        const auto isGap = [&](double t) {
            if (!haveLast) {
                haveLast = true;
                lastT = t;
                return false;
            }
            const double dt = t - lastT;
            lastT = t;
            bool gap = false;
            if (emaDt < 0.0) {
                emaDt = dt;  // bootstrap sul primo intervallo osservato
            } else if (dt > std::max(3.0 * emaDt, kMinGapSeconds)) {
                gap = true;
                emaDt = dt;  // adotta subito il nuovo periodo (fine pausa o cambio Hz)
            } else {
                emaDt = 0.9 * emaDt + 0.1 * dt;
            }
            return gap;
        };

        if (data.size() > static_cast<std::size_t>(2 * plot.width)) {
            // ----- Decimazione min/max per colonna di pixel -----------------
            // Con 15000 punti in finestra sarebbe inutile (e lento) disegnare
            // ogni segmento: per ogni colonna si traccia l'escursione min-max,
            // preservando picchi e forma del segnale.
            int currentCol = INT_MIN;
            double colMin = 0.0, colMax = 0.0;
            for (const auto& s : data) {
                if (s.t < xMin_ || s.t > xMax_) {
                    continue;
                }
                if (isGap(s.t)) {
                    if (currentCol != INT_MIN) {
                        const int x = plot.x + currentCol;
                        scratchPoints_.emplace_back(x, toY(colMax));
                        scratchPoints_.emplace_back(x, toY(colMin));
                        currentCol = INT_MIN;
                    }
                    flushSegment();
                }
                const int col = static_cast<int>((s.t - xMin_) * sx);
                const double v = plottedValue(ch, s);
                if (col != currentCol) {
                    if (currentCol != INT_MIN) {
                        const int x = plot.x + currentCol;
                        scratchPoints_.emplace_back(x, toY(colMax));
                        scratchPoints_.emplace_back(x, toY(colMin));
                    }
                    currentCol = col;
                    colMin = colMax = v;
                } else {
                    colMin = std::min(colMin, v);
                    colMax = std::max(colMax, v);
                }
            }
            if (currentCol != INT_MIN) {
                const int x = plot.x + currentCol;
                scratchPoints_.emplace_back(x, toY(colMax));
                scratchPoints_.emplace_back(x, toY(colMin));
            }
        } else {
            // ----- Polilinea diretta (pochi punti per pixel) ------------------
            for (const auto& s : data) {
                if (s.t < xMin_ || s.t > xMax_) {
                    continue;
                }
                if (isGap(s.t)) {
                    flushSegment();
                }
                scratchPoints_.emplace_back(toX(s.t), toY(plottedValue(ch, s)));
            }
        }

        flushSegment();
    }
}

void PlotCanvas::drawLegend(wxDC& dc, const wxRect& plot) const
{
    wxFont smallFont(wxFontInfo(8));
    dc.SetFont(smallFont);

    int visibleCount = 0;
    for (const bool v : visible_) {
        visibleCount += v ? 1 : 0;
    }
    if (visibleCount == 0) {
        return;
    }

    // Etichetta di ogni riga: "A0: 23.4 °C" (ultimo campione convertito con
    // G = a*V + b) se ci sono dati in finestra, altrimenti solo "A0". Il
    // valore è live: la legenda legge calibrations_ direttamente a ogni
    // frame, senza copie né notifiche dal CalibrationPanel.
    std::array<wxString, kNumAnalogChannels> labels;
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        if (!visible_[chIdx]) {
            continue;
        }
        wxString label = wxString::Format("A%d", ch);
        if (!snapshot_[chIdx].empty()) {
            const double volts = snapshot_[chIdx].back().volts();
            const double converted = calibrations_[chIdx].convert(volts);
            label += wxString::Format(": %.2f %s", converted,
                                      wxString(calibrations_[chIdx].unit));
        }
        labels[chIdx] = label;
    }

    const int rowH = 16;
    int maxTextW = 0;
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        if (!visible_[chIdx]) {
            continue;
        }
        maxTextW = std::max(maxTextW, dc.GetTextExtent(labels[chIdx]).x);
    }
    const int boxW = 36 + maxTextW;
    const int boxH = 6 + visibleCount * rowH;
    const wxRect box(plot.x + 10, plot.y + 8, boxW, boxH);

    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 230)));
    dc.SetPen(wxPen(wxColour(190, 190, 190)));
    dc.DrawRectangle(box);

    int row = 0;
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        if (!visible_[chIdx]) {
            continue;
        }
        const int y = box.y + 6 + row * rowH;
        dc.SetPen(wxPen(colours_[chIdx], 2));
        dc.DrawLine(box.x + 6, y + 6, box.x + 24, y + 6);
        dc.SetTextForeground(wxColour(60, 60, 60));
        dc.DrawText(labels[chIdx], box.x + 30, y);
        ++row;
    }
}

bool PlotCanvas::exportPng(const wxString& path) const
{
    wxBitmap bitmap(kExportWidth, kExportHeight);
    {
        wxMemoryDC mdc(bitmap);
        render(mdc, wxSize(kExportWidth, kExportHeight));
        mdc.SelectObject(wxNullBitmap);
    }
    return bitmap.ConvertToImage().SaveFile(path, wxBITMAP_TYPE_PNG);
}

// ===========================================================================
// GraphPanel
// ===========================================================================

GraphPanel::GraphPanel(wxWindow* parent, IUserActions& actions,
                       const AnalogDataBuffer& buffer,
                       const ChannelCalibrations& calibrations)
    : wxPanel(parent)
    , actions_(actions)
    , buffer_(buffer)
    , calibrations_(calibrations)
{
    notebook_ = new wxNotebook(this, wxID_ANY);

    canvases_[0] = buildTab(tr(StringId::GpTabAll), -1);
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto& label = calibrations_[static_cast<std::size_t>(ch)].label;
        const wxString title = label.empty() ? wxString::Format("A%d", ch) : wxString(label);
        canvases_[static_cast<std::size_t>(ch + 1)] = buildTab(title, ch);
    }

    notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &GraphPanel::onPageChanged, this);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(notebook_, 1, wxEXPAND | wxALL, 4);
    SetSizer(outer);
}

PlotCanvas* GraphPanel::buildTab(const wxString& title, int soloChannel)
{
    auto* page = new wxPanel(notebook_);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* controls = new wxBoxSizer(wxHORIZONTAL);

    auto* canvas = new PlotCanvas(page, buffer_, calibrations_, soloChannel);

    if (soloChannel < 0) {
        // --- Scheda combinata: checkbox di visibilità per canale + stato
        //     della registrazione CSV (non ha senso ripeterlo nelle schede
        //     a canale singolo: la registrazione è unica, non per-canale).
        for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
            auto* check = new wxCheckBox(page, wxID_ANY,
                                         wxString::Format("A%d", ch));
            check->SetValue(true);
            check->Bind(wxEVT_CHECKBOX, [canvas, ch](wxCommandEvent& e) {
                canvas->setChannelVisible(ch, e.IsChecked());
            });
            channelChecks_[static_cast<std::size_t>(ch)] = check;
            controls->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        }

        controls->AddStretchSpacer(1);

        // LED verde quando il consumatore sta scrivendo, rosso quando è fermo.
        csvLed_ = new LedIndicator(page, wxColour(46, 204, 64), wxColour(200, 40, 40), 14);
        controls->Add(csvLed_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

        csvPathText_ = new wxStaticText(page, wxID_ANY, tr(StringId::GpCsvNone));
        controls->Add(csvPathText_, 0, wxALIGN_CENTER_VERTICAL);

        controls->AddStretchSpacer(1);
    } else {
        // --- Scheda a canale singolo: nessuna checkbox (il canale è fisso),
        //     Auto Y attivo di default per adattarsi subito alla scala della
        //     grandezza convertita, qualunque essa sia.
        canvas->setContinuousAutoscaleY(true);
        controls->AddStretchSpacer(1);
    }

    auto* autoYCheck = new wxCheckBox(page, wxID_ANY, tr(StringId::GpAutoYCheck));
    autoYCheck->SetValue(canvas->continuousAutoscaleY());
    autoYCheck->Bind(wxEVT_CHECKBOX, [canvas](wxCommandEvent& e) {
        canvas->setContinuousAutoscaleY(e.IsChecked());
    });
    autoYChecks_[static_cast<std::size_t>(soloChannel + 1)] = autoYCheck;
    controls->Add(autoYCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    auto* autoscaleBtn = new wxButton(page, wxID_ANY, tr(StringId::GpBtnAutoscale),
                                      wxDefaultPosition, wxDefaultSize,
                                      wxBU_EXACTFIT);
    autoscaleBtn->Bind(wxEVT_BUTTON,
                       [canvas](wxCommandEvent&) { canvas->autoscaleYOnce(); });
    controls->Add(autoscaleBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    auto* followBtn = new wxButton(page, wxID_ANY, tr(StringId::GpBtnFollow),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxBU_EXACTFIT);
    followBtn->Bind(wxEVT_BUTTON,
                    [canvas](wxCommandEvent&) { canvas->followNow(); });
    controls->Add(followBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    auto* resetBtn = new wxButton(page, wxID_ANY, tr(StringId::GpBtnReset),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxBU_EXACTFIT);
    resetBtn->Bind(wxEVT_BUTTON,
                   [canvas](wxCommandEvent&) { canvas->resetView(); });
    controls->Add(resetBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto* pngBtn = new wxButton(page, wxID_ANY, "PNG",
                                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pngBtn->Bind(wxEVT_BUTTON,
                 [this](wxCommandEvent&) { actions_.onExportPngRequested(); });
    controls->Add(pngBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    sizer->Add(controls, 0, wxEXPAND | wxALL, 4);
    sizer->Add(canvas, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
    page->SetSizer(sizer);

    notebook_->AddPage(page, title);
    return canvas;
}

void GraphPanel::onPageChanged(wxBookCtrlEvent& WXUNUSED(event))
{
    // Rinfresca subito la scheda appena diventata attiva con l'ultimo
    // (now, following) noto: senza questo resterebbe con l'ultimo frame
    // renderizzato quando era attiva l'ultima volta, fino al prossimo tick
    // del GraphController (che potrebbe volerci fino a 100 ms a 10 FPS).
    const int idx = notebook_->GetSelection();
    if (idx >= 0 && idx < kTabCount) {
        canvases_[static_cast<std::size_t>(idx)]->refreshData(lastNow_, lastFollowing_);
    }
}

void GraphPanel::refreshData(double now, bool following)
{
    lastNow_ = now;
    lastFollowing_ = following;
    const int idx = notebook_->GetSelection();
    if (idx >= 0 && idx < kTabCount) {
        // Solo la scheda attiva: copyWindow() costerebbe 7 volte tanto per
        // dati che comunque non sono visibili sulle altre 6 schede.
        canvases_[static_cast<std::size_t>(idx)]->refreshData(now, following);
    }
}

bool GraphPanel::exportPng(const wxString& path) const
{
    const int idx = notebook_->GetSelection();
    if (idx < 0 || idx >= kTabCount) {
        return false;
    }
    return canvases_[static_cast<std::size_t>(idx)]->exportPng(path);
}

double GraphPanel::timeWindowSeconds() const
{
    return canvases_[0]->timeWindow();
}

void GraphPanel::setTimeWindowSeconds(double seconds)
{
    for (auto* canvas : canvases_) {
        canvas->setTimeWindow(seconds);
    }
}

bool GraphPanel::continuousAutoscaleY() const
{
    return canvases_[0]->continuousAutoscaleY();
}

void GraphPanel::setContinuousAutoscaleY(bool enabled)
{
    // Solo la scheda combinata: le schede a canale singolo restano
    // indipendenti (Auto Y locale, attivo di default) e non devono essere
    // resettate solo perché l'utente ha aperto/confermato il SettingsDialog
    // senza toccare questa opzione.
    canvases_[0]->setContinuousAutoscaleY(enabled);
    autoYChecks_[0]->SetValue(enabled);
}

void GraphPanel::setChannelTabTitle(int channel, const wxString& label)
{
    if (channel < 0 || channel >= kNumAnalogChannels) {
        return;
    }
    const wxString title = label.IsEmpty() ? wxString::Format("A%d", channel) : label;
    notebook_->SetPageText(static_cast<std::size_t>(channel + 1), title);
}

void GraphPanel::setCsvPath(const wxString& path)
{
    csvLed_->setOn(true);  // verde: il consumatore sta scrivendo
    csvPathText_->SetLabel(tr(StringId::GpCsvPrefix) + path);
    csvPathText_->SetToolTip(path);
    Layout();
}

void GraphPanel::clearCsvPath()
{
    csvLed_->setOn(false);  // rosso: nessuna registrazione in corso
    csvPathText_->SetLabel(tr(StringId::GpCsvNone));
    csvPathText_->UnsetToolTip();
    Layout();
}

} // namespace am
