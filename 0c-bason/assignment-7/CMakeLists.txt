cmake_minimum_required(VERSION 3.20)
project(basonlite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    basonlite
    src/page_manager.cpp
    src/btree.cpp
    src/codec/bason_record.cpp
    src/wal/wal.cpp
)

target_include_directories(basonlite PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(GTest REQUIRED)

add_executable(
    btree_test test/btree_test.cpp
)
target_link_libraries(
    btree_test
    PRIVATE
    basonlite
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME btree_test COMMAND btree_test)

add_executable(
    page_manager_test test/page_manager_test.cpp
)
target_link_libraries(
    page_manager_test
    PRIVATE
    basonlite
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME page_manager_test COMMAND page_manager_test)

enable_testing()