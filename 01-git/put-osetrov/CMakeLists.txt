cmake_minimum_required(VERSION 3.27)

project(put-osetrov)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C++ Standart: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

add_subdirectory(lib)

add_executable(put-osetrov main.cpp)

target_include_directories(
    put-osetrov
    PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/lib")

target_link_libraries(put-osetrov PRIVATE lib)
