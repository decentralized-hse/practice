cmake_minimum_required(VERSION 3.14)
project(BASONRedis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


set(REDIS_SOURCES
    src/redis/bason_redis_store.cpp
    src/redis/persistence_manager.cpp
    src/redis/bason_redis_server.cpp
    src/redis/bason_redis_client.cpp
)


add_library(basonredis STATIC ${REDIS_SOURCES})
target_include_directories(basonredis PUBLIC src)
target_link_libraries(basonredis)


add_executable(basonredis-server example/server.cpp)
target_include_directories(basonredis-server PRIVATE src)
target_link_libraries(basonredis-server basonredis)


add_executable(basonredis-client example/client.cpp)
target_include_directories(basonredis-client PRIVATE src)
target_link_libraries(basonredis-client basonredis)


enable_testing()

add_executable(test_store test/test_store.cpp)
target_include_directories(test_store PRIVATE src)
target_link_libraries(test_store basonredis GTest::gtest_main)

add_executable(test_server test/test_server.cpp)
target_include_directories(test_server PRIVATE src)
target_link_libraries(test_server basonredis GTest::gtest_main)

add_executable(test_persistence test/test_persistence.cpp)
target_include_directories(test_persistence PRIVATE src)
target_link_libraries(test_persistence basonredis GTest::gtest_main)

add_executable(test_client test/test_client.cpp)
target_include_directories(test_client PRIVATE src)
target_link_libraries(test_client basonredis GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_store)
gtest_discover_tests(test_server)
gtest_discover_tests(test_persistence)
gtest_discover_tests(test_client)
