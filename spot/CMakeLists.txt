cmake_minimum_required(VERSION 3.20)
project(spot C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND CAPO_HEADERS
    CAPO.h
)

list(APPEND CAPO_SOURCES
    CAPO.c
)

add_library(spotlib STATIC ${CAPO_HEADERS} ${CAPO_SOURCES})
target_compile_definitions(spotlib PRIVATE NDEBUG)
target_link_libraries(spotlib bast-core abc-core tree-sitter)

add_executable(spot-cli CAPO.cli.c)
target_compile_definitions(spot-cli PRIVATE NDEBUG)
target_link_libraries(spot-cli spotlib bast-core abc-core tree-sitter)
set_target_properties(spot-cli PROPERTIES OUTPUT_NAME spot)

add_subdirectory(test)
