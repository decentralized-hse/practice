cmake_minimum_required(VERSION 3.16)

project(07-crdt-cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party)

add_subdirectory(libs)

# TODO: add other crdt-types

add_subdirectory(tests)
