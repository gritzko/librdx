cmake_minimum_required(VERSION 3.10)
project(jar)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

include(CTest)

include_directories(
    ${WEBKIT_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ..
    .
)

add_executable(jar
    js_bindings.c
    io.c
    main.c
    io.js.c
)

target_link_libraries(jar
    ${WEBKIT_LIBRARIES}
    abc
)

if(NOT APPLE)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
endif()
