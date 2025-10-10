#include <math.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/FILE.h"
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

short io_callback(int fd, poller* p) { return 0; }
void io_file_finalize(JSObjectRef object) {}
void io_map_finalize(JSObjectRef object) {}

int find_fd(JSContextRef ctx, JSObjectRef self, JSValueRef* exception) {
    JS_MAKE_STRING(fdkey, "_fd");
    JS_GET_PROPERTY(fdv, self, fdkey);
    JS_TO_NUMBER(fdd, fdv);
    if (isnan(fdd)) return -1;
    return (int)fdd;
}

static JSClassRef JSIOFileClass = NULL;
static JSClassRef JSIOMapClass = NULL;

// fd.Write(ta) -> int
JS_DEFINE_FN(io_file_write) {
    if (argc == 0) JS_THROW("fo.Write() requires data to write");

    void* bytes = NULL;
    size_t len = 0;
    char page[4096];

    // poller* fd = find_fd(ctx, self, exception);
    // if (fd == NULL) JS_THROW("this object is not a file");
    int fd;

    u8cs ta;

    if (JS_ARG_IS_STRING(0)) {  // TODO io.utf8()
        len = JSStringGetUTF8CString((JSStringRef)args[0], page, 4096);
        if (len > 0) len--;
        bytes = page;
    } else if (JS_ARG_IS_TARRAY(0)) {
        JS_ARG_TA_u8s(0, ta);
    }

    int wn = write(fd, ta[0], $len(ta));
    // fprintf(stderr, "fd %i len %lu ret %i\n", fd->poll.fd, len, wn);
    if (wn >= 0) {
        POLAddEvents(fd, POLLOUT);
    } else {
        perror("write x");
        JS_THROW(strerror(errno));
    }

    JS_MAKE_NUMBER(ret, wn);
    return ret;
}

// fd.Read(ta) -> int
JS_DEFINE_FN(io_file_read) {
    if (argc == 0) JS_THROW("fo.Write() requires data to write");

    if (!JS_ARG_IS_TARRAY(0)) JS_THROW("not a TypedArray");

    u8s ta = {};
    JS_ARG_TA_u8s(0, ta);

    int fd = find_fd(ctx, self, exception);
    // if (fd == NULL) JS_THROW("this object is not a file");

    int rn = read(fd, *ta, $len(ta));
    if (rn >= 0) {
        POLAddEvents(fd, POLLIN);
    } else {
        JS_THROW(strerror(errno));
    }

    JS_MAKE_NUMBER(num, rn);
    return num;
}

JS_DEFINE_FN(io_file_close) {
    JS_UNPROTECT(self);
    JS_MAKE_UNDEFINED(u);
    return u;
}

JSObjectRef JSTimer = NULL;

int timeout_cb(int fd) {
    if (JSTimer == NULL) return INT32_MAX;
    JSValueRef exception = NULL;  // todo
    JSValueRef ret =
        JSObjectCallAsFunction(JSctx, JSTimer, NULL, 0, NULL, &exception);
    int next = 1000;
    if (JSValueIsNumber(JSctx, ret)) {
        next = JSValueToNumber(JSctx, ret, NULL);
    }
    return next;
}

// io.timer(fn)
JSValueRef io_timer(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                    size_t argc, const JSValueRef args[],
                    JSValueRef* exception) {
    JS_TRACE("io_timer");
    if (argc < 1 || !JSValueIsObject(ctx, args[0]) ||
        !JSObjectIsFunction(ctx, (JSObjectRef)args[0])) {
        JS_THROW("io.timer(function)");
    }
    if (JSTimer != NULL) JSValueUnprotect(JSctx, JSTimer);
    JSTimer = (JSObjectRef)args[0];
    POLTrackTime(timeout_cb);
    JSValueProtect(ctx, JSTimer);
    return JSValueMakeUndefined(ctx);
}

// io.wake(ms)
JSValueRef io_wake(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[],
                   JSValueRef* exception) {
    if (argc != 1 || !JSValueIsNumber(ctx, args[0])) {
        JS_THROW("io.wakeTimer(ms)");
    }
    if (JSTimer == NULL) JS_THROW("no timer set");
    double ms = JSValueToNumber(ctx, args[0], exception);
    // get function
    JSObjectRef fn = (JSObjectRef)args[0];
    POLAddTime((int)ms);
    return JSValueMakeUndefined(ctx);
}

