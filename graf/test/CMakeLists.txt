add_executable(JOIN01test JOIN01.c)
target_link_options(JOIN01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JOIN01test PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(JOIN01test graflib dog abc-core)
add_test(NAME JOIN01test COMMAND JOIN01test)

add_test(NAME GRAFcli_diff_artifact
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/diff_test.sh
        $<TARGET_FILE:graf-cli>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#  toy.sh — full-pipeline smoke test: be post x3, graf diff, merge,
#  blame, weave, index/status.  Exercises dog/DOG.md §8 entry points.
add_test(NAME GRAFtoy
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/toy.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(GRAFtoy PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")
