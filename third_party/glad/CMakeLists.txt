add_library(glad STATIC src/gl.c)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Generated code. Warnings in it are not ours to fix.
target_compile_options(glad PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/w,-w>)
