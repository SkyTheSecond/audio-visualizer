#include <audioviz/stft.hpp>

#include <audioviz/fft.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace audioviz {
double Spectrogram::bin_frequency(std::size_t bin) const {
    return static_cast<double>(bin) * static_cast<double>(sample_rate) /
           static_cast<double>(config.frame_size);
}

double Spectrogram::frame_time(std::size_t frame) const {
    return static_cast<double>(frame * config.hop_size) / static_cast<double>(sample_rate);
}

double Spectrogram::peak_magnitude() const {
    double peak = 0.0;
    for (double value : magnitudes) peak = std::max(peak, value);
    return peak;
}

Spectrogram compute_spectrogram(std::span<const double> mono, int sample_rate,
                                const StftConfig& config) {
    if (!is_power_of_two(config.frame_size)) {
        throw std::invalid_argument("compute_spectrogram: frame_size must be a power of two");
    }
    if (config.hop_size == 0) {
        throw std::invalid_argument("compute_spectrogram: hop_size must be greater than zero");
    }

    Spectrogram spectrogram;
    spectrogram.sample_rate = sample_rate;
    spectrogram.config = config;
    spectrogram.bin_count = config.frame_size / 2 + 1;

    if (mono.size() < config.frame_size) {
        return spectrogram;  // too short for even one frame
    }

    spectrogram.frame_count = 1 + (mono.size() - config.frame_size) / config.hop_size;
    spectrogram.magnitudes.resize(spectrogram.frame_count * spectrogram.bin_count);

    const std::vector<double> window = make_window(config.window, config.frame_size);

    // Reused scratch buffer so the loop performs no allocation.
    std::vector<complex> buffer(config.frame_size);

    for (std::size_t frame = 0; frame < spectrogram.frame_count; frame++) {
        const std::size_t offset = frame * config.hop_size;

        for (std::size_t i = 0; i < config.frame_size; i++) {
            buffer[i] = complex{mono[offset + i] * window[i], 0.0};
        }

        fft_inplace(buffer, false);

        // Keep the lower half only; for real input the upper half mirrors it.
        for (std::size_t bin = 0; bin < spectrogram.bin_count; bin++) {
            spectrogram.at(frame, bin) = std::abs(buffer[bin]);
        }
    }

    return spectrogram;
}

double to_decibels(double magnitude, double reference, double floor_db) {
    if (magnitude <= 0.0 || reference <= 0.0) {
        return floor_db;
    }

    // 20*log10 is the amplitude convention; power would be 10*log10.
    const double db = 20.0 * std::log10(magnitude / reference);
    return std::max(db, floor_db);
}

bool write_pgm(const Spectrogram& spectrogram, const std::string& path) {
    if (spectrogram.frame_count == 0 || spectrogram.bin_count == 0) {
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    const std::size_t width = spectrogram.frame_count;
    const std::size_t height = spectrogram.bin_count;

    // Binary greyscale PGM: magic number, dimensions, max value, then bytes.
    out << "P5\n" << width << " " << height << "\n255\n";

    const double peak = spectrogram.peak_magnitude();
    const double floor_db = -80.0;

    std::vector<unsigned char> row(width);

    // PGM rows go top to bottom and high frequencies belong at the top.
    for (std::size_t y = 0; y < height; y++) {
        const std::size_t bin = height - 1 - y;

        for (std::size_t frame = 0; frame < width; frame++) {
            const double db = to_decibels(spectrogram.at(frame, bin), peak, floor_db);
            const double normalised = (db - floor_db) / (0.0 - floor_db);
            row[frame] = static_cast<unsigned char>(std::clamp(normalised, 0.0, 1.0) * 255.0);
        }

        out.write(reinterpret_cast<const char*>(row.data()), static_cast<std::streamsize>(row.size()));
    }

    return out.good();
}
}  // namespace audioviz
