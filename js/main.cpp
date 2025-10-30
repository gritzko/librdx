#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "JABC.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSContextRef.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSValueRef.h"

thread_local JSGlobalContextRef JABC_CONTEXT;
thread_local JSObjectRef JABC_GLOBAL_OBJECT;

JSValueRef JSPropertyMessage = NULL;
JSValueRef JSPropertyStack = NULL;

JSGlobalContextRef JSInit() {
    JABC_CONTEXT = JSGlobalContextCreate(NULL);
    //  JSGlobalContextRetain(JABC_CONTEXT);
    JABC_GLOBAL_OBJECT = JSContextGetGlobalObject(JABC_CONTEXT);

    JSStringRef propMsg = JSStringCreateWithUTF8CString("message");
    JSPropertyMessage = JSValueMakeString(JABC_CONTEXT, propMsg);
    JSStringRelease(propMsg);
    JSValueProtect(JABC_CONTEXT, JSPropertyMessage);

    JSStringRef stackMsg = JSStringCreateWithUTF8CString("stack");
    JSPropertyStack = JSValueMakeString(JABC_CONTEXT, stackMsg);
    JSStringRelease(stackMsg);
    JSValueProtect(JABC_CONTEXT, JSPropertyStack);

    return JABC_CONTEXT;
}

ok64 JABCioInstall();
ok64 JABCioUninstall();
ok64 JABCutf8Install();
ok64 JABCutf8Uninstall();
ok64 JABCbrixInstall();
ok64 JABCbrixUninstall();
ok64 JABCtestInstall();
ok64 JABCtestUninstall();

ok64 JABCInstallModules() {
    JABCioInstall();
    JABCutf8Install();
    JABCbrixInstall();
    JABCtestInstall();
    return 0;
}

ok64 JABCUninstallModules() {
    JABCioUninstall();
    JABCutf8Uninstall();
    JABCbrixUninstall();
    JABCtestUninstall();
    return 0;
}

void JABCClose() {
    JSValueUnprotect(JABC_CONTEXT, JSPropertyStack);
    JSValueUnprotect(JABC_CONTEXT, JSPropertyMessage);
    JSGlobalContextRelease(JABC_CONTEXT);
    JABC_CONTEXT = NULL;
    JABC_GLOBAL_OBJECT = NULL;
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

void JABCDump(JSContextRef ctx, JSValueRef exception) {
    JSStringRef errorStr = JSValueToStringCopy(ctx, exception, NULL);
    size_t errorSize = JSStringGetMaximumUTF8CStringSize(errorStr);
    char* errorUTF8 = (char*)malloc(errorSize);
    JSStringGetUTF8CString(errorStr, errorUTF8, errorSize);

    printf("JSC error: %s\n", errorUTF8);

    free(errorUTF8);
    JSStringRelease(errorStr);
}

void JABCReport(JSValueRef exception) {
    JS_TRACE("something is wrong");
    char page[PAGESIZE];
    const char* msg;
    if (JSValueIsString(JABC_CONTEXT, exception)) {
        size_t len =
            JSStringGetUTF8CString((JSStringRef)exception, page, PAGESIZE);
        if (len > 0) len--;
        msg = page;
    }
    if (JSValueIsObject(JABC_CONTEXT, exception) &&
        JSObjectHasPropertyForKey(JABC_CONTEXT, (JSObjectRef)exception,
                                  JSPropertyStack, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JABC_CONTEXT, (JSObjectRef)exception, JSPropertyStack, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGESIZE);
        printf("LEN %li\n", len);
        msg = page;
        fprintf(stderr, "JavaScript exception: %s\n", page);
    }
    if (JSValueIsObject(JABC_CONTEXT, exception) &&
        JSObjectHasPropertyForKey(JABC_CONTEXT, (JSObjectRef)exception,
                                  JSPropertyMessage, NULL)) {
        JSValueRef ref = JSObjectGetPropertyForKey(
            JABC_CONTEXT, (JSObjectRef)exception, JSPropertyMessage, NULL);
        size_t len = JSStringGetUTF8CString((JSStringRef)ref, page, PAGESIZE);
        page[len - 1] = '\n';
        size_t len2 = JSStringGetUTF8CString((JSStringRef)exception, page + len,
                                             PAGESIZE - len);
        printf("LEN %li %li\n", len, len2);
        fprintf(stderr, "JavaScript exception: %s\n", page);
    }
    if (JSValueIsObject(JABC_CONTEXT, exception)) {
        JABCDump(JABC_CONTEXT, exception);
        JSObjectPropertiesDump(JABC_CONTEXT, (JSObjectRef)exception);
        msg = "see above";
    } else {
        JABCDump(JABC_CONTEXT, exception);
    }
}

void JABCExecute(const char* script) {
    fprintf(stderr, "Starting:\n%s\n", script);
    // Convert C string to JSC string
    JSStringRef js_code = JSStringCreateWithUTF8CString(script);

    JSValueRef exception = NULL;
    // Execute script with default options
    JSEvaluateScript(JABC_CONTEXT, js_code, NULL, NULL, 1, &exception);
    if (exception != NULL) {
        JABCReport(exception);
    } else {
        fprintf(stderr, "Finished normally\n");
    }

    // Cleanup JS string resources
    JSStringRelease(js_code);
}

#define HELP_BOILERPLATE ""
#define VERSION_BOILERPLATE "jabc v0.0.1"

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

    JSInit();
    JABCInstallModules();

    if (eval_code != NULL) {
        JABCExecute(eval_code);
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
        char* script = (char*)malloc(len + 1);
        fread(script, 1, len, f);
        script[len] = '\0';
        fclose(f);

        JABCExecute(script);

        free(script);
    }

    POLLoop(POLNever);

    JABCUninstallModules();
    JABCClose();

    return 0;
}
