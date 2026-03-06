cmake_minimum_required(VERSION 3.20)
project(be_test C)
set(CMAKE_C_STANDARD 23)
include(CTest)
include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(BEtest BE.c)
target_link_options(BEtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BEtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BEtest beagle bast abc)
add_test(NAME BEtest COMMAND BEtest)

add_executable(VERtest VER.c)
target_link_options(VERtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(VERtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(VERtest beagle bast abc)
add_test(NAME VERtest COMMAND VERtest)

add_test(NAME BEclitest COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/cli-test.sh
         $<TARGET_FILE:be-cli>)

if (WITH_INET)
add_executable(BESRVtest BESRV.c ../../be/BESRV.c)
target_link_options(BESRVtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BESRVtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BESRVtest beagle bast abc)
add_test(NAME BESRVtest COMMAND BESRVtest)

add_executable(BESYNCtest BESYNC.c ../../be/BESRV.c)
target_link_options(BESYNCtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BESYNCtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BESYNCtest beagle bast abc)
add_test(NAME BESYNCtest COMMAND BESYNCtest)
endif()
