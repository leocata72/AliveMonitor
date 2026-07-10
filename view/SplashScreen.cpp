/**
 * @file SplashScreen.cpp
 * @brief Implementazione dello splash screen di avvio.
 */
#include "view/SplashScreen.h"

#include <wx/dcmemory.h>
#include <wx/font.h>

#include "Version.h"

namespace am {

namespace {

wxBitmap buildSplashBitmap()
{
    const wxSize size(420, 240);
    wxBitmap bitmap(size);

    wxMemoryDC dc(bitmap);
    const wxColour background(214, 211, 206);  // grigio chiaro stile "classic UI".
    dc.SetBackground(wxBrush(background));
    dc.Clear();

    // Bordo sottile.
    dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, size.x, size.y);

    // Titolo.
    const wxFont titleFont(24, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(titleFont);
    dc.SetTextForeground(*wxBLACK);
    const wxString title = "Alive Monitor";
    const wxSize titleExtent = dc.GetTextExtent(title);
    dc.DrawText(title, (size.x - titleExtent.x) / 2, 55);

    // Sottotitolo.
    const wxFont subtitleFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    dc.SetFont(subtitleFont);
    const wxString subtitle = "Simple Control Software";
    const wxSize subtitleExtent = dc.GetTextExtent(subtitle);
    dc.DrawText(subtitle, (size.x - subtitleExtent.x) / 2, 100);

    // Separatore.
    dc.SetPen(wxPen(wxColour(150, 150, 150), 1));
    dc.DrawLine(30, 160, size.x - 30, 160);

    // Piè di pagina: versione e crediti.
    const wxFont footerFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    dc.SetFont(footerFont);
    dc.SetTextForeground(wxColour(60, 60, 60));

    const wxString versionLine = wxString::Format("Version %s", kAppVersion);
    const wxSize versionExtent = dc.GetTextExtent(versionLine);
    dc.DrawText(versionLine, (size.x - versionExtent.x) / 2, 180);

    const wxString creditsLine = "by: Leonardo Catalano & Claude";
    const wxSize creditsExtent = dc.GetTextExtent(creditsLine);
    dc.DrawText(creditsLine, (size.x - creditsExtent.x) / 2, 202);

    dc.SelectObject(wxNullBitmap);
    return bitmap;
}

} // namespace

wxSplashScreen* showSplashScreen(int safetyTimeoutMs)
{
    return new wxSplashScreen(
        buildSplashBitmap(),
        wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
        safetyTimeoutMs,
        nullptr, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSIMPLE_BORDER | wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR);
}

} // namespace am
