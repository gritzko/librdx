cmake_minimum_required(VERSION 3.20)
project(librdx C CXX)

set(CMAKE_C_STANDARD 23)
find_package(benchmark)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fPIC -fno-exceptions)
set(BENCH_CXX_FLAGS ${CMAKE_CXX_FLAGS} -O3 -v)
set(BENCH_LDD_FLAGS -pthread benchmark::benchmark)

include_directories(..)
#find_package(sodium CONFIG)

add_executable(SKILbench SKIL.cpp)
target_compile_options(SKILbench PRIVATE ${BENCH_CXX_FLAGS})
target_link_libraries(SKILbench PRIVATE ${BENCH_LDD_FLAGS} abc rdx2)

