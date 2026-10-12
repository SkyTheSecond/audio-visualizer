#include "app.hpp"

// glad must come before GLFW. GLFW_INCLUDE_NONE (set in CMake) already stops GLFW from
// pulling in another GL header, but this order is the safe habit.
#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>
#include <cstdio>
#include <format>

namespace gui {
namespace {

struct GlVersion {
    int major;
    int minor;
};

// Core-profile versions to try, newest first. 4.6 is the target; 4.5 still has Direct State
// Access and 4.3 still has debug output (both have fallbacks). 4.1 is the macOS ceiling, and
// 3.3 is the oldest that ImGui's OpenGL3 backend is happy with.
constexpr std::array<GlVersion, 5> gl_versions{{{4, 6}, {4, 5}, {4, 3}, {4, 1}, {3, 3}}};

void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

// Create the window on the newest core context the driver offers, or nullptr.
GLFWwindow* create_window(const char* title, int width, int height) {
    // Unavailable versions are expected while probing; keep only the last complaint.
    static std::string last_error;
    glfwSetErrorCallback([](int, const char* description) { last_error = description; });

    GLFWwindow* window = nullptr;
    for (const GlVersion version : gl_versions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
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

bool App::init(const char* title, int width, int height) {
    glfwSetErrorCallback(on_glfw_error);
    if (!glfwInit()) {
        std::fprintf(stderr, "failed to initialise GLFW\n");
        return false;
    }
    glfw_ready_ = true;

    // Window hints must be set BEFORE the window is created. create_window() adds the version.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Required by macOS for any 3.2+ core context; harmless elsewhere, since core already
    // drops everything forward-compatible mode removes.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    window_ = create_window(title, width, height);
    if (window_ == nullptr) {
        return false;
    }

    // The context must be current on this thread before ANY GL call, including gladLoadGL.
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // vsync: cap the loop at the monitor refresh rate

    // Fill in every OpenGL function pointer, using GLFW as the lookup function.
    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::fprintf(stderr, "failed to load OpenGL functions\n");
        return false;
    }

    std::printf("glad loaded : OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version),
                GLAD_VERSION_MINOR(version));
    std::printf("GL_VERSION  : %s\n", gl_string(GL_VERSION));
    std::printf("GL_RENDERER : %s\n", gl_string(GL_RENDERER));
    gl_info_ = std::format("{}\n{}", gl_string(GL_VERSION), gl_string(GL_RENDERER));

    enable_gl_debug_output();

    IMGUI_CHECKVERSION();  // catches header/source version mismatches
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // The second argument installs ImGui's GLFW callbacks (mouse, keyboard, scroll).
    ImGui_ImplGlfw_InitForOpenGL(window_, true);

    // ImGui compiles its own shaders, so it needs the GLSL version matching the context we
    // actually got (from 3.3 on, GLSL 4.1 is "410", 3.3 is "330", and so on).
    const int gl_major = glfwGetWindowAttrib(window_, GLFW_CONTEXT_VERSION_MAJOR);
    const int gl_minor = glfwGetWindowAttrib(window_, GLFW_CONTEXT_VERSION_MINOR);
    const std::string glsl_version = std::format("#version {}{}0 core", gl_major, gl_minor);
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    imgui_ready_ = true;

    return true;
}

App::~App() {
    // Reverse order of init(), skipping whatever init() never got to.
    if (imgui_ready_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    if (window_ != nullptr) glfwDestroyWindow(window_);
    if (glfw_ready_) glfwTerminate();
}

bool App::should_close() const { return glfwWindowShouldClose(window_) != 0; }

void App::begin_frame() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void App::end_frame() {
    ImGui::Render();

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}
}  // namespace gui
