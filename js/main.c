#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JS.h"

thread_local JSGlobalContextRef JSctx;
thread_local JSObjectRef JSglobal;

void io_install();

JSGlobalContextRef create_js_context() {
    JSctx = JSGlobalContextCreate(NULL);
    JSglobal = JSContextGetGlobalObject(JSctx);

    io_install();

    return JSctx;
}

void execute_js(JSGlobalContextRef ctx, const char* script) {
    // Convert C string to JSC string
    JSStringRef js_code = JSStringCreateWithUTF8CString(script);

    JSValueRef exception = NULL;
    char page[128], *msg;
    size_t page_len = 128;
    // Execute script with default options
    JSEvaluateScript(ctx, js_code, NULL, NULL, 1, &exception);
    if (exception != NULL) {
        if (JSValueIsString(ctx, exception)) {
            size_t len =
                JSStringGetUTF8CString((JSStringRef)exception, page, page_len);
            if (len > 0) len--;
            msg = page;
        } else {
            msg = "some exception";
        }
        fprintf(stderr, "uncaught exception: %s\n", msg);
    }
    // Cleanup JS string resources
    JSStringRelease(js_code);
}

int poll_loop();

#define HELP_BOILERPLATE ""
#define VERSION_BOILERPLATE ""

int _pro_depth = 0;

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
        JSGlobalContextRef ctx = create_js_context();
        execute_js(ctx, eval_code);
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
    JSGlobalContextRef ctx = create_js_context();

    // Execute script and run event loop
    execute_js(ctx, script);
    // TODO one global context?
    poll_loop();

    // Cleanup
    JSGlobalContextRelease(ctx);
    free(script);

    return 0;
}
