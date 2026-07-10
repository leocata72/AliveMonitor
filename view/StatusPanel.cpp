/**
 * @file StatusPanel.cpp
 * @brief Implementazione della barra di stato inferiore.
 */
#include "view/StatusPanel.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace am {

StatusPanel::StatusPanel(wxWindow* parent)
    : wxPanel(parent)
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    fpsText_       = addField(sizer, "FPS: -");
    rateText_      = addField(sizer, "Acq: - Hz");
    receivedText_  = addField(sizer, "RX: 0");
    lostText_      = addField(sizer, "Persi: 0");
    crcText_       = addField(sizer, "CRC: 0");
    serialErrText_ = addField(sizer, "Err: 0");
    connTimeText_  = addField(sizer, "Conn: --:--:--");
    cpuText_       = addField(sizer, "CPU: n/d");

    SetSizer(sizer);
}

wxStaticText* StatusPanel::addField(wxSizer* sizer, const wxString& initial)
{
    auto* text = new wxStaticText(this, wxID_ANY, initial);
    sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    sizer->Add(new wxStaticText(this, wxID_ANY, "|"), 0,
               wxALIGN_CENTER_VERTICAL);
    return text;
}

void StatusPanel::update(const BoardState::Snapshot& snapshot,
                         double graphFps,
                         std::optional<double> cpuUsage)
{
    fpsText_->SetLabel(wxString::Format("FPS: %.1f", graphFps));

    rateText_->SetLabel(
        snapshot.measuredRateHz > 0.0
            ? wxString::Format("Acq: %d Hz (eff. %.1f)",
                               snapshot.requestedRateHz, snapshot.measuredRateHz)
            : wxString::Format("Acq: %d Hz", snapshot.requestedRateHz));

    receivedText_->SetLabel(
        wxString::Format("RX: %llu",
                         static_cast<unsigned long long>(snapshot.stats.packetsReceived)));
    lostText_->SetLabel(
        wxString::Format("Persi: %llu",
                         static_cast<unsigned long long>(snapshot.stats.packetsLost)));
    crcText_->SetLabel(
        wxString::Format("CRC: %llu",
                         static_cast<unsigned long long>(snapshot.stats.crcErrors)));
    serialErrText_->SetLabel(
        wxString::Format("Err: %llu",
                         static_cast<unsigned long long>(snapshot.stats.serialErrors)));

    if (snapshot.connection == ConnectionState::Connected) {
        const auto total = static_cast<long long>(snapshot.connectedSeconds);
        connTimeText_->SetLabel(
            wxString::Format("Conn: %02lld:%02lld:%02lld",
                             total / 3600, (total / 60) % 60, total % 60));
    } else {
        connTimeText_->SetLabel("Conn: --:--:--");
    }

    cpuText_->SetLabel(cpuUsage.has_value()
                           ? wxString::Format("CPU: %.1f%%", *cpuUsage)
                           : wxString("CPU: n/d"));
    Layout();
}

} // namespace am
