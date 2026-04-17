# beagle tests

if (WITH_MILL)
    add_test(NAME mill
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/mill.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(mill PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR}")
endif()

# clone-git.sh and mill-tags.sh are manual heavyweight integration
# scripts (require $HOME/src/git + ssh localhost); run directly.
