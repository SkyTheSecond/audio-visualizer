add_executable(shazam_cli main.cpp)
target_link_libraries(shazam_cli PRIVATE shazam)
target_compile_options(shazam_cli PRIVATE -Wall -Wextra)
