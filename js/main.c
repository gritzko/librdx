#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSContextRef.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/POL.h"
#include "abc/PRO.h"

thread_local JSGlobalContextRef JSctx;
thread_local JSObjectRef JSglobal;

JSValueRef JSPropertyMessage = NULL;
JSValueRef JSPropertyStack = NULL;

JSGlobalContextRef JSInit() {
    JSctx = JSGlobalContextCreate(NULL);
    JSGlobalContextRetain(JSctx);
    JSglobal = JSContextGetGlobalObject(JSctx);

    JSStringRef propMsg = JSStringCreateWithUTF8CString("message");
    JSPropertyMessage = JSValueMakeString(JSctx, propMsg);

    JSStringRef stackMsg = JSStringCreateWithUTF8CString("stack");
    JSPropertyMessage = JSValueMakeString(JSctx, stackMsg);

    return JSctx;
}

void JSClose() { JSGlobalContextRelease(JSctx); }

ok64 io_install();
ok64 utf8_install();

ok64 JSInstallModules() {
    sane(1);
    call(io_install);
    call(utf8_install);
    done;
}

// Utility: Convert JSStringRef to null-terminated C string
char* JSStringRefToCString(JSStringRef str) {
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* buffer = (char*)malloc(maxSize);
    if (!buffer) return NULL;
    JSStringGetUTF8CString(str, buffer, maxSize);
    return buffer;
}

// Main: Dump all enumerable properties of a JS object
void JSObjectPropertiesDump(JSContextRef ctx, JSObjectRef obj) {
    if (!JSValueIsObject(ctx, obj)) {
        printf("Not an object.\n");
        return;
    }

    JSPropertyNameArrayRef props = JSObjectCopyPropertyNames(ctx, obj);
    size_t count = JSPropertyNameArrayGetCount(props);

    for (size_t i = 0; i < count; i++) {
        JSStringRef nameRef = JSPropertyNameArrayGetNameAtIndex(props, i);
        char* name = JSStringRefToCString(nameRef);
        JSValueRef exception = NULL;
        JSValueRef value = JSObjectGetProperty(ctx, obj, nameRef, &exception);
        JSStringRef valueStrRef = JSValueToStringCopy(ctx, value, &exception);
        char* valueStr = JSStringRefToCString(valueStrRef);

        printf("\t%s: %s\n", name ? name : "(null)",
               valueStr ? valueStr : "(null)");

        free(name);
        free(valueStr);
        JSStringRelease(valueStrRef);
    }

    JSPropertyNameArrayRelease(props);
}

void JSDump(JSContextRef ctx, JSValueRef exception) {
    JSStringRef errorStr = JSValueToStringCopy(ctx, exception, NULL);
    size_t errorSize = JSStringGetMaximumUTF8CStringSize(errorStr);
    char* errorUTF8 = (char*)malloc(errorSize);
    JSStringGetUTF8CString(errorStr, errorUTF8, errorSize);

    printf("JSC error: %s\n", errorUTF8);

    free(errorUTF8);
    JSStringRelease(errorStr);
}

void JSReport(JSValueRef exception) {
    JS_TRACE("something is wrong");
    char page[PAGE_SIZE], *msg;
    if (JSValueIsString(JSctx, exception)) {
        size_t len =
            JSStringGetUTF8CString((JSStringRef)exception, page, PAGE_SIZE);
        if (len > 0) len--;
        msg = page;
    } else if (JSValueIsObject(JSctx, exception) &&
               JSObjectHasPropertyForKey(JSctx, (JSObjectRef)exception,
                                         JSPropertyStack, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JSctx, (JSObjectRef)exception, JSPropertyStack, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGE_SIZE);
        printf("LEN %li\n", len);
        msg = page;
    } else if (JSValueIsObject(JSctx, exception) &&
               JSObjectHasPropertyForKey(JSctx, (JSObjectRef)exception,
                                         JSPropertyMessage, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JSctx, (JSObjectRef)exception, JSPropertyMessage, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGE_SIZE);
        page[len - 1] = '\n';
        size_t len2 = JSStringGetUTF8CString((JSStringRef)exception, page + len,
                                             PAGE_SIZE - len);
        printf("LEN %li %li\n", len, len2);
        msg = page;
    } else if (JSValueIsObject(JSctx, exception)) {
        JSDump(JSctx, exception);
        JSObjectPropertiesDump(JSctx, exception);
        msg = "see above";
    } else {
        JSDump(JSctx, exception);
    }
    if (*msg) fprintf(stderr, "JavaScript exception: %s\n", msg);
}

void JSExecute(const char* script) {
    fprintf(stderr, "Starting:\n%s\n", script);
    // Convert C string to JSC string
    JSStringRef js_code = JSStringCreateWithUTF8CString(script);

    JSValueRef exception = NULL;
    // Execute script with default options
    JSEvaluateScript(JSctx, js_code, NULL, NULL, 1, &exception);
    if (exception != NULL) {
        JSReport(exception);
    } else {
        fprintf(stderr, "Finished normally\n");
    }

    // Cleanup JS string resources
    JSStringRelease(js_code);
}

#define HELP_BOILERPLATE ""
#define VERSION_BOILERPLATE ""

u8 _pro_depth = 0;

int main(int argc, char** argv) {
    char* eval_code = NULL;
    char* script_file = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            fprintf(stderr, VERSION_BOILERPLATE);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, HELP_BOILERPLATE);
            return 0;
        } else if (strcmp(argv[i], "--eval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --eval requires code argument\n");
                return 1;
            }
            eval_code = argv[i + 1];
            i++;  // Skip code argument
        } else {
            script_file = argv[i];
            break;
        }
    }

    /* Handle --eval
    if (eval_code != NULL) {
        JSGlobalContextRef ctx = JSCreate();
        JSInstallModules();
        JSExecute(eval_code);
        JSGlobalContextRelease(ctx);
        return 0;
    }*/

    // Handle script file
    if (!script_file) {
        fprintf(stderr, "Error: No script or --eval provided\n");
        fprintf(stderr, HELP_BOILERPLATE);
        return 1;
    }

    // Read JS file
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char* script = malloc(len + 1);
    fread(script, 1, len, f);
    script[len] = '\0';
    fclose(f);

    // Initialize runtime components
    JSGlobalContextRef ctx = JSInit();

    JSInstallModules();

    printf("Execute script and run event loop\n");
    JSExecute(script);
    // TODO one global context?
    POLLoop(POLNever);

    free(script);
    JSClose();

    return 0;
}
