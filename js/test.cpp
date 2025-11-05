//
// Created by gritzko on 10/30/25.
//
#include "JABC.hpp"

JSObjectRef JABCTest = NULL;
extern const char* test_js;

ok64 JABCtestInstall() {
    JSStringRef fn = JSStringCreateWithUTF8CString("test");
    JABCTest = JSObjectMake(JABC_CONTEXT, NULL, NULL);
    JSObjectSetProperty(JABC_CONTEXT, JABC_GLOBAL_OBJECT, fn, JABCTest,
                        kJSPropertyAttributeNone, NULL);
    JSStringRelease(fn);
    JABCExecute(test_js);
    return OK;
}

ok64 JABCtestUninstall() { return OK; }
