#include "spectrogram_image.hpp"
#include "texture.hpp"

#include <audioviz/audio.hpp>
#include <audioviz/stft.hpp>

// glad must come before GLFW. GLFW_INCLUDE_NONE (set in CMake) already stops GLFW from
// pulling in another GL header, but this order is the safe habit.
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace {

struct GlVersion {
    int major;
    int minor;
};

// Core-profile versions to try, newest first. 4.6 is the target; 4.5 still has Direct State
// Access and 4.3 still has debug output (both have fallbacks below). 4.1 is the macOS
// ceiling, and 3.3 is the oldest that ImGui's OpenGL3 backend is happy with.
constexpr std::array<GlVersion, 5> kGlVersions{{{4, 6}, {4, 5}, {4, 3}, {4, 1}, {3, 3}}};

void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

// Create the window on the newest core context the driver offers, or nullptr.
GLFWwindow* create_window() {
    // Unavailable versions are expected while probing; keep only the last complaint.
    static std::string last_error;
    glfwSetErrorCallback([](int, const char* description) { last_error = description; });

    GLFWwindow* window = nullptr;
    for (const GlVersion version : kGlVersions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
        window = glfwCreateWindow(1280, 720, "audio visualizer - spectrogram", nullptr, nullptr);
        if (window != nullptr) break;
    }

    glfwSetErrorCallback(on_glfw_error);
    if (window == nullptr) {
        std::fprintf(stderr, "no OpenGL 3.3+ core context available: %s\n", last_error.c_str());
    }
    return window;
}

// glGetString returns unsigned bytes. Wrap it so callers get a normal string.
const char* gl_string(GLenum name) {
    const GLubyte* text = glGetString(name);
    return text != nullptr ? reinterpret_cast<const char*>(text) : "unknown";
}

// Called by the driver whenever it detects API misuse.
void APIENTRY on_gl_debug(GLenum /*source*/, GLenum /*type*/, GLuint /*id*/, GLenum severity,
                          GLsizei /*length*/, const GLchar* message, const void* /*user*/) {
    const char* label = "low";
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        label = "HIGH";
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        label = "MEDIUM";
    }
    std::fprintf(stderr, "[GL %s] %s\n", label, message);
}

// Turn on debug output, if the context we got is a debug context that supports it.
void enable_gl_debug_output() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) == 0) {
        return;  // release build, or a platform that ignores the debug hint (macOS)
    }
    if (!GLAD_GL_VERSION_4_3 && !GLAD_GL_KHR_debug) {
        std::fprintf(stderr, "note: GL debug output is not supported by this driver\n");
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_debug, nullptr);

    // Drivers chatter about harmless things at "notification" severity. Silence those.
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                          GL_FALSE);
}

}  // namespace

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
    const audioviz::Spectrogram spectrogram = audioviz::compute_spectrogram(mono, audio->sample_rate);

    // Pure CPU work: magnitudes to coloured pixels. No GL involved yet.
    const gui::Image image = gui::spectrogram_to_image(spectrogram);

    // --- window and OpenGL context -----------------------------------------
    glfwSetErrorCallback(on_glfw_error);
    if (!glfwInit()) {
        std::fprintf(stderr, "failed to initialise GLFW\n");
        return 1;
    }

    // Window hints must be set BEFORE the window is created. create_window() adds the version.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Required by macOS for any 3.2+ core context; harmless elsewhere, since core already
    // drops everything forward-compatible mode removes.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWwindow* window = create_window();
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    // The context must be current on this thread before ANY GL call, including gladLoadGL.
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync: cap the loop at the monitor refresh rate

    // Fill in every OpenGL function pointer, using GLFW as the lookup function.
    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::fprintf(stderr, "failed to load OpenGL functions\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("glad loaded : OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version),
                GLAD_VERSION_MINOR(version));
    std::printf("GL_VERSION  : %s\n", gl_string(GL_VERSION));
    std::printf("GL_RENDERER : %s\n", gl_string(GL_RENDERER));

    enable_gl_debug_output();

    // --- ImGui -------------------------------------------------------------
    IMGUI_CHECKVERSION();  // catches header/source version mismatches
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // The second argument installs ImGui's GLFW callbacks (mouse, keyboard, scroll).
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    // ImGui compiles its own shaders, so it needs the GLSL version matching the context we
    // actually got (from 3.3 on, GLSL 4.1 is "410", 3.3 is "330", and so on).
    const int gl_major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    const int gl_minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    const std::string glsl_version = std::format("#version {}{}0 core", gl_major, gl_minor);
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());

    const std::string gl_info = std::format("{}\n{}", gl_string(GL_VERSION), gl_string(GL_RENDERER));

    {
        // Everything that owns OpenGL objects lives inside this block, so the closing
        // brace destroys them while the context still exists.
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

        // --- main loop -----------------------------------------------------
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

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
            ImGui::TextWrapped("%s", gl_info.c_str());
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

            ImGui::Render();

            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    }  // <- texture is destroyed here, while the context is still alive

    // --- shutdown, in reverse order of setup -------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
