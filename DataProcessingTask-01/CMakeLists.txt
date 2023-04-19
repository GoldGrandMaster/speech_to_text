# CMakeList.txt : CMake project for data_processing_task, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("data_processing_task")

add_subdirectory(ctranslate2)
# Add source to this project's executable.
add_executable (data_processing_task "DataProcessingTask.cpp" "DataProcessingTask.h")
target_link_libraries (data_processing_task PRIVATE ctranslate2)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET data_processing_task PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
