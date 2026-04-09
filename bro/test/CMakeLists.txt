cmake_minimum_required(VERSION 3.20)
project(bro C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(NAVtest NAV.c)
target_link_options(NAVtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(NAVtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(NAVtest brolib dog abc-core)
add_test(NAME NAVtest COMMAND NAVtest)
