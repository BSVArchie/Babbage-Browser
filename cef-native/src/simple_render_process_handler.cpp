// cef_native/src/simple_render_process_handler.cpp
#include "../include/simple_render_process_handler.h"
#include "../include/wallet/IdentityHandler.h"
#include "wrapper/cef_helpers.h"
#include "include/cef_v8.h"
#include <iostream>

SimpleRenderProcessHandler::SimpleRenderProcessHandler() {}

void SimpleRenderProcessHandler::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {

    CEF_REQUIRE_RENDERER_THREAD();

    CefRefPtr<CefV8Value> global = context->GetGlobal();

    // Create the bitcoinBrowser object
    CefRefPtr<CefV8Value> bitcoinBrowser = CefV8Value::CreateObject(nullptr, nullptr);
    global->SetValue("bitcoinBrowser", bitcoinBrowser, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Create the identity object inside bitcoinBrowser
    CefRefPtr<CefV8Value> identityObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("identity", identityObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind the IdentityHandler instance
    CefRefPtr<IdentityHandler> identityHandler = new IdentityHandler();

    identityObject->SetValue("get",
        CefV8Value::CreateFunction("get", identityHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    identityObject->SetValue("markBackedUp",
        CefV8Value::CreateFunction("markBackedUp", identityHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);
}
