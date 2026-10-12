#include <shazam/fft.hpp>
#include <bit>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace shazam {
bool is_power_of_two(std::size_t n) { return n != 0 && std::has_single_bit(n); }

namespace {
std::vector<complex> fft_recursive_impl(std::span<const complex> input, double sign) {
    const std::size_t n = input.size();

    if (n == 1) {
        return std::vector<complex>(input.begin(), input.end());
    }
    if (!is_power_of_two(n)) {
        throw std::invalid_argument("fft_recursive: size must be a power of two");
    }

    const std::size_t half = n / 2;
    std::vector<complex> even(half);
    std::vector<complex> odd(half);

    for (std::size_t i = 0; i < half; i++) {
        even[i] = input[2 * i];
        odd[i] = input[2 * i + 1];
    }

    const std::vector<complex> E = fft_recursive_impl(even, sign);
    const std::vector<complex> O = fft_recursive_impl(odd, sign);

    std::vector<complex> output(n);
    for (std::size_t k = 0; k < half; k++) {
        const complex twiddle = std::polar(
            1.0, sign * 2.0 * std::numbers::pi * static_cast<double>(k) / static_cast<double>(n));
        const complex t = twiddle * O[k];

        output[k] = E[k] + t;
        output[k + half] = E[k] - t;
    }

    return output;
}

void bit_reverse_permute(std::span<complex> data) {
    const std::size_t n = data.size();
    const u32 bit_count = static_cast<u32>(std::countr_zero(n));

    for (std::size_t i = 0; i < n; i++) {
        std::size_t reversed = 0;
        for (u32 bit = 0; bit < bit_count; bit++)
            if (i & (std::size_t{1} << bit)) reversed |= std::size_t{1} << (bit_count - bit - 1);
        if (i < reversed) std::swap(data[i], data[reversed]);
    }
}
}  // namespace

std::vector<complex> fft_recursive(std::span<const complex> input, bool inverse) {
    std::vector<complex> output = fft_recursive_impl(input, inverse ? 1.0 : -1.0);

    if (inverse) {
        const double n = static_cast<double>(output.size());
        for (complex& value : output) {
            value /= n;
        }
    }
    return output;
}

void fft_inplace(std::span<complex> data, bool inverse) {
    const std::size_t n = data.size();
    if (n <= 1) return;
    if (!is_power_of_two(n)) {
        throw std::invalid_argument("fft_inplace: size must be a power of two");
    }

    bit_reverse_permute(data);

    const double sign = inverse ? +1.0 : -1.0;

    for (std::size_t size = 2; size <= n; size *= 2) {
        const std::size_t half = size / 2;
        for (std::size_t k = 0; k < half; k++) {
            const complex twiddle = std::polar(
                1.0, sign * std::numbers::pi * static_cast<double>(k) / static_cast<double>(half));

            for (std::size_t block = 0; block < n; block += size) {
                const complex even = data[block + k];
                const complex odd = data[block + k + half] * twiddle;

                data[block + k] = even + odd;
                data[block + k + half] = even - odd;
            }
        }
    }

    if (inverse) {
        const double scale = 1.0 / static_cast<double>(n);
        for (complex& value : data) value *= scale;
    }
}

std::vector<complex> fft(std::span<const complex> input) {
    std::vector<complex> output(input.begin(), input.end());
    fft_inplace(output, false);
    return output;
}

std::vector<complex> ifft(std::span<const complex> input) {
    std::vector<complex> output(input.begin(), input.end());
    fft_inplace(output, true);
    return output;
}

std::vector<complex> fft(std::span<const double> input) {
    std::vector<complex> output(input.size());
    for (std::size_t i = 0; i < input.size(); i++) output[i] = complex{input[i], 0.0};
    fft_inplace(output, false);
    return output;
}
}  // namespace shazam
