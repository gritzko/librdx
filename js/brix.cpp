//
// Created by gritzko on 10/27/25.
//
#include "JABC.h"
#include "JavaScriptCore/JSBase.h"
#include "JavaScriptCore/JSStringRef.h"
#include "JavaScriptCore/JSTypedArray.h"
#include "JavaScriptCore/JSValueRef.h"
#include "abc/PRO.h"
#include "rdx/BRIX2.h"

/*
 *   brix.Open("hello") // ?
 *   var obj1 = brix.Get("2s-18b") // ? replica-1, 0-1 -> {"title":id}
 *   var obj = brix.GetByTag("servers")
 *   io.log(obj.message)
 *   obj.time = io.now()
 *   obj.addr = {host:"google.com",port:443}
 *   io.log(""+obj)
 *   var hash = brix.Seal()
 *
 *   brix.Join("bonjour")
 *
 *   brix.Merge()
 *
 *   brix.TagPrivate()
 *   brix.TagPublic()
 */