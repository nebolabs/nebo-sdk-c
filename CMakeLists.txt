cmake_minimum_required(VERSION 3.16)
project(nebo-sdk-c C)

set(CMAKE_C_STANDARD 11)

# SDK library (core + schema builder)
add_library(nebo-sdk
    src/nebo.c
    src/schema.c
)

target_include_directories(nebo-sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Calculator example
add_executable(calculator examples/calculator/main.c)
target_link_libraries(calculator nebo-sdk)
