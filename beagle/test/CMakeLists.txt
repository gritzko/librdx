# beagle tests

if (WITH_MILL)
    add_test(NAME mill
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/mill.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(mill PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR}")
endif()

# workflow.sh exercises be's get/put/post/delete dispatch against a
# toy worktree — mirrors sniff/test/workflow.sh but through `be`.
add_test(NAME BEworkflow
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/workflow.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEworkflow PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

# clone-git.sh and mill-tags.sh are manual heavyweight integration
# scripts (require $HOME/src/git + ssh localhost); run directly.
