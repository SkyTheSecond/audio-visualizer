add_executable(shazam_tests
    test_smoke.cpp
    test_dft.cpp
    test_fft.cpp
    test_window.cpp
    test_audio.cpp
)

target_link_libraries(shazam_tests PRIVATE shazam GTest::gtest_main)
target_compile_options(shazam_tests PRIVATE -Wall -Wextra)

include(GoogleTest)
gtest_discover_tests(shazam_tests)
