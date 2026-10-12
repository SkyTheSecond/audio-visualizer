add_library(shazam STATIC
    shazam.cpp
    dft.cpp
    fft.cpp
    window.cpp
    audio.cpp
    stft.cpp
)

target_include_directories(shazam PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(shazam PRIVATE
    ${SHAZAM_STRICT_WARNINGS}
)
