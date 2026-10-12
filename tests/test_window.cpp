#include <gtest/gtest.h>

#include <audioviz/fft.hpp>
#include <audioviz/window.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using audioviz::complex;
using audioviz::WindowType;

namespace {

TEST(Window, RectangularIsAllOnes) {
    for (double value : audioviz::make_window(WindowType::Rectangular, 32)) {
        EXPECT_DOUBLE_EQ(value, 1.0);
    }
}

TEST(Window, HannStartsAtZeroAndPeaksAtOne) {
    const std::vector<double> w = audioviz::make_window(WindowType::Hann, 1024);

    EXPECT_NEAR(w.front(), 0.0, 1e-12);
    EXPECT_NEAR(w[w.size() / 2], 1.0, 1e-12);

    // Periodic, so the last sample is one step short of returning to zero
    // rather than being zero itself.
    EXPECT_GT(w.back(), 0.0);
    EXPECT_LT(w.back(), 0.01);
}

TEST(Window, HammingHasNonZeroEndpoints) {
    const std::vector<double> w = audioviz::make_window(WindowType::Hamming, 512);
    EXPECT_NEAR(w.front(), 0.08, 1e-12);
}

/// Measure leakage directly. Put a tone deliberately between two bins, then
/// sum the spectral energy far away from it. A window should cut that a lot.
TEST(Window, HannLeaksLessThanRectangular) {
    const std::size_t n = 1024;
    const double tone_bin = 50.5;  // deliberately between bins 50 and 51

    std::vector<double> signal(n);
    for (std::size_t t = 0; t < n; ++t) {
        signal[t] = std::cos(2.0 * std::numbers::pi * tone_bin * static_cast<double>(t) /
                             static_cast<double>(n));
    }

    auto far_field_energy = [&](WindowType type) {
        std::vector<double> frame = signal;
        const std::vector<double> window = audioviz::make_window(type, n);
        audioviz::apply_window(frame, window);

        const std::vector<complex> spectrum = audioviz::fft(std::span<const double>(frame));

        // Sum energy in the lower half, skipping 10 bins either side of the
        // tone. Anything left is leakage.
        double energy = 0.0;
        for (std::size_t k = 0; k < n / 2; ++k) {
            const double distance = std::abs(static_cast<double>(k) - tone_bin);
            if (distance > 10.0) {
                energy += std::norm(spectrum[k]);
            }
        }
        return energy;
    };

    const double rectangular = far_field_energy(WindowType::Rectangular);
    const double hann = far_field_energy(WindowType::Hann);

    // In practice this is a factor of many thousands, not merely "less".
    EXPECT_LT(hann, rectangular / 100.0);
}

}  // namespace
