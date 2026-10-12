#include <gtest/gtest.h>

#include <audioviz/audio.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void push_tag(std::vector<unsigned char>& out, const char* tag) {
    for (int i = 0; i < 4; i++) {
        out.push_back(static_cast<unsigned char>(tag[i]));
    }
}

void push_u32(std::vector<unsigned char>& out, std::uint32_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

void push_u16(std::vector<unsigned char>& out, std::uint16_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

// Build a 16-bit PCM WAV in memory. `with_extra_chunk` inserts a LIST chunk before
// `data`, which is exactly the case that breaks readers assuming data starts at 44.
std::vector<unsigned char> make_wav(const std::vector<std::int16_t>& samples,
                                    std::uint16_t channels, std::uint32_t sample_rate,
                                    bool with_extra_chunk) {
    std::vector<unsigned char> body;

    push_tag(body, "fmt ");
    push_u32(body, 16);
    push_u16(body, 1);                                         // PCM
    push_u16(body, channels);
    push_u32(body, sample_rate);
    push_u32(body, sample_rate * channels * 2);                // byte rate
    push_u16(body, static_cast<std::uint16_t>(channels * 2));  // block align
    push_u16(body, 16);                                        // bits per sample

    if (with_extra_chunk) {
        push_tag(body, "LIST");
        push_u32(body, 4);
        push_tag(body, "INFO");
    }

    push_tag(body, "data");
    push_u32(body, static_cast<std::uint32_t>(samples.size() * 2));
    for (std::int16_t sample : samples) {
        push_u16(body, static_cast<std::uint16_t>(sample));
    }

    std::vector<unsigned char> file;
    push_tag(file, "RIFF");
    push_u32(file, static_cast<std::uint32_t>(body.size() + 4));
    push_tag(file, "WAVE");
    file.insert(file.end(), body.begin(), body.end());
    return file;
}

std::filesystem::path write_temp_wav(const std::vector<unsigned char>& bytes, const char* name) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;

    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    out.close();

    return path;
}

TEST(Wav, ReadsMonoPcm16) {
    const std::vector<std::int16_t> samples = {0, 16384, -16384, 32767, -32768};
    const auto path = write_temp_wav(make_wav(samples, 1, 44100, false), "audioviz_test_mono.wav");

    const auto result = audioviz::load_wav(path);
    ASSERT_TRUE(result.has_value()) << result.error();

    EXPECT_EQ(result->sample_rate, 44100);
    EXPECT_EQ(result->channels, 1);
    EXPECT_EQ(result->frame_count(), samples.size());

    EXPECT_NEAR(result->samples[0], 0.0, 1e-9);
    EXPECT_NEAR(result->samples[1], 0.5, 1e-4);
    EXPECT_NEAR(result->samples[2], -0.5, 1e-4);
    EXPECT_NEAR(result->samples[4], -1.0, 1e-9);

    std::filesystem::remove(path);
}

// The regression test that matters: data does not start at byte 44 here.
TEST(Wav, FindsDataChunkAfterOtherChunks) {
    const std::vector<std::int16_t> samples = {1000, 2000, 3000, 4000};
    const auto path = write_temp_wav(make_wav(samples, 1, 22050, true), "audioviz_test_extra.wav");

    const auto result = audioviz::load_wav(path);
    ASSERT_TRUE(result.has_value()) << result.error();

    EXPECT_EQ(result->sample_rate, 22050);
    EXPECT_EQ(result->frame_count(), samples.size());
    EXPECT_NEAR(result->samples[0], 1000.0 / 32768.0, 1e-9);

    std::filesystem::remove(path);
}

TEST(Wav, DownmixesStereoToMono) {
    // Interleaved L R: (1.0, 0.0) then (0.0, 1.0). Both average to 0.5.
    const std::vector<std::int16_t> samples = {32767, 0, 0, 32767};
    const auto path = write_temp_wav(make_wav(samples, 2, 44100, false), "audioviz_test_stereo.wav");

    const auto result = audioviz::load_wav(path);
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result->channels, 2);
    EXPECT_EQ(result->frame_count(), 2u);

    const std::vector<double> mono = audioviz::to_mono(*result);
    ASSERT_EQ(mono.size(), 2u);
    EXPECT_NEAR(mono[0], 0.5, 1e-4);
    EXPECT_NEAR(mono[1], 0.5, 1e-4);

    std::filesystem::remove(path);
}

TEST(Wav, RejectsNonWavFile) {
    const std::vector<unsigned char> junk = {'n', 'o', 'p', 'e'};
    const auto path = write_temp_wav(junk, "audioviz_test_junk.wav");

    const auto result = audioviz::load_wav(path);
    EXPECT_FALSE(result.has_value());

    std::filesystem::remove(path);
}

}  // namespace
