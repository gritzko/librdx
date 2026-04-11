add_executable(GURItest GURI.c)
target_compile_options(GURItest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(GURItest beaglelib dog abc-core)
add_test(NAME GURItest COMMAND GURItest)
