#ifndef ABC_JS_H
#define ABC_JS_H
#include <stdlib.h>
#include <threads.h>

#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "JavaScriptCore/JavaScript.h"

extern thread_local JSGlobalContextRef JSctx;
extern thread_local JSObjectRef JSglobal;

#define JS_DEFINE_FN(fn)                                                    \
    JSValueRef fn(JSContextRef ctx, JSObjectRef function, JSObjectRef self, \
                  size_t argc, const JSValueRef args[], JSValueRef* exception)

#define JS_ARG_IS_STRING(n) JSValueIsString(ctx, args[n])
#define JS_ARG_IS_OBJECT(n) JSValueIsObject(ctx, args[n])
#define JS_ARG_IS_TARRAY(n) \
    (JSValueGetTypedArrayType(ctx, args[n], NULL) != kJSTypedArrayTypeNone)

#define JS_ARG_TO_STRING(var, n) \
    JSStringRef var = JSValueToStringCopy(ctx, args[n], exception);

#define JS_THROW(msg)                                            \
    {                                                            \
        JSStringRef errMsg = JSStringCreateWithUTF8CString(msg); \
        *exception = JSValueMakeString(ctx, errMsg);             \
        JSStringRelease(errMsg);                                 \
        return JSValueMakeUndefined(ctx);                        \
    }

#define JS_ARG_TA_u8s(n, ta)                                                   \
    ta[0] =                                                                    \
        JSObjectGetTypedArrayBytesPtr(ctx, (JSObjectRef)args[n], exception);   \
    if (*exception != NULL || ta[0] == NULL) JS_THROW("not a TypedArray");     \
    ta[1] = ta[0] + JSObjectGetTypedArrayByteLength(ctx, (JSObjectRef)args[n], \
                                                    exception);

#define JS_MAKE_NUMBER(n, i) JSValueRef n = JSValueMakeNumber(ctx, i)

#define JS_MAKE_UNDEFINED(n) JSValueRef n = JSValueMakeUndefined(ctx);

#define JS_MAKE_OBJECT(n, class, ptr) \
    JSObjectRef n = JSObjectMake(ctx, class, ptr);

#define JS_MAKE_STRING(n, str)                             \
    JSStringRef _##n = JSStringCreateWithUTF8CString(str); \
    JSValueRef n = JSValueMakeString(ctx, _##n);           \
    JSStringRelease(_##n);

#define JS_MAKE_CLASS(n, f)                                  \
    JSClassDefinition n##ClassDef = kJSClassDefinitionEmpty; \
    n##ClassDef.className = #n;                              \
    n##ClassDef.finalize = f;                                \
    JSClassRef n = JSClassCreate(&n##ClassDef);              \
    JSClassRetain(n);

#define JS_PROTECT(obj) JSValueProtect(ctx, obj)
#define JS_UNPROTECT(obj) JSValueUnprotect(ctx, obj)

#define JS_SET_PROPERTY_RO(obj, key, val)         \
    JSObjectSetPropertyForKey(ctx, obj, key, val, \
                              kJSPropertyAttributeReadOnly, exception);

#define JS_GET_PROPERTY(n, obj, key) \
    JSValueRef n = JSObjectGetPropertyForKey(ctx, obj, key, exception);

#define JS_TO_NUMBER(n, val) double n = JSValueToNumber(ctx, val, exception);

#define JS_API_OBJECT(o, n)                                    \
    JSObjectRef o = JSObjectMake(JSctx, NULL, NULL);           \
    {                                                          \
        JSStringRef ioName = JSStringCreateWithUTF8CString(n); \
        JSObjectSetProperty(JSctx, JSglobal, ioName, o,        \
                            kJSPropertyAttributeNone, NULL);   \
        JSStringRelease(ioName);                               \
    }

#define JS_SET_PROPERTY_FN(o, n, f)                                         \
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
