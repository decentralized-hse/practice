cmake_minimum_required(VERSION 3.16)

project(07-crdt-cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(third_party)

add_subdirectory(libs)

# TODO: add other crdt-types

add_subdirectory(tests)
