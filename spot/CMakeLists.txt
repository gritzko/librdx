cmake_minimum_required(VERSION 3.20)
project(spot C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND SPOT_HEADERS
    CAPO.h
    CAPOi.h
    HUNK.h
    INST.h
    LESS.h
    NEIL.h
    SPOT.h
)

list(APPEND SPOT_SOURCES
    CAPO.c
    CAT.c
    DIF2.c
    GREP.c
    HUNK.c
    INST.c
    LESS.c
    NEIL.c
    SPOT.c
)

add_library(spotlib STATIC ${SPOT_HEADERS} ${SPOT_SOURCES})
target_compile_definitions(spotlib PRIVATE NDEBUG)
target_link_libraries(spotlib tok abc-core)

add_executable(spot-cli CAPO.cli.c)
target_compile_definitions(spot-cli PRIVATE NDEBUG)
target_link_libraries(spot-cli spotlib tok abc-core)
set_target_properties(spot-cli PROPERTIES OUTPUT_NAME spot)

add_subdirectory(test)

if (WITH_FUZZ)
    add_subdirectory(fuzz)
endif()
