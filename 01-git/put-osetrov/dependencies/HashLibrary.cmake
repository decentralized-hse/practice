include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    hash_library
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(hash_library)

message(STATUS "HashLibrary source dir: ${hash_library_SOURCE_DIR}")
