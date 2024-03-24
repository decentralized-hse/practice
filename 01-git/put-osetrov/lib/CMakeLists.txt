add_library(
    lib
    empty_dir.cpp
    git_put.cpp
    read_content.cpp
    sha256.cpp)

include(../dependencies/HashLibrary.cmake)

target_include_directories(lib PRIVATE ${hash_library_SOURCE_DIR})
