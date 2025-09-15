// cef_native/src/simple_render_process_handler.cpp
#include "../../include/handlers/simple_render_process_handler.h"
#include "../../include/core/IdentityHandler.h"
#include "../../include/core/NavigationHandler.h"
#include "../../include/core/PanelHandler.h"
#include "../../include/core/AddressHandler.h"
#include "wrapper/cef_helpers.h"
#include "include/cef_v8.h"
#include <iostream>

SimpleRenderProcessHandler::SimpleRenderProcessHandler() {
    std::cout << "🔧 SimpleRenderProcessHandler constructor called!" << std::endl;
    std::cout << "🔧 Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "🔧 Thread ID: " << GetCurrentThreadId() << std::endl;
}

void SimpleRenderProcessHandler::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {

    CEF_REQUIRE_RENDERER_THREAD();

    std::cout << "🔧 OnContextCreated called for browser ID: " << browser->GetIdentifier() << std::endl;
    std::cout << "🔧 Frame URL: " << frame->GetURL().ToString() << std::endl;
    std::cout << "🔧 Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "🔧 Thread ID: " << GetCurrentThreadId() << std::endl;
    std::cout << "🔧 RENDER PROCESS HANDLER IS WORKING!" << std::endl;
    std::cout << "🔧 THIS IS THE RENDER PROCESS HANDLER!" << std::endl;

    // Check if this is the overlay browser
    if (frame->GetURL().ToString().find("/overlay") != std::string::npos) {
        std::cout << "🎯 OVERLAY BROWSER V8 CONTEXT CREATED!" << std::endl;
        std::cout << "🎯 Setting up bitcoinBrowser for overlay browser" << std::endl;
    }

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

    CefRefPtr<CefV8Value> overlayPanelObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("overlayPanel", overlayPanelObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind PanelHandler
    CefRefPtr<PanelHandler> panelHandler = new PanelHandler();
    overlayPanelObject->SetValue("open",
        CefV8Value::CreateFunction("open", panelHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    // Create the address object
    CefRefPtr<CefV8Value> addressObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("address", addressObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind AddressHandler
    CefRefPtr<AddressHandler> addressHandler = new AddressHandler();
    addressObject->SetValue("generate",
        CefV8Value::CreateFunction("generate", addressHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);
}

bool SimpleRenderProcessHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {

    CEF_REQUIRE_RENDERER_THREAD();

    std::string message_name = message->GetName();
    std::cout << "📨 Render process received message: " << message_name << std::endl;
    std::cout << "🔍 Browser ID: " << browser->GetIdentifier() << std::endl;
    std::cout << "🔍 Frame URL: " << frame->GetURL().ToString() << std::endl;
    std::cout << "🔍 Source Process: " << source_process << std::endl;

        if (message_name == "address_generate_response") {
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            std::string addressDataJson = args->GetString(0);

            std::cout << "✅ Address generation response received: " << addressDataJson << std::endl;
            std::cout << "🔍 Browser ID: " << browser->GetIdentifier() << std::endl;
            std::cout << "🔍 Frame URL: " << frame->GetURL().ToString() << std::endl;

        // Execute JavaScript to handle the response
        std::string js = "if (window.onAddressGenerated) { window.onAddressGenerated(" + addressDataJson + "); }";
        frame->ExecuteJavaScript(js, frame->GetURL(), 0);

        return true;
    }

    if (message_name == "address_generate_error") {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        std::string errorMessage = args->GetString(0);

        std::cout << "❌ Address generation error received: " << errorMessage << std::endl;

        // Execute JavaScript to handle the error
        std::string js = "if (window.onAddressError) { window.onAddressError('" + errorMessage + "'); }";
        frame->ExecuteJavaScript(js, frame->GetURL(), 0);

        return true;
    }

    return false;
}
