cmake_minimum_required(VERSION 3.20)
project(sniff C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. .)

add_library(snifflib STATIC SNIFF.h SNIFF.c)
target_link_libraries(snifflib abc-core)
