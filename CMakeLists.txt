cmake_minimum_required(VERSION 3.25)
project(shazam LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Adding google test
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest)

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

# Our own stuff
add_subdirectory(src/lib)
add_subdirectory(src/cli)
add_subdirectory(src/gui)

enable_testing()
add_subdirectory(tests)
