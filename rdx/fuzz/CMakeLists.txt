cmake_minimum_required(VERSION 3.20)
project(rdx_fuzz C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(../..)
include_directories(../../abc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZ_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUZZ_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUZZ_LDD_FLAGS}")

# Compile RDX sources directly so they get fuzzer coverage instrumentation
set(RDX_FUZZ_SRCS
    ../RDX.c ../MEM.c ../JDR.rl.c ../JDR.c ../WAL.c ../TLV.c
    ../UTF8.c ../DIFF.c ../SKIL.c ../SLIK.c ../VFY.c ../Y.c
    ../RB.c ../STRIP.c ../RAP.c
)

add_executable(VFYfuzz VFY.c ${RDX_FUZZ_SRCS})
target_link_libraries(VFYfuzz abc sodium)

add_executable(AAfuzz AA.c ${RDX_FUZZ_SRCS})
target_link_libraries(AAfuzz abc sodium)

add_executable(ABfuzz AB.c ${RDX_FUZZ_SRCS})
target_link_libraries(ABfuzz abc sodium)

add_executable(RBfuzz RB.c ${RDX_FUZZ_SRCS})
target_link_libraries(RBfuzz abc sodium)

add_executable(DRDXfuzz DRDX.c ${RDX_FUZZ_SRCS})
target_link_libraries(DRDXfuzz abc sodium)

add_executable(SKILfuzz SKIL.c ${RDX_FUZZ_SRCS})
target_link_libraries(SKILfuzz abc sodium)

add_executable(JDRfuzz JDR.c ${RDX_FUZZ_SRCS})
target_link_libraries(JDRfuzz abc sodium)

add_executable(SLIKfuzz SLIK.c ${RDX_FUZZ_SRCS})
target_link_libraries(SLIKfuzz abc sodium)

add_executable(SLIK2fuzz SLIK2.c ${RDX_FUZZ_SRCS})
target_link_libraries(SLIK2fuzz abc sodium)

add_executable(SLIK3fuzz SLIK3.c ${RDX_FUZZ_SRCS})
target_link_libraries(SLIK3fuzz abc sodium)
