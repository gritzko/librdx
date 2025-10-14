#include <math.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include "JABC.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/POL.h"
#include "abc/TCP.h"

short io_callback(int fd, poller* p) { return 0; }
void io_file_finalize(JSObjectRef object) {}
void io_map_finalize(JSObjectRef object) {}

static JSClassRef JABC_IO_FILE_CLASS = NULL;
static JSClassRef JABC_IO_MAP_CLASS = NULL;

static JSStringRef JS_KEY_IO = NULL;

static JSStringRef JS_STATUS_READ = NULL;
static JSStringRef JS_STATUS_WRITE = NULL;
static JSStringRef JS_STATUS_READWRITE = NULL;
static JSStringRef JS_STATUS_ERROR = NULL;

thread_local JSObjectRef* JS_FILES;

JSObjectRef JABCio_STD[3];

JSObjectRef JABCioMakeFileObject(int fd, JSValueRef* exception) {
    if (fd >= POLMaxFiles()) {
        *exception = JSOfCString("file descriptor out of bound");
        return NULL;
    }
    JSObjectRef fileObject =
        JSObjectMake(JABC_CONTEXT, JABC_IO_FILE_CLASS, NULL);
    // The point of JABC is to hold no C pointers in JS, no JS references in C.
    // This is a pointer into a static array, so we can do that :)
    JSObjectSetPrivate(fileObject, JS_FILES + fd);
    // Again, a file object gets filed into a static array.
    JS_FILES[fd] = fileObject;
    JSValueProtect(JABC_CONTEXT, fileObject);
    return fileObject;
}

int JABCioFileGetDescriptor(JSContextRef ctx, JSObjectRef self,
                            JSValueRef* exception) {
    JSObjectRef* ptr = JSObjectGetPrivate(self);
    if (ptr == NULL || ptr < JS_FILES || ptr >= JS_FILES + POLMaxFiles()) {
        *exception = JSOfCString("read(): not a file");
        return -1;
    } else {
        ptrdiff_t d = ptr - JS_FILES;
        return (int)d;
    }
}

void JABCioCloseFile(int fd) {
    JSObjectRef self = JS_FILES[fd];
    if (self != NULL) {
        JSValueUnprotect(JABC_CONTEXT, self);
        JS_FILES[fd] = NULL;
    }
    POLIgnoreEvents(fd);  // only sockets, but who cares
    close(fd);
}

#define JABCCall(n)                                                        \
    JSValueRef n(JSContextRef ctx, JSObjectRef function, JSObjectRef self, \
                 size_t argc, const JSValueRef args[], JSValueRef* exception)
#define JABCArgGetNumber(a, n, err)
#define JABCArgGetString(a, n, err)
#define JABCArgGetTypedArray(a, n, err)

// fd.Write(ta) -> int
JABCCall(JABCioFileWrite) {
    if (argc == 0) {
        *exception = JSOfCString("file.write(string|typedarray)");
        return JSValueMakeUndefined(ctx);
    }

    char page[4096];

    int fd = JABCioFileGetDescriptor(ctx, self, exception);
    if (fd == -1) return JSValueMakeUndefined(ctx);

    u8s ta = {};
    b8 tofree = NO;

    if (JSValueIsString(ctx, args[0])) {  // TODO io.utf8()
        JABCutf8CopyStringValue(ctx, ta, args[0], exception);
        tofree = YES;
    } else if (JSValueGetTypedArrayType(ctx, args[0], NULL) !=
               kJSTypedArrayTypeNone) {
        ta[0] =
            JSObjectGetTypedArrayBytesPtr(ctx, (JSObjectRef)args[0], exception);
        ta[1] = ta[0] + JSObjectGetTypedArrayByteLength(
                            ctx, (JSObjectRef)args[0], exception);
    } else {
        *exception = JSOfCString("file.write(string|typedarray)");
        return JSValueMakeUndefined(ctx);
    }

    int wn = write(fd, ta[0], $len(ta));
    // fprintf(stderr, "fd %i len %lu ret %i\n", fd->poll.fd, len, wn);

    if (wn < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            POLAddEvents(fd, POLLOUT);
        } else {
            JSStringRef errMsg = JSStringCreateWithUTF8CString(strerror(errno));
            *exception = JSValueMakeString(ctx, errMsg);
            JSStringRelease(errMsg);
        }
    }

    if (tofree) $u8free((u8csp)ta);
    JS_MAKE_NUMBER(ret, wn);
    return ret;
}

