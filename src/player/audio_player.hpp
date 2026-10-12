#pragma once

#include <audioviz/audio.hpp>

#include <atomic>
#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace player {

// Plays one AudioBuffer through the default output device and owns the playhead.
//
// The device pulls audio on its own thread. The only state shared with the UI thread is
// two atomics (position and playing), so neither side ever waits on a lock.
class AudioPlayer {
public:
    // Opens the default output device. Starts paused, at the beginning.
    static std::expected<std::unique_ptr<AudioPlayer>, std::string> open(
        const audioviz::AudioBuffer& audio);

    ~AudioPlayer();
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    void play();  // from the end, starts over
    void pause();
    bool playing() const { return playing_.load(); }

    // Playhead and length in sample frames (one sample per channel).
    std::size_t position() const { return position_.load(); }
    std::size_t frame_count() const { return frame_count_; }
    int sample_rate() const { return sample_rate_; }

    // Move the playhead, in seconds. Clamped to the song.
    void seek_seconds(double seconds);
    double position_seconds() const;
    double duration_seconds() const;

    // The system audio API miniaudio picked: "Core Audio", "WASAPI", "PulseAudio", ...
    // "Null" means it found no usable output device and is discarding the audio.
    const char* backend_name() const;

    // Called on the audio thread: write `frames` frames of interleaved float samples.
    void fill(float* out, std::size_t frames);

private:
    AudioPlayer() = default;

    struct Device;  // the miniaudio device, kept out of this header
    std::unique_ptr<Device> device_;

    std::vector<float> samples_;  // interleaved, never modified after open()
    std::size_t channels_ = 0;
    std::size_t frame_count_ = 0;
    int sample_rate_ = 0;

    std::atomic<std::size_t> position_{0};
    std::atomic<bool> playing_{false};
};
}  // namespace player
