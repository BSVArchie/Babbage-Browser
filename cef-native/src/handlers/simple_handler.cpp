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
#include <fstream>
#include "../../include/core/WalletService.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

extern void CreateOverlayBrowserIfNeeded(HINSTANCE hInstance);
extern void CreateTestOverlayWithSeparateProcess(HINSTANCE hInstance);

std::string SimpleHandler::pending_panel_;
bool SimpleHandler::needs_overlay_reload_ = false;

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

void SimpleHandler::TriggerDeferredPanel(const std::string& panel) {
    CefRefPtr<CefBrowser> overlay = SimpleHandler::GetOverlayBrowser();
    if (overlay && overlay->GetMainFrame()) {
        std::string js = "window.triggerPanel('" + panel + "')";
        overlay->GetMainFrame()->ExecuteJavaScript(js, overlay->GetMainFrame()->GetURL(), 0);
        std::cout << "🧠 Deferred panel triggered after delay: " << panel << std::endl;
    } else {
        std::cout << "⚠️ Overlay browser still not ready. Skipping panel trigger." << std::endl;
    }
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
    std::cout << "❌ Load error for role: " << role_ << std::endl;
    std::wcout << L"❌ Load error: " << std::wstring(failedUrl)
               << L" - " << std::wstring(errorText) << std::endl;
    std::cout << "❌ Error code: " << errorCode << std::endl;

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
    std::cout << "📡 Loading state for role " << role_ << ": " << (isLoading ? "loading..." : "done") << std::endl;
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "📡 Loading state for role " << role_ << ": " << (isLoading ? "loading..." : "done") << std::endl;
    debugLog.close();

    if (role_ == "overlay") {
        std::cout << "📡 Overlay URL: " << browser->GetMainFrame()->GetURL().ToString() << std::endl;
    }

    if (!isLoading) {
        if (role_ == "overlay") {
            // Log that we're about to inject the API
            std::ofstream debugLog("debug_output.log", std::ios::app);
            debugLog << "🔧 OVERLAY LOADED - About to inject bitcoinBrowser API" << std::endl;
            debugLog.close();

            // Inject the bitcoinBrowser API when overlay finishes loading
            extern void InjectBitcoinBrowserAPI(CefRefPtr<CefBrowser> browser);
            InjectBitcoinBrowserAPI(browser);
        } else if (role_ == "webview") {
            // Inject the bitcoinBrowser API into webview browser as well
            std::cout << "🔧 WEBVIEW BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
            std::ofstream debugLog("debug_output.log", std::ios::app);
            debugLog << "🔧 WEBVIEW BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
            debugLog.close();

            extern void InjectBitcoinBrowserAPI(CefRefPtr<CefBrowser> browser);
            InjectBitcoinBrowserAPI(browser);
        } else if (role_ == "header") {
            // Inject the bitcoinBrowser API into header browser (where React app runs)
            std::cout << "🔧 HEADER BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
            std::ofstream debugLog("debug_output.log", std::ios::app);
            debugLog << "🔧 HEADER BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
            debugLog.close();

            extern void InjectBitcoinBrowserAPI(CefRefPtr<CefBrowser> browser);
            InjectBitcoinBrowserAPI(browser);
        }

        // Overlay-specific logic
        if (role_ == "overlay") {
            // Check if we need to reload the overlay
            if (needs_overlay_reload_) {
                std::cout << "🔄 Overlay finished loading, now reloading React app" << std::endl;
                needs_overlay_reload_ = false;
                browser->GetMainFrame()->LoadURL("http://127.0.0.1:5137/overlay");
                std::cout << "🔄 LoadURL called for overlay reload" << std::endl;
                return; // Don't process pending panels yet, wait for reload to complete
            }

            // Handle pending panel triggers
            if (!pending_panel_.empty()) {
                std::string panel = pending_panel_;
                std::cout << "🕒 OnLoadingStateChange: Creating deferred trigger for panel: " << panel << std::endl;

                // Clear pending_panel_ immediately to prevent duplicate deferred triggers
                SimpleHandler::pending_panel_.clear();

                // Delay JS execution slightly to ensure React is mounted
                // Use a simple function call instead of lambda to avoid CEF bind issues
                CefPostDelayedTask(TID_UI, base::BindOnce(&SimpleHandler::TriggerDeferredPanel, panel), 100);
            }
        }
    }
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();

    std::cout << "✅ OnAfterCreated for role: " << role_ << std::endl;

    if (role_ == "webview") {
        webview_browser_ = browser;
        std::cout << "📡 WebView browser reference stored." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "📡 WebView browser reference stored. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    } else if (role_ == "header") {
        std::cout << "🧭 header browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🧭 header browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    }else if (role_ == "overlay") {
        overlay_browser_ = browser;
        std::cout << "🪟 Overlay browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🪟 Overlay browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
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

    if (message_name == "address_generate") {
        std::cout << "🔑 Address generation requested via process message" << std::endl;

        // Generate address using WalletService
        WalletService walletService;
        nlohmann::json addressData = walletService.generateAddress();

        std::cout << "✅ Address generated: " << addressData.dump() << std::endl;

        // Send response back to the render process
        CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("address_generate_response");
        CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();

        // Create response object
        nlohmann::json responseJson;
        responseJson["success"] = true;
        responseJson["data"] = addressData;

        responseArgs->SetString(0, responseJson.dump());

        // Send response back to the frame that requested it
        frame->SendProcessMessage(PID_RENDERER, response);

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
        if (overlay && overlay->GetMainFrame() && !overlay->IsLoading()) {
            std::cout << "🔍 Browser state - ID: " << overlay->GetIdentifier()
                      << ", URL: " << overlay->GetMainFrame()->GetURL().ToString()
                      << ", IsLoading: " << overlay->IsLoading() << std::endl;
            std::string js = "window.triggerPanel('" + panel + "')";
            overlay->GetMainFrame()->ExecuteJavaScript(js, overlay->GetMainFrame()->GetURL(), 0);
            std::cout << "🧠 Triggering overlay panel immediately: " << panel << std::endl;
        } else {
            std::cout << "🕒 Deferring overlay panel trigger until browser is ready: " << panel << std::endl;
            // Always update pending_panel_ to the latest request
            SimpleHandler::pending_panel_ = panel;
            std::cout << "🕒 Set pending_panel_ to: " << panel << std::endl;
        }

        return true;
    }

    if (message_name == "overlay_hide") {
        std::cout << "🪟 Hiding overlay HWND" << std::endl;
        std::cout << "�� Before hide - EXSTYLE: 0x" << std::hex << GetWindowLong(g_overlay_hwnd, GWL_EXSTYLE) << std::endl;
        ShowWindow(g_overlay_hwnd, SW_HIDE);
        std::cout << "🪟 After hide - EXSTYLE: 0x" << std::hex << GetWindowLong(g_overlay_hwnd, GWL_EXSTYLE) << std::endl;
        return true;
    }

    if (message_name == "overlay_show") {
        std::cout << "🪟 overlay_show message received" << std::endl;
        std::cout << "🪟 Showing existing overlay HWND" << std::endl;
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

    if (message_name == "address_generate") {
        std::cout << "🔑 Address generation requested from browser ID: " << browser->GetIdentifier() << std::endl;

        try {
            // Call WalletService to generate address
            WalletService walletService;
            nlohmann::json addressData = walletService.generateAddress();

            std::cout << "✅ Address generated successfully: " << addressData.dump() << std::endl;

            // Send result back to the requesting browser
            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("address_generate_response");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, addressData.dump());

                    browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
                    std::cout << "📤 Address data sent back to browser" << std::endl;
                    std::cout << "🔍 Browser ID: " << browser->GetIdentifier() << std::endl;
                    std::cout << "🔍 Frame URL: " << browser->GetMainFrame()->GetURL().ToString() << std::endl;

        } catch (const std::exception& e) {
            std::cout << "❌ Address generation failed: " << e.what() << std::endl;

            // Send error response
            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("address_generate_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, e.what());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }

    return false;
}

CefRefPtr<CefRequestHandler> SimpleHandler::GetRequestHandler() {
    return this;
}

void SimpleHandler::SetRenderHandler(CefRefPtr<CefRenderHandler> handler) {
    render_handler_ = handler;
}

CefRefPtr<CefRenderHandler> SimpleHandler::GetRenderHandler() {
    return render_handler_;
}
