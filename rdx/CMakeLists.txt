cmake_minimum_required(VERSION 3.20)
project(rdx C)

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

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

list(APPEND RDX_HEADERS 
RDX.h
RDXC.h
JDR.h
Y.h
RDXZ.h
)

list(APPEND RDX_SOURCES 
JDR.c
RDX.c
JDR.rl.c
ryu/d2s.c
)

add_library(rdx STATIC ${RDX_HEADERS} ${RDX_SOURCES})
target_link_libraries(rdx abc sodium)

add_executable(rdx-cli RDX.cli.c)
set_target_properties(rdx-cli PROPERTIES OUTPUT_NAME rdx)
target_link_libraries(rdx-cli rdx)

add_subdirectory(test)

#if (WITH_BENCH)
#add_subdirectory(bench EXCLUDE_FROM_ALL)
#endif()
#
if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
