#include <math.h>
#include <poll.h>
#include <stddef.h>
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
#include "abc/TCP.h"

short io_callback(int fd, poller* p) { return 0; }
void io_file_finalize(JSObjectRef object) {}
void io_map_finalize(JSObjectRef object) {}

static JSClassRef JSIOFileClass = NULL;
static JSClassRef JSIOMapClass = NULL;

static JSStringRef JS_KEY_IO = NULL;

static JSStringRef JS_STATUS_READ = NULL;
static JSStringRef JS_STATUS_WRITE = NULL;
static JSStringRef JS_STATUS_READWRITE = NULL;
static JSStringRef JS_STATUS_ERROR = NULL;

thread_local JSObjectRef* JS_FILES;

JSObjectRef IO_STD[3];

JSObjectRef JSMakeFile(int fd, JSValueRef* exception);

int JSFileFD(JSContextRef ctx, JSObjectRef self, JSValueRef* exception) {
    JSObjectRef* ptr = JSObjectGetPrivate(self);
    if (ptr == NULL || ptr < JS_FILES || ptr > JS_FILES + POLMaxFiles()) {
        *exception = JSOfCString("read(): not a file");
        return -1;
    } else {
        ptrdiff_t d = ptr - JS_FILES;
        return (int)d;
    }
}

