#ifndef ABC_JS_H
#define ABC_JS_H
#include <threads.h>

#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "JavaScriptCore/JavaScript.h"

extern thread_local JSGlobalContextRef JSctx;
extern thread_local JSObjectRef JSglobal;

#define JS_DEFINE_FN(fn)                                    \
    JSValueRef fn(JSContextRef JSctx, JSObjectRef function, \
                  JSObjectRef thisObject, size_t argc,      \
                  const JSValueRef args[], JSValueRef* exception);

#define JS_API_OBJECT(o, n)                                    \
    JSObjectRef o = JSObjectMake(JSctx, NULL, NULL);           \
    {                                                          \
        JSStringRef ioName = JSStringCreateWithUTF8CString(n); \
        JSObjectSetProperty(JSctx, JSglobal, ioName, o,        \
                            kJSPropertyAttributeNone, NULL);   \
        JSStringRelease(ioName);                               \
    }

#define JS_INSTALL_FN(o, n, f)                                              \
    {                                                                       \
        JSStringRef fn = JSStringCreateWithUTF8CString(n);                  \
        JSObjectSetProperty(JSctx, o, fn,                                   \
                            JSObjectMakeFunctionWithCallback(JSctx, fn, f), \
                            kJSPropertyAttributeNone, NULL);                \
        JSStringRelease(fn);                                                \
    }

JS_DEFINE_FN(io_std_in);
JS_DEFINE_FN(io_std_out);
JS_DEFINE_FN(io_std_err);

#define JS_ADD_METHOD(o, n, fn)                                  \
    {                                                            \
        JSStringRef someName = JSStringCreateWithUTF8CString(n); \
        JSObjectRef someFunc =                                   \
            JSObjectMakeFunctionWithCallback(ctx, someName, fn); \
        JSObjectSetProperty(ctx, o, someName, someFunc,          \
                            kJSPropertyAttributeNone, NULL);     \
        JSStringRelease(someName);                               \
    }

JSGlobalContextRef JSCreate();
void JSExecute(const char* script);
void JSReport(JSValueRef exception);

#endif
