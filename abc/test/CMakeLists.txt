cmake_minimum_required(VERSION 3.20)
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

add_executable(HASHtest HASH.c)
target_link_options(HASHtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(HASHtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME HASHtest COMMAND HASHtest)

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

add_executable(COMBtest COMB.c)
target_link_libraries(COMBtest)
target_link_options(COMBtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(COMBtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME COMBtest COMMAND COMBtest)

add_executable(ARENtest AREN.c)
target_link_libraries(ARENtest)
target_link_options(ARENtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(ARENtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME ARENtest COMMAND ARENtest)

add_executable(UTF8test UTF8.c ../UTF8.c)
target_link_libraries(UTF8test)
target_link_options(UTF8test PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(UTF8test PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME UTF8test COMMAND UTF8test)

add_executable(JSONtest JSON.c ../JSON.c ../JSON.rl.c)
target_link_options(JSONtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JSONtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME JSONtest COMMAND JSONtest)

add_executable(URItest URI.c ../URI.rl.c ../URI.c)
target_link_options(URItest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(URItest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME URItest COMMAND URItest)

add_executable(HTTPtest HTTP.c ../HTTP.rl.c ../HTTP.c)
target_link_options(HTTPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(HTTPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME HTTPtest COMMAND HTTPtest)

add_executable(UDPtest UDP.c)
target_link_options(UDPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(UDPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME UDPtest COMMAND UDPtest)

add_executable(TCPtest TCP.c)
target_link_options(TCPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(TCPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME TCPtest COMMAND TCPtest)

add_executable(POLLtest POLL.c ../POLL.c)
target_link_options(POLLtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(POLLtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME POLLtest COMMAND POLLtest)

add_executable(SKIPtest SKIP.c)
target_link_options(SKIPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SKIPtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME SKIPtest COMMAND SKIPtest)
