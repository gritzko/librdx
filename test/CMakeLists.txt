cmake_minimum_required(VERSION 3.27)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(..)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(01test 01.c)
target_link_options(01test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(01test PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME 01test COMMAND 01test)

add_executable(Stest $.c)
target_link_options(Stest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(Stest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME $test COMMAND Stest)

add_executable(Btest B.c)
target_link_options(Btest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(Btest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME Btest COMMAND Btest)

add_executable(INTtest INT.c)
target_link_options(INTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(INTtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME INTtest COMMAND INTtest)

add_executable(PROtest PRO.c)
add_test(NAME PROtest COMMAND PROtest)

add_executable(HEAPtest HEAP.c)
target_link_options(HEAPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(HEAPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME HEAPtest COMMAND HEAPtest)

add_executable(TLVtest TLV.c)
target_link_options(TLVtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(TLVtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME TLVtest COMMAND TLVtest)

add_executable(LISTtest LIST.c)
target_link_options(LISTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(LISTtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME LISTtest COMMAND LISTtest)

add_executable(BINtest BIN.c)
target_link_options(BINtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BINtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME BINtest COMMAND BINtest)

add_executable(MMAPtest MMAP.c)
target_link_options(MMAPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MMAPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME MMAPtest COMMAND MMAPtest)

add_executable(FILEtest FILE.c)
target_link_options(FILEtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(FILEtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME FILEtest COMMAND FILEtest)

#add_executable(RDXtest RDX.c)
#target_link_options(RDXtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(RDXtest PRIVATE ${TEST_CXX_FLAGS})
#add_test(NAME RDXtest COMMAND RDXtest)

add_executable(SHAtest SHA.c)
target_link_libraries(SHAtest sodium)
target_link_options(SHAtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SHAtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME SHAtest COMMAND SHAtest)

add_executable(ZINTtest ZINT.c)
target_link_libraries(ZINTtest)
target_link_options(ZINTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(ZINTtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME ZINTtest COMMAND ZINTtest)

add_executable(LINEtest LINE.c)
target_link_libraries(LINEtest)
target_link_options(LINEtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(LINEtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME LINEtest COMMAND LINEtest)

#add_executable(CITRtest CITR.c)
#target_link_options(CITRtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(CITRtest PRIVATE ${TEST_CXX_FLAGS})
#add_test(NAME CITRtest COMMAND CITRtest)

add_executable(MARKtest MARK.c ../MARK.c ../MARK.rl.c)
target_link_options(MARKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(MARKtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME MARKtest COMMAND MARKtest)
