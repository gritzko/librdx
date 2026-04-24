# beagle tests

if (WITH_MILL)
    add_test(NAME mill
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/mill.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(mill PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=mill")
endif()

# workflow.sh exercises be's get/put/post/delete dispatch against a
# toy worktree — mirrors sniff/test/workflow.sh but through `be`.
add_test(NAME BEworkflow
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/workflow.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEworkflow PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=BEworkflow")

# worktree.sh — `be get <localdir>` clones a local primary.
# Worktree layout: `.dogs` symlink to primary + local `.sniff` file.
add_test(NAME BEworktree
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/worktree.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEworktree PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=BEworktree")

# history.sh — put/post, delete/post, edit/post, then checkout an
# older commit by SHA (version handle = at.log tail).  No ssh.
add_test(NAME BEhistory
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/history.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(BEhistory PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=BEhistory")

if (WITH_SSH)
    # roundtrip.sh — full git <-> be flow: seed origin, clone over ssh,
    # make a worktree, edit, commit, push, diff.
    add_test(NAME BEroundtrip
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/roundtrip.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(BEroundtrip PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=BEroundtrip")

    # Toy-repo matrix (3 commits, v1+v2 tags, master tip).  Each script
    # builds the same toy and rsync-compares git-side and keeper-side
    # worktrees after the same operation.  -from-git uses a vanilla git
    # server; -from-keeper seeds a keeper mirror first and serves it
    # via `keeper upload-pack`.  Requires passwordless ssh to localhost.
    foreach(toy blobs-from-git tag-blobs-from-keeper clone-from-git clone-from-keeper tags-from-git tags-from-keeper)
        add_test(NAME ${toy}
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/${toy}.sh
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties(${toy} PROPERTIES
            ENVIRONMENT "BIN=${DOG_BIN_DIR};TMP=${DOG_TEST_TMP_ROOT};TEST_ID=${toy}")
    endforeach()
endif()

# clone-git.sh and mill-tags.sh are manual heavyweight integration
# scripts (require $HOME/src/git + ssh localhost); run directly.
