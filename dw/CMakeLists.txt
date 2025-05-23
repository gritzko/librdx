

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

list(APPEND DW_HEADERS 
    CLEX.h
)

list(APPEND DW_SOURCES 
    CLEX.rl.c
    CLEX.c
)

add_library(dw STATIC ${DW_HEADERS} ${DW_SOURCES})
target_link_libraries(dw rdx abc sodium)

add_executable(clex-cli CLEX.cli.c CLEX.rl.c CLEX.c)
target_link_libraries(clex-cli dw)
set_target_properties(clex-cli PROPERTIES OUTPUT_NAME clex)

add_subdirectory(test)
