cmake_minimum_required(VERSION 3.20)
project(librdx C CXX)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(..)
#find_package(sodium CONFIG)

set(FUZZ_CXX_FLAGS ${CMAKE_CXX_FLAGS} $<$<C_COMPILER_ID:Clang>:-fsanitize=fuzzer,address> -g -O2 -DABC_NOTRACE)
set(FUZZ_LDD_FLAGS ${CMAKE_LDD_FLAGS} $<$<C_COMPILER_ID:Clang>:-fsanitize=fuzzer,address> -DABC_NOTRACE)

add_executable(HEAP HEAP.c)
target_link_options(HEAP PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(HEAP PRIVATE ${FUZZ_CXX_FLAGS})

add_executable(HASHfuzz HASH.cpp)
target_link_options(HASHfuzz PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(HASHfuzz PRIVATE ${FUZZ_CXX_FLAGS})

add_executable(HASHdfuzz HASHd.c)
target_link_options(HASHdfuzz PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(HASHdfuzz PRIVATE ${FUZZ_CXX_FLAGS})

add_executable(ZINT ZINT.c)
target_link_options(ZINT PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(ZINT PRIVATE ${FUZZ_CXX_FLAGS})

add_executable(SORT SORT.c ../SORT.c)
target_link_options(SORT PRIVATE ${FUZZ_LDD_FLAGS})
target_compile_options(SORT PRIVATE ${FUZZ_CXX_FLAGS})

#add_executable(SST SST.c)
#target_link_options(SST PRIVATE ${FUZZ_LDD_FLAGS})
#target_compile_options(SST PRIVATE ${FUZZ_CXX_FLAGS})
