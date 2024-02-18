include(FetchContent)

FetchContent_Declare(
        PicoSHA2
        GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
        GIT_TAG master
)
FetchContent_MakeAvailable(PicoSHA2)

FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG master
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG master
)
FetchContent_MakeAvailable(fmt)
