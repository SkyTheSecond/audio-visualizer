#include "audio_player.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <cmath>
#include <format>

namespace player {

struct AudioPlayer::Device {
    ma_device device;
};

namespace {
// miniaudio calls this on its own thread whenever the device wants more audio.
void data_callback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frames) {
    static_cast<AudioPlayer*>(device->pUserData)->fill(static_cast<float*>(output), frames);
}
}  // namespace

std::expected<std::unique_ptr<AudioPlayer>, std::string> AudioPlayer::open(
    const audioviz::AudioBuffer& audio) {
    if (audio.channels <= 0 || audio.sample_rate <= 0 || audio.samples.empty()) {
        return std::unexpected("nothing to play");
    }

    // The device keeps a pointer to the player, so the player must never move: hence the
    // private constructor and the unique_ptr.
    std::unique_ptr<AudioPlayer> self(new AudioPlayer());
    self->samples_.assign(audio.samples.begin(), audio.samples.end());  // double -> float
    self->channels_ = static_cast<std::size_t>(audio.channels);
    self->frame_count_ = audio.frame_count();
    self->sample_rate_ = audio.sample_rate;

    // We describe our data; miniaudio converts it to whatever the hardware runs at.
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = static_cast<ma_uint32>(audio.channels);
    config.sampleRate = static_cast<ma_uint32>(audio.sample_rate);
    config.dataCallback = data_callback;
    config.pUserData = self.get();

    self->device_ = std::make_unique<Device>();
    if (ma_device_init(nullptr, &config, &self->device_->device) != MA_SUCCESS) {
        self->device_.reset();
        return std::unexpected("could not open an audio output device");
    }

    // The device runs for the player's whole life. Pausing just means writing silence,
    // which is instant, unlike stopping and restarting the hardware.
    if (ma_device_start(&self->device_->device) != MA_SUCCESS) {
        return std::unexpected("could not start the audio output device");
    }
    return self;
}

AudioPlayer::~AudioPlayer() {
    // Stops the audio thread before samples_ goes away.
    if (device_) ma_device_uninit(&device_->device);
}

void AudioPlayer::play() {
    if (position_.load() >= frame_count_) position_.store(0);
    playing_.store(true);
}

void AudioPlayer::pause() { playing_.store(false); }

void AudioPlayer::seek_seconds(double seconds) {
    const double frame = std::clamp(seconds * sample_rate_, 0.0, static_cast<double>(frame_count_));
    position_.store(static_cast<std::size_t>(frame));
}

double AudioPlayer::position_seconds() const {
    return static_cast<double>(position_.load()) / sample_rate_;
}

double AudioPlayer::duration_seconds() const {
    return static_cast<double>(frame_count_) / sample_rate_;
}

const char* AudioPlayer::backend_name() const {
    return ma_get_backend_name(device_->device.pContext->backend);
}

void AudioPlayer::fill(float* out, std::size_t frames) {
    // Real-time rules apply here: no locks, no allocation, no printing. Just copy.
    std::size_t start = position_.load();
    std::size_t copied = 0;

    if (playing_.load() && start < frame_count_) {
        copied = std::min(frames, frame_count_ - start);
        const float* from = samples_.data() + start * channels_;
        std::copy(from, from + copied * channels_, out);

        // Advance, unless the UI seeked while we were copying. Then its position wins and
        // the few milliseconds we just wrote are simply the last of the old position.
        if (position_.compare_exchange_strong(start, start + copied) &&
            start + copied >= frame_count_) {
            playing_.store(false);  // reached the end
        }
    }

    std::fill(out + copied * channels_, out + frames * channels_, 0.0f);
}
}  // namespace player
