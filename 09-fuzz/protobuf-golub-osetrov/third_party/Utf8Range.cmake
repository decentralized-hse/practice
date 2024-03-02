include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    utf8_range
    GIT_REPOSITORY https://github.com/protocolbuffers/utf8_range
    GIT_TAG main
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(utf8_range)
