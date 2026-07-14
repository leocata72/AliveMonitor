/**
 * @file StatusPanel.cpp
 * @brief Implementazione dell'adapter sulla wxStatusBar nativa.
 */
#include "view/StatusPanel.h"

#include <wx/statusbr.h>

#include "i18n/Strings.h"

namespace am {
namespace {

// Indici dei campi della status bar (ordine di visualizzazione).
constexpr int kFieldFps = 0;
constexpr int kFieldRate = 1;
constexpr int kFieldReceived = 2;
constexpr int kFieldLost = 3;
constexpr int kFieldCrc = 4;
constexpr int kFieldSerialErr = 5;
constexpr int kFieldConnTime = 6;
constexpr int kFieldCpu = 7;

} // namespace

StatusPanel::StatusPanel(wxStatusBar* bar)
    : bar_(bar)
{
    // Larghezze: fisse dove il contenuto ha ampiezza nota, -1 (elastico) sul
    // campo frequenza, il più variabile. L'ultimo campo lascia comunque
    // spazio al size-grip nativo.
    static const int widths[kFieldCount] = { 80, -1, 110, 110, 90, 90, 150, 100 };
    bar_->SetStatusWidths(kFieldCount, widths);

    setField(kFieldFps, "FPS: -");
    setField(kFieldReceived, "RX: 0");
    setField(kFieldCrc, "CRC: 0");
    setField(kFieldCpu, tr(StringId::StCpuNone));
}

void StatusPanel::setField(int field, const wxString& text)
{
    if (field < 0 || field >= kFieldCount || bar_ == nullptr) {
        return;
    }
    auto& cached = last_[static_cast<std::size_t>(field)];
    if (cached != text) {
        cached = text;
        bar_->SetStatusText(text, field);
    }
}

void StatusPanel::update(const BoardState::Snapshot& snapshot,
                         double graphFps,
                         std::optional<double> cpuUsage)
{
    setField(kFieldFps, wxString::Format("FPS: %.1f", graphFps));

    setField(kFieldRate,
        snapshot.measuredRateHz > 0.0
            ? wxString::Format(tr(StringId::StAcqFmt),
                               snapshot.requestedRateHz, snapshot.measuredRateHz)
            : wxString::Format(tr(StringId::StAcqFmtNoEff), snapshot.requestedRateHz));

    setField(kFieldReceived,
        wxString::Format("RX: %llu",
                         static_cast<unsigned long long>(snapshot.stats.packetsReceived)));
    setField(kFieldLost,
        wxString::Format(tr(StringId::StLostFmt),
                         static_cast<unsigned long long>(snapshot.stats.packetsLost)));
    setField(kFieldCrc,
        wxString::Format("CRC: %llu",
                         static_cast<unsigned long long>(snapshot.stats.crcErrors)));
    setField(kFieldSerialErr,
        wxString::Format(tr(StringId::StErrFmt),
                         static_cast<unsigned long long>(snapshot.stats.serialErrors)));

    if (snapshot.connection == ConnectionState::Connected) {
        const auto total = static_cast<long long>(snapshot.connectedSeconds);
        setField(kFieldConnTime,
            tr(StringId::StConnPrefix)
            + wxString::Format("%02lld:%02lld:%02lld",
                               total / 3600, (total / 60) % 60, total % 60));
    } else {
        setField(kFieldConnTime, tr(StringId::StConnPrefix) + "--:--:--");
    }

    setField(kFieldCpu, cpuUsage.has_value()
                            ? wxString::Format(tr(StringId::StCpuFmt), *cpuUsage)
                            : tr(StringId::StCpuNone));
}

} // namespace am
