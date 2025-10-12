#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSContextRef.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/POL.h"
#include "abc/PRO.h"

thread_local JSGlobalContextRef JS_CONTEXT;
thread_local JSObjectRef JS_GLOBAL_OBJECT;

JSValueRef JSPropertyMessage = NULL;
JSValueRef JSPropertyStack = NULL;

JSGlobalContextRef JSInit() {
    JS_CONTEXT = JSGlobalContextCreate(NULL);
    //  JSGlobalContextRetain(JS_CONTEXT);
    JS_GLOBAL_OBJECT = JSContextGetGlobalObject(JS_CONTEXT);

    JSStringRef propMsg = JSStringCreateWithUTF8CString("message");
    JSPropertyMessage = JSValueMakeString(JS_CONTEXT, propMsg);
    JSStringRelease(propMsg);
    JSValueProtect(JS_CONTEXT, JSPropertyMessage);

    JSStringRef stackMsg = JSStringCreateWithUTF8CString("stack");
    JSPropertyStack = JSValueMakeString(JS_CONTEXT, stackMsg);
    JSStringRelease(stackMsg);
    JSValueProtect(JS_CONTEXT, JSPropertyStack);

    return JS_CONTEXT;
}

ok64 JARioInstall();
ok64 JARioUninstall();
ok64 JARutf8Install();
ok64 JARutf8Uninstall();

ok64 JARInstallModules() {
    sane(1);
    call(JARioInstall);
    call(JARutf8Install);
    done;
}

ok64 JARUninstallModules() {
    sane(1);
    call(JARioUninstall);
    call(JARutf8Uninstall);
    done;
}

void JARClose() {
    JSValueUnprotect(JS_CONTEXT, JSPropertyStack);
    JSValueUnprotect(JS_CONTEXT, JSPropertyMessage);
    JSGlobalContextRelease(JS_CONTEXT);
    JS_CONTEXT = NULL;
    JS_GLOBAL_OBJECT = NULL;
    JSPropertyMessage = NULL;
    JSPropertyStack = NULL;
}

char* JSStringRefToCString(JSStringRef str);

// Main: Dump all enumerable properties of a JS object
void JSObjectPropertiesDump(JSContextRef ctx, const JSObjectRef obj) {
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

void JARDump(JSContextRef ctx, JSValueRef exception) {
    JSStringRef errorStr = JSValueToStringCopy(ctx, exception, NULL);
    size_t errorSize = JSStringGetMaximumUTF8CStringSize(errorStr);
    char* errorUTF8 = (char*)malloc(errorSize);
    JSStringGetUTF8CString(errorStr, errorUTF8, errorSize);

    printf("JSC error: %s\n", errorUTF8);

    free(errorUTF8);
    JSStringRelease(errorStr);
}

void JARReport(JSValueRef exception) {
    JS_TRACE("something is wrong");
    char page[PAGE_SIZE], *msg;
    if (JSValueIsString(JS_CONTEXT, exception)) {
        size_t len =
            JSStringGetUTF8CString((JSStringRef)exception, page, PAGE_SIZE);
        if (len > 0) len--;
        msg = page;
    } else if (JSValueIsObject(JS_CONTEXT, exception) &&
               JSObjectHasPropertyForKey(JS_CONTEXT, (JSObjectRef)exception,
                                         JSPropertyStack, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JS_CONTEXT, (JSObjectRef)exception, JSPropertyStack, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGE_SIZE);
        printf("LEN %li\n", len);
        msg = page;
    } else if (JSValueIsObject(JS_CONTEXT, exception) &&
               JSObjectHasPropertyForKey(JS_CONTEXT, (JSObjectRef)exception,
                                         JSPropertyMessage, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JS_CONTEXT, (JSObjectRef)exception, JSPropertyMessage, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGE_SIZE);
        page[len - 1] = '\n';
        size_t len2 = JSStringGetUTF8CString((JSStringRef)exception, page + len,
                                             PAGE_SIZE - len);
        printf("LEN %li %li\n", len, len2);
        msg = page;
    } else if (JSValueIsObject(JS_CONTEXT, exception)) {
        JARDump(JS_CONTEXT, exception);
        JSObjectPropertiesDump(JS_CONTEXT, (JSObjectRef)exception);
        msg = "see above";
    } else {
        JARDump(JS_CONTEXT, exception);
    }
    if (*msg) fprintf(stderr, "JavaScript exception: %s\n", msg);
}

void JARExecute(const char* script) {
    fprintf(stderr, "Starting:\n%s\n", script);
    // Convert C string to JSC string
    JSStringRef js_code = JSStringCreateWithUTF8CString(script);

    JSValueRef exception = NULL;
    // Execute script with default options
    JSEvaluateScript(JS_CONTEXT, js_code, NULL, NULL, 1, &exception);
    if (exception != NULL) {
        JARReport(exception);
    } else {
        fprintf(stderr, "Finished normally\n");
    }

    // Cleanup JS string resources
    JSStringRelease(js_code);
}

#define HELP_BOILERPLATE ""
#define VERSJARIoN_BOILERPLATE ""

u8 _pro_depth = 0;

int main(int argc, char** argv) {
    char* eval_code = NULL;
    char* script_file = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            fprintf(stderr, VERSJARIoN_BOILERPLATE);
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

    JSInit();
    JARInstallModules();

    if (eval_code != NULL) {
        JARExecute(eval_code);
    }

    // Handle script file
    if (script_file) {
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

        JARExecute(script);

        free(script);
    }

    POLLoop(POLNever);

    JARUninstallModules();
    JARClose();

    return 0;
}
