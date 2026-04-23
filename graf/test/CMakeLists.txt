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

#  get.sh — URI-driven deterministic blob merge (`graf get <uri>`).
add_test(NAME GRAFget
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/get.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(GRAFget PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

#  merge-from-git.sh — toy git source with two branches, keeper fetch
#  drives graf indexing, `graf get path?sha_a&sha_b` merges concurrent
#  disjoint edits.  Requires ssh-to-localhost (same as the beagle
#  git-backed tests).
add_test(NAME GRAFmergeFromGit
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/merge-from-git.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(GRAFmergeFromGit PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")