JSValueRef JABCioFileRead(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef self, size_t argc,
                          const JSValueRef args[], JSValueRef* exception) {
    if (argc == 0 ||
        JSValueGetTypedArrayType(ctx, args[0], NULL) == kJSTypedArrayTypeNone) {
        *exception = JSOfCString("Use: file.read(typedarray)");
        return JSValueMakeUndefined(ctx);
    }
    JSObjectRef arg = JSValueToObject(ctx, args[0], exception);

    u8s ta = {};
    ta[0] = JSObjectGetTypedArrayBytesPtr(ctx, arg, exception);
    ta[1] = ta[0] + JSObjectGetTypedArrayByteLength(ctx, arg, exception);

    int fd = JABCioFileGetDescriptor(ctx, self, exception);
    if (fd == -1) return JSValueMakeUndefined(ctx);

    int rn = read(fd, *ta, $len(ta));

    if (rn < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            POLAddEvents(fd, POLLIN);
        } else {
            JSStringRef errMsg = JSStringCreateWithUTF8CString(strerror(errno));
            *exception = JSValueMakeString(ctx, errMsg);
            JSStringRelease(errMsg);
        }
    }

    JS_MAKE_NUMBER(num, rn);
    return num;
}

JSValueRef JABCioNetClose(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef self, size_t argc,
                          const JSValueRef args[], JSValueRef* exception) {
    int fd = JABCioFileGetDescriptor(ctx, self, exception);
    if (fd == -1) {
        *exception = JSOfCString("file.close()");
    } else {
        JABCioCloseFile(fd);
    }
    return JSValueMakeUndefined(ctx);
}

JSObjectRef JSTimer = NULL;

int timeout_cb(int fd) {
    if (JSTimer == NULL) return INT32_MAX;
    JSValueRef exception = NULL;  // todo
    JSValueRef ret = JSObjectCallAsFunction(JABC_CONTEXT, JSTimer, NULL, 0,
                                            NULL, &exception);
    int next = 1000;
    if (JSValueIsNumber(JABC_CONTEXT, ret)) {
        next = JSValueToNumber(JABC_CONTEXT, ret, NULL);
    }
    return next;
}

// io.timer(fn)
JSValueRef JABCioTimer(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                       size_t argc, const JSValueRef args[],
                       JSValueRef* exception) {
    JS_TRACE("JABCioTimer");
    if (argc < 1 || !JSValueIsObject(ctx, args[0]) ||
        !JSObjectIsFunction(ctx, (JSObjectRef)args[0])) {
        *exception = JSOfCString("io.timer(function)");
        return JSValueMakeUndefined(ctx);
    }
    if (JSTimer != NULL) JSValueUnprotect(JABC_CONTEXT, JSTimer);
    JSTimer = (JSObjectRef)args[0];
    POLTrackTime(timeout_cb);
    JSValueProtect(ctx, JSTimer);
    return JSValueMakeUndefined(ctx);
}

// io.wake(ms)
JSValueRef JABCioWake(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                      size_t argc, const JSValueRef args[],
                      JSValueRef* exception) {
    if (argc != 1 || !JSValueIsNumber(ctx, args[0])) {
        *exception = JSOfCString("io.wake(ms)");
        return JSValueMakeUndefined(ctx);
    }
    if (JSTimer == NULL) {
        *exception = JSOfCString("no timer set");
        return JSValueMakeUndefined(ctx);
    }
    double ms = JSValueToNumber(ctx, args[0], exception);
    // get function
    JSObjectRef fn = (JSObjectRef)args[0];
    POLAddTime((int)ms);
    return JSValueMakeUndefined(ctx);
}

