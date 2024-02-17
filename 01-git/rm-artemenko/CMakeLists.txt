cmake_minimum_required(VERSION 3.16)

project(rm)

set(BIN_NAME ${PROJECT_NAME})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party)

add_executable(${BIN_NAME} main.cpp)

get_filename_component(BIN_INCLUDE_PATH "." ABSOLUTE)
target_include_directories(${BIN_NAME} PRIVATE ${BIN_INCLUDE_PATH})

target_link_libraries(${BIN_NAME} PRIVATE picosha2)
target_link_libraries(${BIN_NAME} PRIVATE argparse)
target_link_libraries(${BIN_NAME} PRIVATE fmt)
