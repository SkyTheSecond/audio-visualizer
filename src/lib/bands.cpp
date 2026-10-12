#include <audioviz/bands.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace audioviz {
std::vector<Band> make_log_bands(std::size_t count, double low_hz, double high_hz, int sample_rate,
                                 std::size_t frame_size) {
    const double nyquist = sample_rate / 2.0;
    if (count == 0 || frame_size < 2 || !(low_hz > 0.0) || !(low_hz < high_hz) ||
        high_hz > nyquist) {
        throw std::invalid_argument("make_log_bands: need count > 0 and 0 < low < high <= Nyquist");
    }

    const double bin_hz = static_cast<double>(sample_rate) / static_cast<double>(frame_size);
    const std::size_t top_bin = frame_size / 2;

    // Edge i sits at low * (high/low)^(i/count). Computing each edge directly, rather than
    // multiplying by a ratio repeatedly, keeps rounding error from piling up.
    auto edge = [&](std::size_t i) {
        const double fraction = static_cast<double>(i) / static_cast<double>(count);
        return low_hz * std::pow(high_hz / low_hz, fraction);
    };

    std::vector<Band> bands(count);
    for (std::size_t i = 0; i < count; i++) {
        Band& band = bands[i];
        band.low_hz = edge(i);
        band.high_hz = edge(i + 1);

        // Bins whose centre k * bin_hz lies in [low, high).
        auto first = static_cast<std::size_t>(std::ceil(band.low_hz / bin_hz));
        auto end = static_cast<std::size_t>(std::ceil(band.high_hz / bin_hz));

        if (first >= end) {
            // No bin centre inside: the band is narrower than a bin. Borrow the nearest one.
            const double centre = std::sqrt(band.low_hz * band.high_hz);
            first = static_cast<std::size_t>(std::lround(centre / bin_hz));
            end = first + 1;
        }

        // Never DC (bin 0), never past Nyquist.
        band.first_bin = std::clamp<std::size_t>(first, 1, top_bin);
        band.last_bin = std::clamp<std::size_t>(end - 1, band.first_bin, top_bin);
    }
    return bands;
}

void band_peaks(std::span<const double> magnitudes, std::span<const Band> bands,
                std::span<double> out) {
    if (out.size() != bands.size()) {
        throw std::invalid_argument("band_peaks: need one output slot per band");
    }
    if (magnitudes.empty()) {
        std::fill(out.begin(), out.end(), 0.0);
        return;
    }

    for (std::size_t i = 0; i < bands.size(); i++) {
        const std::size_t last = std::min(bands[i].last_bin, magnitudes.size() - 1);
        double peak = 0.0;
        for (std::size_t bin = bands[i].first_bin; bin <= last; bin++) {
            peak = std::max(peak, magnitudes[bin]);
        }
        out[i] = peak;
    }
}
}  // namespace audioviz
