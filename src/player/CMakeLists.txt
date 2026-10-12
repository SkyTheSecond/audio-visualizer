add_executable(audioviz_player
    main.cpp
    audio_player.cpp
    bars.cpp
)

target_link_libraries(audioviz_player PRIVATE gui_common miniaudio)
target_compile_options(audioviz_player PRIVATE ${AUDIOVIZ_WARNINGS})
