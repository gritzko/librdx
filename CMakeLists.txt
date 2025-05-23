cmake_minimum_required(VERSION 3.20)
project(rdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.)

option(WITH_ASAN "build with ASAN" OFF)
if (WITH_ASAN)
    set(ASAN_FLAGS "-g -O1 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
endif ()

option(WITH_PROF "build with clang profile data" OFF)
if (WITH_PROF)
    set(PROF_FLAGS "-g -O1 -fprofile-instr-generate -fcoverage-mapping -DABC_INSANE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROF_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROF_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROF_FLAGS}")
endif ()

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

add_subdirectory(abc)
add_subdirectory(rdx)
add_subdirectory(brix)
add_subdirectory(dw)
add_subdirectory(mark)
