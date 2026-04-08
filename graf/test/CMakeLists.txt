add_executable(JOIN01test JOIN01.c)
target_link_options(JOIN01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JOIN01test PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(JOIN01test graflib dog abc-core)
add_test(NAME JOIN01test COMMAND JOIN01test)

add_test(NAME GRAFcli_diff_artifact
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/diff_test.sh
        $<TARGET_FILE:graf-cli>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
