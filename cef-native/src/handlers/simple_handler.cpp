// cef_native/src/simple_handler.cpp
#include "../../include/handlers/simple_handler.h"
#include "../../include/handlers/simple_app.h"
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

extern void CreateOverlayBrowserIfNeeded(HINSTANCE hInstance);

std::string SimpleHandler::pending_panel_;

SimpleHandler::SimpleHandler(const std::string& role) : role_(role) {}

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
CefRefPtr<CefBrowser> SimpleHandler::overlay_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::GetOverlayBrowser() {
    return overlay_browser_;
}


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

    if (!isLoading && role_ == "overlay" && !pending_panel_.empty()) {
        std::string panel = pending_panel_;

        // Delay JS execution slightly to ensure React is mounted
        CefPostDelayedTask(TID_UI, base::BindOnce([]() {
            CefRefPtr<CefBrowser> overlay = SimpleHandler::GetOverlayBrowser();
            if (overlay && overlay->GetMainFrame()) {
                std::string js = "window.triggerPanel('" + SimpleHandler::pending_panel_ + "')";
                overlay->GetMainFrame()->ExecuteJavaScript(js, overlay->GetMainFrame()->GetURL(), 0);
                std::cout << "🧠 Deferred panel triggered after delay: " << SimpleHandler::pending_panel_ << std::endl;
                SimpleHandler::pending_panel_.clear();
            } else {
                std::cout << "⚠️ Overlay browser still not ready. Skipping panel trigger." << std::endl;
            }
        }), 100); // delay in milliseconds
    }
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();

    std::cout << "✅ OnAfterCreated for role: " << role_ << std::endl;

    if (role_ == "webview") {
        webview_browser_ = browser;
        std::cout << "📡 WebView browser reference stored." << std::endl;
    } else if (role_ == "header") {
        std::cout << "🧭 header browser initialized." << std::endl;
    }else if (role_ == "overlay") {
        overlay_browser_ = browser;
        std::cout << "🪟 Overlay browser initialized." << std::endl;
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
    std::cout << "📨 Message received: " << message_name
          << ", Browser ID: " << browser->GetIdentifier() << std::endl;

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

    if (message->GetName() == "overlay_open_panel") {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        std::string panel = args->GetString(0);

        // Create if needed, show HWND, make it interactive
        extern HINSTANCE g_hInstance;
        CreateOverlayBrowserIfNeeded(g_hInstance);
        ShowWindow(g_overlay_hwnd, SW_SHOW);
        LONG exStyle = GetWindowLong(g_overlay_hwnd, GWL_EXSTYLE);
        SetWindowLong(g_overlay_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);

        // Trigger React panel inside overlay
        CefRefPtr<CefBrowser> overlay = SimpleHandler::GetOverlayBrowser();
        if (overlay && overlay->GetMainFrame()) {
            std::string js = "window.triggerPanel('" + panel + "')";
            overlay->GetMainFrame()->ExecuteJavaScript(js, overlay->GetMainFrame()->GetURL(), 0);
            std::cout << "🧠 Triggering overlay panel immediately: " << panel << std::endl;
        } else {
            std::cout << "🕒 Deferring overlay panel trigger until browser is ready: " << panel << std::endl;
            SimpleHandler::pending_panel_ = panel;
        }

        return true;
    }

    if (message_name == "overlay_hide") {
        std::cout << "🪟 Hiding overlay HWND" << std::endl;
        ShowWindow(g_overlay_hwnd, SW_HIDE);
        return true;
    }

    if (message_name == "overlay_show") {
        std::cout << "🪟 Showing overlay HWND" << std::endl;
        ShowWindow(g_overlay_hwnd, SW_SHOW);
        return true;
    }

    if (message_name == "overlay_input") {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        bool enable = args->GetBool(0);
        std::cout << "🪟 Setting overlay input: " << (enable ? "enabled" : "disabled") << std::endl;

        LONG exStyle = GetWindowLong(g_overlay_hwnd, GWL_EXSTYLE);
        if (enable) {
            SetWindowLong(g_overlay_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        } else {
            SetWindowLong(g_overlay_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
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
