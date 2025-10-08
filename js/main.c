#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/PRO.h"

thread_local JSGlobalContextRef JSctx;
thread_local JSObjectRef JSglobal;

JSValueRef JSPropertyMessage = NULL;
JSValueRef JSPropertyStack = NULL;

JSGlobalContextRef JSCreate() {
    JSctx = JSGlobalContextCreate(NULL);
    JSglobal = JSContextGetGlobalObject(JSctx);

    JSStringRef propMsg = JSStringCreateWithUTF8CString("message");
    JSPropertyMessage = JSValueMakeString(JSctx, propMsg);

    JSStringRef stackMsg = JSStringCreateWithUTF8CString("stack");
    JSPropertyMessage = JSValueMakeString(JSctx, stackMsg);

    return JSctx;
}

ok64 io_install();
ok64 utf8_install();

ok64 JSInstallModules() {
    sane(1);
    call(io_install);
    call(utf8_install);
    done;
}

void JSReport(JSValueRef exception) {
    char page[PAGE_SIZE], *msg;
    if (JSValueIsString(JSctx, exception)) {
        size_t len =
            JSStringGetUTF8CString((JSStringRef)exception, page, PAGE_SIZE);
        if (len > 0) len--;
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
        msg = page;
    } else {
        msg = "some exception of unknown nature";
    }
    fprintf(stderr, "uncaught exception: %s\n", msg);
}

void JSExecute(const char* script) {
    fprintf(stderr, "executing: %s\n", script);
    // Convert C string to JSC string
    JSStringRef js_code = JSStringCreateWithUTF8CString(script);

    JSValueRef exception = NULL;
    // Execute script with default options
    JSEvaluateScript(JSctx, js_code, NULL, NULL, 1, &exception);
    if (exception != NULL) JSReport(exception);

    // Cleanup JS string resources
    JSStringRelease(js_code);
}

int poll_loop();

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

    // Handle --eval
    if (eval_code != NULL) {
        JSGlobalContextRef ctx = JSCreate();
        JSInstallModules();
        JSExecute(eval_code);
        poll_loop();
        JSGlobalContextRelease(ctx);
        return 0;
    }

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
    JSGlobalContextRef ctx = JSCreate();

    JSInstallModules();

    // Execute script and run event loop
    JSExecute(script);
    // TODO one global context?
    poll_loop();

    // Cleanup
    JSGlobalContextRelease(ctx);
    free(script);

    return 0;
}
