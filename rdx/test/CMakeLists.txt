cmake_minimum_required(VERSION 3.20)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(RDXtest RDX.c ../RDX.c)
target_link_options(RDXtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXtest abc rdx)
add_test(NAME RDXtest COMMAND RDXtest)

add_executable(JDRtest JDR.c)
target_link_options(JDRtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JDRtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(JDRtest abc rdx)
add_test(NAME JDRtest COMMAND JDRtest ${CMAKE_CURRENT_SOURCE_DIR}/jdr.eq.md)

add_executable(RDXZtest RDXZ.c)
target_link_options(RDXZtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXZtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXZtest abc rdx)
add_test(NAME RDXZtest COMMAND RDXZtest ${CMAKE_CURRENT_SOURCE_DIR}/z.md)

add_executable(RDXYtest RDXY.c)
target_link_options(RDXYtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXYtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXYtest abc rdx)
add_test(NAME RDXYtestFIRST COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.FIRST.md)
add_test(NAME RDXYtestP COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.P.md)
add_test(NAME RDXYtestE COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.E.md)
add_test(NAME RDXYtestX COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.X.md)


