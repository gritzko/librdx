cmake_minimum_required(VERSION 3.27)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

#find_package(sodium CONFIG)
include_directories(. ryu)

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
$.h
$x.h
01.h
ABC.h
ANSI.h
AREN.h
B.h
BIN.h
Bx.h
FILE.h
HASH.h
HASHx.h
HEAPx.h
HEX.h
HEXx.h
INT.h
INTx.h
JSON.h
JSON.rl.h
LEX.h
LEX.rl.h
LSM.h
MARK.h
MARK.rl.h
MARQ.h
MARQ.rl.h
MMAP.h
OK.h
PRO.h
RDX.h
RDXC.h
RDXJ.h
RDXJ.rl.h
RDXY.h
SHA.h
SKIP.h
SKIPx.h
TEST.h
TLV.h
UTF8.h
ZINT.h
)

list(APPEND RDX_SOURCES 
JSON.c
JSON.rl.c
LEX.c
LEX.rl.c
MARK.c
MARK.rl.c
MARQ.c
MARQ.rl.c
RDXJ.c
RDXJ.rl.c
UTF8.c
ryu/d2s.c
)

add_library(librdx STATIC ${RDX_HEADERS} ${RDX_SOURCES})
#target_link_libraries(rdx sodium)

add_subdirectory(cli)

add_subdirectory(test)

add_subdirectory(bench EXCLUDE_FROM_ALL)

add_subdirectory(fuzz EXCLUDE_FROM_ALL)
