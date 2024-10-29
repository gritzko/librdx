cmake_minimum_required(VERSION 3.27)
project(librdxcli C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(.. ../ryu)

#find_package(sodium CONFIG)

#add_executable(01test 01.c)
#target_link_options(01test PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(01test PRIVATE ${TEST_CXX_FLAGS})
#add_test(NAME 01test COMMAND 01test)

add_executable(ok64 OK.c)
target_link_libraries(ok64 librdx)

add_executable(mark2html MARK.HTML.c)
target_link_libraries(mark2html librdx)

add_executable(mark MARK.c)
target_link_libraries(mark librdx)

add_executable(lex LEX.c)
target_link_libraries(lex librdx)

add_executable(rdx RDX.c)
target_link_libraries(rdx librdx)

