/**
 * @file StatusPanel.cpp
 * @brief Implementazione della barra di stato inferiore.
 */
#include "view/StatusPanel.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#include "i18n/Strings.h"

namespace am {

StatusPanel::StatusPanel(wxWindow* parent)
    : wxPanel(parent)
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    // FPS/RX/CRC: sigle tecniche universali, non tradotte (vedi Strings.h).
    fpsText_       = addField(sizer, "FPS: -");
    rateText_      = addField(sizer, wxEmptyString);  // sovrascritto da update() subito dopo
    receivedText_  = addField(sizer, "RX: 0");
    lostText_      = addField(sizer, wxEmptyString);
    crcText_       = addField(sizer, "CRC: 0");
    serialErrText_ = addField(sizer, wxEmptyString);
    connTimeText_  = addField(sizer, wxEmptyString);
    cpuText_       = addField(sizer, tr(StringId::StCpuNone));

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
            ? wxString::Format(tr(StringId::StAcqFmt),
                               snapshot.requestedRateHz, snapshot.measuredRateHz)
            : wxString::Format(tr(StringId::StAcqFmtNoEff), snapshot.requestedRateHz));

    receivedText_->SetLabel(
        wxString::Format("RX: %llu",
                         static_cast<unsigned long long>(snapshot.stats.packetsReceived)));
    lostText_->SetLabel(
        wxString::Format(tr(StringId::StLostFmt),
                         static_cast<unsigned long long>(snapshot.stats.packetsLost)));
    crcText_->SetLabel(
        wxString::Format("CRC: %llu",
                         static_cast<unsigned long long>(snapshot.stats.crcErrors)));
    serialErrText_->SetLabel(
        wxString::Format(tr(StringId::StErrFmt),
                         static_cast<unsigned long long>(snapshot.stats.serialErrors)));

    if (snapshot.connection == ConnectionState::Connected) {
        const auto total = static_cast<long long>(snapshot.connectedSeconds);
        connTimeText_->SetLabel(
            tr(StringId::StConnPrefix)
            + wxString::Format("%02lld:%02lld:%02lld",
                               total / 3600, (total / 60) % 60, total % 60));
    } else {
        connTimeText_->SetLabel(tr(StringId::StConnPrefix) + "--:--:--");
    }

    cpuText_->SetLabel(cpuUsage.has_value()
                           ? wxString::Format(tr(StringId::StCpuFmt), *cpuUsage)
                           : tr(StringId::StCpuNone));
    Layout();
}

} // namespace am
