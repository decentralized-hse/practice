cmake_minimum_required(VERSION 3.10)
project(git_grep)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-g -Wall -Wextra -Wpedantic)

set(BUILD_SHARED_LIBS OFF)
set(USE_HTTPS openssl)

add_subdirectory(thirdparty/libgit2)

add_executable(git_grep git_interface.cpp)
target_link_libraries(git_grep PRIVATE libgit2)
target_include_directories(git_grep PRIVATE thirdparty/libgit2/include)
