/**
 * @file LedIndicator.cpp
 * @brief Implementazione del LED virtuale.
 */
#include "view/LedIndicator.h"

#include <wx/dcbuffer.h>

namespace am {

LedIndicator::LedIndicator(wxWindow* parent, const wxColour& onColour,
                           const wxColour& offColour, int diameter)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition,
               wxSize(diameter + 4, diameter + 4))
    , onColour_(onColour)
    , offColour_(offColour)
    , diameter_(diameter)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);  // richiesto da wxAutoBufferedPaintDC
    Bind(wxEVT_PAINT, &LedIndicator::onPaint, this);
}

void LedIndicator::setOn(bool on)
{
    if (on_ != on) {
        on_ = on;
        Refresh(false);
    }
}

void LedIndicator::setColours(const wxColour& onColour, const wxColour& offColour)
{
    onColour_ = onColour;
    offColour_ = offColour;
    Refresh(false);
}

wxSize LedIndicator::DoGetBestSize() const
{
    return { diameter_ + 4, diameter_ + 4 };
}

void LedIndicator::onPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);

    // Sfondo uguale a quello del genitore (nessun flicker).
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();

    const wxSize size = GetClientSize();
    const int d = diameter_;
    const int x = (size.x - d) / 2;
    const int y = (size.y - d) / 2;

    const wxColour fill = on_ ? onColour_ : offColour_;
    dc.SetBrush(wxBrush(fill));
    dc.SetPen(wxPen(fill.ChangeLightness(60), 1));
    dc.DrawEllipse(x, y, d, d);

    // Piccolo riflesso per un aspetto "moderno".
    if (on_) {
        dc.SetBrush(wxBrush(fill.ChangeLightness(160)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawEllipse(x + d / 4, y + d / 5, d / 3, d / 4);
    }
}

} // namespace am
