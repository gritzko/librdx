cmake_minimum_required(VERSION 3.27)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. . ryu)

option(WITH_ASAN "build with ASAN" OFF)
if (WITH_ASAN)
    set(ASAN_FLAGS "-g -O1 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=600")
set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

list(APPEND RDX_HEADERS 
LSM.h
RDX.h
RDXC.h
RDXJ.h
RDXJ.rl.h
RDXY.h
SKIP.h
SKIPx.h
)

list(APPEND RDX_SOURCES 
RDXJ.c
RDXJ.rl.c
ryu/d2s.c
)

add_library(librdx STATIC ${RDX_HEADERS} ${RDX_SOURCES})
target_link_libraries(librdx libabc sodium)

add_executable(rdx RDX.cli.c)
target_link_libraries(rdx librdx)

add_subdirectory(test)

if (WITH_BENCH)
add_subdirectory(bench EXCLUDE_FROM_ALL)
endif()

if (WITH_FUZZ)
add_subdirectory(fuzz EXCLUDE_FROM_ALL)
endif()
