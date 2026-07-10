/**
 * @file AnalogDataBuffer.cpp
 * @brief Implementazione del contenitore thread-safe dei campioni analogici.
 */
#include "model/AnalogDataBuffer.h"

#include <iomanip>

namespace am {

AnalogDataBuffer::AnalogDataBuffer(std::size_t capacityPerChannel)
    : channels_{ RingBuffer<AnalogSample>(capacityPerChannel),
                 RingBuffer<AnalogSample>(capacityPerChannel),
                 RingBuffer<AnalogSample>(capacityPerChannel),
                 RingBuffer<AnalogSample>(capacityPerChannel),
                 RingBuffer<AnalogSample>(capacityPerChannel),
                 RingBuffer<AnalogSample>(capacityPerChannel) }
{
}

void AnalogDataBuffer::push(double t,
                            const std::array<std::uint16_t, kNumAnalogChannels>& values)
{
    const std::lock_guard lock(mutex_);
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        channels_[static_cast<std::size_t>(ch)].push(
            AnalogSample{ t, values[static_cast<std::size_t>(ch)] });
    }
}

void AnalogDataBuffer::copyWindow(
    double tMin,
    std::array<std::vector<AnalogSample>, kNumAnalogChannels>& out) const
{
    const std::lock_guard lock(mutex_);
    for (std::size_t ch = 0; ch < kNumAnalogChannels; ++ch) {
        auto& dst = out[ch];
        dst.clear(); // mantiene la capacità: nessuna riallocazione a regime

        const auto& src = channels_[ch];
        const std::size_t n = src.size();
        if (n == 0) {
            continue;
        }

        // Ricerca binaria del primo campione con t >= tMin
        // (i campioni sono in ordine cronologico crescente).
        std::size_t lo = 0, hi = n;
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (src[mid].t < tMin) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        dst.reserve(n - lo);
        for (std::size_t i = lo; i < n; ++i) {
            dst.push_back(src[i]);
        }
    }
}

std::optional<double> AnalogDataBuffer::latestTime() const
{
    const std::lock_guard lock(mutex_);
    if (channels_[0].empty()) {
        return std::nullopt;
    }
    return channels_[0].back().t;
}

std::size_t AnalogDataBuffer::sizePerChannel() const
{
    const std::lock_guard lock(mutex_);
    return channels_[0].size();
}

void AnalogDataBuffer::clear()
{
    const std::lock_guard lock(mutex_);
    for (auto& ch : channels_) {
        ch.clear();
    }
}

bool AnalogDataBuffer::exportCsv(std::ostream& os) const
{
    const std::lock_guard lock(mutex_);

    os << "tempo_s";
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        os << ",A" << ch << "_V";
    }
    os << '\n';

    // I sei canali vengono riempiti sempre insieme (stesso push), quindi
    // hanno lo stesso numero di campioni e timestamp allineati.
    const std::size_t n = channels_[0].size();
    os << std::fixed;
    for (std::size_t i = 0; i < n; ++i) {
        os << std::setprecision(6) << channels_[0][i].t;
        for (std::size_t ch = 0; ch < kNumAnalogChannels; ++ch) {
            os << ',' << std::setprecision(4) << channels_[ch][i].volts();
        }
        os << '\n';
    }
    return static_cast<bool>(os);
}

} // namespace am
