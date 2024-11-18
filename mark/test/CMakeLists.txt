cmake_minimum_required(VERSION 3.27)
project(marktest C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(MARQtest MARQ.c ../MARQ.c ../MARQ.rl.c)
target_link_options(MARQtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MARQtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME MARQtest COMMAND MARQtest)

add_executable(MARKtest MARK.c ../MARK.c ../MARQ.c ../MARK.rl.c ../MARQ.rl.c)
target_link_options(MARKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MARKtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME MARKtest COMMAND MARKtest)

