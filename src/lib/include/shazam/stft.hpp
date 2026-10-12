#pragma once

#include <shazam/window.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace shazam {

struct StftConfig {
    std::size_t frame_size = 1024;  // must be a power of two
    std::size_t hop_size = 512;     // 50% overlap with the default frame
    WindowType window = WindowType::Hann;
};

// Magnitude spectrogram, stored row-major with one row per frame.
struct Spectrogram {
    std::vector<double> magnitudes;
    std::size_t frame_count = 0;
    std::size_t bin_count = 0;  // frame_size / 2 + 1
    int sample_rate = 0;
    StftConfig config;

    double at(std::size_t frame, std::size_t bin) const {
        return magnitudes[frame * bin_count + bin];
    }

    double& at(std::size_t frame, std::size_t bin) { return magnitudes[frame * bin_count + bin]; }

    // Centre frequency of a bin, in Hz.
    double bin_frequency(std::size_t bin) const;

    // Start time of a frame, in seconds.
    double frame_time(std::size_t frame) const;

    // Largest magnitude anywhere. Used to normalise for display.
    double peak_magnitude() const;
};

Spectrogram compute_spectrogram(std::span<const double> mono, int sample_rate,
                                const StftConfig& config = {});

// Magnitude in dB relative to `reference`, clamped at `floor_db` so silence isn't -inf.
double to_decibels(double magnitude, double reference, double floor_db = -80.0);

// Write a binary PGM greyscale image with low frequencies at the bottom.
bool write_pgm(const Spectrogram& spectrogram, const std::string& path);
}  // namespace shazam
