add_executable(CAPOtest CAPO.c)
target_compile_options(CAPOtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(CAPOtest spotlib bast abc tree-sitter)
add_test(NAME CAPOtest COMMAND CAPOtest)
