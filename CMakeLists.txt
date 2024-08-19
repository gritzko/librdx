cmake_minimum_required(VERSION 3.27)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

add_executable(ron64 ron64.c)

add_executable(lex2rl lex2rl.c LEX.rl.c LEX.c)

add_executable(md2html md2html.c LEX.rl.c LEX.c MARK.rl.c MARK.c MARK2.rl.c MARK2.c)

add_subdirectory(test)

add_subdirectory(fuzz EXCLUDE_FROM_ALL)
