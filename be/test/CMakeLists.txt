cmake_minimum_required(VERSION 3.20)
project(be_test C)
set(CMAKE_C_STANDARD 23)
include(CTest)
include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

find_library(TREE_SITTER_LIB NAMES tree-sitter REQUIRED)

add_executable(BEtest BE.c)
target_link_options(BEtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BEtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BEtest beagle bast abc ${TREE_SITTER_LIB})
add_test(NAME BEtest COMMAND BEtest)
