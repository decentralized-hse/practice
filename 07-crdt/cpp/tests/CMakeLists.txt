set(TEST_DEPENDENCIES libs isfr)

function(MakeTest test_case)
    message(STATUS "Add test: ${test_case}")

    add_executable(${test_case} "./${test_case}.cpp")
    target_link_libraries(${test_case} ${TEST_DEPENDENCIES} gtest_main)
    add_test(NAME ${test_case}_test COMMAND ${test_case})

endfunction()

message(STATUS "Configuring tests")

MakeTest(test_zip)
MakeTest(test_tlv)
MakeTest(test_isfr)
