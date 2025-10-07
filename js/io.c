#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "JS.h"
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
typedef struct {
    struct pollfd poll;
    int type;  //  0 file
    u64 timeout;
    JSContextRef ctx;
    JSObjectRef machine;
} JSfd;

short io_callback(poller* p) { return 0; }
void io_finalize(JSObjectRef object) {}

int poll_loop(int ms) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    /*while (ms > 0) {
        struct pollfd polls[POLL_MAX_FILES];
        int pollscount = 0;
        eatB(voidp, j, pollers) polls[pollscount++] = *(struct pollfd*)j;
        poll(polls, pollscount, ms);
        for (int i = 0; i < pollscount; i++) {
            if (!polls[i].revents) continue;
            //...
        }
    }*/
    return 0;
}

void io_install() {
    JS_API_OBJECT(io, "io");
    JS_INSTALL_FN(io, "StdIn", io_std_in);
    JS_INSTALL_FN(io, "StdErr", io_std_err);
    JS_INSTALL_FN(io, "StdOut", io_std_out);
}

JSfd* find_fd(JSObjectRef thisObject) { return NULL; }

JSValueRef throw_error(JSContextRef ctx, const char* msg,
                       JSValueRef* exception) {
    JSStringRef errMsg = JSStringCreateWithUTF8CString(msg);
    *exception = JSValueMakeString(ctx, errMsg);
    JSStringRelease(errMsg);
    return JSValueMakeUndefined(ctx);
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

    JSfd* fd = JSObjectGetPrivate(thisObject);
    if (fd == nil)
        return throw_error(ctx, "this object is not a file", exception);

    int wn = write(fd->poll.fd, bytes, len);
    // fprintf(stderr, "fd %i len %lu ret %i\n", fd->poll.fd, len, wn);
    if (wn >= 0) {
        fd->poll.events |= POLLOUT;
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

    JSfd* fd = find_fd(thisObject);
    if (fd == nil)
        return throw_error(ctx, "this object is not a file", exception);

    int rn = read(fd->poll.fd, bytes, len);
    if (rn >= 0) {
        fd->poll.events |= POLLIN;
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
        poller p = {.fd = {.fd = which},
                    .callback = io_callback,
                    .payload = fileObject,
                    .timeout = POLnever};
        ok64 o = POLtrack(&p);
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
