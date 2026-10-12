#include "app.hpp"
#include "spectrogram_image.hpp"
#include "texture.hpp"

#include <audioviz/audio.hpp>
#include <audioviz/stft.hpp>

#include <imgui.h>

#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <file.wav>\n", argv[0]);
        return 1;
    }

    // --- load and analyse before touching graphics -------------------------
    // A bad file should fail fast, without ever opening a window.
    const auto audio = audioviz::load_wav(argv[1]);
    if (!audio) {
        std::fprintf(stderr, "error: %s\n", audio.error().c_str());
        return 1;
    }

    const std::vector<double> mono = audioviz::to_mono(*audio);
    const audioviz::Spectrogram spectrogram =
        audioviz::compute_spectrogram(mono, audio->sample_rate);

    // Pure CPU work: magnitudes to coloured pixels. No GL involved yet.
    const gui::Image image = gui::spectrogram_to_image(spectrogram);

    // --- window, OpenGL and ImGui ------------------------------------------
    gui::App app;
    if (!app.init("audioviz - spectrogram")) {
        return 1;
    }

    // Declared after `app`, so it is destroyed first, while the GL context still exists.
    gui::Texture texture;
    if (image.width > 0) {
        auto created = gui::Texture::create(image.width, image.height, image.rgba);
        if (created) {
            texture = std::move(*created);
        } else {
            std::fprintf(stderr, "texture: %s\n", created.error().c_str());
        }
    }

    float zoom = 1.0f;
    bool show_demo = false;

    // --- main loop ---------------------------------------------------------
    while (!app.should_close()) {
        app.begin_frame();

        ImGui::Begin("Audio");
        ImGui::Text("file        : %s", argv[1]);
        ImGui::Text("sample rate : %d Hz", audio->sample_rate);
        ImGui::Text("channels    : %d", audio->channels);
        ImGui::Text("duration    : %.2f s", audio->duration_seconds());
        ImGui::Separator();
        ImGui::Text("frames      : %zu", spectrogram.frame_count);
        ImGui::Text("bins        : %zu", spectrogram.bin_count);
        ImGui::Text("bin width   : %.2f Hz", spectrogram.bin_frequency(1));
        ImGui::Text("frame step  : %.1f ms", spectrogram.frame_time(1) * 1000.0);
        ImGui::Separator();
        ImGui::SliderFloat("zoom", &zoom, 0.1f, 4.0f);
        ImGui::Checkbox("ImGui demo window", &show_demo);
        ImGui::Separator();
        ImGui::TextWrapped("%s", app.gl_info().c_str());
        ImGui::End();

        ImGui::Begin("Spectrogram");
        if (texture) {
            const ImVec2 size(static_cast<float>(texture.width()) * zoom,
                              static_cast<float>(texture.height()) * zoom);

            // ImTextureID is a pointer in older ImGui and an integer in newer ones.
            // This C-style cast is correct for either.
            ImGui::Image((ImTextureID)(std::uintptr_t)texture.id(), size);
        } else {
            ImGui::TextUnformatted("no texture; see the terminal for the reason");
        }
        ImGui::End();

        if (show_demo) {
            ImGui::ShowDemoWindow(&show_demo);
        }

        app.end_frame();
    }

    return 0;
}  // texture is destroyed here, then app: GL objects before the context
