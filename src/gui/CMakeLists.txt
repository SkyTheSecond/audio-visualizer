add_executable(shazam_gui
    main.cpp
    spectrogram_image.cpp
    texture.cpp
)

target_link_libraries(shazam_gui PRIVATE shazam imgui glad)
target_compile_options(shazam_gui PRIVATE ${SHAZAM_WARNINGS})
