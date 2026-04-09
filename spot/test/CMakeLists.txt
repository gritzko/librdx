add_executable(CAPOtest CAPO.c)
target_compile_options(CAPOtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(CAPOtest spotlib dog abc-core)
add_test(NAME CAPOtest COMMAND CAPOtest)

add_executable(SPOTtest SPOT.c)
target_compile_options(SPOTtest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTtest spotlib dog abc-core)
add_test(NAME SPOTtest COMMAND SPOTtest)

add_executable(SPOTreTest SPOTre.c)
target_compile_options(SPOTreTest PRIVATE ${TEST_CXX_FLAGS})
target_link_libraries(SPOTreTest spotlib dog abc-core)
add_test(NAME SPOTreTest COMMAND SPOTreTest)

add_test(NAME SPOTcli_slice_init
    COMMAND bash -c "$<TARGET_FILE:spot-cli> -s 'T n = {a[0],a[1]};' \
        spot/test/data/slice_init.c 2>/dev/null \
        | diff -u spot/test/data/slice_init.spot -"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Hili regression: forces SPOT_COLOR=1 so the producer emits per-token
# hili spans, then HUNKu8sFeedText turns them into git-diff '+' line
# prefixes.  Catches both:
#   - lowercase placeholder accidentally matching multi-token groups
#     like (*fp)  (line 3 must NOT have '+')
#   - dedup of overlapping match ranges from the SPOT engine
#     (line 1 must NOT have '+')
add_test(NAME SPOTcli_slice_init_hili
    COMMAND bash -c "SPOT_COLOR=1 $<TARGET_FILE:spot-cli> \
        -s 'T n = {a[0],a[1]};' spot/test/data/slice_init.c 2>/dev/null \
        | diff -u spot/test/data/slice_init.hili -"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
