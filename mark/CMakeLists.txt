cmake_minimum_required(VERSION 3.27)
project(libmark C)

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

list(APPEND MARK_HEADERS 
MARK.h
MARK.rl.h
MARQ.h
MARQ.rl.h
)

list(APPEND MARK_SOURCES 
MARK.c
MARK.rl.c
MARQ.c
MARQ.rl.c
)

add_library(libmark STATIC ${MARK_HEADERS} ${MARK_SOURCES})
target_link_libraries(libmark libabc)

add_executable(mark2html MARKhtml.cli.c)
target_link_libraries(mark2html libmark)

add_executable(mark MARK.cli.c)
target_link_libraries(mark libmark)

add_subdirectory(test)

if (WITH_BENCH)
add_subdirectory(bench EXCLUDE_FROM_ALL)
endif()

if (WITH_FUZZ)
add_subdirectory(fuzz EXCLUDE_FROM_ALL)
endif()
