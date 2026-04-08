cmake_minimum_required(VERSION 3.20)
project(graf C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND GRAF_HEADERS
    GRAF.h
    JOIN.h
    NEIL.h
    TDIFF.h
)

list(APPEND GRAF_SOURCES
    GRAF.c
    DIFF.c
    JOIN.c
    MERGE.c
    INST.c
    NEIL.c
    TDIFF.c
)

add_library(graflib STATIC ${GRAF_HEADERS} ${GRAF_SOURCES})
target_compile_definitions(graflib PRIVATE NDEBUG)
target_link_libraries(graflib dog abc-core)

add_executable(graf-cli GRAF.cli.c)
target_compile_definitions(graf-cli PRIVATE NDEBUG)
target_link_libraries(graf-cli graflib dog abc-core)
add_dependencies(graf-cli bro-cli)
set_target_properties(graf-cli PROPERTIES OUTPUT_NAME graf)

# graf is built next to bro so HOMEResolveSibling finds it.
add_custom_command(TARGET graf-cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bro-cli> ${CMAKE_CURRENT_BINARY_DIR}/bro
)

add_subdirectory(test)

if (WITH_FUZZ)
    add_subdirectory(fuzz)
endif()
