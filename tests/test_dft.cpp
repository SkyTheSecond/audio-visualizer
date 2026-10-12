#include <gtest/gtest.h>

#include <shazam/dft.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using shazam::complex;

namespace {
TEST(DFT, ConstantSignalHasOnlyDcComponent) {
    const std::size_t n = 16;
    std::vector<complex> x(n, complex{1.0, 0.0});

    const std::vector<complex> spectrum = shazam::dft(x);

    EXPECT_NEAR(spectrum[0].real(), static_cast<double>(n), 1e-9);
    EXPECT_NEAR(spectrum[0].imag(), 0.0, 1e-9);

    for (std::size_t k = 1; k < n; k++) {
        EXPECT_NEAR(std::abs(spectrum[k]), 0.0, 1e-9) << "bin " << k;
    }
}

TEST(DFT, ImpulseHasFlatSpectrum) {
    const std::size_t n = 32;
    std::vector<complex> x(n, complex{0.0, 0.0});
    x[0] = complex{1.0, 0.0};

    const std::vector<complex> spectrum = shazam::dft(x);

    for (std::size_t k = 0; k < n; k++) {
        EXPECT_NEAR(std::abs(spectrum[k] - complex{1.0, 0.0}), 0.0, 1e-9) << "bin " << k;
    }
}

TEST(DFT, PureToneLandsInOneBin) {
    const std::size_t n = 64;
    const std::size_t tone_bin = 5;

    std::vector<complex> x(n);
    for (std::size_t t = 0; t < n; ++t) {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(tone_bin) *
                             static_cast<double>(t) / static_cast<double>(n);

        x[t] = complex{std::cos(angle), 0.0};
    }

    const std::vector<complex> spectrum = shazam::dft(x);

    EXPECT_NEAR(std::abs(spectrum[tone_bin]), n / 2.0, 1e-9);
    EXPECT_NEAR(std::abs(spectrum[n - tone_bin]), n / 2.0, 1e-9);

    for (std::size_t k = 0; k < n; k++) {
        if (k == tone_bin || k == n - tone_bin) continue;
        EXPECT_NEAR(std::abs(spectrum[k]), 0.0, 1e-9) << "bin " << k;
    }
}

TEST(DFT, InverseUndoesForward) {
    const std::size_t n = 48;

    std::vector<complex> x(n);
    for (std::size_t t = 0; t < n; t++) {
        x[t] =
            complex{std::sin(0.3 * static_cast<double>(t)), std::cos(0.7) * static_cast<double>(t)};
    }

    const std::vector<complex> restored = shazam::idft(shazam::dft(x));

    for (std::size_t t = 0; t < n; t++) {
        EXPECT_NEAR(std::abs(restored[t] - x[t]), 0.0, 1e-9) << "sample " << t;
    }
}
}  // namespace
