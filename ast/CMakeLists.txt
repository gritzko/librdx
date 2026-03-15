cmake_minimum_required(VERSION 3.20)
project(ast C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

# Vendored tree-sitter 0.26.6 (single-compilation-unit build)
add_library(tree-sitter STATIC tree-sitter/lib.c)
target_include_directories(tree-sitter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter/include)
target_include_directories(tree-sitter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter)

# Collect all tree-sitter parser sources (flat dirs + nested dirs)
# Exclude vendored tree-sitter/ directory
file(GLOB PARSER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/scanner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/grammars/*/src/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/grammars/*/src/scanner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*/src/scanner.c
)
list(FILTER PARSER_SOURCES EXCLUDE REGEX "tree-sitter/")

list(APPEND BAST_HEADERS
    BAST.h
)

list(APPEND BAST_SOURCES
    BAST.c
    sm/SM.c
    sm/SM.rl.c
    sm/SMI.rl.c
    ../json/BASON.c
    ${PARSER_SOURCES}
)

add_library(bast STATIC ${BAST_HEADERS} ${BAST_SOURCES})
target_link_libraries(bast abc tree-sitter)

add_executable(bast-cli BAST.cli.c)
target_link_libraries(bast-cli bast abc tree-sitter)
set_target_properties(bast-cli PROPERTIES OUTPUT_NAME bast)

add_subdirectory(test)
add_subdirectory(sm/test)
