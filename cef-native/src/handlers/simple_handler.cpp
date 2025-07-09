// cef_native/src/simple_handler.cpp
#include "../../include/handlers/simple_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/base/cef_bind.h"
#include "include/cef_v8.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/cef_task.h"
#include "base/cef_callback.h"
#include "base/internal/cef_callback_internal.h"
#include <windows.h>
#include <iostream>
#include <string>

SimpleHandler::SimpleHandler(const std::string& role) : role_(role) {}

// SimpleHandler::SimpleHandler() {}

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() {
    return this;
}

CefRefPtr<CefDisplayHandler> SimpleHandler::GetDisplayHandler() {
    return this;
}

CefRefPtr<CefLoadHandler> SimpleHandler::GetLoadHandler() {
    return this;
}

CefRefPtr<CefBrowser> SimpleHandler::webview_browser_ = nullptr;

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
#if defined(OS_WIN)
    SetWindowText(browser->GetHost()->GetWindowHandle(), std::wstring(title).c_str());
#endif
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
    std::wcout << L"❌ Load error: " << std::wstring(failedUrl)
               << L" - " << std::wstring(errorText) << std::endl;

    if (frame->IsMain()) {
        std::string html = "<html><body><h1>Failed to load</h1><p>URL: " +
                           failedUrl.ToString() + "</p><p>Error: " +
                           errorText.ToString() + "</p></body></html>";

        std::string encoded_html;
        for (char c : html) {
            if (isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '.' || c == '-' || c == '_' || c == ':')
                encoded_html += c;
            else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
                encoded_html += buf;
            }
        }

        std::string data_url = "data:text/html," + encoded_html;
        frame->LoadURL(data_url);
    }
}

void SimpleHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
    std::cout << "📡 Loading state: " << (isLoading ? "loading..." : "done") << std::endl;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();

    std::cout << "✅ OnAfterCreated for role: " << role_ << std::endl;

    if (role_ == "webview") {
        webview_browser_ = browser;
        std::cout << "📡 WebView browser reference stored." << std::endl;
    } else if (role_ == "shell") {
        std::cout << "🧭 Shell browser initialized." << std::endl;
        // Optional: store shell_browser_ if you need to send messages to it later
    }
}

// bool SimpleHandler::OnProcessMessageReceived(
//     CefRefPtr<CefBrowser> browser,
//     CefRefPtr<CefFrame> frame,
//     CefProcessId source_process,
//     CefRefPtr<CefProcessMessage> message) {

//     CEF_REQUIRE_UI_THREAD();  // Ensure this is running on the UI thread

//     std::string msg_name = message->GetName();
//     std::cout << "📨 Browser received message: " << msg_name << std::endl;

//     if (msg_name == "navigate") {
//         std::string path = message->GetArgumentList()->GetString(0);

//         static int counter = 0;
//         std::cout << "🚀 JS-triggered navigate #" << ++counter << " to: " << path << std::endl;

//         // Prepend "http://" if necessary
//         if (!(path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0)) {
//             path = "http://" + path;
//         }

//         std::cout << "🌐 Loading URL from SimpleHandler: " << path << std::endl;

//         // Post the navigation task to the UI thread using CefPostTask
//         CefPostTask(TID_UI, base::BindOnce([](CefRefPtr<CefBrowser> browser, std::string path) {
//             // Ensure browser and frame are valid
//             if (!browser || !browser->GetMainFrame()) {
//                 std::cout << "⚠️ Error: Invalid browser or frame!" << std::endl;
//                 return;
//             }

//             std::cout << "🌐 Navigating to: " << path << std::endl;  // This is already added
//             browser->GetMainFrame()->LoadURL(path);  // Perform the actual navigation
//             std::cout << "🌐 Navigation command executed for: " << path << std::endl;
//         }, browser, path));

//         return true;  // Message handled
//     }

//     return false;  // Message not handled
// }

bool SimpleHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message
) {
    CEF_REQUIRE_UI_THREAD();

    std::string message_name = message->GetName();
    std::cout << "📨 Message received: " << message_name << std::endl;

    if (message_name == "navigate") {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        std::string path = args->GetString(0);

        // Normalize protocol
        if (!(path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0)) {
            path = "http://" + path;
        }

        std::cout << "🔁 Forwarding navigation to webview: " << path << std::endl;

        if (SimpleHandler::webview_browser_ && SimpleHandler::webview_browser_->GetMainFrame()) {
            SimpleHandler::webview_browser_->GetMainFrame()->LoadURL(path);
        } else {
            std::cout << "⚠️ WebView browser not available or not fully initialized." << std::endl;
        }

        return true;
    }

    return false;
}
