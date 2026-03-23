cmake_minimum_required(VERSION 3.20)
project(capo C)

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

add_library(capolib STATIC ${CAPO_HEADERS} ${CAPO_SOURCES})
target_compile_definitions(capolib PRIVATE NDEBUG)
target_link_libraries(capolib bast-core abc-core tree-sitter)

add_executable(capo-cli CAPO.cli.c)
target_compile_definitions(capo-cli PRIVATE NDEBUG)
target_link_libraries(capo-cli capolib bast-core abc-core tree-sitter)
set_target_properties(capo-cli PROPERTIES OUTPUT_NAME capo)

add_subdirectory(test)
