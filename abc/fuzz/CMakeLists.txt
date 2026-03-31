cmake_minimum_required(VERSION 3.20)
project(librdx C CXX)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include_directories(../..)
include_directories(..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUZZ_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUZZ_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FUZZ_LDD_FLAGS}")

add_executable(HEAPfuzz HEAP.c ../RON.c)

add_executable(HASHfuzz HASH.cpp)

add_executable(HASHdfuzz HASHd.c)

add_executable(ZINTfuzz ZINT.c "${CMAKE_CURRENT_SOURCE_DIR}/../ZINT.c")

add_executable(ZINT2fuzz ZINT2.c)
target_link_libraries(ZINT2fuzz abc)

add_executable(SORTfuzz SORT.c ../SORT.c ../RON.c)

add_executable(DIFFfuzz DIFF.c ../RON.c)

add_executable(URIfuzz URI.c ../URI.c ../URI.rl.c)

add_executable(SLOGfuzz SLOG.c ../SLOG.c)
target_link_libraries(SLOGfuzz abc)

add_executable(NFAfuzz NFA.c ../RON.c)

add_executable(MSETfuzz MSET.c ../RON.c)

#add_executable(SST SST.c)
#target_link_options(SST PRIVATE "${FUZZ_LDD_FLAGS}")
#target_compile_options(SST PRIVATE "${FUZZ_C_FLAGS}")
