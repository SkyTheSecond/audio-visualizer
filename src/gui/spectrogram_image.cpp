#include "spectrogram_image.hpp"

#include <algorithm>
#include <cstddef>

namespace gui {
namespace {
struct Rgb {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// Heat colormap: black -> red -> yellow -> white as value goes 0 -> 1.
Rgb heat_color(double value) {
    auto ramp = [](double x) {
        return static_cast<unsigned char>(std::clamp(x, 0.0, 1.0) * 255.0);
    };

    const double v = std::clamp(value, 0.0, 1.0);

    // Each channel ramps up a third later than the previous one: red, then green, then blue.
    return Rgb{ramp(v * 3.0), ramp(v * 3.0 - 1.0), ramp(v * 3.0 - 2.0)};
}
}  // namespace

Image spectrogram_to_image(const audioviz::Spectrogram& spectrogram, double dynamic_range_db) {
    Image image;

    if (spectrogram.frame_count == 0 || spectrogram.bin_count == 0) {
        return image;
    }

    image.width = static_cast<int>(spectrogram.frame_count);
    image.height = static_cast<int>(spectrogram.bin_count);
    image.rgba.resize(spectrogram.frame_count * spectrogram.bin_count * 4);

    const double range = std::max(dynamic_range_db, 1.0);
    const double floor_db = -range;
    const double reference = spectrogram.peak_magnitude();

    for (std::size_t row = 0; row < spectrogram.bin_count; row++) {
        // Row 0 is the top of the image; low frequencies belong at the bottom.
        const std::size_t bin = spectrogram.bin_count - 1 - row;

        for (std::size_t frame = 0; frame < spectrogram.frame_count; frame++) {
            const double db = audioviz::to_decibels(spectrogram.at(frame, bin), reference, floor_db);

            // floor_db maps to 0.0, and 0 dB (the loudest cell) maps to 1.0.
            const Rgb color = heat_color((db - floor_db) / range);

            const std::size_t index = (row * spectrogram.frame_count + frame) * 4;
            image.rgba[index + 0] = color.r;
            image.rgba[index + 1] = color.g;
            image.rgba[index + 2] = color.b;
            image.rgba[index + 3] = 255;
        }
    }

    return image;
}
}  // namespace gui
