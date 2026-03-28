cmake_minimum_required(VERSION 3.20)
project(tok C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

list(APPEND TOK_HEADERS
    TOK.h
    CT.h
    GOT.h
    PYT.h
    JST.h
    RST.h
    JAT.h
    CPPT.h
    CST.h
    HTMT.h
    CSST.h
    JSONT.h
    SHT.h
    RBT.h
    HST.h
    MLT.h
    JLT.h
    PHPT.h
    AGDT.h
    VERT.h
)

list(APPEND TOK_SOURCES
    TOK.c
    CT.c
    CT.rl.c
    GOT.c
    GOT.rl.c
    PYT.c
    PYT.rl.c
    JST.c
    JST.rl.c
    RST.c
    RST.rl.c
    JAT.c
    JAT.rl.c
    CPPT.c
    CPPT.rl.c
    CST.c
    CST.rl.c
    HTMT.c
    HTMT.rl.c
    CSST.c
    CSST.rl.c
    JSONT.c
    JSONT.rl.c
    SHT.c
    SHT.rl.c
    RBT.c
    RBT.rl.c
    HST.c
    HST.rl.c
    MLT.c
    MLT.rl.c
    JLT.c
    JLT.rl.c
    PHPT.c
    PHPT.rl.c
    AGDT.c
    AGDT.rl.c
    VERT.c
    VERT.rl.c
)

list(APPEND TOK_HEADERS JOIN.h)
list(APPEND TOK_SOURCES JOIN.c)

add_library(tok STATIC ${TOK_HEADERS} ${TOK_SOURCES})
target_link_libraries(tok abc-core)

add_executable(toktok toktok.cli.c)
target_link_libraries(toktok tok abc-core)

add_executable(tokjoin tokjoin.cli.c)
target_link_libraries(tokjoin tok abc-core)

add_subdirectory(test)

if (WITH_FUZZ)
add_subdirectory(fuzz)
endif()
