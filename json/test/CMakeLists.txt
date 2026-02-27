cmake_minimum_required(VERSION 3.20)
project(json C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(TLKVtest TLKV.c ../../abc/TLV.c)
target_link_options(TLKVtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(TLKVtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(TLKVtest abc)
add_test(NAME TLKVtest COMMAND TLKVtest)
