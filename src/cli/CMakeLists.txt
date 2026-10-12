add_executable(shazam_cli main.cpp)
target_link_libraries(shazam_cli PRIVATE shazam)
target_compile_options(shazam_cli PRIVATE ${SHAZAM_WARNINGS})

# MinGW's libstdc++ keeps std::print's Windows console support in a separate library.
if(MINGW)
    target_link_libraries(shazam_cli PRIVATE stdc++exp)
endif()
