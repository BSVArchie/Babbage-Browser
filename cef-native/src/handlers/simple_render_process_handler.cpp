// cef_native/src/simple_render_process_handler.cpp
#include "../../include/handlers/simple_render_process_handler.h"
#include "../../include/core/IdentityHandler.h"
#include "../../include/core/NavigationHandler.h"
#include "wrapper/cef_helpers.h"
#include "include/cef_v8.h"
#include <iostream>

SimpleRenderProcessHandler::SimpleRenderProcessHandler() {}

void SimpleRenderProcessHandler::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {

    std::cout << "🚀 [V8] OnContextCreated START" << std::endl;

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

    // Create the navigation object inside bitcoinBrowser
    CefRefPtr<CefV8Value> navigationObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("navigation", navigationObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind the NavigationHandler instance
    CefRefPtr<NavigationHandler> navigationHandler = new NavigationHandler();

    navigationObject->SetValue("navigate",
        CefV8Value::CreateFunction("navigate", navigationHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    // Create the overlayPanel object
    std::cout << "🚀 V8: Starting overlayPanel setup" << std::endl;

    // CefRefPtr<CefV8Value> overlayPanelObject = CefV8Value::CreateObject(nullptr, nullptr);
    // if (!overlayPanelObject) {
    // std::cout << "❌ V8: overlayPanelObject creation failed" << std::endl;
    // } else {
    // std::cout << "✅ V8: overlayPanelObject created" << std::endl;
    // bitcoinBrowser->SetValue("overlayPanel", overlayPanelObject, V8_PROPERTY_ATTRIBUTE_READONLY);
    // std::cout << "✅ V8: overlayPanel attached to bitcoinBrowser" << std::endl;
    // }
    CefRefPtr<CefV8Value> overlayPanelObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("overlayPanel", overlayPanelObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind PanelHandler
    CefRefPtr<PanelHandler> panelHandler = new PanelHandler();
    overlayPanelObject->SetValue("open",
        CefV8Value::CreateFunction("open", panelHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);
}
