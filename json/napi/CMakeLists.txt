cmake_minimum_required(VERSION 3.15)
project(bason_napi C)

set(CMAKE_C_STANDARD 23)

# cmake-js provides these variables
include_directories(${CMAKE_JS_INC})

# Project root for includes
set(LIBRDX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${LIBRDX_ROOT})

# Collect abc sources, excluding CLI mains and test files
file(GLOB ABC_SOURCES ${LIBRDX_ROOT}/abc/*.c)
list(FILTER ABC_SOURCES EXCLUDE REGEX ".*\\.cli\\.c$")

# BASON sources
set(BASON_SOURCES
    ${LIBRDX_ROOT}/json/BASON.c
    ${LIBRDX_ROOT}/json/BIFF.c
    ${LIBRDX_ROOT}/abc/TLV.c
)

add_library(bason SHARED
    bason_napi.c
    ${BASON_SOURCES}
    ${ABC_SOURCES}
    ${CMAKE_JS_SRC}
)

set_target_properties(bason PROPERTIES
    PREFIX ""
    SUFFIX ".node"
)

target_link_libraries(bason ${CMAKE_JS_LIB})

# Node headers
execute_process(
    COMMAND node -p "require('node-addon-api').include_dir"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE NAPI_INCLUDE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
# System node headers
target_include_directories(bason PRIVATE /usr/include/node)
if(NAPI_INCLUDE)
    target_include_directories(bason PRIVATE ${NAPI_INCLUDE})
endif()
