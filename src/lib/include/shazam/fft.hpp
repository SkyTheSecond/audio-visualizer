#pragma once

#include <shazam/dft.hpp>

#include <span>
#include <vector>

namespace shazam {

bool is_power_of_two(std::size_t n);

std::vector<complex> fft_recursive(std::span<const complex> input, bool inverse = false);

void fft_inplace(std::span<complex> data, bool inverse = false);

std::vector<complex> fft(std::span<const complex> input);
std::vector<complex> ifft(std::span<const complex> input);

std::vector<complex> fft(std::span<const double> input);
}  // namespace shazam
