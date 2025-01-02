cmake_minimum_required(VERSION 3.20)
project(marktest C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(MARQtest MARQ.c)
target_link_options(MARQtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MARQtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(MARQtest mark)
add_test(NAME MARQtest COMMAND MARQtest)

add_executable(MARKtest MARK.c)
target_link_options(MARKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MARKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(MARKtest mark)
add_test(NAME MARKtest COMMAND MARKtest)

