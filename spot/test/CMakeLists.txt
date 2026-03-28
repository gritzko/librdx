add_executable(CAPOtest CAPO.c)
target_compile_options(CAPOtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(CAPOtest spotlib tok abc-core)
add_test(NAME CAPOtest COMMAND CAPOtest)

add_executable(SPOTtest SPOT.c)
target_compile_options(SPOTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTtest spotlib tok abc-core)
add_test(NAME SPOTtest COMMAND SPOTtest)

add_executable(SPOTreTest SPOTre.c)
target_compile_options(SPOTreTest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTreTest spotlib tok abc-core)
add_test(NAME SPOTreTest COMMAND SPOTreTest)
