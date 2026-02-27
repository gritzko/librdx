cmake_minimum_required(VERSION 3.20)
project(json C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

list(APPEND JSON_HEADERS
    TLKV.h
    BASON.h
)

list(APPEND JSON_SOURCES
    BASON.c
)

add_subdirectory(test)
