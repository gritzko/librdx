cmake_minimum_required(VERSION 3.20)
project(rdx C)

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
    RDX2.h
    JDR2.h
    BRIX2.h
)

list(APPEND RDX_SOURCES 
    RDX2.c
    JDR.rl.c
    JDR2.c
    BRIX2.c
)

add_library(rdx STATIC ${RDX_HEADERS} ${RDX_SOURCES} MAN.h)
target_link_libraries(rdx abc sodium)

add_subdirectory(test)

#if (WITH_BENCH)
#add_subdirectory(bench EXCLUDE_FROM_ALL)
#endif()
#
if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
