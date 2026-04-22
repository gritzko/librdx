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

# worktree.sh — `be get <localdir>` clones a local primary.
# Phase 2: single `.dogs` symlink + local `.sniff/` dir.
add_test(NAME BEworktree
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/worktree.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEworktree PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

if (WITH_SSH)
# history.sh — put/post, delete/post, edit/post, then checkout an
# older commit by SHA (version handle = at.log tail).
add_test(NAME BEhistory
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/history.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEhistory PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

# roundtrip.sh — full git <-> be flow: seed origin, clone over ssh,
# make a worktree, edit, commit, push, diff.
    add_test(NAME BEroundtrip
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/roundtrip.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(BEroundtrip PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR}")
endif()

# clone-git.sh and mill-tags.sh are manual heavyweight integration
# scripts (require $HOME/src/git + ssh localhost); run directly.
