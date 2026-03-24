cmake_minimum_required(VERSION 3.14)
project(bason-codec CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source files
set(BASON_SOURCES
    src/ron64.cpp
    src/path_utils.cpp
    src/bason_codec.cpp
    src/strictness.cpp
    src/flatten.cpp
    src/json_converter.cpp
)

# Library
add_library(bason ${BASON_SOURCES})
target_include_directories(bason PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# basondump tool
add_executable(basondump tools/basondump.cpp)
target_link_libraries(basondump bason)

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test files
add_executable(test_bason
    test/test_codec.cpp
    test/test_ron64.cpp
    test/test_path.cpp
    test/test_json.cpp
    test/test_flatten.cpp
    test/test_fuzz.cpp
    test/test_performance.cpp
)

target_link_libraries(test_bason
    bason
    gtest
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_bason)

# Install targets
install(TARGETS bason basondump
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY src/
    DESTINATION include/bason
    FILES_MATCHING PATTERN "*.h"
)
