#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace shazam {

// Decoded PCM audio, normalised to roughly [-1.0, 1.0].
// Samples are interleaved: for stereo, L R L R L R ...
struct AudioBuffer {
    std::vector<double> samples;
    int sample_rate = 0;
    int channels = 0;

    // Number of sample frames (one frame = one sample per channel).
    std::size_t frame_count() const;

    double duration_seconds() const;
};

// Read an uncompressed WAV file: 16-bit PCM or 32-bit IEEE float, any channel count.
// Returns an error message rather than throwing.
std::expected<AudioBuffer, std::string> load_wav(const std::filesystem::path& path);

// Average all channels down to one. Mono input is returned unchanged.
std::vector<double> to_mono(const AudioBuffer& audio);
}  // namespace shazam
