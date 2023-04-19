cmake_minimum_required(VERSION 3.24)
project(audio_systems)

set(CMAKE_CXX_STANDARD 17)

if (WIN32 OR MSVC)
    set(CMAKE_PREFIX_PATH "C:/dev/vcpkg/installed/x64-windows/share")
endif ()

if (MSVC)
    add_compile_options(/Od)
else()
    add_compile_options(-O0)
endif()

set(TARGET ${PROJECT_NAME})
set(TEST_TARGET audio_systems_test)

find_package(FFMPEG REQUIRED COMPONENTS avcodec avformat avutil) # TODO: use avresample 
find_package(SDL2 CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

add_executable(${TEST_TARGET} "main.cpp")
add_library(${TARGET} STATIC "audio_async.cpp" "audio_async.hpp" "audio_decoder.cpp" "audio_decoder.hpp" )

target_include_directories(${TARGET} PUBLIC .)

target_link_libraries(${TARGET} ${CMAKE_THREAD_LIBS_INIT} $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static> SndFile::sndfile)

target_include_directories(${TARGET} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${TARGET} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${TARGET} ${FFMPEG_LIBRARIES})

target_link_libraries(${TEST_TARGET} ${TARGET})

