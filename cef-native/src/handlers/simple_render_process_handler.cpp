// cef_native/src/simple_render_process_handler.cpp
#include "../../include/handlers/simple_render_process_handler.h"
#include "../../include/core/IdentityHandler.h"
#include "../../include/core/NavigationHandler.h"
#include "../../include/core/AddressHandler.h"
#include "wrapper/cef_helpers.h"
#include "include/cef_v8.h"
#include <iostream>
#include <fstream>

// Handler for cefMessage.send() function
class CefMessageSendHandler : public CefV8Handler {
public:
    CefMessageSendHandler() {}

    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override {

        CEF_REQUIRE_RENDERER_THREAD();

        if (arguments.size() < 1) {
            exception = "cefMessage.send() requires at least one argument (message name)";
            return true;
        }

        std::string messageName = arguments[0]->GetStringValue();
        std::cout << "📤 cefMessage.send() called with message: " << messageName << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "📤 cefMessage.send() called with message: " << messageName << std::endl;
        debugLog.close();

        // Create the process message
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(messageName);
        CefRefPtr<CefListValue> args = message->GetArgumentList();

        // Add arguments if provided
        for (size_t i = 1; i < arguments.size(); i++) {
            if (arguments[i]->IsString()) {
                args->SetString(i - 1, arguments[i]->GetStringValue());
            } else if (arguments[i]->IsBool()) {
                args->SetBool(i - 1, arguments[i]->GetBoolValue());
            } else if (arguments[i]->IsInt()) {
                args->SetInt(i - 1, arguments[i]->GetIntValue());
            } else if (arguments[i]->IsDouble()) {
                args->SetDouble(i - 1, arguments[i]->GetDoubleValue());
            }
        }

        // Send the message to the browser process
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        if (context && context->GetFrame()) {
            context->GetFrame()->SendProcessMessage(PID_BROWSER, message);
            std::cout << "✅ Process message sent to browser process: " << messageName << std::endl;
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "✅ Process message sent to browser process: " << messageName << std::endl;
            debugLog2.close();
        } else {
            std::cout << "❌ Failed to get frame context for sending process message" << std::endl;
            std::ofstream debugLog3("debug_output.log", std::ios::app);
            debugLog3 << "❌ Failed to get frame context for sending process message" << std::endl;
            debugLog3.close();
        }

        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(CefMessageSendHandler);
};

// Handler for overlay.close() function
class OverlayCloseHandler : public CefV8Handler {
public:
    OverlayCloseHandler() {}

    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override {

        CEF_REQUIRE_RENDERER_THREAD();

        std::cout << "🎯 overlay.close() called from overlay browser" << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🎯 overlay.close() called from overlay browser" << std::endl;
        debugLog.close();

        // Send overlay_close message via cefMessage
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        if (context && context->GetFrame()) {
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("overlay_close");
            context->GetFrame()->SendProcessMessage(PID_BROWSER, message);

            std::cout << "✅ overlay.close() sent overlay_close message" << std::endl;
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "✅ overlay.close() sent overlay_close message" << std::endl;
            debugLog2.close();
        }

        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(OverlayCloseHandler);
};

SimpleRenderProcessHandler::SimpleRenderProcessHandler() {
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "🔧 SimpleRenderProcessHandler constructor called!" << std::endl;
    debugLog << "🔧 Process ID: " << GetCurrentProcessId() << std::endl;
    debugLog << "🔧 Thread ID: " << GetCurrentThreadId() << std::endl;
    debugLog.close();
}

void SimpleRenderProcessHandler::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {

    CEF_REQUIRE_RENDERER_THREAD();

    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "🔧 OnContextCreated called for browser ID: " << browser->GetIdentifier() << std::endl;
    debugLog << "🔧 Frame URL: " << frame->GetURL().ToString() << std::endl;
    debugLog << "🔧 Process ID: " << GetCurrentProcessId() << std::endl;
    debugLog << "🔧 Thread ID: " << GetCurrentThreadId() << std::endl;
    debugLog << "🔧 RENDER PROCESS HANDLER IS WORKING!" << std::endl;
    debugLog << "🔧 THIS IS THE RENDER PROCESS HANDLER!" << std::endl;
    debugLog.close();

    // Check if this is an overlay browser (any browser that's not the main root browser)
    std::string url = frame->GetURL().ToString();
    bool isMainBrowser = (url == "http://127.0.0.1:5137" || url == "http://127.0.0.1:5137/");
    bool isOverlayBrowser = !isMainBrowser && url.find("127.0.0.1:5137") != std::string::npos;

    if (isOverlayBrowser) {
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "🎯 OVERLAY BROWSER V8 CONTEXT CREATED!" << std::endl;
        debugLog2 << "🎯 URL: " << url << std::endl;
        debugLog2 << "🎯 Setting up bitcoinBrowser for overlay browser" << std::endl;
        debugLog2.close();
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

    // overlayPanel object removed - now using process-per-overlay architecture

    // Create the overlay object (for overlay browsers only)
    if (isOverlayBrowser) {
        std::ofstream debugLog3("debug_output.log", std::ios::app);
        debugLog3 << "🎯 Creating overlay object for URL: " << url << std::endl;
        debugLog3.close();

        CefRefPtr<CefV8Value> overlayObject = CefV8Value::CreateObject(nullptr, nullptr);
        bitcoinBrowser->SetValue("overlay", overlayObject, V8_PROPERTY_ATTRIBUTE_READONLY);

        // Add close method for overlay browsers - uses cefMessage internally
        overlayObject->SetValue("close",
            CefV8Value::CreateFunction("close", new OverlayCloseHandler()),
            V8_PROPERTY_ATTRIBUTE_NONE);

        std::ofstream debugLog4("debug_output.log", std::ios::app);
        debugLog4 << "🎯 Overlay object created with close method" << std::endl;
        debugLog4.close();
    } else {
        std::ofstream debugLog5("debug_output.log", std::ios::app);
        debugLog5 << "🎯 NOT creating overlay object for URL: " << url << std::endl;
        debugLog5 << "🎯 isMainBrowser: " << (isMainBrowser ? "true" : "false") << std::endl;
        debugLog5.close();
    }

    // Create the address object
    CefRefPtr<CefV8Value> addressObject = CefV8Value::CreateObject(nullptr, nullptr);
    bitcoinBrowser->SetValue("address", addressObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Bind AddressHandler
    CefRefPtr<AddressHandler> addressHandler = new AddressHandler();
    addressObject->SetValue("generate",
        CefV8Value::CreateFunction("generate", addressHandler),
        V8_PROPERTY_ATTRIBUTE_NONE);

    // Create the cefMessage object for process communication
    CefRefPtr<CefV8Value> cefMessageObject = CefV8Value::CreateObject(nullptr, nullptr);
    global->SetValue("cefMessage", cefMessageObject, V8_PROPERTY_ATTRIBUTE_READONLY);

    // Create the send function for cefMessage
    CefRefPtr<CefV8Value> sendFunction = CefV8Value::CreateFunction("send", new CefMessageSendHandler());
    cefMessageObject->SetValue("send", sendFunction, V8_PROPERTY_ATTRIBUTE_NONE);
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
