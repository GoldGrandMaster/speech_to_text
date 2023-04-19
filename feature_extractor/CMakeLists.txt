cmake_minimum_required(VERSION 3.24)
project(feature_extractor)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")

if (WIN32 OR MSVC)
    set(CMAKE_PREFIX_PATH "C:/dev/vcpkg/installed/x64-windows/share")
endif ()

if (MSVC)
    add_compile_options(/Od)
else()
    add_compile_options(-O0)
endif()

set(TARGET ${PROJECT_NAME})
set(TEST_TARGET feature_extractor_test)

if(EXISTS "/benchmark")
  add_subdirectory(benchmark)
else()
  message("added global benchmark")
endif()


find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(${TEST_TARGET} "main.cpp")
add_library(${TARGET} STATIC "feature_extractor.cpp" "feature_extractor.hpp" "tokenizer.cpp" "tokenizer.hpp")

target_include_directories(${TARGET} PUBLIC .)

target_link_libraries(${TARGET} PRIVATE ${CMAKE_THREAD_LIBS_INIT} benchmark FFTW3::fftw3 FFTW3::fftw3f FFTW3::fftw3l nlohmann_json::nlohmann_json)

target_link_libraries(${TEST_TARGET} ${TARGET} nlohmann_json::nlohmann_json)


