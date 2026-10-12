add_executable(audioviz_cli main.cpp)
target_link_libraries(audioviz_cli PRIVATE audioviz)
target_compile_options(audioviz_cli PRIVATE ${AUDIOVIZ_WARNINGS})

# MinGW's libstdc++ keeps std::print's Windows console support in a separate library.
if(MINGW)
    target_link_libraries(audioviz_cli PRIVATE stdc++exp)
endif()
