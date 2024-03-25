cmake_minimum_required(VERSION 3.10)

project(mkdir-andruschenko)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(LLVM_ENABLE_PROJECTS clang;lldb)
set(LLVM_ENABLE_RUNTIMES libcxx;libcxxabi;libunwind)
set(LLDB_EXPORT_ALL_SYMBOLS 1)

add_executable(mkdir-andruschenko mkdir.c)

find_package(OpenSSL REQUIRED)
target_link_libraries(mkdir-andruschenko OpenSSL::Crypto)
