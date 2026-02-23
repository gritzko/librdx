cmake_minimum_required(VERSION 3.20)
project(rdx_fuzz C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(../..)
include_directories(../../abc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZ_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUZZ_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUZZ_LDD_FLAGS}")

add_executable(VFYfuzz VFY.c)
target_link_libraries(VFYfuzz abc rdx2 sodium)

add_executable(AAfuzz AA.c)
target_link_libraries(AAfuzz abc rdx2 sodium)

add_executable(ABfuzz AB.c)
target_link_libraries(ABfuzz abc rdx2 sodium)

add_executable(RBfuzz RB.c)
target_link_libraries(RBfuzz abc rdx2 sodium)

add_executable(DRDXfuzz DRDX.c)
target_link_libraries(DRDXfuzz abc rdx2 sodium)

add_executable(SKILfuzz SKIL.c)
target_link_libraries(SKILfuzz abc rdx2 sodium)

add_executable(JDRfuzz JDR.c)
target_link_libraries(JDRfuzz abc rdx2 sodium)

add_executable(SLIKfuzz SLIK.c)
target_link_libraries(SLIKfuzz abc rdx2 sodium)

add_executable(SLIK2fuzz SLIK2.c)
target_link_libraries(SLIK2fuzz abc rdx2 sodium)

add_executable(SLIK3fuzz SLIK3.c)
target_link_libraries(SLIK3fuzz abc rdx2 sodium)

