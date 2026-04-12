cmake_minimum_required(VERSION 3.20)
project(dog C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(TOK01test TOK01.c)
target_link_options(TOK01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(TOK01test PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(TOK01test dog abc-core)
add_test(NAME TOK01test COMMAND TOK01test)

add_executable(BRCTtest BRCT.c)
target_link_options(BRCTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BRCTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BRCTtest dog abc-core)
add_test(NAME BRCTtest COMMAND BRCTtest)

add_executable(DEFtest DEF.c)
target_link_options(DEFtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(DEFtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(DEFtest dog abc-core)
add_test(NAME DEFtest COMMAND DEFtest)

add_executable(DEF01test DEF01.c)
target_link_options(DEF01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(DEF01test PRIVATE ${TEST_CXX_FLAGS})
target_compile_definitions(DEF01test PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(DEF01test dog abc-core)
add_test(NAME DEF01test COMMAND DEF01test)

add_executable(IGNOtest IGNO.c)
target_link_options(IGNOtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(IGNOtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(IGNOtest dog abc-core)
add_test(NAME IGNOtest COMMAND IGNOtest)

