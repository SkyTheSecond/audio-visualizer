#pragma once

#include <audioviz/dft.hpp>
#include <audioviz/window.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace audioviz {

// The magnitude spectrum of one frame at a time: window, FFT, keep |X[k]| for the lower half.
// This is the inner step of the STFT. It owns its window and scratch buffers, so analyze()
// never allocates and can run every frame of a render loop.
class SpectrumAnalyzer {
public:
    // Throws std::invalid_argument if frame_size is not a power of two.
    SpectrumAnalyzer(std::size_t frame_size, WindowType window);

    std::size_t frame_size() const { return window_.size(); }
    std::size_t bin_count() const { return magnitudes_.size(); }  // frame_size / 2 + 1

    // `frame` must hold exactly frame_size() samples. The returned magnitudes stay valid
    // until the next call.
    std::span<const double> analyze(std::span<const double> frame);

private:
    std::vector<double> window_;
    std::vector<complex> buffer_;
    std::vector<double> magnitudes_;
};
}  // namespace audioviz
