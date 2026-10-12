#include <gtest/gtest.h>

#include <audioviz/bands.hpp>
#include <audioviz/spectrum.hpp>
#include <audioviz/stft.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {

std::vector<double> make_tone(double frequency, int sample_rate, std::size_t count) {
    std::vector<double> signal(count);
    for (std::size_t t = 0; t < count; t++) {
        signal[t] = std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(t) /
                             static_cast<double>(sample_rate));
    }
    return signal;
}

// A full-scale tone sitting exactly on a bin peaks at N/4 with a Hann window: N/2 from the
// DFT of a real sinusoid, times 0.5, the Hann window's average value ("coherent gain").
// The player uses N/4 as its 0 dB reference, so this pins that number down.
TEST(SpectrumAnalyzer, FullScaleToneOnABinPeaksAtQuarterN) {
    const std::size_t n = 1024;
    const std::size_t bin = 64;
    const std::vector<double> tone = make_tone(static_cast<double>(bin), static_cast<int>(n), n);

    audioviz::SpectrumAnalyzer analyzer(n, audioviz::WindowType::Hann);
    const auto magnitudes = analyzer.analyze(tone);

    ASSERT_EQ(magnitudes.size(), n / 2 + 1);
    EXPECT_NEAR(magnitudes[bin], n / 4.0, 1e-6);
}

// The STFT is now built on the analyzer, so any frame of a spectrogram must equal the
// analyzer run on that frame's samples.
TEST(SpectrumAnalyzer, MatchesTheSpectrogramFrameByFrame) {
    const int sample_rate = 11025;
    const std::vector<double> signal = make_tone(700.0, sample_rate, 8000);

    const audioviz::Spectrogram s = audioviz::compute_spectrogram(signal, sample_rate);
    audioviz::SpectrumAnalyzer analyzer(s.config.frame_size, s.config.window);

    const std::size_t frame = 5;
    const auto magnitudes = analyzer.analyze(
        std::span<const double>(signal).subspan(frame * s.config.hop_size, s.config.frame_size));

    for (std::size_t bin = 0; bin < s.bin_count; bin++) {
        EXPECT_DOUBLE_EQ(magnitudes[bin], s.at(frame, bin)) << "bin " << bin;
    }
}

TEST(SpectrumAnalyzer, RejectsBadSizes) {
    EXPECT_THROW(audioviz::SpectrumAnalyzer(1000, audioviz::WindowType::Hann),
                 std::invalid_argument);

    audioviz::SpectrumAnalyzer analyzer(256, audioviz::WindowType::Hann);
    const std::vector<double> wrong(100, 0.0);
    EXPECT_THROW(analyzer.analyze(wrong), std::invalid_argument);
}

TEST(Bands, EdgesAreGeometricAndContiguous) {
    const auto bands = audioviz::make_log_bands(10, 50.0, 12800.0, 44100, 2048);
    ASSERT_EQ(bands.size(), 10u);

    EXPECT_NEAR(bands.front().low_hz, 50.0, 1e-9);
    EXPECT_NEAR(bands.back().high_hz, 12800.0, 1e-6);

    // 50 -> 12800 is 8 octaves over 10 bands: every band is 0.8 octaves wide.
    for (std::size_t i = 0; i < bands.size(); i++) {
        EXPECT_NEAR(std::log2(bands[i].high_hz / bands[i].low_hz), 0.8, 1e-9) << "band " << i;
        if (i + 1 < bands.size()) {
            EXPECT_DOUBLE_EQ(bands[i].high_hz, bands[i + 1].low_hz) << "band " << i;
        }
    }
}

TEST(Bands, EveryBandHasUsableBins) {
    const std::size_t frame_size = 2048;
    const auto bands = audioviz::make_log_bands(48, 40.0, 16000.0, 44100, frame_size);

    for (std::size_t i = 0; i < bands.size(); i++) {
        EXPECT_GE(bands[i].first_bin, 1u) << "band " << i;  // never DC
        EXPECT_LE(bands[i].first_bin, bands[i].last_bin) << "band " << i;
        EXPECT_LE(bands[i].last_bin, frame_size / 2) << "band " << i;
        if (i > 0) {
            EXPECT_GE(bands[i].first_bin, bands[i - 1].first_bin) << "band " << i;
        }
    }
}

TEST(Bands, RejectsNonsense) {
    EXPECT_THROW(audioviz::make_log_bands(0, 40.0, 16000.0, 44100, 2048), std::invalid_argument);
    EXPECT_THROW(audioviz::make_log_bands(8, 500.0, 100.0, 44100, 2048), std::invalid_argument);
    EXPECT_THROW(audioviz::make_log_bands(8, 40.0, 30000.0, 44100, 2048), std::invalid_argument);
}

// End to end: a 1 kHz tone through the analyzer must light up the band containing 1 kHz.
TEST(Bands, ToneLightsUpItsOwnBand) {
    const int sample_rate = 44100;
    const std::size_t frame_size = 2048;
    const auto bands = audioviz::make_log_bands(32, 40.0, 16000.0, sample_rate, frame_size);

    audioviz::SpectrumAnalyzer analyzer(frame_size, audioviz::WindowType::Hann);
    const auto magnitudes = analyzer.analyze(make_tone(1000.0, sample_rate, frame_size));

    std::vector<double> peaks(bands.size());
    audioviz::band_peaks(magnitudes, bands, peaks);

    const auto loudest =
        static_cast<std::size_t>(std::max_element(peaks.begin(), peaks.end()) - peaks.begin());
    EXPECT_LE(bands[loudest].low_hz, 1000.0);
    EXPECT_GT(bands[loudest].high_hz, 1000.0);
}

}  // namespace
