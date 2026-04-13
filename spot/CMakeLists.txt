cmake_minimum_required(VERSION 3.20)
project(spot C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)


include_directories(.. .)

list(APPEND SPOT_HEADERS
    CAPO.h
    CAPOi.h
    LESS.h
    SPOT.h
)

list(APPEND SPOT_SOURCES
    CAPO.c
    GREP.c
    LESS.c
    SPOT.c
)

add_library(spotlib STATIC ${SPOT_HEADERS} ${SPOT_SOURCES})
target_compile_definitions(spotlib PRIVATE NDEBUG)
target_link_libraries(spotlib dog abc-core)

add_custom_target(spot-version ALL
    COMMAND ${CMAKE_COMMAND}
        -DSRC=${CMAKE_CURRENT_SOURCE_DIR}
        -DOUT=${CMAKE_CURRENT_BINARY_DIR}/SPOT_VERSION.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/SPOT_VERSION.h
)

add_executable(spot-cli CAPO.cli.c)
target_compile_definitions(spot-cli PRIVATE NDEBUG)
target_include_directories(spot-cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(spot-cli spot-version bro-cli)
target_link_libraries(spot-cli spotlib dog abc-core)
set_target_properties(spot-cli PROPERTIES OUTPUT_NAME spot RUNTIME_OUTPUT_DIRECTORY ${DOG_BIN_DIR})

add_subdirectory(test)

if (WITH_FUZZ)
    add_subdirectory(fuzz)
endif()
