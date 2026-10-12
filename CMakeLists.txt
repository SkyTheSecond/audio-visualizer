cmake_minimum_required(VERSION 3.25)
project(audio_visualizer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AUDIOVIZ_BUILD_GUI "Build the OpenGL spectrogram viewer (needs a windowing system)" ON)
option(AUDIOVIZ_WARNINGS_AS_ERRORS "Fail the build on any compiler warning in our own code (CI turns this on)" OFF)

# A compiler can accept -std=c++23 while its standard library still lacks pieces we use.
# The classic case: Clang 18 with GCC's libstdc++ (Ubuntu 24.04's default `clang`) has no
# std::expected, and GCC 13 has no <print>. Fail here, clearly, rather than mid-build.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles([[
    #include <expected>
    #include <format>
    #include <print>
    int main() {
        std::expected<int, int> value{1};
        return std::format("{}", *value).size() == 1 ? 0 : 1;
    }
]] AUDIOVIZ_HAS_CXX23_LIBRARY)
if(NOT AUDIOVIZ_HAS_CXX23_LIBRARY)
    message(FATAL_ERROR
        "This compiler's standard library is missing C++23 features the project needs "
        "(std::expected, <format>, <print>). Use GCC 14+, Clang 19+ (or Clang 18 with "
        "-stdlib=libc++), a current Xcode, or Visual Studio 2022 17.7+. "
        "Delete the build directory after switching compilers.")
endif()

# Warning flags are spelled differently by MSVC (and clang-cl) than by GCC/Clang.
if(MSVC)
    set(AUDIOVIZ_WARNINGS /W4 /permissive-)
    set(AUDIOVIZ_STRICT_WARNINGS ${AUDIOVIZ_WARNINGS})
    add_compile_options(/utf-8)
else()
    set(AUDIOVIZ_WARNINGS -Wall -Wextra)
    set(AUDIOVIZ_STRICT_WARNINGS ${AUDIOVIZ_WARNINGS} -Wpedantic)
endif()
if(AUDIOVIZ_WARNINGS_AS_ERRORS)
    # Only our targets use these lists; vendored code keeps its own warning settings.
    if(MSVC)
        list(APPEND AUDIOVIZ_WARNINGS /WX)
        list(APPEND AUDIOVIZ_STRICT_WARNINGS /WX)
    else()
        list(APPEND AUDIOVIZ_WARNINGS -Werror)
        list(APPEND AUDIOVIZ_STRICT_WARNINGS -Werror)
    endif()
endif()

# Adding google test
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
# MSVC: link gtest against the same DLL runtime (/MD) as everything else.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest)

if(AUDIOVIZ_BUILD_GUI)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw)

    # GLAD supplies every OpenGL declaration. Without this GLFW's header would also pull in
    # the system GL header and the two clash. INTERFACE: applies to whoever links glfw.
    target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE)

    add_subdirectory(third_party/glad)
    add_subdirectory(third_party/imgui)
endif()

# Our own stuff
add_subdirectory(src/lib)
add_subdirectory(src/cli)
if(AUDIOVIZ_BUILD_GUI)
    add_subdirectory(src/gui)
endif()

enable_testing()
add_subdirectory(tests)
