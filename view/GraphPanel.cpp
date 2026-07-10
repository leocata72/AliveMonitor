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
#include <wx/sizer.h>
#include <wx/stattext.h>

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

PlotCanvas::PlotCanvas(wxWindow* parent, const AnalogDataBuffer& buffer)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
               wxFULL_REPAINT_ON_RESIZE)
    , buffer_(buffer)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(400, 300));

    visible_.fill(true);
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
            const double v = s.volts();
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    }

    if (lo > hi) {
        // Nessun dato visibile: intervallo predefinito dell'ADC.
        yMin_ = 0.0;
        yMax_ = kAdcReferenceVolt;
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
    yMin_ = 0.0;
    yMax_ = kAdcReferenceVolt;
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

    // --- Griglia e etichette asse Y (tensione [V]) ----------------------------
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
        const wxString label = wxString::Format("%.*f V", decimalsY, v);
        const wxSize ext = dc.GetTextExtent(label);
        dc.DrawText(label, plot.x - ext.x - 6, y - ext.y / 2);
    }

    // Titolo asse X.
    const wxString xTitle = "Tempo [s]";
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
                const int col = static_cast<int>((s.t - xMin_) * sx);
                const double v = s.volts();
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
                scratchPoints_.emplace_back(toX(s.t), toY(s.volts()));
            }
        }

        if (scratchPoints_.size() >= 2) {
            dc.DrawLines(static_cast<int>(scratchPoints_.size()),
                         scratchPoints_.data());
        }
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

    const int rowH = 16;
    const int boxW = 64;
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
        dc.DrawText(wxString::Format("A%d", ch), box.x + 30, y);
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
                       const AnalogDataBuffer& buffer)
    : wxPanel(parent)
    , actions_(actions)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // --- Barra dei controlli del grafico -------------------------------------
    auto* controls = new wxBoxSizer(wxHORIZONTAL);

    canvas_ = new PlotCanvas(this, buffer);

    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        auto* check = new wxCheckBox(this, wxID_ANY,
                                     wxString::Format("A%d", ch));
        check->SetValue(true);
        check->Bind(wxEVT_CHECKBOX, [this, ch](wxCommandEvent& e) {
            canvas_->setChannelVisible(ch, e.IsChecked());
        });
        channelChecks_[static_cast<std::size_t>(ch)] = check;
        controls->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    }

    controls->AddStretchSpacer(1);

    // Stato della registrazione CSV continua (LED + percorso file), centrato
    // nello spazio libero tra i canali e i controlli di scala/esportazione:
    // LED verde quando il consumatore sta scrivendo, rosso quando è fermo.
    csvLed_ = new LedIndicator(this, wxColour(46, 204, 64), wxColour(200, 40, 40), 14);
    controls->Add(csvLed_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    csvPathText_ = new wxStaticText(this, wxID_ANY, "CSV: nessuna registrazione");
    controls->Add(csvPathText_, 0, wxALIGN_CENTER_VERTICAL);

    controls->AddStretchSpacer(1);

    autoYCheck_ = new wxCheckBox(this, wxID_ANY, "Auto Y");
    autoYCheck_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
        canvas_->setContinuousAutoscaleY(e.IsChecked());
    });
    controls->Add(autoYCheck_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    auto* autoscaleBtn = new wxButton(this, wxID_ANY, "Autoscala",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxBU_EXACTFIT);
    autoscaleBtn->Bind(wxEVT_BUTTON,
                       [this](wxCommandEvent&) { canvas_->autoscaleYOnce(); });
    controls->Add(autoscaleBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    auto* followBtn = new wxButton(this, wxID_ANY, "Segui",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxBU_EXACTFIT);
    followBtn->Bind(wxEVT_BUTTON,
                    [this](wxCommandEvent&) { canvas_->followNow(); });
    controls->Add(followBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    auto* resetBtn = new wxButton(this, wxID_ANY, "Reset",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxBU_EXACTFIT);
    resetBtn->Bind(wxEVT_BUTTON,
                   [this](wxCommandEvent&) { canvas_->resetView(); });
    controls->Add(resetBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto* pngBtn = new wxButton(this, wxID_ANY, "PNG",
                                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pngBtn->Bind(wxEVT_BUTTON,
                 [this](wxCommandEvent&) { actions_.onExportPngRequested(); });
    controls->Add(pngBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    sizer->Add(controls, 0, wxEXPAND | wxALL, 4);
    sizer->Add(canvas_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
    SetSizer(sizer);
}

void GraphPanel::refreshData(double now, bool following)
{
    canvas_->refreshData(now, following);
}

bool GraphPanel::exportPng(const wxString& path) const
{
    return canvas_->exportPng(path);
}

double GraphPanel::timeWindowSeconds() const
{
    return canvas_->timeWindow();
}

void GraphPanel::setTimeWindowSeconds(double seconds)
{
    canvas_->setTimeWindow(seconds);
}

bool GraphPanel::continuousAutoscaleY() const
{
    return canvas_->continuousAutoscaleY();
}

void GraphPanel::setContinuousAutoscaleY(bool enabled)
{
    canvas_->setContinuousAutoscaleY(enabled);
    autoYCheck_->SetValue(enabled);
}

void GraphPanel::setCsvPath(const wxString& path)
{
    csvLed_->setOn(true);  // verde: il consumatore sta scrivendo
    csvPathText_->SetLabel("CSV: " + path);
    csvPathText_->SetToolTip(path);
    Layout();
}

void GraphPanel::clearCsvPath()
{
    csvLed_->setOn(false);  // rosso: nessuna registrazione in corso
    csvPathText_->SetLabel("CSV: nessuna registrazione");
    csvPathText_->UnsetToolTip();
    Layout();
}

} // namespace am
