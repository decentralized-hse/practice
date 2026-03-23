cmake_minimum_required(VERSION 3.20)
project(wal_project VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)

option(WAL_DISABLE_FSYNC "Disable fsync calls in WalWriter::sync()" OFF)

FetchContent_Declare(
  blake3_src
  GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
  GIT_TAG 1.5.1
)

FetchContent_Populate(blake3_src)

add_library(blake3 STATIC
  ${blake3_src_SOURCE_DIR}/c/blake3.c
  ${blake3_src_SOURCE_DIR}/c/blake3_dispatch.c
  ${blake3_src_SOURCE_DIR}/c/blake3_portable.c
)

target_include_directories(blake3 PUBLIC
  ${blake3_src_SOURCE_DIR}/c
)

target_compile_definitions(blake3 PRIVATE
  BLAKE3_NO_SSE2
  BLAKE3_NO_SSE41
  BLAKE3_NO_AVX2
  BLAKE3_NO_AVX512
)

add_library(wal
  src/bason_record.cpp
  src/wal.cpp
)

target_include_directories(wal
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(wal PUBLIC cxx_std_20)

target_link_libraries(wal PRIVATE blake3)

if (WAL_DISABLE_FSYNC)
  target_compile_definitions(wal PUBLIC WAL_DISABLE_FSYNC=1)
endif()

add_executable(test_basic tests/test_basic.cpp)
target_link_libraries(test_basic PRIVATE wal)

add_executable(test_rotate tests/test_rotate.cpp)
target_link_libraries(test_rotate PRIVATE wal)

add_executable(test_corruption tests/test_corruption.cpp)
target_link_libraries(test_corruption PRIVATE wal)

add_executable(test_perf tests/test_perf.cpp)
target_link_libraries(test_perf PRIVATE wal)

add_executable(test_crash tests/test_crash.cpp)
target_link_libraries(test_crash PRIVATE wal)