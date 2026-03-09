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

add_executable(BASONtest BASON.c ../BASON.c ../../abc/TLV.c)
target_link_options(BASONtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BASONtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BASONtest abc)
add_test(NAME BASONtest COMMAND BASONtest)

add_executable(BIFFtest BIFF.c ../BIFF.c ../PASS.c ../BASON.c ../../abc/TLV.c)
target_link_options(BIFFtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BIFFtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BIFFtest abc)
add_test(NAME BIFFtest COMMAND BIFFtest)
