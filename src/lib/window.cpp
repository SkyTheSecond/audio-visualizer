#include <shazam/window.hpp>

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace shazam {
std::string_view window_name(WindowType type) {
    switch (type) {
        case WindowType::Rectangular:
            return "Rectangular";
        case WindowType::Hann:
            return "Hann";
        case WindowType::Hamming:
            return "Hamming";
    }
    return "Unknown";
}

std::vector<double> make_window(WindowType type, std::size_t size) {
    std::vector<double> window(size, 1.0);

    if (type == WindowType::Rectangular || size <= 1) {
        return window;
    }

    const double denominator = static_cast<double>(size);

    for (std::size_t i = 0; i < size; i++) {
        const double phase = 2.0 * std::numbers::pi * static_cast<double>(i) / denominator;

        switch (type) {
            case WindowType::Hann:
                window[i] = 0.5 - 0.5 * std::cos(phase);
                break;
            case WindowType::Hamming:
                window[i] = 0.54 - 0.46 * std::cos(phase);
                break;
            case WindowType::Rectangular:
                break;
        }
    }

    return window;
}

void apply_window(std::span<double> frame, std::span<const double> window) {
    if (frame.size() != window.size()) {
        throw std::invalid_argument("apply_window: size mismatch");
    }

    for (std::size_t i = 0; i < frame.size(); i++) {
        frame[i] *= window[i];
    }
}
}  // namespace shazam
