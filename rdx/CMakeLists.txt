cmake_minimum_required(VERSION 3.20)
project(rdx2 C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

option(WITH_ASAN "build with ASAN" OFF)
if (WITH_ASAN)
    set(ASAN_FLAGS "-g -O1 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
endif ()

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

list(APPEND RDX_HEADERS
    RDX.h
    DIFF.h
    JDR.h
)

list(APPEND RDX_SOURCES
    RDX.c
    MEM.c
    JDR.rl.c
    JDR.c
    WAL.c
    TLV.c
    UTF8.c
    DIFF.c
    SKIL.c
    VFY.c
    Y.c
)

add_library(rdx2 STATIC ${RDX_HEADERS} ${RDX_SOURCES})
target_link_libraries(rdx2 abc sodium)

add_executable(rdx RDX.cli.c)
target_link_libraries(rdx rdx2)
add_executable(rdx.tlv RDX.cli.c)
target_link_libraries(rdx.tlv rdx2)
add_executable(rdx.skil RDX.cli.c)
target_link_libraries(rdx.skil rdx2)

add_subdirectory(test)

if (WITH_BENCH)
add_subdirectory(bench)
endif()

if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
