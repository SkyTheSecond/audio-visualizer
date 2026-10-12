# Hand-written build for Dear ImGui, which ships no CMake of its own.
add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

# The OpenGL3 backend dlopen()s libGL on Linux; older glibc keeps dlopen in libdl.
target_link_libraries(imgui PUBLIC glfw ${CMAKE_DL_LIBS})
