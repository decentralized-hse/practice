cmake_minimum_required(VERSION 3.27)

project(put-osetrov)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C++ Standart: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

include(third_party/Utf8Range.cmake)

find_package(Protobuf REQUIRED)

add_executable(fuzz
    fuzz.cpp ../protobuf-golub-osetrov.cpp ../utils.cpp ../protobuf-golub-osetrov.pb.cc)

target_include_directories(fuzz PRIVATE ${utf8_range_SOURCE_DIR})
target_link_libraries(fuzz ${PROTOBUF_LIBRARY} utf8_validity)

target_compile_options(fuzz PRIVATE -fsanitize=fuzzer,address -DFUZZING_ENABLED)
target_link_options(fuzz PRIVATE -fsanitize=fuzzer,address)
