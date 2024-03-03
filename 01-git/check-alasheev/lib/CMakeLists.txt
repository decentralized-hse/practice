add_library(
    lib
    validate.cpp
)

include(../dependencies/HashLibrary.cmake)
include_directories(include)

target_include_directories(lib PRIVATE ${hash_library_SOURCE_DIR} ${fmt_library_SOURCE_DIR} ${fmt_library_SOURCE_DIR}/include)
target_link_libraries(lib PRIVATE fmt::fmt)
