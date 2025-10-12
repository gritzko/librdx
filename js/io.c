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
#include "abc/OK.h"
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
JSValueRef IOFileWrite(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                       size_t argc, const JSValueRef args[],
                       JSValueRef* exception) {
    if (argc == 0) {
        *exception = JSOfCString("file.write(string|typedarray)");
        return JSValueMakeUndefined(ctx);
    }

    char page[4096];

    int fd = JSFileFD(ctx, self, exception);
    if (fd == -1) return JSValueMakeUndefined(ctx);

    u8cs ta;

    if (JS_ARG_IS_STRING(0)) {  // TODO io.utf8()
        size_t len = JSStringGetUTF8CString((JSStringRef)args[0], page, 4096);
        if (len > 0) len--;
        ta[0] = (u8*)page;
        ta[1] = (u8*)page + len;  // FUCKIN FIXME
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

    JS_MAKE_NUMBER(ret, wn);
    return ret;
}

// fd.Read(ta) -> int
JSValueRef IOFileRead(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                      size_t argc, const JSValueRef args[],
                      JSValueRef* exception) {
    if (argc == 0 ||
        JSValueGetTypedArrayType(ctx, args[0], NULL) == kJSTypedArrayTypeNone) {
        *exception = JSOfCString("Use: file.read(typedarray)");
        return JSValueMakeUndefined(ctx);
    }
    JSObjectRef arg = args[0];

    u8s ta = {};
    ta[0] = JSObjectGetTypedArrayBytesPtr(ctx, arg, exception);
    ta[1] = ta[0] + JSObjectGetTypedArrayByteLength(ctx, arg, exception);

    int fd = JSFileFD(ctx, self, exception);
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

void JSCloseFile(int fd) {
    JSObjectRef self = JS_FILES[fd];
    JSValueUnprotect(JS_CONTEXT, self);
    JS_FILES[fd] = NULL;
    POLIgnoreEvents(fd);  // only sockets, but who cares
    close(fd);
}

JSValueRef IONetClose(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                      size_t argc, const JSValueRef args[],
                      JSValueRef* exception) {
    int fd = JSFileFD(ctx, self, exception);
    if (fd == -1) {
        *exception = JSOfCString("file.close()");
    } else {
        JSCloseFile(fd);
    }
    return JSValueMakeUndefined(ctx);
}

JSObjectRef JSTimer = NULL;

int timeout_cb(int fd) {
    if (JSTimer == NULL) return INT32_MAX;
    JSValueRef exception = NULL;  // todo
    JSValueRef ret =
        JSObjectCallAsFunction(JS_CONTEXT, JSTimer, NULL, 0, NULL, &exception);
    int next = 1000;
    if (JSValueIsNumber(JS_CONTEXT, ret)) {
        next = JSValueToNumber(JS_CONTEXT, ret, NULL);
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
        *exception = JSOfCString("io.timer(function)");
        return JSValueMakeUndefined(ctx);
    }
    if (JSTimer != NULL) JSValueUnprotect(JS_CONTEXT, JSTimer);
    JSTimer = (JSObjectRef)args[0];
    POLTrackTime(timeout_cb);
    JSValueProtect(ctx, JSTimer);
    return JSValueMakeUndefined(ctx);
}

// io.wake(ms)
JSValueRef IOWake(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                  size_t argc, const JSValueRef args[], JSValueRef* exception) {
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

JSValueRef IOLog(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                 size_t argc, const JSValueRef args[], JSValueRef* exception) {
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

// -> file.io("r"|"w"|"error")
short IONetOnEvent(int fd, struct poller* p) {
    fprintf(stderr, "got event %i %i\n", fd, p->revents);
    JSObjectRef file = JS_FILES[fd];
    if (file == NULL) return 0;  //?
    JSValueRef fnv = JSObjectGetPropertyForKey(
        JS_CONTEXT, file, JSValueMakeString(JS_CONTEXT, JS_KEY_IO), NULL);
    if (fnv == NULL || !JSValueIsObject(JS_CONTEXT, fnv) ||
        !JSObjectIsFunction(JS_CONTEXT, (JSObjectRef)fnv)) {
        p->events = 0;
        return 0;
    }
    JSObjectRef fn = (JSObjectRef)fnv;
    JSValueRef exception = NULL;

    if (!(p->revents & (POLLIN | POLLOUT))) {
        JSValueRef arg = JSValueMakeString(JS_CONTEXT, JS_STATUS_ERROR);
        int err = 0;
        socklen_t errlen = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
        if (err != 0) {
            arg = JSOfCString(strerror(err));
        }
        p->events = 0;
        JSObjectCallAsFunction(JS_CONTEXT, fn, file, 1, &arg, NULL);
        IONetClose(JS_CONTEXT, NULL, file, 0, NULL, &exception);
        if (exception) JSReport(exception), exception = NULL;
    }
    if (p->revents & POLLIN) {
        JSValueRef arg = JSValueMakeString(JS_CONTEXT, JS_STATUS_READ);
        p->events &= ~POLLIN;
        JSObjectCallAsFunction(JS_CONTEXT, fn, file, 1, &arg, &exception);
        if (exception) JSReport(exception), exception = NULL;
    }
    if (p->revents & POLLOUT) {
        JSValueRef arg = JSValueMakeString(JS_CONTEXT, JS_STATUS_WRITE);
        p->events &= ~POLLOUT;
        JSObjectCallAsFunction(JS_CONTEXT, fn, file, 1, &arg, &exception);
        if (exception) JSReport(exception), exception = NULL;
    }

    return p->events;  //?
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
    IONetOnEvent(fd, p);
    return p->events | POLLIN;
}

JSValueRef IONothing(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                     size_t argc, const JSValueRef args[],
                     JSValueRef* exception) {
    fprintf(stderr, "nothing\n");
    return JSValueMakeUndefined(ctx);
}

ok64 JSu8BString(JSStringRef str, u8B into) {
    u8sp idle = Bidle(into);
    size_t fact = JSStringGetUTF8CString(str, (char*)*idle, $len(idle));
    if (fact < 1) return badarg;
    idle[0] += fact - 1;
    return OK;
}

ok64 JSu8BValueString(JSContextRef ctx, JSValueRef val, u8B into) {
    JSStringRef str = JSValueToStringCopy(ctx, val, NULL);
    ok64 o = JSu8BString(str, into);
    JSStringRelease(str);
    return o;
}

JSValueRef IONetConnect(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef self, size_t argc, const JSValueRef args[],
                        JSValueRef* exception) {
    if (argc < 2 || !JSValueIsString(ctx, args[0]) ||
        !JSValueIsObject(ctx, args[1]) ||
        !JSObjectIsFunction(ctx, (JSObjectRef)args[1])) {
        *exception = JSOfCString("io.connect('tcp://google.com:80', function)");
        return JSValueMakeUndefined(ctx);
    }
    a_pad(u8, uri, 1024);
    ok64 o = JSu8BValueString(ctx, args[0], uri);

    JSObjectRef callback = (JSObjectRef)args[1];

    int cfd;
    o = TCPConnect(&cfd, uri_datac, NO);
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
    JSObjectSetProperty(JS_CONTEXT, file, JS_KEY_IO, callback,
                        kJSPropertyAttributeNone, NULL);

    return file;
}

JSObjectRef JSMakeFile(int fd, JSValueRef* exception) {
    if (fd >= POLMaxFiles()) {
        *exception = JSOfCString("file descriptor out of bound");
        return NULL;
    }
    JSObjectRef fileObject = JSObjectMake(JS_CONTEXT, JSIOFileClass, NULL);
    JSObjectSetPrivate(fileObject, JS_FILES + fd);
    JS_FILES[fd] = fileObject;
    JSValueProtect(JS_CONTEXT, fileObject);
    return fileObject;
}

inline JSValueRef JSOfCString(const char* str) {
    JSStringRef tmp = JSStringCreateWithUTF8CString(str);
    JSValueRef n = JSValueMakeString(JS_CONTEXT, tmp);
    JSStringRelease(tmp);
    return n;
}

// io.StdErr() -> fd
JSValueRef IOStdio(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[], JSValueRef* exception,
                   int which) {
    if (IO_STD[which] == NULL) {
        JSObjectRef fileObject = JSMakeFile(which, exception);
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
    JSObjectRef fileObject = JSObjectMake(JS_CONTEXT, JSIOMapClass, NULL);
    JSValueRef ta = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, buf[0], Bsize(buf), MMapDeallocator,
        buf[3], exception);
    if (*exception != NULL) return JSValueMakeUndefined(ctx);
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
        {"write", IOFileWrite,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"read", IOFileRead,
         kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"close", IONetClose,
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
    JS_SET_PROPERTY_FN(io, "mmap", IOFileMap);
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

ok64 IOUninstall() {
    JSClassRelease(JSIOFileClass);
    JSClassRelease(JSIOMapClass);
    JSStringRelease(JS_KEY_IO);
    JSStringRelease(JS_STATUS_ERROR);
    JSStringRelease(JS_STATUS_READ);
    JSStringRelease(JS_STATUS_WRITE);
    JSStringRelease(JS_STATUS_READWRITE);
    JSValueUnprotect(JS_CONTEXT, IO_STD[0]);
    JSValueUnprotect(JS_CONTEXT, IO_STD[1]);
    JSValueUnprotect(JS_CONTEXT, IO_STD[2]);
    IO_STD[0] = NULL;
    IO_STD[1] = NULL;
    IO_STD[2] = NULL;
    JS_KEY_IO = NULL;
    JS_STATUS_ERROR = NULL;
    JS_STATUS_READ = NULL;
    JS_STATUS_WRITE = NULL;
    JS_STATUS_READWRITE = NULL;
    JSIOFileClass = NULL;
    JSIOMapClass = NULL;

    for (int i = 0; i < POLMaxFiles(); i++)
        if (JS_FILES[i] != NULL) {
            JSCloseFile(i);
        }
    POLFree();
    return OK;
}
