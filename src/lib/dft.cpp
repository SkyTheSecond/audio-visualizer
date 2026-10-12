#include <shazam/dft.hpp>

#include <cmath>
#include <numbers>

namespace shazam {
namespace {
std::vector<complex> transform(std::span<const complex> input, double sign) {
    const std::size_t n = input.size();
    std::vector<complex> output(n);

    for (std::size_t k = 0; k < n; ++k) {
        complex sum{0.0, 0.0};

        for (std::size_t t = 0; t < n; ++t) {
            const double angle = sign * 2.0 * std::numbers::pi * static_cast<double>((k * t) % n) /
                                 static_cast<double>(n);

            sum += input[t] * complex{std::cos(angle), std::sin(angle)};
        }

        output[k] = sum;
    }

    return output;
}
}  // namespace

std::vector<complex> dft(std::span<const complex> input) { return transform(input, -1.0); }

std::vector<complex> idft(std::span<const complex> input) {
    std::vector<complex> output = transform(input, +1.0);

    const double n = static_cast<double>(output.size());
    for (complex& value : output) {
        value /= n;
    }

    return output;
}
}  // namespace shazam
