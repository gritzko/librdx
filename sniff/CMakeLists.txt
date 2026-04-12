cmake_minimum_required(VERSION 3.20)
project(sniff C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

add_library(snifflib STATIC SNIFF.h SNIFF.c CHE.h CHE.c COM.h COM.c IGNO.h IGNO.c)
target_link_libraries(snifflib keeplib gitcompat dog abc-core)

add_executable(sniff-cli SNIFF.cli.c ../abc/FSW.c)
target_link_libraries(sniff-cli snifflib)
set_target_properties(sniff-cli PROPERTIES OUTPUT_NAME sniff RUNTIME_OUTPUT_DIRECTORY ${DOG_BIN_DIR})

add_subdirectory(test)
