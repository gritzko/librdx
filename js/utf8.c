#include "JS.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/OK.h"

void FreeDeallocator(void* bytes, void* deallocatorContext) { free(bytes); }

JSValueRef utf8_en(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[],
                   JSValueRef* exception) {
    if (argc != 1 || !JS_ARG_IS_STRING(0)) JS_THROW("utf8(string)->utf8");
    JSStringRef str = JSValueToStringCopy(ctx, args[0], exception);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    char* bytes = malloc(maxSize);
    size_t factlen = JSStringGetUTF8CString(str, bytes, maxSize);
    JSObjectRef ta2 = JSObjectMakeTypedArrayWithBytesNoCopy(
        ctx, kJSTypedArrayTypeUint8Array, bytes, factlen, FreeDeallocator,
        bytes, exception);
    return ta2;
}

JSValueRef utf8_de(JSContextRef ctx, JSObjectRef function, JSObjectRef self,
                   size_t argc, const JSValueRef args[],
                   JSValueRef* exception) {
    JSValueRef n = JSValueMakeUndefined(ctx);
    return n;
}

ok64 utf8_install() {
    JS_API_OBJECT(utf8, "utf8");
    JS_SET_PROPERTY_FN(utf8, "en", utf8_en);
    JS_SET_PROPERTY_FN(utf8, "Encode", utf8_en);
    JS_SET_PROPERTY_FN(utf8, "de", utf8_de);
    JS_SET_PROPERTY_FN(utf8, "Decode", utf8_de);
    return OK;
}