// fd.Write(ta) -> int
JSValueRef io_file_write(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    if (argc == 0) JS_THROW("fo.Write() requires data to write");

    void* bytes = NULL;
    size_t len = 0;
    char page[4096];

    int fd = JSFileFD(ctx, self, exception);
    if (fd == -1) return JSValueMakeUndefined(ctx);

    u8cs ta;

    if (JS_ARG_IS_STRING(0)) {  // TODO io.utf8()
        len = JSStringGetUTF8CString((JSStringRef)args[0], page, 4096);
        if (len > 0) len--;
        bytes = page;
        ta[0] = page;
        ta[1] = page + len;  // FUCKIN FIXME
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
JSValueRef io_file_read(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef self, size_t argc, const JSValueRef args[],
                        JSValueRef* exception) {
    if (argc == 0) JS_THROW("fo.Write() requires data to write");

    if (!JS_ARG_IS_TARRAY(0)) JS_THROW("not a TypedArray");

    u8s ta = {};
    JS_ARG_TA_u8s(0, ta);

    int fd = JSFileFD(ctx, self, exception);
    if (fd == -1) return JSValueMakeUndefined(ctx);

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
JSValueRef IONetClose(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                      size_t argc, const JSValueRef args[],
                      JSValueRef* exception) {
    // get fd
    // Unprotect
    // NULL
    // close
    return JSValueMakeUndefined(ctx);
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
JSValueRef IOTimer(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[],
                   JSValueRef* exception) {
    JS_TRACE("IOTimer");
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
JSValueRef IOWake(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                  size_t argc, const JSValueRef args[], JSValueRef* exception) {
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

JSValueRef IOLog(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
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

JSValueRef IONetListen(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                       size_t argc, const JSValueRef args[],
                       JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSValueRef IONetAccept(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                       size_t argc, const JSValueRef args[],
                       JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

// -> file.io("r"|"w"|"rw"|"error")
short IONetOnEvent(int fd, struct poller* p) {
    fprintf(stderr, "got event %i %i\n", fd, p->revents);
    JSObjectRef file = JS_FILES[fd];
    if (file == NULL) return 0;  //?
    JSValueRef fn = JSObjectGetPropertyForKey(
        JSctx, file, JSValueMakeString(JSctx, JS_KEY_IO), NULL);
    if (fn == NULL) return 0;

    JSValueRef arg = NULL;
    switch (p->revents) {
        case POLLIN:
            arg = JSValueMakeString(JSctx, JS_STATUS_READ);
            break;
        case POLLOUT:
            arg = JSValueMakeString(JSctx, JS_STATUS_WRITE);
            break;
        case POLLIN | POLLOUT:
            arg = JSValueMakeString(JSctx, JS_STATUS_READWRITE);
            break;
        default: {
            int err = 0;
            socklen_t errlen = sizeof(err);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
            if (err != 0) {
                arg = JSOfCString(strerror(err));
            } else {
                arg = JSValueMakeString(JSctx, JS_STATUS_ERROR);
            }
        }
    }

    fprintf(stderr, "calling the callback\n");
    JSValueRef ret = JSObjectCallAsFunction(JSctx, fn, file, 1, &arg, NULL);

    return p->events;
}

short IONetOnConnect(int fd, struct poller* p) {
    fprintf(stderr, "connected!\n");
    int err = 0;
    socklen_t errlen = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (err != 0) {
        const char* msg = strerror(err);
    } else {
        p->callback = IONetOnEvent;
    }
    // listen for POLLIN
    // JS_FILES[fd];
    return IONetOnEvent(fd, p);
}

JSValueRef IONothing(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                     size_t argc, const JSValueRef args[],
                     JSValueRef* exception) {
    fprintf(stderr, "nothing\n");
    return JSValueMakeUndefined(ctx);
}

JSValueRef IONetConnect(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef self, size_t argc, const JSValueRef args[],
                        JSValueRef* exception) {
    if (argc < 2 || !JSValueIsObject(ctx, args[1]) ||
        !JSObjectIsFunction(ctx, (JSObjectRef)args[1])) {
        JS_THROW("io.connect('google.com:80', function)");
    }
    JSStringRef address = args[0];
    JSObjectRef callback = (JSObjectRef)args[1];

    aNETAddress(addr, "127.0.0.1", "8888");
    $println(Bu8cpast(addr));
    int cfd;
    ok64 o = TCPConnect(&cfd, addr, NO);
    if (o != OK) {
        *exception = JSOfCString(strerror(errno));  //"connect() error"));
        return JSValueMakeUndefined(ctx);
    }
    poller p = {.events = POLLOUT,
                .callback = IONetOnConnect,
                .timeout_ms = 3000,
                .payload = JS_FILES + cfd};
    POLTrackEvents(cfd, p);

    JSObjectRef file = JSMakeFile(cfd, exception);
    JSObjectSetProperty(JSctx, file, JS_KEY_IO, callback,
                        kJSPropertyAttributeNone, NULL);

    return file;
}

JSObjectRef JSMakeFile(int fd, JSValueRef* exception) {
    if (fd >= POLMaxFiles()) {
        *exception = JSOfCString("file descriptor out of bound");
        return NULL;
    }
    JSObjectRef fileObject = JSObjectMake(JSctx, JSIOFileClass, NULL);
    JSObjectSetPrivate(fileObject, JS_FILES + fd);
    JS_FILES[fd] = fileObject;
    JSValueProtect(JSctx, fileObject);
    return fileObject;
}

inline JSValueRef JSOfCString(const char* str) {
    JSStringRef tmp = JSStringCreateWithUTF8CString(str);
    JSValueRef n = JSValueMakeString(JSctx, tmp);
    JSStringRelease(tmp);
    return n;
}

// io.StdErr() -> fd
JSValueRef IOStdio(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[], JSValueRef* exception,
                   int which) {
    if (IO_STD[which] == NULL) {
        JSObjectRef fileObject = JSMakeFile(which, exception);
        JS_PROTECT(fileObject);
        poller p = {
            .callback = io_callback, .payload = fileObject, .timeout_ms = 1000};
        ok64 o = POLTrackEvents(which, p);
        IO_STD[which] = fileObject;
    }
    return IO_STD[which];
}

JS_DEFINE_FN(IOStdIn) {
    return IOStdio(ctx, function, self, argc, args, exception, STDIN_FILENO);
}

JS_DEFINE_FN(IOStdOut) {
    return IOStdio(ctx, function, self, argc, args, exception, STDOUT_FILENO);
}

JS_DEFINE_FN(IOStdErr) {
    return IOStdio(ctx, function, self, argc, args, exception, STDERR_FILENO);
}

void MMapDeallocator(void* bytes, void* deallocatorContext) {
    Bu8 buf = {bytes, bytes, bytes, deallocatorContext};
    FILEunmap(buf);
}

JSValueRef IOFileMap(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                     size_t argc, const JSValueRef args[],
                     JSValueRef* exception) {
    if (!JSValueIsString(ctx, args[0])) JS_THROW("no path");

    u8 page[PAGESIZE];
    size_t len =
        JSStringGetUTF8CString((JSStringRef)args[0], (char*)page, PAGESIZE);

    Bu8 buf = {};
    u8csc path = {page, page + len - 1};
    ok64 o = FILEmapro(buf, path);
    JSObjectRef fileObject = JSObjectMake(JSctx, JSIOMapClass, NULL);
    JSValueRef ta = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, buf[0], Bsize(buf), MMapDeallocator,
        buf[3], exception);
    if (*exception != NULL) {
        JS_THROW("no typed array for you");
    }
    JSObjectSetPropertyForKey(ctx, fileObject, JSOfCString("buf"), ta,
                              kJSPropertyAttributeReadOnly, exception);
    return fileObject;
}

extern const char* io_js;

ok64 IOInstall() {
    POLInit(1024);

    JS_FILES = malloc(sizeof(JSObjectRef) * POLMaxFiles());

    static JSStaticFunction file_statics[] = {
        {"io", IONothing, 0},
        {"write", io_file_write,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"read", io_file_read,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"close", io_file_close,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {NULL, NULL, 0}};
    static JSClassDefinition fc_definition = {
        0,                      // version
        kJSClassAttributeNone,  // attributes
        "File",                 // class name
        NULL,                   // parentClass
        NULL,                   // staticValues
        file_statics,           // method table
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL};
    JSIOFileClass = JSClassCreate(&fc_definition);

    JS_MAKE_CLASS(FileMMap, io_map_finalize);
    JSIOMapClass = FileMMap;

    JS_API_OBJECT(io, "io");
    JS_SET_PROPERTY_FN(io, "stdIn", IOStdIn);
    JS_SET_PROPERTY_FN(io, "stdErr", IOStdErr);
    JS_SET_PROPERTY_FN(io, "stdOut", IOStdOut);
    JS_SET_PROPERTY_FN(io, "mapFile", IOFileMap);
    JS_SET_PROPERTY_FN(io, "timer", IOTimer);
    JS_SET_PROPERTY_FN(io, "wake", IOWake);
    JS_SET_PROPERTY_FN(io, "log", IOLog);
    JS_SET_PROPERTY_FN(io, "listen", IONetListen);
    JS_SET_PROPERTY_FN(io, "accept", IONetAccept);
    JS_SET_PROPERTY_FN(io, "connect", IONetConnect);

    JS_KEY_IO = JSStringCreateWithUTF8CString("io");
    JS_STATUS_READ = JSStringCreateWithUTF8CString("r");
    JS_STATUS_WRITE = JSStringCreateWithUTF8CString("w");
    JS_STATUS_READWRITE = JSStringCreateWithUTF8CString("rw");
    JS_STATUS_ERROR = JSStringCreateWithUTF8CString("error");
    // io.stderr.WriteLine("Hello world!")
    // response = io.stdin.ReadLine()
    // io.stdout.WriteLine("You said: '"+response+"'");

    JSExecute(io_js);

    return OK;
}

ok64 io_uninstall() {
    JSClassRelease(JSIOFileClass);
    JSClassRelease(JSIOMapClass);
    JSStringRelease(JS_KEY_IO);
    // etc
    return OK;
}
