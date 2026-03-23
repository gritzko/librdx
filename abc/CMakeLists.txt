cmake_minimum_required(VERSION 3.20)
project(libabc C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. . ryu)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_library(sodium NAMES sodium HINTS /usr /usr/local)
find_package(CURL QUIET)
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LZ4 liblz4)
    if (WITH_ROCKS)
        pkg_check_modules(ROCKSDB REQUIRED rocksdb)
    endif()
endif()

list(APPEND ABC_HEADERS
S.h
Sx.h
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
LSM.h
JSON.h
JSON.rl.h
LEX.h
MMAP.h
MSET.h
MSETx.h
NEST.h
OK.h
PAGE.h
PRO.h
SHA.h
SKIP.h
SKIPx.h
SLOG.h
SORT.h
TEST.h
TLV.h
TTY.h
UTF8.h
ZINT.h
URI.h
NET.h
UDP.h
HTTP.h
CURL.h
PACK.h
PATH.h
BSD.h
)

list(APPEND ABC_SOURCES
NEST.c
FILE.c
INT.c
JSON.c
JSON.rl.c
LEX.c
LEX.rl.c
LSM.c
MMAP.c
PAGE.c
POL.c
RON.c
SLOG.c
SORT.c
URI.rl.c
URI.c
NET.c
HTTP.rl.c
HTTP.c
TCP.c
TLV.c
TTY.c
UDP.c
UTF8.c
ZINT.c
CURL.c
PACK.c
PATH.c
BSD.c
ryu/d2s.c
)

if (WITH_ROCKS)
list(APPEND ABC_HEADERS ROCK.h)
list(APPEND ABC_SOURCES ROCK.c)
endif()

# Minimal abc-core: no heavy deps — for standalone tools like capo
set(ABC_CORE_SOURCES ${ABC_SOURCES})
list(REMOVE_ITEM ABC_CORE_SOURCES CURL.c NET.c HTTP.c HTTP.rl.c TCP.c UDP.c POL.c PACK.c)
add_library(abc-core STATIC ${ABC_HEADERS} ${ABC_CORE_SOURCES})

# Full abc with all deps (only if sodium+curl+lz4 found)
if (sodium AND CURL_FOUND AND LZ4_FOUND)
    add_library(abc STATIC ${ABC_HEADERS} ${ABC_SOURCES})
    target_link_libraries(abc sodium CURL::libcurl ${LZ4_LIBRARIES})
    if (WITH_ROCKS)
        target_link_libraries(abc ${ROCKSDB_LIBRARIES})
    endif()
else()
    message(STATUS "abc: full build skipped (missing sodium/curl/lz4)")
endif()

add_executable(ok64 OK.cli.c)
target_link_libraries(ok64 abc-core)

add_executable(lex LEX.cli.c)
target_link_libraries(lex abc-core)

add_subdirectory(test)

if (WITH_BENCH)
add_subdirectory(bench)
endif()

if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