JSValueRef io_log(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                  size_t argc, const JSValueRef args[], JSValueRef* exception) {
    if (argc != 1 || !JSValueIsString(ctx, args[0])) {
        JS_THROW("io.log(string)");
    }
    JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* bytes = malloc(maxSize);
    size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
    fputs(bytes, stderr);
    fputc('\n', stderr);
    free(bytes);
    return JSValueMakeUndefined(ctx);
}

JSValueRef io_net_listen(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSValueRef io_net_accept(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSValueRef io_net_connect(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef self, size_t argc,
                          const JSValueRef args[], JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSValueRef io_net_close(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef self, size_t argc, const JSValueRef args[],
                        JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSObjectRef JSio_std[3];

// io.StdErr() -> fd
JSValueRef io_std_io(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                     size_t argc, const JSValueRef args[],
                     JSValueRef* exception, int which) {
    if (JSio_std[which] == NULL) {
        JSObjectRef fileObject = JSObjectMake(ctx, JSIOFileClass, NULL);
        JS_PROTECT(fileObject);
        JS_ADD_METHOD(fileObject, "Write", io_file_write);
        JS_ADD_METHOD(fileObject, "Read", io_file_read);
        JS_ADD_METHOD(fileObject, "Close", io_file_close);
        JS_MAKE_NUMBER(fd, which);
        JS_MAKE_STRING(key, "_fd");
        JS_SET_PROPERTY_RO(fileObject, key, fd);
        poller p = {
            .callback = io_callback, .payload = fileObject, .timeout_ms = 1000};
        ok64 o = POLTrackEvents(which, p);
        JSio_std[which] = fileObject;
    }
    return JSio_std[which];
}

JS_DEFINE_FN(io_std_in) {
    return io_std_io(ctx, function, self, argc, args, exception, STDIN_FILENO);
}

JS_DEFINE_FN(io_std_out) {
    return io_std_io(ctx, function, self, argc, args, exception, STDOUT_FILENO);
}

JS_DEFINE_FN(io_std_err) {
    return io_std_io(ctx, function, self, argc, args, exception, STDERR_FILENO);
}

void MMapDeallocator(void* bytes, void* deallocatorContext) {
    Bu8 buf = {bytes, bytes, bytes, deallocatorContext};
    FILEunmap(buf);
}

JS_DEFINE_FN(io_file_map) {
    if (!JSValueIsString(ctx, args[0])) JS_THROW("no path");

    u8 page[PAGESIZE];
    size_t len =
        JSStringGetUTF8CString((JSStringRef)args[0], (char*)page, PAGESIZE);

    Bu8 buf = {};
    u8csc path = {page, page + len - 1};
    ok64 o = FILEmapro(buf, path);
    JS_MAKE_OBJECT(fileObject, JSIOMapClass, NULL);
    JSValueRef ta = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, buf[0], Bsize(buf), MMapDeallocator,
        buf[3], exception);
    if (*exception != NULL) {
        JS_THROW("no typed array for you");
    }
    JS_MAKE_STRING(keyBuf, "buf");
    JS_SET_PROPERTY_RO(fileObject, keyBuf, ta);
    return fileObject;
}

extern const char* io_js;

ok64 io_install() {
    POLInit(1024);

    JS_MAKE_CLASS(FileDesc, io_file_finalize);
    JSIOFileClass = FileDesc;

    JS_MAKE_CLASS(FileMMap, io_map_finalize);
    JSIOMapClass = FileMMap;

    JS_API_OBJECT(io, "io");
    JS_SET_PROPERTY_FN(io, "stdIn", io_std_in);
    JS_SET_PROPERTY_FN(io, "stdErr", io_std_err);
    JS_SET_PROPERTY_FN(io, "stdOut", io_std_out);
    JS_SET_PROPERTY_FN(io, "mapFile", io_file_map);
    JS_SET_PROPERTY_FN(io, "timer", io_timer);
    JS_SET_PROPERTY_FN(io, "wake", io_wake);
    JS_SET_PROPERTY_FN(io, "log", io_log);
    JS_SET_PROPERTY_FN(io, "listen", io_net_listen);
    JS_SET_PROPERTY_FN(io, "accept", io_net_accept);
    JS_SET_PROPERTY_FN(io, "connect", io_net_connect);
    JS_SET_PROPERTY_FN(io, "close", io_net_close);

    // io.stderr.WriteLine("Hello world!")
    // response = io.stdin.ReadLine()
    // io.stdout.WriteLine("You said: '"+response+"'");

    JSExecute(io_js);

    return OK;
}

ok64 io_uninstall() {
    JSClassRelease(JSIOFileClass);
    JSClassRelease(JSIOMapClass);
    return OK;
}
