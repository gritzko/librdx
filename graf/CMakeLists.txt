cmake_minimum_required(VERSION 3.20)
project(graf C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND GRAF_HEADERS
    GRAF.h
    DAG.h
    JOIN.h
    NEIL.h
    TDIFF.h
    WEAVE.h
)

list(APPEND GRAF_SOURCES
    GRAF.c
    BLAME.c
    DAG.c
    DIFF.c
    JOIN.c
    MERGE.c
    INST.c
    NEIL.c
    TDIFF.c
    WEAVE.c
)

add_library(graflib STATIC ${GRAF_HEADERS} ${GRAF_SOURCES})
target_compile_definitions(graflib PRIVATE NDEBUG)
target_link_libraries(graflib dog abc-core)

add_executable(graf-cli GRAF.cli.c)
target_compile_definitions(graf-cli PRIVATE NDEBUG)
target_link_libraries(graf-cli graflib dog abc-core)
add_dependencies(graf-cli bro-cli)
set_target_properties(graf-cli PROPERTIES OUTPUT_NAME graf RUNTIME_OUTPUT_DIRECTORY ${DOG_BIN_DIR})

add_subdirectory(test)

if (WITH_FUZZ)
    add_subdirectory(fuzz)
endif()
