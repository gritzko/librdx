cmake_minimum_required(VERSION 3.20)
project(json_fuzz C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(../..)
include_directories(../../abc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZ_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUZZ_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUZZ_LDD_FLAGS}")

add_executable(BIFFfuzz BIFF.c ../BIFF.c ../PASS.c ../BASON.c ../../abc/TLV.c)
target_link_libraries(BIFFfuzz abc)

add_executable(BIFFNfuzz BIFFN.c ../BIFF.c ../PASS.c ../BASON.c ../../abc/TLV.c)
target_link_libraries(BIFFNfuzz abc)
