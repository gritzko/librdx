cmake_minimum_required(VERSION 3.20)
project(ast C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

find_library(TREE_SITTER_LIB NAMES tree-sitter REQUIRED)

# Collect all tree-sitter parser sources (flat dirs + nested dirs)
file(GLOB PARSER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/scanner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/grammars/*/src/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/grammars/*/src/scanner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/scanner.c
)

list(APPEND BAST_HEADERS
    BAST.h
)

list(APPEND BAST_SOURCES
    BAST.c
    ../json/BASON.c
    ${PARSER_SOURCES}
)

add_library(bast STATIC ${BAST_HEADERS} ${BAST_SOURCES})
target_link_libraries(bast abc ${TREE_SITTER_LIB})

add_executable(bast-cli BAST.cli.c)
target_link_libraries(bast-cli bast abc ${TREE_SITTER_LIB})
set_target_properties(bast-cli PROPERTIES OUTPUT_NAME bast)
