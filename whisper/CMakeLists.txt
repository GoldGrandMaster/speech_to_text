cmake_minimum_required(VERSION 3.24)
project(whisper)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/Od)
else()
    add_compile_options(-O0)
endif()

set(TARGET ${PROJECT_NAME})
set(TEST_TARGET whisper_test)

add_subdirectory(audio_systems)
add_subdirectory(feature_extractor)

if(EXISTS "/benchmark")
  add_subdirectory(benchmark)
else()
  message("added global benchmark")
endif()

add_subdirectory(ctranslate2)

add_executable(${TEST_TARGET} "main.cpp")
add_library(${TARGET} STATIC "whisper_fast.cpp" "whisper_fast.hpp" "whisper_stream.cpp" "whisper_stream.hpp" )

target_link_libraries(${TEST_TARGET} ${TARGET})
target_link_libraries(${TARGET} audio_systems feature_extractor benchmark ctranslate2)
target_include_directories(${TARGET} PRIVATE ${AudioSystems_SOURCE_DIR})

