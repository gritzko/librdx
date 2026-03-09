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
    BIFF.h
)

list(APPEND JSON_SOURCES
    BASON.c
    BIFF.c
    PASS.c
)

add_executable(bason bason.cli.c BASON.c ../abc/TLV.c)
target_link_libraries(bason abc)

add_executable(borge borge.cli.c BIFF.c PASS.c BASON.c ../abc/TLV.c)
target_link_libraries(borge abc)

add_executable(biff biff.cli.c BIFF.c PASS.c BASON.c ../abc/TLV.c)
target_link_libraries(biff abc)

add_subdirectory(test)

if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
