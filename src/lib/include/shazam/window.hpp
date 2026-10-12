#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace shazam {

enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
};

std::string_view window_name(WindowType type);

std::vector<double> make_window(WindowType type, std::size_t size);

void apply_window(std::span<double> frame, std::span<const double> window);
}  // namespace shazam
