#include <gtest/gtest.h>

#include <shazam/dft.hpp>
#include <shazam/fft.hpp>

#include <random>
#include <stdexcept>
#include <vector>

using shazam::complex;

namespace {

std::vector<complex> random_signal(std::size_t n, unsigned seed = 1234) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    std::vector<complex> x(n);

    for (complex& value : x) value = complex{dist(rng), dist(rng)};

    return x;
}

double max_difference(const std::vector<complex>& a, const std::vector<complex>& b) {
    double worst = 0.0;
    for (std::size_t i = 0; i < a.size(); i++) worst = std::max(worst, std::abs(a[i] - b[i]));

    return worst;
}

TEST(FFTRecursive, MatchesNaiveDFT) {
    for (std::size_t n : {1u, 2u, 4u, 8u, 64u, 256u, 1024u}) {
        const std::vector<complex> x = random_signal(n);

        EXPECT_LT(max_difference(shazam::fft_recursive(x), shazam::dft(x)), 1e-9) << "n = " << n;
    }
}

TEST(FFTRecursive, InverseUndoesForward) {
    for (std::size_t n : {2u, 16u, 512u}) {
        const std::vector<complex> x = random_signal(n);
        const std::vector<complex> spectrum = shazam::fft_recursive(x);

        const std::vector<complex> restored = shazam::fft_recursive(spectrum, true);
        EXPECT_LT(max_difference(restored, x), 1e-9) << "n = " << n;
    }
}

TEST(FFTRecursive, RejectsNonPowerOfTwo) {
    const std::vector<complex> x(100);
    EXPECT_THROW(shazam::fft_recursive(x), std::invalid_argument);
}

TEST(FFT, MatchesNaiveDFT) {
    for (std::size_t n : {1u, 2u, 4u, 8u, 64u, 256u, 1024u}) {
        const std::vector<complex> x = random_signal(n);

        EXPECT_LT(max_difference(shazam::fft(x), shazam::dft(x)), 1e-9) << "n = " << n;
    }
}

TEST(FFT, MatchesRecursive) {
    for (std::size_t n : {1u, 2u, 4u, 8u, 64u, 256u, 1024u}) {
        const std::vector<complex> x = random_signal(n);

        EXPECT_LT(max_difference(shazam::fft(x), shazam::fft_recursive(x)), 1e-9) << "n = " << n;
    }
}

TEST(FFT, InverseUndoesForward) {
    for (std::size_t n : {2u, 16u, 512u}) {
        const std::vector<complex> x = random_signal(n);
        const std::vector<complex> spectrum = shazam::fft(x);

        const std::vector<complex> restored = shazam::ifft(spectrum);
        EXPECT_LT(max_difference(restored, x), 1e-9) << "n = " << n;
    }
}

TEST(FFT, ImpulseHasFlatSpectrum) {
    const std::size_t n = 64;
    std::vector<complex> x(n, complex{0.0, 0.0});
    x[0] = complex{1.0, 0.0};

    for (const complex& bin : shazam::fft(x)) {
        EXPECT_NEAR(std::abs(bin - complex{1.0, 0.0}), 0.0, 1e-12);
    }
}

TEST(FFT, PureToneLandsInOneBin) {
    const std::size_t n = 1024;
    const std::size_t tone_bin = 50;

    std::vector<double> x(n);
    for (std::size_t t = 0; t < n; t++) {
        x[t] = std::cos(2.0 * std::numbers::pi * static_cast<double>(tone_bin) *
                        static_cast<double>(t) / static_cast<double>(n));
    }

    const std::vector<complex> spectrum = shazam::fft(std::span<const double>(x));

    EXPECT_NEAR(std::abs(spectrum[tone_bin]), n / 2.0, 1e-6);
    EXPECT_NEAR(std::abs(spectrum[n - tone_bin]), n / 2.0, 1e-6);

    for (std::size_t k = 0; k < n; k++) {
        if (k == tone_bin || k == n - tone_bin) {
            continue;
        }
        EXPECT_LT(std::abs(spectrum[k]), 1e-8) << "bin " << k;
    }
}

TEST(FFT, ConservesEnergy) {
    const std::size_t n = 256;
    const std::vector<complex> x = random_signal(n);
    const std::vector<complex> spectrum = shazam::fft(x);

    double time_energy = 0.0;
    for (const complex& value : x) {
        time_energy += std::norm(value);
    }

    double freq_energy = 0.0;
    for (const complex& bin : spectrum) {
        freq_energy += std::norm(bin);
    }

    EXPECT_NEAR(time_energy, freq_energy / static_cast<double>(n), 1e-9);
}

TEST(FFT, RejectsNonPowerOfTwo) {
    const std::vector<complex> x(100);
    EXPECT_THROW(shazam::fft(x), std::invalid_argument);
}
}  // namespace
