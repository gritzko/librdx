cmake_minimum_required(VERSION 3.20)
project(librdx C CXX)

set(CMAKE_C_STANDARD 23)
find_package(benchmark)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fPIC -fno-exceptions)
set(BENCH_CXX_FLAGS ${CMAKE_CXX_FLAGS} -O3 -v)
set(BENCH_LDD_FLAGS benchmark::benchmark)

include_directories(..)
#find_package(sodium CONFIG)

add_executable(HEAPbench HEAP.cpp)
target_compile_options(HEAPbench PRIVATE ${BENCH_CXX_FLAGS})
target_link_libraries(HEAPbench PRIVATE ${BENCH_LDD_FLAGS})

add_executable(SORTbench SORT.cpp ../SORT.c)
target_compile_options(SORTbench PRIVATE ${BENCH_CXX_FLAGS})
target_link_libraries(SORTbench PRIVATE ${BENCH_LDD_FLAGS})
