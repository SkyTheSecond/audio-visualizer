# Hand-written build for miniaudio. Only miniaudio.h, miniaudio.c and LICENSE are vendored.
add_library(miniaudio STATIC miniaudio.c)

target_include_directories(miniaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Third-party code. Warnings in it are not ours to fix.
target_compile_options(miniaudio PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/w,-w>)

# miniaudio loads its backends (CoreAudio, WASAPI, ALSA, PulseAudio, ...) at runtime, so
# beyond threads it only needs libm and libdl on Unix-likes.
find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX)
    target_link_libraries(miniaudio PUBLIC m)
endif()
