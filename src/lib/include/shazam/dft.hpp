#pragma once

#include <complex>
#include <span>
#include <vector>

namespace shazam {

using u32 = uint32_t;
using usize = std::size_t;
using complex = std::complex<double>;

std::vector<complex> dft(std::span<const complex> input);

std::vector<complex> idft(std::span<const complex> input);
}  // namespace shazam
