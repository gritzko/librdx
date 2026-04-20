cmake_minimum_required(VERSION 3.20)
project(git_test C)
set(CMAKE_C_STANDARD 23)
include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(GITtest GIT.c)
target_link_options(GITtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(GITtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(GITtest gitcompat abc-core)
add_test(NAME GITtest COMMAND GITtest)

add_executable(PKTtest PKT.c)
target_link_options(PKTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(PKTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(PKTtest gitcompat abc-core)
add_test(NAME PKTtest COMMAND PKTtest)

add_executable(GITPACKtest PACK.c)
target_link_options(GITPACKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(GITPACKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(GITPACKtest gitcompat abc-core)
add_test(NAME GITPACKtest COMMAND GITPACKtest)

if (WITH_SSH)
    add_executable(FETCHtest FETCH.c)
    target_link_options(FETCHtest PRIVATE ${TEST_LDD_FLAGS})
    target_compile_options(FETCHtest PRIVATE ${TEST_CXX_FLAGS})
    target_link_libraries(FETCHtest gitcompat abc-core)
    add_test(NAME FETCHtest COMMAND FETCHtest)

    add_executable(ROUNDtest ROUND.c)
    target_link_options(ROUNDtest PRIVATE ${TEST_LDD_FLAGS})
    target_compile_options(ROUNDtest PRIVATE ${TEST_CXX_FLAGS})
    target_link_libraries(ROUNDtest gitcompat abc-core)
    add_test(NAME ROUNDtest COMMAND ROUNDtest)

endif()

add_executable(WALKtest WALK.c)
target_link_options(WALKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(WALKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(WALKtest keeplib gitcompat dog abc-core)
add_test(NAME WALKtest COMMAND WALKtest)

add_executable(ZINFtest ZINF.c)
target_link_options(ZINFtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(ZINFtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(ZINFtest gitcompat abc-core)
add_test(NAME ZINFtest COMMAND ZINFtest)

add_executable(KEEPtest KEEP.c)
target_link_options(KEEPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(KEEPtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(KEEPtest keeplib gitcompat dog abc-core)
add_test(NAME KEEPtest COMMAND KEEPtest)

add_executable(UNPKtest UNPK.c)
target_link_options(UNPKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(UNPKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(UNPKtest keeplib gitcompat dog abc-core)
add_test(NAME UNPKtest COMMAND UNPKtest)

add_executable(BOOKMARKtest BOOKMARK.c)
target_link_options(BOOKMARKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BOOKMARKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BOOKMARKtest keeplib gitcompat dog abc-core)
add_test(NAME BOOKMARKtest COMMAND BOOKMARKtest)

add_executable(SYNCFRAMEtest SYNCFRAME.c)
target_link_options(SYNCFRAMEtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SYNCFRAMEtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SYNCFRAMEtest keeplib gitcompat dog abc-core)
add_test(NAME SYNCFRAMEtest COMMAND SYNCFRAMEtest)

#  End-to-end get/post loopback: keeper sync (server) vs.
#  keeper get file:// (client).  Exercises the full TLV wire.
add_test(NAME SYNCloopback
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/sync-loopback.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(SYNCloopback PROPERTIES
    ENVIRONMENT "BIN=${DOG_BIN_DIR}")

#  Full clone + push + pull round-trip via `be://localhost/...`
#  (plain protocol over ssh).  Requires passwordless ssh + $HOME/tmp.
if (WITH_SSH)
    add_test(NAME SYNCssh
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/sync-ssh.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(SYNCssh PROPERTIES
        ENVIRONMENT "BIN=${DOG_BIN_DIR}")
endif()
