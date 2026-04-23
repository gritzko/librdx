add_executable(SNIFFtest SNIFF.c)
target_link_libraries(SNIFFtest snifflib)
add_test(NAME SNIFFtest COMMAND SNIFFtest)

add_test(NAME SNIFFworkflow
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/workflow.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(SNIFFworkflow PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

#  patch.sh — 3-way merge toys (sniff patch <ref>).  Needs ssh
#  localhost (same transport as beagle/test/blobs-from-git.sh).
add_test(NAME SNIFFpatch
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/patch.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(SNIFFpatch PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")
