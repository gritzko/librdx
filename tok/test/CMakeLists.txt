cmake_minimum_required(VERSION 3.20)
project(tok C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(TOK01test TOK01.c)
target_link_options(TOK01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(TOK01test PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(TOK01test tok abc-core)
add_test(NAME TOK01test COMMAND TOK01test)

add_executable(JOIN01test JOIN01.c)
target_link_options(JOIN01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JOIN01test PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(JOIN01test tok abc-core)
add_test(NAME JOIN01test COMMAND JOIN01test)

add_executable(BRCTtest BRCT.c)
target_link_options(BRCTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BRCTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BRCTtest tok abc-core)
add_test(NAME BRCTtest COMMAND BRCTtest)
