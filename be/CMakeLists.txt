cmake_minimum_required(VERSION 3.20)
project(be C)
set(CMAKE_C_STANDARD 23)
include(CTest)
include_directories(.. .)

if (NOT WITH_ROCKS)
    message(STATUS "be: skipping (WITH_ROCKS not set)")
    return()
endif()

find_library(TREE_SITTER_LIB NAMES tree-sitter REQUIRED)

add_library(beagle STATIC BE.c BESYNC.c ../json/BIFF.c)
target_link_libraries(beagle bast abc ${TREE_SITTER_LIB})

add_executable(be-cli BE.cli.c)
target_link_libraries(be-cli beagle bast abc ${TREE_SITTER_LIB})
set_target_properties(be-cli PROPERTIES OUTPUT_NAME be)

if (WITH_INET)
add_executable(be-srv BE.http.c BESRV.c)
target_link_libraries(be-srv beagle bast abc ${TREE_SITTER_LIB})
endif()

add_subdirectory(test)
