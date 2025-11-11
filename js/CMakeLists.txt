cmake_minimum_required(VERSION 3.10)
project(jabc)

set(CMAKE_C_STANDARD 11)

find_library(JSC_LIB
    NAMES javascriptcoregtk-4.1 javascriptcoregtk-4.0
    PATH_SUFFIXES 
    PATHS /usr/lib/x86_64-linux-gnu/ /usr/lib
  )
find_path(JSC_INCLUDE_DIR
    NAMES JavaScriptCore/JSBase.h
    PATH_SUFFIXES webkitgtk-4.1/ webkitgtk-4.0/
  )

#TODO MacOS paths

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

include(CTest)

include_directories(
  ${JSC_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ..
    .
)
add_executable(jabc
    io.cpp
    main.cpp
    io.js.c
    test.js.c
    utf8.cpp
    brix.cpp test.cpp )

target_link_libraries(jabc
    ${JSC_LIB}
    abc
    rdx
)
