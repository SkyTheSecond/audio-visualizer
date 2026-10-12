#include <audioviz/audio.hpp>
#include <audioviz/stft.hpp>

#include <print>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println(stderr, "usage: {} <file.wav> [out.pgm]", argv[0]);
        return 1;
    }

    const auto audio = audioviz::load_wav(argv[1]);
    if (!audio) {
        std::println(stderr, "error: {}", audio.error());
        return 1;
    }

    std::println("sample rate : {} Hz", audio->sample_rate);
    std::println("channels    : {}", audio->channels);
    std::println("frames      : {}", audio->frame_count());
    std::println("duration    : {:.2f} s", audio->duration_seconds());

    const std::vector<double> mono = audioviz::to_mono(*audio);
    const audioviz::Spectrogram spectrogram =
        audioviz::compute_spectrogram(mono, audio->sample_rate);

    std::println("");
    std::println("spectrogram : {} frames x {} bins", spectrogram.frame_count,
                 spectrogram.bin_count);
    std::println("bin width   : {:.2f} Hz", spectrogram.bin_frequency(1));
    std::println("frame step  : {:.1f} ms", spectrogram.frame_time(1) * 1000.0);

    const std::string output = argc >= 3 ? argv[2] : "spectrogram.pgm";
    if (!audioviz::write_pgm(spectrogram, output)) {
        std::println(stderr, "failed to write {}", output);
        return 1;
    }
    std::println("wrote       : {}", output);

    return 0;
}
