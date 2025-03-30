cmake_minimum_required(VERSION 3.10)

project(ConPick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("/opt/homebrew/opt/nlohmann-json/include")
include_directories(/opt/homebrew/opt/googletest/include)

# Завершающий файл с main
add_executable(con_pick main.cpp con_pick.cpp)

# Исполняемый файл для тестов без main
add_executable(con_pick_tests test_con_pick.cpp con_pick.cpp)
target_include_directories(con_pick_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(con_pick_tests
    PRIVATE
    /opt/homebrew/opt/googletest/lib/libgtest.a
    /opt/homebrew/opt/googletest/lib/libgtest_main.a
    pthread
)

enable_testing()
add_test(NAME ConPickTests COMMAND con_pick_tests)
