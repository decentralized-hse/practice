cmake_minimum_required(VERSION 3.10)

project(DirectoryCreator VERSION 1.0 DESCRIPTION "[01-git][mkdir]")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LLVM_ENABLE_PROJECTS clang;lldb)
set(LLVM_ENABLE_RUNTIMES libcxx;libcxxabi;libunwind)
set(LLDB_EXPORT_ALL_SYMBOLS 1)

add_executable(mkdir mkdir.cpp)
find_package(OpenSSL REQUIRED)

target_link_libraries(mkdir OpenSSL::Crypto)
