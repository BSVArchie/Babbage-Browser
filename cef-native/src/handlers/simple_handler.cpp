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
    }else if (role_ == "overlay") {
        std::cout << "🪟 Overlay browser initialized." << std::endl;
        // Optional: store overlay_browser_ if you want dynamic control later
    }

    std::cout << "🧭 Browser Created → role: " << role_
          << ", ID: " << browser->GetIdentifier()
          << ", IsPopup: " << browser->IsPopup()
          << ", MainFrame URL: " << browser->GetMainFrame()->GetURL().ToString()
          << std::endl;
}

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

void SimpleHandler::SetRenderHandler(CefRefPtr<CefRenderHandler> handler) {
    render_handler_ = handler;
}

CefRefPtr<CefRenderHandler> SimpleHandler::GetRenderHandler() {
    return render_handler_;
}
