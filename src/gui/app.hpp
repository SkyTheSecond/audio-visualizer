#pragma once

#include <string>

struct GLFWwindow;

namespace gui {

// One OS window with an OpenGL core context and Dear ImGui on top: the setup every program
// in this project shares. GLFW and ImGui are global, so only one App may exist at a time.
//
// Anything owning OpenGL objects (a Texture, say) must be declared AFTER the App, so C++
// destroys it first, while the context still exists.
class App {
public:
    App() = default;
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Open the window on the newest OpenGL core context available and set up ImGui.
    // Prints the reason and returns false on failure.
    bool init(const char* title, int width = 1280, int height = 720);

    // True once the user has asked to close the window.
    bool should_close() const;

    // Poll input and start an ImGui frame. Build the UI between this and end_frame().
    void begin_frame();

    // Clear the background, draw ImGui on top, and present the frame.
    void end_frame();

    // "GL_VERSION\nGL_RENDERER", for showing in the UI.
    const std::string& gl_info() const { return gl_info_; }

private:
    GLFWwindow* window_ = nullptr;
    bool glfw_ready_ = false;
    bool imgui_ready_ = false;
    std::string gl_info_;
};
}  // namespace gui
