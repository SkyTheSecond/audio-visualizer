add_executable(shazam_tests
    test_smoke.cpp
    test_dft.cpp
    test_fft.cpp
    test_window.cpp
    test_audio.cpp
    test_stft.cpp
)

target_link_libraries(shazam_tests PRIVATE shazam GTest::gtest_main)
target_compile_options(shazam_tests PRIVATE ${SHAZAM_WARNINGS})

include(GoogleTest)
# List the tests when ctest runs rather than right after linking, so building never has to
# execute the test binary (which fails when cross-compiling).
gtest_discover_tests(shazam_tests DISCOVERY_MODE PRE_TEST)
