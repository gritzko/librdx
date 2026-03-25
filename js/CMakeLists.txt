enable_language(CXX)

if(APPLE)
    find_library(JSC_LIB JavaScriptCore)
    # macOS: headers are in the framework, no extra include path needed
    set(JSC_INCLUDE_DIR "")
else()
    find_library(JSC_LIB
        NAMES javascriptcoregtk-4.1 javascriptcoregtk-4.0
        PATHS /usr/lib/x86_64-linux-gnu/ /usr/lib
    )
    find_path(JSC_INCLUDE_DIR
        NAMES JavaScriptCore/JSBase.h
        PATH_SUFFIXES webkitgtk-4.1/ webkitgtk-4.0/
    )
endif()

if(NOT JSC_LIB)
    message(WARNING "js: JavaScriptCore not found, skipping")
    return()
endif()

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
