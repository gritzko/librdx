cmake_minimum_required(VERSION 3.20)
project(libabc C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_library(sodium 
	NAMES sodium 
	HINTs /usr /usr/local 
	REQUIRED)

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

list(APPEND ABC_HEADERS 
$.h
$x.h
01.h
ABC.h
ANSI.h
AREN.h
B.h
BIN.h
Bx.h
BUF.h
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
MMAP.h
NEST.h
OK.h
PRO.h
SHA.h
SKIP.h
SKIPx.h
SORT.h
TEST.h
TLV.h
UTF8.h
ZINT.h
URI.h
NET.h
UDP.h
HTTP.h
)

list(APPEND ABC_SOURCES 
NEST.c
FILE.c
INT.c
JSON.c
JSON.rl.c
LEX.c
LEX.rl.c
MMAP.c
SORT.c
URI.rl.c
URI.c
HTTP.rl.c
HTTP.c
TCP.c
TLV.c
UDP.c
UTF8.c
)

add_library(abc STATIC ${ABC_HEADERS} ${ABC_SOURCES})
target_link_libraries(abc sodium)

add_executable(ok64 OK.cli.c)
target_link_libraries(ok64 abc)

add_executable(lex LEX.cli.c)
target_link_libraries(lex abc)

add_subdirectory(test)

if (WITH_BENCH)
add_subdirectory(bench)
endif()

if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
