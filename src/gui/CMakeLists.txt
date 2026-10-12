# Everything a windowed program in this project shares: window/GL/ImGui setup, textures,
# spectrogram images.
add_library(gui_common STATIC
    app.cpp
    spectrogram_image.cpp
    texture.cpp
)

target_include_directories(gui_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gui_common PUBLIC audioviz imgui glad)
target_compile_options(gui_common PRIVATE ${AUDIOVIZ_WARNINGS})

add_executable(audioviz_gui main.cpp)
target_link_libraries(audioviz_gui PRIVATE gui_common)
target_compile_options(audioviz_gui PRIVATE ${AUDIOVIZ_WARNINGS})