JSValueRef JABCioLog(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                     size_t argc, const JSValueRef args[],
                     JSValueRef* exception) {
    for (int i = 0; i < argc; i++) {
        if (JSValueIsString(ctx, args[i])) {
            JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
            size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
            char* bytes = malloc(maxSize);
            size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
            int n = write(STDERR_FILENO, bytes, factlen);
            free(bytes);
            JSStringRelease(str);
        } else if (kJSTypedArrayTypeNone !=
                   JSValueGetTypedArrayType(ctx, args[i], exception)) {
            JSObjectRef obj = JSValueToObject(ctx, args[i], NULL);
            void* bytes = JSObjectGetTypedArrayBytesPtr(ctx, obj, exception);
            size_t len = JSObjectGetTypedArrayByteLength(ctx, obj, exception);
            int n = write(STDERR_FILENO, bytes, len);
        } else {
            *exception = JSOfCString("io.log( (string|typedarray)* )");
            return JSValueMakeUndefined(ctx);
        }
    }
    write(STDERR_FILENO, "\n", 1);
    fsync(STDERR_FILENO);
    return JSValueMakeUndefined(ctx);
}

JSValueRef JABCioNetListen(JSContextRef ctx, JSObjectRef function,
                           JSObjectRef self, size_t argc,
                           const JSValueRef args[], JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

JSValueRef JABCioNetAccept(JSContextRef ctx, JSObjectRef function,
                           JSObjectRef self, size_t argc,
                           const JSValueRef args[], JSValueRef* exception) {
    return JSValueMakeUndefined(ctx);
}

// -> file.io("r"|"w"|"error")
short JABCioNetOnEvent(int fd, struct poller* p) {
    fprintf(stderr, "got event %i %i\n", fd, p->revents);
    JSObjectRef file = JS_FILES[fd];
    if (file == NULL) return 0;  //?
    JSValueRef fnv = JSObjectGetPropertyForKey(
        JABC_CONTEXT, file, JSValueMakeString(JABC_CONTEXT, JS_KEY_IO), NULL);
    if (fnv == NULL || !JSValueIsObject(JABC_CONTEXT, fnv) ||
        !JSObjectIsFunction(JABC_CONTEXT, (JSObjectRef)fnv)) {
        p->events = 0;
        return 0;
    }
    JSObjectRef fn = (JSObjectRef)fnv;
    JSValueRef exception = NULL;

    if (!(p->revents & (POLLIN | POLLOUT))) {
        JSValueRef arg = JSValueMakeString(JABC_CONTEXT, JS_STATUS_ERROR);
        int err = 0;
        socklen_t errlen = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
        if (err != 0) {
            arg = JSOfCString(strerror(err));
        }
        p->events = 0;
        JSObjectCallAsFunction(JABC_CONTEXT, fn, file, 1, &arg, NULL);
        JABCioNetClose(JABC_CONTEXT, NULL, file, 0, NULL, &exception);
        if (exception) JABCReport(exception), exception = NULL;
    }
    if (p->revents & POLLIN) {
        JSValueRef arg = JSValueMakeString(JABC_CONTEXT, JS_STATUS_READ);
        p->events &= ~POLLIN;
        JSObjectCallAsFunction(JABC_CONTEXT, fn, file, 1, &arg, &exception);
        if (exception) JABCReport(exception), exception = NULL;
    }
    if (p->revents & POLLOUT) {
        JSValueRef arg = JSValueMakeString(JABC_CONTEXT, JS_STATUS_WRITE);
        p->events &= ~POLLOUT;
        JSObjectCallAsFunction(JABC_CONTEXT, fn, file, 1, &arg, &exception);
        if (exception) JABCReport(exception), exception = NULL;
    }

    return p->events;  //?
}

short JABCioNetOnConnect(int fd, struct poller* p) {
    fprintf(stderr, "connected!\n");
    int err = 0;
    socklen_t errlen = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (err != 0) {
        const char* msg = strerror(err);
    } else {
        p->callback = JABCioNetOnEvent;
    }
    JABCioNetOnEvent(fd, p);
    return p->events | POLLIN;
}

JSValueRef JABCioNothing(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    fprintf(stderr, "nothing\n");
    return JSValueMakeUndefined(ctx);
}

JSValueRef JABCioNetConnect(JSContextRef ctx, JSObjectRef function,
                            JSObjectRef self, size_t argc,
                            const JSValueRef args[], JSValueRef* exception) {
    if (argc < 2 || !JSValueIsString(ctx, args[0]) ||
        !JSValueIsObject(ctx, args[1]) ||
        !JSObjectIsFunction(ctx, (JSObjectRef)args[1])) {
        *exception = JSOfCString("io.connect('tcp://google.com:80', function)");
        return JSValueMakeUndefined(ctx);
    }
    a_pad(u8, uri, 1024);
    ok64 o = JABCutf8bFeedValueRef(uri, ctx, args[0]);

    JSObjectRef callback = (JSObjectRef)args[1];

    int cfd;
    o = TCPConnect(&cfd, uri_datac, NO);
    if (o != OK) {
        *exception = JSOfCString(strerror(errno));  //"connect() error"));
        return JSValueMakeUndefined(ctx);
    }
    poller p = {.events = POLLOUT,
                .callback = JABCioNetOnConnect,
                .timeout_ms = 3000,
                .payload = JS_FILES + cfd};
    POLTrackEvents(cfd, p);

    JSObjectRef file = JABCioMakeFileObject(cfd, exception);
    JSObjectSetProperty(JABC_CONTEXT, file, JS_KEY_IO, callback,
                        kJSPropertyAttributeNone, NULL);

    return file;
}

// io.StdErr() -> fd
JSValueRef JABCioStdio(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                       size_t argc, const JSValueRef args[],
                       JSValueRef* exception, int which) {
    if (JABCio_STD[which] == NULL) {
        JSObjectRef fileObject = JABCioMakeFileObject(which, exception);
        poller p = {
            .callback = io_callback, .payload = fileObject, .timeout_ms = 1000};
        ok64 o = POLTrackEvents(which, p);
        JABCio_STD[which] = fileObject;
    }
    return JABCio_STD[which];
}

JS_DEFINE_FN(JABCioStdIn) {
    return JABCioStdio(ctx, function, self, argc, args, exception,
                       STDIN_FILENO);
}

JS_DEFINE_FN(JABCioStdOut) {
    return JABCioStdio(ctx, function, self, argc, args, exception,
                       STDOUT_FILENO);
}

JS_DEFINE_FN(JABCioStdErr) {
    return JABCioStdio(ctx, function, self, argc, args, exception,
                       STDERR_FILENO);
}

void mmap_free(void* bytes, void* deallocatorContext) {
    Bu8 buf = {bytes, bytes, bytes, deallocatorContext};
    FILEunmap(buf);
}

JSValueRef JABCioFileMap(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef self, size_t argc, const JSValueRef args[],
                         JSValueRef* exception) {
    if (!JSValueIsString(ctx, args[0])) {
        *exception = JSOfCString("io.mmap(path)");
        return JSValueMakeUndefined(ctx);
    }

    u8 page[PAGESIZE];
    size_t len =
        JSStringGetUTF8CString((JSStringRef)args[0], (char*)page, PAGESIZE);

    Bu8 buf = {};
    u8csc path = {page, page + len - 1};
    ok64 o = FILEmapro(buf, path);
    JSObjectRef fileObject =
        JSObjectMake(JABC_CONTEXT, JABC_IO_MAP_CLASS, NULL);
    JSValueRef ta = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, buf[0], Bsize(buf), mmap_free, buf[3],
        exception);
    if (*exception != NULL) return JSValueMakeUndefined(ctx);
    JSObjectSetPropertyForKey(ctx, fileObject, JSOfCString("buf"), ta,
                              kJSPropertyAttributeReadOnly, exception);
    return fileObject;
}

