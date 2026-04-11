cmake_minimum_required(VERSION 3.20)
project(beagle C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND BEAGLE_HEADERS
    GURI.h
)

list(APPEND BEAGLE_SOURCES
    GURI.c
)

add_library(beaglelib STATIC ${BEAGLE_HEADERS} ${BEAGLE_SOURCES})
target_compile_definitions(beaglelib PRIVATE NDEBUG)
target_link_libraries(beaglelib dog abc-core)

add_executable(be-cli BE.cli.c)
target_compile_definitions(be-cli PRIVATE NDEBUG)
target_link_libraries(be-cli beaglelib dog abc-core)
set_target_properties(be-cli PROPERTIES OUTPUT_NAME be)

add_subdirectory(test)
