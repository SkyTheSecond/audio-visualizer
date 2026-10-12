#include <audioviz/audio.hpp>

#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <iterator>

namespace audioviz {
std::size_t AudioBuffer::frame_count() const {
    return channels > 0 ? samples.size() / static_cast<std::size_t>(channels) : 0;
}

double AudioBuffer::duration_seconds() const {
    return sample_rate > 0 ? static_cast<double>(frame_count()) / static_cast<double>(sample_rate)
                           : 0.0;
}

namespace {
// WAV is little-endian. Reading byte by byte and shifting is correct on any host.
std::uint16_t read_u16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t read_u32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

bool tag_equals(const unsigned char* p, const char* tag) { return std::memcmp(p, tag, 4) == 0; }

// Values of the wFormatTag field in the fmt chunk.
constexpr std::uint16_t format_pcm = 0x0001;
constexpr std::uint16_t format_float = 0x0003;
constexpr std::uint16_t format_extensible = 0xFFFE;
}  // namespace

std::expected<AudioBuffer, std::string> load_wav(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(std::format("cannot open '{}'", path.string()));
    }

    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());

    if (bytes.size() < 12) {
        return std::unexpected("file is too small to be a WAV");
    }
    if (!tag_equals(&bytes[0], "RIFF") || !tag_equals(&bytes[8], "WAVE")) {
        return std::unexpected("not a RIFF/WAVE file");
    }

    // Walk the chunk list. Data does NOT always start at byte 44: real files put
    // LIST, fact and other chunks before it.
    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint32_t sample_rate = 0;
    std::uint16_t bits_per_sample = 0;
    bool have_format = false;

    const unsigned char* data_start = nullptr;
    std::size_t data_size = 0;

    std::size_t position = 12;
    while (position + 8 <= bytes.size()) {
        const unsigned char* id = &bytes[position];
        const std::uint32_t chunk_size = read_u32(&bytes[position + 4]);
        const std::size_t body = position + 8;

        if (body + chunk_size > bytes.size()) break;  // truncated: don't read past the end

        if (tag_equals(id, "fmt ") && chunk_size >= 16) {
            format = read_u16(&bytes[body + 0]);
            channels = read_u16(&bytes[body + 2]);
            sample_rate = read_u32(&bytes[body + 4]);
            bits_per_sample = read_u16(&bytes[body + 14]);

            // WAVE_FORMAT_EXTENSIBLE hides the real tag in the first two bytes of a GUID.
            if (format == format_extensible && chunk_size >= 40) {
                format = read_u16(&bytes[body + 24]);
            }
            have_format = true;
        } else if (tag_equals(id, "data")) {
            data_start = &bytes[body];
            data_size = chunk_size;
        }

        // Chunks are padded to an even length.
        position = body + chunk_size + (chunk_size % 2);
    }

    if (!have_format) {
        return std::unexpected("no 'fmt ' chunk found");
    }
    if (data_start == nullptr) {
        return std::unexpected("no 'data' chunk found");
    }
    if (channels == 0 || sample_rate == 0) {
        return std::unexpected("invalid channel count or sample rate");
    }

    AudioBuffer audio;
    audio.sample_rate = static_cast<int>(sample_rate);
    audio.channels = static_cast<int>(channels);

    if (format == format_pcm && bits_per_sample == 16) {
        const std::size_t count = data_size / 2;
        audio.samples.resize(count);

        for (std::size_t i = 0; i < count; i++) {
            const auto raw = static_cast<std::int16_t>(read_u16(data_start + i * 2));
            audio.samples[i] = static_cast<double>(raw) / 32768.0;
        }
    } else if (format == format_float && bits_per_sample == 32) {
        const std::size_t count = data_size / 4;
        audio.samples.resize(count);

        for (std::size_t i = 0; i < count; i++) {
            // memcpy is the only well-defined way to reinterpret bytes as a float.
            float value = 0.0f;
            std::memcpy(&value, data_start + i * 4, 4);
            audio.samples[i] = static_cast<double>(value);
        }
    } else {
        return std::unexpected(
            std::format("unsupported WAV format (tag {}, {} bits). "
                        "Convert with: ffmpeg -i in -c:a pcm_s16le out.wav",
                        format, bits_per_sample));
    }

    return audio;
}

std::vector<double> to_mono(const AudioBuffer& audio) {
    if (audio.channels <= 1) {
        return audio.samples;
    }

    const std::size_t channels = static_cast<std::size_t>(audio.channels);
    const std::size_t frames = audio.frame_count();

    std::vector<double> mono(frames);
    for (std::size_t frame = 0; frame < frames; frame++) {
        double sum = 0.0;
        for (std::size_t channel = 0; channel < channels; channel++) {
            sum += audio.samples[frame * channels + channel];
        }
        mono[frame] = sum / static_cast<double>(channels);
    }

    return mono;
}
}  // namespace audioviz