extern const char* io_js;

ok64 JABCioInstall() {
    POLInit(1024);

    JS_FILES = malloc(sizeof(JSObjectRef) * POLMaxFiles());

    static JSStaticFunction file_statics[] = {
        {"io", JABCioNothing, 0},
        {"write", JABCioFileWrite,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"read", JABCioFileRead,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"close", JABCioNetClose,
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
    JABC_IO_FILE_CLASS = JSClassCreate(&fc_definition);

    JS_MAKE_CLASS(FileMMap, io_map_finalize);
    JABC_IO_MAP_CLASS = FileMMap;

    JS_API_OBJECT(io, "io");
    JS_SET_PROPERTY_FN(io, "stdIn", JABCioStdIn);
    JS_SET_PROPERTY_FN(io, "stdErr", JABCioStdErr);
    JS_SET_PROPERTY_FN(io, "stdOut", JABCioStdOut);
    JS_SET_PROPERTY_FN(io, "mmap", JABCioFileMap);
    JS_SET_PROPERTY_FN(io, "timer", JABCioTimer);
    JS_SET_PROPERTY_FN(io, "wake", JABCioWake);
    JS_SET_PROPERTY_FN(io, "log", JABCioLog);
    JS_SET_PROPERTY_FN(io, "listen", JABCioNetListen);
    JS_SET_PROPERTY_FN(io, "accept", JABCioNetAccept);
    JS_SET_PROPERTY_FN(io, "connect", JABCioNetConnect);

    JS_KEY_IO = JSStringCreateWithUTF8CString("io");
    JS_STATUS_READ = JSStringCreateWithUTF8CString("r");
    JS_STATUS_WRITE = JSStringCreateWithUTF8CString("w");
    JS_STATUS_READWRITE = JSStringCreateWithUTF8CString("rw");
    JS_STATUS_ERROR = JSStringCreateWithUTF8CString("error");
    // io.stderr.WriteLine("Hello world!")
    // response = io.stdin.ReadLine()
    // io.stdout.WriteLine("You said: '"+response+"'");

    JABCExecute(io_js);

    return OK;
}

ok64 JABCioUninstall() {
    JSClassRelease(JABC_IO_FILE_CLASS);
    JSClassRelease(JABC_IO_MAP_CLASS);
    JSStringRelease(JS_KEY_IO);
    JSStringRelease(JS_STATUS_ERROR);
    JSStringRelease(JS_STATUS_READ);
    JSStringRelease(JS_STATUS_WRITE);
    JSStringRelease(JS_STATUS_READWRITE);
    JSValueUnprotect(JABC_CONTEXT, JABCio_STD[0]);
    JSValueUnprotect(JABC_CONTEXT, JABCio_STD[1]);
    JSValueUnprotect(JABC_CONTEXT, JABCio_STD[2]);
    JABCio_STD[0] = NULL;
    JABCio_STD[1] = NULL;
    JABCio_STD[2] = NULL;
    JS_KEY_IO = NULL;
    JS_STATUS_ERROR = NULL;
    JS_STATUS_READ = NULL;
    JS_STATUS_WRITE = NULL;
    JS_STATUS_READWRITE = NULL;
    JABC_IO_FILE_CLASS = NULL;
    JABC_IO_MAP_CLASS = NULL;

    for (int i = 0; i < POLMaxFiles(); i++)
        if (JS_FILES[i] != NULL) {
            JABCioCloseFile(i);
        }
    POLFree();
    return OK;
}
