/**
 * @file LedIndicator.h
 * @brief LED virtuale: widget circolare acceso/spento con colori configurabili.
 *
 * VIEW (MVC). Usato per lo stato connessione (verde/rosso) e per lo stato
 * reale delle uscite digitali (verde/grigio).
 */
#pragma once

#include <wx/colour.h>
#include <wx/window.h>

namespace am {

class LedIndicator : public wxWindow {
public:
    LedIndicator(wxWindow* parent,
                 const wxColour& onColour = wxColour(46, 204, 64),
                 const wxColour& offColour = wxColour(120, 120, 120),
                 int diameter = 16);

    void setOn(bool on);
    [[nodiscard]] bool isOn() const { return on_; }

    void setColours(const wxColour& onColour, const wxColour& offColour);

protected:
    wxSize DoGetBestSize() const override;

private:
    void onPaint(wxPaintEvent& event);

    wxColour onColour_;
    wxColour offColour_;
    int diameter_;
    bool on_ = false;
};

} // namespace am
