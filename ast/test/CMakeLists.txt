cmake_minimum_required(VERSION 3.20)
project(ast-test C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(CTest)

include_directories(.. ../..)

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

add_executable(BASTtest BAST.c)
target_link_options(BASTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(BASTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(BASTtest bast abc)
add_test(NAME BASTtest COMMAND BASTtest)

add_executable(CSStest CSS.c)
target_link_options(CSStest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(CSStest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(CSStest bast abc)
add_test(NAME CSStest COMMAND CSStest)

add_executable(SPOTtest SPOT.c)
target_link_options(SPOTtest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SPOTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTtest bast abc)
add_test(NAME SPOTtest COMMAND SPOTtest)

add_executable(SPOTreTest SPOTre.c)
target_link_options(SPOTreTest PRIVATE ${TEST_LDD_FLAGS})
target_compile_options(SPOTreTest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTreTest bast abc)
add_test(NAME SPOTreTest COMMAND SPOTreTest)
