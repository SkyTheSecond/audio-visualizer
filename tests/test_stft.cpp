#include <gtest/gtest.h>

#include <audioviz/stft.hpp>

#include <cmath>
#include <numbers>
#include <vector>

namespace {

std::vector<double> make_tone(double frequency, int sample_rate, double seconds) {
    const std::size_t count = static_cast<std::size_t>(static_cast<double>(sample_rate) * seconds);

    std::vector<double> signal(count);
    for (std::size_t t = 0; t < count; t++) {
        signal[t] = std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(t) /
                             static_cast<double>(sample_rate));
    }
    return signal;
}

std::size_t loudest_bin(const audioviz::Spectrogram& s, std::size_t frame) {
    std::size_t best = 0;
    double best_value = -1.0;

    for (std::size_t bin = 0; bin < s.bin_count; bin++) {
        if (s.at(frame, bin) > best_value) {
            best_value = s.at(frame, bin);
            best = bin;
        }
    }
    return best;
}

TEST(Stft, ShapeIsCorrect) {
    const int sample_rate = 11025;
    const std::vector<double> signal = make_tone(440.0, sample_rate, 1.0);

    audioviz::StftConfig config;  // 1024 / 512 / Hann
    const audioviz::Spectrogram s = audioviz::compute_spectrogram(signal, sample_rate, config);

    EXPECT_EQ(s.bin_count, 513u);
    EXPECT_EQ(s.frame_count, 1 + (signal.size() - 1024) / 512);
    EXPECT_EQ(s.magnitudes.size(), s.frame_count * s.bin_count);
}

TEST(Stft, BinFrequencyMathIsRight) {
    const std::vector<double> signal = make_tone(440.0, 11025, 1.0);
    const audioviz::Spectrogram s = audioviz::compute_spectrogram(signal, 11025);

    EXPECT_NEAR(s.bin_frequency(0), 0.0, 1e-9);
    EXPECT_NEAR(s.bin_frequency(1), 11025.0 / 1024.0, 1e-9);

    // The last bin is Nyquist: half the sample rate.
    EXPECT_NEAR(s.bin_frequency(s.bin_count - 1), 11025.0 / 2.0, 1e-9);
}

TEST(Stft, SteadyToneStaysInTheSameBin) {
    const int sample_rate = 11025;
    const double frequency = 1000.0;
    const std::vector<double> signal = make_tone(frequency, sample_rate, 2.0);

    const audioviz::Spectrogram s = audioviz::compute_spectrogram(signal, sample_rate);
    ASSERT_GT(s.frame_count, 10u);

    const auto expected = static_cast<std::size_t>(std::round(frequency * 1024.0 / sample_rate));

    // Every frame should agree, within one bin of rounding.
    for (std::size_t frame = 0; frame < s.frame_count; frame++) {
        const std::size_t found = loudest_bin(s, frame);
        EXPECT_LE(found > expected ? found - expected : expected - found, 1u) << "frame " << frame;
    }
}

// The real test: a chirp sweeping upward must trace a rising line.
TEST(Stft, ChirpRisesOverTime) {
    const int sample_rate = 11025;
    const double seconds = 3.0;
    const double start_hz = 200.0;
    const double end_hz = 4000.0;

    const std::size_t count = static_cast<std::size_t>(sample_rate * seconds);
    std::vector<double> signal(count);

    // Linear chirp: f(t) = start + (end - start) * t / T, and phase is its integral.
    for (std::size_t n = 0; n < count; n++) {
        const double t = static_cast<double>(n) / sample_rate;
        const double phase =
            2.0 * std::numbers::pi * (start_hz * t + (end_hz - start_hz) * t * t / (2.0 * seconds));
        signal[n] = std::sin(phase);
    }

    const audioviz::Spectrogram s = audioviz::compute_spectrogram(signal, sample_rate);
    ASSERT_GT(s.frame_count, 20u);

    const std::size_t first = loudest_bin(s, 2);
    const std::size_t middle = loudest_bin(s, s.frame_count / 2);
    const std::size_t last = loudest_bin(s, s.frame_count - 3);

    EXPECT_LT(first, middle);
    EXPECT_LT(middle, last);

    const double expected_mid_hz = (start_hz + end_hz) / 2.0;
    EXPECT_NEAR(s.bin_frequency(middle), expected_mid_hz, 60.0);
}

TEST(Stft, DecibelConversionClampsSilence) {
    EXPECT_NEAR(audioviz::to_decibels(1.0, 1.0), 0.0, 1e-9);
    EXPECT_NEAR(audioviz::to_decibels(0.5, 1.0), -6.02, 0.01);
    EXPECT_NEAR(audioviz::to_decibels(0.0, 1.0), -80.0, 1e-9);
}

TEST(Stft, ShortSignalProducesNoFrames) {
    const std::vector<double> tiny(100, 0.0);
    const audioviz::Spectrogram s = audioviz::compute_spectrogram(tiny, 11025);
    EXPECT_EQ(s.frame_count, 0u);
}

}  // namespace
