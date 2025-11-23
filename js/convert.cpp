//
// Created by gritzko on 11/12/25.
//
#include <cmath>

#include "JABC.hpp"
#include "abc/ABC.hpp"
#include "abc/PRO.h"
#include "rdx/RDX.hpp"

a_cstr(RDX_EMPTY_TUPLE, "p\01\00");

ok64 JABCu8bImport(u8bp builder, JSContextRef ctx, JSValueRef val,
                   JSValueRef* exception);

ok64 JABCu8bImportString(u8bp builder, JSContextRef ctx, JSStringRef str,
                         JSValueRef* exception) {
    sane(builder != nullptr);
    call(TLVu8bInto, builder, RDX_STRING);
    call(u8bFeed1, builder, 0);
    size_t maxSize = JSStringGetMaximumUTF8CStringSize(str);
    // todo call(u8bReserve, builder, maxSize);
    u8sp idle = u8bIdle(builder);
    size_t factlen = JSStringGetUTF8CString(str, (char*)*idle, maxSize);
    *idle += factlen-1; // null term
    call(TLVu8bOuto, builder, RDX_STRING);
    done;
}

ok64 JABCu8bImportObject(u8bp builder, JSContextRef ctx, JSObjectRef obj,
                         JSValueRef* exception) {
    sane(builder != nullptr);
    call(TLVu8bInto, builder, RDX_EULER);
    call(u8bFeed1, builder, 0);
    JSPropertyNameArrayRef keys = JSObjectCopyPropertyNames(ctx, obj);
    size_t len = JSPropertyNameArrayGetCount(keys);
    for (size_t i = 0; i < len; i++) {
        JSStringRef key = JSPropertyNameArrayGetNameAtIndex(keys, i);
        call(TLVu8bInto, builder, RDX_TUPLE);
        call(u8bFeed1, builder, 0);
        call(JABCu8bImportString, builder, ctx, key, exception);
        JSValueRef val = JSObjectGetProperty(ctx, obj, key, exception);
        call(JABCu8bImport, builder, ctx, val, exception);
        call(TLVu8bOuto, builder, RDX_TUPLE);
    }
    call(TLVu8bOuto, builder, RDX_EULER);
    done;
}

ok64 JABCu8bImport(u8bp builder, JSContextRef ctx, JSValueRef val,
                   JSValueRef* exception) {
    // u8bAllocate();
    //  u8bReserve()
    sane(u8bOK(builder) && ctx != nullptr && val != nullptr);
    JSType t = JSValueGetType(ctx, val);
    switch (t) {
        case kJSTypeUndefined: {
            call(u8bFeed, builder, RDX_EMPTY_TUPLE);
            done;
        }
        case kJSTypeNull:
            break;
        case kJSTypeBoolean:
            break;
        case kJSTypeNumber: {
            double d = JSValueToNumber(ctx, val, exception);
            rdx r = {};
            if (floor(d) != d) {
                r.type = RDX_FLOAT;
                r.f = d;
                call(RDXu8sFeedF, u8bIdle(builder), &r);
            } else {
                r.type = RDX_INT;
                r.i = (i64)d;
                call(RDXu8sFeedI, u8bIdle(builder), &r);
            }
            done;
        }
        case kJSTypeString: {
            JSStringRef str = JSValueToStringCopy(ctx, val, exception);
            return JABCu8bImportString(builder, ctx, str, exception);
        }
        case kJSTypeObject: {
            return JABCu8bImportObject(builder, ctx, (JSObjectRef)val,
                                       exception);
        }
    }
    done;
}

ok64 JABCrdxExport(rdxb reader, JSContextRef ctx, JSValueRef* val,
                   JSValueRef* exception) {
    sane(reader != nullptr && rdxbDataLen(reader) > 0);
    rdxp top = rdxbLast(reader);
    switch (top->type) {
        case RDX_FLOAT:
            *val = JSValueMakeNumber(ctx, top->f);
            done;
        case RDX_INT:
            *val = JSValueMakeNumber(ctx, top->i);
            done;
        default:
            fail(notimplyet);
    }
    done;
}