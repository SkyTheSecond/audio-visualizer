add_library(audioviz STATIC
    audioviz.cpp
    dft.cpp
    fft.cpp
    window.cpp
    audio.cpp
    stft.cpp
)

target_include_directories(audioviz PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(audioviz PRIVATE
    ${AUDIOVIZ_STRICT_WARNINGS}
)
