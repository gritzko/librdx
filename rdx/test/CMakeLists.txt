cmake_minimum_required(VERSION 3.20)
project(librdx C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

#find_package(sodium CONFIG)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(RDXtest RDX.c RDX.c)
target_link_options(RDXtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RDXtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RDXtest abc rdx2)
add_test(NAME RDXtest COMMAND RDXtest)

add_executable(ABtest AB.c AB.c)
target_link_options(ABtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(ABtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(ABtest abc rdx2)
add_test(NAME ABtest COMMAND ABtest)

add_executable(VFYtest VFY.c)
target_link_options(VFYtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(VFYtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(VFYtest abc rdx2)
add_test(NAME VFYtest COMMAND VFYtest)

add_executable(SKILtest SKIL.c)
target_link_options(SKILtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SKILtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SKILtest abc rdx2)
add_test(NAME SKILtest COMMAND SKILtest)

add_executable(SLIKtest SLIK.c)
target_link_options(SLIKtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SLIKtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SLIKtest abc rdx2)
add_test(NAME SLIKtest COMMAND SLIKtest)

#add_executable(SLIKPACKtest SLIKPACK.c)
#target_link_options(SLIKPACKtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(SLIKPACKtest PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(SLIKPACKtest abc rdx2)
#add_test(NAME SLIKPACKtest COMMAND SLIKPACKtest)

#add_executable(SLIK2test SLIK2.c)
#target_link_options(SLIK2test PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(SLIK2test PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(SLIK2test abc rdx2)
#add_test(NAME SLIK2test COMMAND SLIK2test)

add_executable(YLOOKUPtest YLOOKUP.c)
target_link_options(YLOOKUPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(YLOOKUPtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(YLOOKUPtest abc rdx2)
add_test(NAME YLOOKUPtest COMMAND YLOOKUPtest)

#if (WITH_INET)
#add_test(NAME HTTPtest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/http-test.sh $<TARGET_FILE:rdx-cli>)
#endif()

#add_test(NAME Querytest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/query-test.sh $<TARGET_FILE:rdx-cli>)

add_executable(JDRtest JDR.c)
target_link_options(JDRtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(JDRtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(JDRtest abc rdx2)
add_test(NAME JDRtest COMMAND JDRtest)

add_executable(RBtest RB.c)
target_link_options(RBtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RBtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RBtest abc rdx2)
add_test(NAME RBtest COMMAND RBtest)

add_executable(STRIPtest STRIP.c)
target_link_options(STRIPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(STRIPtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(STRIPtest abc rdx2)
add_test(NAME STRIPtest COMMAND STRIPtest)

add_executable(RAPtest RAP.c)
target_link_options(RAPtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(RAPtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(RAPtest abc rdx2)
add_test(NAME RAPtest COMMAND RAPtest)

add_executable(rdxDIFFtest DIFF.c)
target_link_options(rdxDIFFtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(rdxDIFFtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(rdxDIFFtest abc rdx2)
add_test(NAME rdxDIFFtest COMMAND rdxDIFFtest)

#add_executable(BRIXtest LSM.c)
#target_link_options(BRIXtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(BRIXtest PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(BRIXtest abc rdx2)
#add_test(NAME BRIXtest COMMAND BRIXtest)

#add_executable(RDXZtest RDXZ.c)
#target_link_options(RDXZtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(RDXZtest PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(RDXZtest abc rdx)
#add_test(NAME RDXZtest COMMAND RDXZtest ${CMAKE_CURRENT_SOURCE_DIR}/z.md)
#
#add_executable(RDXYtest RDXY.c)
#target_link_options(RDXYtest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(RDXYtest PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(RDXYtest abc rdx)
#add_test(NAME RDXYtestFIRST COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.FIRST.md)
#add_test(NAME RDXYtestP COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.P.md)
#add_test(NAME RDXYtestE COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.E.md)
#add_test(NAME RDXYtestX COMMAND RDXYtest ${CMAKE_CURRENT_SOURCE_DIR}/y.X.md)

#add_executable(RDX2MergeTest RDX2.c)
#target_link_options(RDX2MergeTest PRIVATE ${TEST_LDD_FLAGS})
#target_compile_options(RDX2MergeTest PRIVATE ${TEST_CXX_FLAGS})
#target_link_libraries(RDX2MergeTest abc rdx)
#file(GLOB JDRS ${CMAKE_CURRENT_SOURCE_DIR}/*.jdr)
#add_test(NAME RDX2MergeTest COMMAND RDX2MergeTest ${JDRS})


