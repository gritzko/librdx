#ifndef ABC_JS_H
#define ABC_JS_H
#include <stdlib.h>
#include <threads.h>

#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include "abc/ABC.hpp"

extern thread_local JSGlobalContextRef JABC_CONTEXT;
extern thread_local JSObjectRef JABC_GLOBAL_OBJECT;

#define JS_DEFINE_FN(fn)                                                    \
    JSValueRef fn(JSContextRef ctx, JSObjectRef function, JSObjectRef self, \
                  size_t argc, const JSValueRef args[], JSValueRef* exception)

#define JS_ARG_IS_STRING(n) JSValueIsString(ctx, args[n])
#define JS_ARG_IS_OBJECT(n) JSValueIsObject(ctx, args[n])
#define JS_ARG_IS_TARRAY(n) \
    (JSValueGetTypedArrayType(ctx, args[n], NULL) != kJSTypedArrayTypeNone)

#define JS_ARG_TO_STRING(var, n) \
    JSStringRef var = JSValueToStringCopy(ctx, args[n], exception);

#define JS_TRACE(str) fprintf(stderr, "%s:%i\t%s\n", __FILE__, __LINE__, str);

#define JS_MAKE_NUMBER(n, i) JSValueRef n = JSValueMakeNumber(ctx, i)

#define JS_MAKE_CLASS(n, f)                                  \
    JSClassDefinition n##ClassDef = kJSClassDefinitionEmpty; \
    n##ClassDef.className = #n;                              \
    n##ClassDef.finalize = f;                                \
    JSClassRef n = JSClassCreate(&n##ClassDef);

#define JS_PROTECT(obj) JSValueProtect(ctx, obj)
#define JS_UNPROTECT(obj) JSValueUnprotect(ctx, obj)

#define JS_SET_PROPERTY_RO(obj, key, val)         \
    JSObjectSetPropertyForKey(ctx, obj, key, val, \
                              kJSPropertyAttributeReadOnly, exception);

#define JS_GET_PROPERTY(n, obj, key) \
    JSValueRef n = JSObjectGetPropertyForKey(ctx, obj, key, exception);

#define JS_TO_NUMBER(n, val) double n = JSValueToNumber(ctx, val, exception);

#define JS_API_OBJECT(o, n)                                              \
    JSObjectRef o = JSObjectMake(JABC_CONTEXT, NULL, NULL);              \
    {                                                                    \
        JSStringRef ioName = JSStringCreateWithUTF8CString(n);           \
        JSObjectSetProperty(JABC_CONTEXT, JABC_GLOBAL_OBJECT, ioName, o, \
                            kJSPropertyAttributeNone, NULL);             \
        JSStringRelease(ioName);                                         \
    }

#define JS_SET_PROPERTY_FN(o, n, f)                                \
    {                                                              \
        JSStringRef fn = JSStringCreateWithUTF8CString(n);         \
        JSObjectSetProperty(                                       \
            JABC_CONTEXT, o, fn,                                   \
            JSObjectMakeFunctionWithCallback(JABC_CONTEXT, fn, f), \
            kJSPropertyAttributeNone, NULL);                       \
        JSStringRelease(fn);                                       \
    }

#define JS_ADD_METHOD(o, n, fn)                                  \
    {                                                            \
        JSStringRef someName = JSStringCreateWithUTF8CString(n); \
        JSObjectRef someFunc =                                   \
            JSObjectMakeFunctionWithCallback(ctx, someName, fn); \
        JSObjectSetProperty(ctx, o, someName, someFunc,          \
                            kJSPropertyAttributeNone, NULL);     \
        JSStringRelease(someName);                               \
    }

JSValueRef JABCutf8cpMakeValueRef(JSContextRef ctx, utf8cp str);
ok64 JABCutf8bFeedStringRef(u8b into, JSStringRef str);
ok64 JABCutf8bFeedValueRef(u8b into, JSContextRef ctx, JSValueRef val);
JSValueRef JABCutf8CopyStringValue(JSContextRef ctx, u8sp into, JSValueRef val,
                                   JSValueRef* exception);

JSValueRef JSOfCString(const char* str);

void JABCExecute(const char* script);
void JABCReport(JSValueRef exception);

#endif
