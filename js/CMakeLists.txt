enable_language(CXX)

find_library(JSC_LIB
    NAMES javascriptcoregtk-6.0 javascriptcoregtk-4.1 javascriptcoregtk-4.0
    PATHS /usr/lib/x86_64-linux-gnu/ /usr/lib
)
find_path(JSC_INCLUDE_DIR
    NAMES JavaScriptCore/JSBase.h
    PATH_SUFFIXES webkitgtk-6.0/ webkitgtk-4.1/ webkitgtk-4.0/
)

if(NOT JSC_LIB OR NOT JSC_INCLUDE_DIR)
    message(WARNING "js: JavaScriptCore not found, skipping")
    return()
endif()

set(TEST_CXX_FLAGS "-g")
set(TEST_LDD_FLAGS "-g")

include(CTest)

include_directories(
    ${JSC_INCLUDE_DIR}
    ..
    .
)

add_executable(jabc
    main.cpp
    io.cpp
    utf8.cpp
    convert.cpp
    test.cpp
    io.js.c
    test.js.c
)
target_link_libraries(jabc ${JSC_LIB} abc)

add_test(NAME JABCtest COMMAND jabc --eval "test.run()")
