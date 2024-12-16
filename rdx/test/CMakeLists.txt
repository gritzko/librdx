cmake_minimum_required(VERSION 3.20)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(LSMtest LSM.c)
target_link_options(LSMtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(LSMtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME LSMtest COMMAND LSMtest)

add_executable(RDXtest RDX.c)
target_link_options(RDXtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME RDXtest COMMAND RDXtest)

add_executable(RDXJtest RDXJ.c ../RDXJ.rl.c ../RDXJ.c ../ryu/d2s.c)
target_link_options(RDXJtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXJtest PRIVATE ${TEST_CXX_FLAGS})
add_test(NAME RDXJtest COMMAND RDXJtest)

add_executable(RDXZtest RDXZ.c ../RDXJ.rl.c ../RDXJ.c ../ryu/d2s.c)
target_link_options(RDXZtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXZtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXZtest libabc)
add_test(NAME RDXZtest COMMAND RDXZtest ${CMAKE_CURRENT_SOURCE_DIR}/RDXZ.rdxj)

add_executable(RDXYtest RDXY.c ../RDXJ.rl.c ../RDXJ.c ../ryu/d2s.c)
target_link_options(RDXYtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXYtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXYtest libabc)
add_test(NAME RDXYtestFIRST COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/RDXY.FIRST.rdxj)
add_test(NAME RDXYtestP COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/RDXY.P.rdxj)
add_test(NAME RDXYtestE COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/RDXY.E.rdxj)
add_test(NAME RDXYtestX COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/RDXY.X.rdxj)


