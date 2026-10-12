#include "app.hpp"
#include "audio_player.hpp"
#include "bars.hpp"
#include "scrub.hpp"

#include <audioviz/audio.hpp>
#include <audioviz/bands.hpp>
#include <audioviz/spectrum.hpp>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <vector>

namespace {
constexpr std::size_t frame_size = 2048;  // at 44.1 kHz: 46 ms of audio, 21.5 Hz per bin
constexpr std::size_t bar_count = 64;
constexpr double lowest_hz = 40.0;
constexpr double highest_hz = 16000.0;
constexpr double range_db = 60.0;           // how far below full scale a bar reaches zero
constexpr double tilt_db_per_octave = 3.0;  // lift the treble so it isn't dwarfed by bass

std::string format_time(double seconds) {
    const int whole = static_cast<int>(seconds);
    return std::format("{}:{:02}", whole / 60, whole % 60);
}

// Copy the frame.size() samples centred on `centre` into `frame`, silence past either end.
// Centred, not ending at the playhead: the speakers lag the playhead by a few milliseconds,
// and looking half a frame ahead roughly cancels that out.
void gather_frame(std::span<const double> mono, std::size_t centre, std::span<double> frame) {
    const auto start =
        static_cast<std::ptrdiff_t>(centre) - static_cast<std::ptrdiff_t>(frame.size() / 2);
    for (std::size_t i = 0; i < frame.size(); i++) {
        const std::ptrdiff_t index = start + static_cast<std::ptrdiff_t>(i);
        const bool inside = index >= 0 && index < static_cast<std::ptrdiff_t>(mono.size());
        frame[i] = inside ? mono[static_cast<std::size_t>(index)] : 0.0;
    }
}

void toggle(player::AudioPlayer& audio) {
    if (audio.playing()) {
        audio.pause();
    } else {
        audio.play();
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <file.wav>\n", argv[0]);
        return 1;
    }

    // --- load the song and open the audio device, before any window ------
    const auto audio = audioviz::load_wav(argv[1]);
    if (!audio) {
        std::fprintf(stderr, "error: %s\n", audio.error().c_str());
        return 1;
    }

    const std::vector<double> mono = audioviz::to_mono(*audio);

    // Stay safely below Nyquist for low sample-rate files (11025 Hz tops out at 5512 Hz).
    const double high_hz = std::min(highest_hz, 0.95 * audio->sample_rate / 2.0);
    const std::vector<audioviz::Band> bands =
        audioviz::make_log_bands(bar_count, lowest_hz, high_hz, audio->sample_rate, frame_size);

    auto opened = player::AudioPlayer::open(*audio);
    if (!opened) {
        std::fprintf(stderr, "audio: %s\n", opened.error().c_str());
        return 1;
    }
    player::AudioPlayer& song = **opened;
    std::printf("audio out   : %s\n", song.backend_name());

    // --- window ------------------------------------------------------------
    gui::App app;
    if (!app.init("audioviz - player", 1100, 600)) {
        return 1;
    }
    ImGui::GetIO().IniFilename = nullptr;  // one fixed full-window layout: nothing to save

    audioviz::SpectrumAnalyzer analyzer(frame_size, audioviz::WindowType::Hann);
    std::vector<double> frame(frame_size);
    std::vector<double> loudness(bands.size());
    std::vector<double> targets(bands.size());
    player::BarMeter meter(bands.size());
    player::HoldToScrub back(-1.0);
    player::HoldToScrub ahead(+1.0);

    // A full-scale sine sitting on a bin peaks at N/4 through a Hann window (see the test).
    const double reference = static_cast<double>(frame_size) / 4.0;
    const std::string title = std::filesystem::path(argv[1]).filename().string();

    song.play();

    while (!app.should_close()) {
        app.begin_frame();
        const double dt = ImGui::GetIO().DeltaTime;

        // --- what is sounding right now? -----------------------------------
        gather_frame(mono, song.position(), frame);
        audioviz::band_peaks(analyzer.analyze(frame), bands, loudness);
        for (std::size_t i = 0; i < bands.size(); i++) {
            const double centre_hz = std::sqrt(bands[i].low_hz * bands[i].high_hz);
            targets[i] =
                player::bar_target(loudness[i], reference, centre_hz, range_db, tilt_db_per_octave);
        }
        meter.update(targets, dt);

        // --- one ImGui window filling the whole OS window ------------------
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("player", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        ImGui::TextUnformatted(title.c_str());

        // Bars take everything except the two transport rows at the bottom.
        const float transport_height = 2.0f * ImGui::GetFrameHeightWithSpacing();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 bars_size(available.x, std::max(0.0f, available.y - transport_height));
        player::draw_bars(meter, origin, bars_size);
        ImGui::Dummy(bars_size);  // claim the space, so the transport lands below the bars

        // --- transport -----------------------------------------------------
        // The seek bar. Dragging it scrubs, too.
        float position = static_cast<float>(song.position_seconds());
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##seek", &position, 0.0f,
                               static_cast<float>(song.duration_seconds()), "")) {
            song.seek_seconds(position);
        }

        ImGui::Button("<< 5s");
        const bool back_held = ImGui::IsItemActive();
        ImGui::SameLine();
        // "###play" makes the ID just "play", so it stays the same whichever label shows.
        if (ImGui::Button(song.playing() ? "Pause###play" : "Play###play", ImVec2(80.0f, 0.0f))) {
            toggle(song);
        }
        ImGui::SameLine();
        ImGui::Button("5s >>");
        const bool ahead_held = ImGui::IsItemActive();
        ImGui::SameLine();
        ImGui::Text("%s / %s", format_time(song.position_seconds()).c_str(),
                    format_time(song.duration_seconds()).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("   space: play/pause   arrows or buttons: tap 5 s, hold to scrub");

        ImGui::End();

        // Keyboard and buttons drive the same two scrubbers.
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            toggle(song);
        }
        const double seek = back.update(back_held || ImGui::IsKeyDown(ImGuiKey_LeftArrow), dt) +
                            ahead.update(ahead_held || ImGui::IsKeyDown(ImGuiKey_RightArrow), dt);
        if (seek != 0.0) {
            song.seek_seconds(song.position_seconds() + seek);
        }

        app.end_frame();
    }

    return 0;
}
