cmake_minimum_required(VERSION 3.20)
project(brix C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(BRIXtest BRIX.c)
target_link_options(BRIXtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BRIXtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BRIXtest abc rdx brix)
add_test(NAME BRIXtest COMMAND BRIXtest)
