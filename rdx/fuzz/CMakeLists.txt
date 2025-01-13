cmake_minimum_required(VERSION 3.20)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(..)
#find_package(sodium CONFIG)

set(FUZZ_CXX_FLAGS ${CMAKE_CXX_FLAGS} $<$<C_COMPILER_ID:Clang>:-fsanitize=fuzzer,address> -g -O2 -DABC_NOTRACE)
set(FUZZ_LDD_FLAGS ${CMAKE_LDD_FLAGS} $<$<C_COMPILER_ID:Clang>:-fsanitize=fuzzer,address> -DABC_NOTRACE)

add_executable(JDRaa JDRaa.c)
target_link_options(JDRaa PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(JDRaa PRIVATE ${FUZZ_CXX_FLAGS})
target_link_libraries(JDRaa abc rdx)

