cmake_minimum_required(VERSION 3.20)
project(rdx C)

set(CMAKE_C_STANDARD 23)

# Buffer accessors return T** slice pointers into struct members;
# GCC -O3 strict aliasing breaks overlapping slice access patterns.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.)

option(WITH_JS "build with JavaScriptCore" OFF)

option(WITH_ASAN "build with ASAN" OFF)
if (WITH_ASAN)
    set(ASAN_FLAGS "-g -O1 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
endif ()

option(WITH_PROF "build with clang profile data" OFF)
if (WITH_PROF)
    set(PROF_FLAGS "-g -O1 -fprofile-instr-generate -fcoverage-mapping -DABC_INSANE")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROF_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROF_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROF_FLAGS}")
endif ()

option(WITH_IR "emit LLVM IR (.ll) instead of object code" OFF)
if (WITH_IR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -emit-llvm -S")
    # disable linking — IR files are not linkable
    set(CMAKE_C_LINK_EXECUTABLE "echo 'skip link: <TARGET>'")
    set(CMAKE_C_CREATE_STATIC_LIBRARY "echo 'skip ar: <TARGET>'")
endif ()

option(WITH_FUZZ "build with libFuzzer" OFF)
if (WITH_FUZZ)
    set(FUZZ_FLAGS "-fsanitize=fuzzer")
    set(FUZZ_CXX_FLAGS "${FUZZ_FLAGS}")
    set(FUZZ_C_FLAGS "${FUZZ_FLAGS}")
    set(FUZZ_LDD_FLAGS "${FUZZ_FLAGS}")
    add_compile_options(-fsanitize=fuzzer-no-link)
endif ()

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")
set(BENCH_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -v")
set(BENCH_LDD_FLAGS benchmark::benchmark)

add_subdirectory(abc)
add_subdirectory(dog)
add_subdirectory(bro)
add_subdirectory(spot)
add_subdirectory(graf)
#add_subdirectory(fork)
if (WITH_JS)
    add_subdirectory(js)
endif()
#add_subdirectory(cli)
