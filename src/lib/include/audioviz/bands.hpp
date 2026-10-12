#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace audioviz {

// A frequency range and the FFT bins whose centres fall inside it.
struct Band {
    double low_hz = 0.0;
    double high_hz = 0.0;
    std::size_t first_bin = 0;  // inclusive
    std::size_t last_bin = 0;   // inclusive
};

// Split [low_hz, high_hz] into `count` bands with geometrically spaced edges, so every band
// spans the same musical interval. Every band gets at least one bin; at the bottom, where a
// bin is wider than a band, neighbouring bands can share one.
// Throws std::invalid_argument unless count > 0 and 0 < low_hz < high_hz <= Nyquist.
std::vector<Band> make_log_bands(std::size_t count, double low_hz, double high_hz, int sample_rate,
                                 std::size_t frame_size);

// The loudest magnitude among each band's bins. `out` needs one slot per band.
void band_peaks(std::span<const double> magnitudes, std::span<const Band> bands,
                std::span<double> out);
}  // namespace audioviz
