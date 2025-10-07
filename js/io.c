#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/POL.h"

// SM convention
// .state = Closed Connecting Connected
// .ConnectedonRead()
// ._onRead()

// io.Connect() io.Listen() io.Open() io.Wait()...
// fd.Write(ta) -> int
// fd.Read(ta) -> int
// fd.XxxOnRead()
// fd.XxxOnWrite()
// fd.Close()

short io_callback(poller* p) { return 0; }
void io_finalize(JSObjectRef object) {}

JSValueRef JSPropertyFD = NULL;

int poll_loop(int ms) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return 0;
}

extern unsigned int ___js_io_js_len;
extern unsigned char ___js_io_js[];

JSValueRef throw_error(JSContextRef ctx, const char* msg,
                       JSValueRef* exception) {
    JSStringRef errMsg = JSStringCreateWithUTF8CString(msg);
    *exception = JSValueMakeString(ctx, errMsg);
    JSStringRelease(errMsg);
    return JSValueMakeUndefined(ctx);
}

poller* find_fd(JSContextRef ctx, JSObjectRef thisObject,
                JSValueRef* exception) {
    JSValueRef fdv =
        JSObjectGetPropertyForKey(ctx, thisObject, JSPropertyFD, NULL);
    double fdd = JSValueToNumber(ctx, fdv, exception);
    if (isnan(fdd)) return NULL;
    fprintf(stderr, "looging for %i\n", (int)fdd);
    return POLfind((int)fdd);
}

static JSClassRef fileClass = NULL;

// fd.Write(ta) -> int
JSValueRef io_file_write(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef thisObject, size_t argc,
                         const JSValueRef args[], JSValueRef* exception) {
    if (argc == 0) {
        return throw_error(ctx, "fo.Write() requires data to write", exception);
    }

    void* bytes = NULL;
    size_t len = 0;
    char page[4096];

    if (JSValueIsString(ctx, args[0])) {
        len = JSStringGetUTF8CString((JSStringRef)args[0], page, 4096);
        if (len > 0) len--;
        bytes = page;
    } else if (JSValueIsObject(ctx, args[0])) {
        bytes =
            JSObjectGetTypedArrayBytesPtr(ctx, (JSObjectRef)args[0], exception);
        if (*exception != NULL) return JSValueMakeUndefined(ctx);
        if (bytes == NULL)
            return throw_error(ctx, "not a TypedArray", exception);
        len = JSObjectGetTypedArrayByteLength(ctx, (JSObjectRef)args[0],
                                              exception);
    }

    poller* fd = find_fd(ctx, thisObject, exception);
    if (fd == NULL)
        return throw_error(ctx, "this object is not a file", exception);

    int wn = write(fd->fd.fd, bytes, len);
    // fprintf(stderr, "fd %i len %lu ret %i\n", fd->poll.fd, len, wn);
    if (wn >= 0) {
        fd->fd.events |= POLLOUT;
    } else {
        perror("write x");
        return throw_error(ctx, strerror(errno), exception);
    }

    return JSValueMakeNumber(ctx, wn);
}

// fd.Read(ta) -> int
JSValueRef io_file_read(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef thisObject, size_t argc,
                        const JSValueRef args[], JSValueRef* exception) {
    if (argc == 0) {
        return throw_error(ctx, "fo.Write() requires data to write", exception);
    }

    if (!JSValueIsObject(ctx, args[0]))
        return throw_error(ctx, "not a TypedArray", exception);

    void* bytes =
        JSObjectGetTypedArrayBytesPtr(ctx, (JSObjectRef)args[0], exception);
    if (*exception != NULL) return JSValueMakeUndefined(ctx);
    if (bytes == NULL) return throw_error(ctx, "not a TypedArray", exception);
    size_t len =
        JSObjectGetTypedArrayByteLength(ctx, (JSObjectRef)args[0], exception);

    poller* fd = find_fd(ctx, thisObject, exception);
    if (fd == NULL) throw_error(ctx, "this object is not a file", exception);

    int rn = read(fd->fd.fd, bytes, len);
    if (rn >= 0) {
        fd->fd.events |= POLLIN;
    } else {
        return throw_error(ctx, strerror(errno), exception);
    }

    return JSValueMakeNumber(ctx, rn);
}

JSValueRef io_file_close(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef thisObject, size_t argc,
                         const JSValueRef args[], JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSObjectRef JSio_std[3];

// io.StdErr() -> fd
JSValueRef io_std_io(JSContextRef ctx, JSObjectRef function,
                     JSObjectRef thisObject, size_t argc,
                     const JSValueRef args[], JSValueRef* exception,
                     int which) {
    if (fileClass == NULL) {
        JSClassDefinition classDef = kJSClassDefinitionEmpty;
        classDef.finalize = io_finalize;
        fileClass = JSClassCreate(&classDef);
    }
    if (JSio_std[which] == NULL) {
        JSObjectRef fileObject = JSObjectMake(ctx, fileClass, NULL);
        JSValueProtect(ctx, fileObject);
        JS_ADD_METHOD(fileObject, "Write", io_file_write);
        JS_ADD_METHOD(fileObject, "Read", io_file_read);
        JS_ADD_METHOD(fileObject, "Close", io_file_close);
        JSValueRef fd = JSValueMakeNumber(ctx, (double)which);
        JSObjectSetPropertyForKey(ctx, fileObject, JSPropertyFD, fd,
                                  kJSPropertyAttributeReadOnly, NULL);
        poller p = {.fd = {.fd = which},
                    .callback = io_callback,
                    .payload = fileObject,
                    .timeout = POLnever};
        ok64 o = POLtrack(&p);
        poller* test = POLfind(which);
        if (test == NULL) printf("oopsie\n");
        JSio_std[which] = fileObject;
    }
    return JSio_std[which];
}

JSValueRef io_std_in(JSContextRef ctx, JSObjectRef function,
                     JSObjectRef thisObject, size_t argc,
                     const JSValueRef args[], JSValueRef* exception) {
    return io_std_io(ctx, function, thisObject, argc, args, exception,
                     STDIN_FILENO);
}

JSValueRef io_std_out(JSContextRef ctx, JSObjectRef function,
                      JSObjectRef thisObject, size_t argc,
                      const JSValueRef args[], JSValueRef* exception) {
    return io_std_io(ctx, function, thisObject, argc, args, exception,
                     STDOUT_FILENO);
}

JSValueRef io_std_err(JSContextRef ctx, JSObjectRef function,
                      JSObjectRef thisObject, size_t argc,
                      const JSValueRef args[], JSValueRef* exception) {
    return io_std_io(ctx, function, thisObject, argc, args, exception,
                     STDERR_FILENO);
}

extern const char* io_js;

ok64 io_install() {
    POLinit();

    JSStringRef propFD = JSStringCreateWithUTF8CString("_fd");
    JSPropertyFD = JSValueMakeString(JSctx, propFD);

    JS_API_OBJECT(io, "io");
    JS_INSTALL_FN(io, "StdIn", io_std_in);
    JS_INSTALL_FN(io, "StdErr", io_std_err);
    JS_INSTALL_FN(io, "StdOut", io_std_out);
    // io.stderr.WriteLine("Hello world!")
    // response = io.stdin.ReadLine()
    // io.stdout.WriteLine("You said: '"+response+"'");

    JSExecute(io_js);

    return OK;
}
