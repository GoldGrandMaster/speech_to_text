cmake_minimum_required(VERSION 3.24)
project(benchmark)

if (MSVC)
    add_compile_options(/Od)
else()
    add_compile_options(-O0)
endif()

set(TARGET ${PROJECT_NAME})
set(TEST_TARGET benchmark_test)

add_executable(${TEST_TARGET} "main.cpp")
add_library(${TARGET} STATIC "instrumentor.cpp" "instrumentor.hpp")

target_include_directories(${TARGET} PUBLIC .)

target_link_libraries(${TEST_TARGET} ${TARGET})
