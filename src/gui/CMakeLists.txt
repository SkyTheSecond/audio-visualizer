add_executable(audioviz_gui
    main.cpp
    spectrogram_image.cpp
    texture.cpp
)

target_link_libraries(audioviz_gui PRIVATE audioviz imgui glad)
target_compile_options(audioviz_gui PRIVATE ${AUDIOVIZ_WARNINGS})
