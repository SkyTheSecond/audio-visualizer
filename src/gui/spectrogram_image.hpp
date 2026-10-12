#pragma once

#include <shazam/stft.hpp>

#include <vector>

namespace gui {

// A CPU-side image: 4 bytes per pixel (R, G, B, A), rows stored top to bottom.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
};

// Render a spectrogram as a heat-map image: width is frames (time), height is bins
// (frequency) with low frequencies at the bottom. Anything more than `dynamic_range_db`
// below the loudest cell is black. Returns an empty image if there are no frames.
Image spectrogram_to_image(const shazam::Spectrogram& spectrogram, double dynamic_range_db = 80.0);
}  // namespace gui
