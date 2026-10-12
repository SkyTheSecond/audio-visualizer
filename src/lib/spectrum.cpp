#include <audioviz/spectrum.hpp>

#include <audioviz/fft.hpp>

#include <cmath>
#include <stdexcept>

namespace audioviz {
SpectrumAnalyzer::SpectrumAnalyzer(std::size_t frame_size, WindowType window) {
    if (!is_power_of_two(frame_size)) {
        throw std::invalid_argument("SpectrumAnalyzer: frame_size must be a power of two");
    }
    window_ = make_window(window, frame_size);
    buffer_.resize(frame_size);
    magnitudes_.resize(frame_size / 2 + 1);
}

std::span<const double> SpectrumAnalyzer::analyze(std::span<const double> frame) {
    if (frame.size() != window_.size()) {
        throw std::invalid_argument("SpectrumAnalyzer::analyze: frame has the wrong length");
    }

    for (std::size_t i = 0; i < frame.size(); i++) {
        buffer_[i] = complex{frame[i] * window_[i], 0.0};
    }

    fft_inplace(buffer_, false);

    // Keep the lower half only; for real input the upper half mirrors it.
    for (std::size_t bin = 0; bin < magnitudes_.size(); bin++) {
        magnitudes_[bin] = std::abs(buffer_[bin]);
    }
    return magnitudes_;
}
}  // namespace audioviz
