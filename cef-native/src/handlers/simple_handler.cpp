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
#include <filesystem>
#include <cstdlib>
#include "../../include/core/WalletService.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

extern void CreateTestOverlayWithSeparateProcess(HINSTANCE hInstance);
extern void CreateWalletOverlayWithSeparateProcess(HINSTANCE hInstance);
extern void CreateBackupOverlayWithSeparateProcess(HINSTANCE hInstance);

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
CefRefPtr<CefBrowser> SimpleHandler::header_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::overlay_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::settings_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::wallet_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::backup_browser_ = nullptr;
CefRefPtr<CefBrowser> SimpleHandler::GetOverlayBrowser() {
    return overlay_browser_;
}
CefRefPtr<CefBrowser> SimpleHandler::GetHeaderBrowser() {
    return header_browser_;
}

CefRefPtr<CefBrowser> SimpleHandler::GetWebviewBrowser() {
    return webview_browser_;
}

CefRefPtr<CefBrowser> SimpleHandler::GetSettingsBrowser() {
    return settings_browser_;
}
CefRefPtr<CefBrowser> SimpleHandler::GetWalletBrowser() {
    return wallet_browser_;
}
CefRefPtr<CefBrowser> SimpleHandler::GetBackupBrowser() {
    return backup_browser_;
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

    if (role_ == "backup") {
        std::cout << "📡 Backup URL: " << browser->GetMainFrame()->GetURL().ToString() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "📡 Backup URL: " << browser->GetMainFrame()->GetURL().ToString() << std::endl;
        debugLog.close();
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
        } else if (role_ == "settings") {
            // Inject the bitcoinBrowser API into settings browser
            std::cout << "🔧 SETTINGS BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
            std::ofstream debugLog("debug_output.log", std::ios::app);
            debugLog << "🔧 SETTINGS BROWSER LOADED - Injecting bitcoinBrowser API" << std::endl;
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
        header_browser_ = browser;
        std::cout << "🧭 header browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🧭 header browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    } else if (role_ == "overlay") {
        overlay_browser_ = browser;
        std::cout << "🪟 Overlay browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🪟 Overlay browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    } else if (role_ == "settings") {
        settings_browser_ = browser;
        std::cout << "⚙️ Settings browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "⚙️ Settings browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    } else if (role_ == "wallet") {
        wallet_browser_ = browser;
        std::cout << "💰 Wallet browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💰 Wallet browser initialized. ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();
    } else if (role_ == "backup") {
        backup_browser_ = browser;
        std::cout << "💾 Backup browser initialized." << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💾 Backup browser initialized. ID: " << browser->GetIdentifier() << std::endl;
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

    // Duplicate address_generate handler removed - keeping the one at line 489


    if (message_name == "force_repaint") {
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🔄 Force repaint requested for " << role_ << " browser" << std::endl;
        debugLog.close();

        if (browser) {
            browser->GetHost()->Invalidate(PET_VIEW);
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "🔄 Browser invalidated for " << role_ << " browser" << std::endl;
            debugLog2.close();
        }
        return true;
    }

    if (message_name == "identity_status_check") {
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🔍 Identity status check requested" << std::endl;
        debugLog << "🔍 Current working directory: " << std::filesystem::current_path().string() << std::endl;

        // Get the same path that the Go daemon uses
        const char* homeDir = std::getenv("USERPROFILE");
        std::string identityPath = std::string(homeDir) + "\\AppData\\Roaming\\BabbageBrowser\\identity.json";
        debugLog << "🔍 Looking for identity file at: " << identityPath << std::endl;
        debugLog.close();

        nlohmann::json response;

        // Check if identity.json file exists locally
        std::ifstream identityFile(identityPath);
        if (identityFile.good()) {
            try {
                // File exists - read it and check status
                nlohmann::json identity;
                identityFile >> identity;
                identityFile.close();

                bool backedUp = identity.value("backedUp", false);

                response["exists"] = true;
                response["needsBackup"] = !backedUp;
                response["identity"] = identity;

                std::ofstream debugLog2("debug_output.log", std::ios::app);
                debugLog2 << "📁 Identity file exists, backedUp: " << (backedUp ? "YES" : "NO") << std::endl;
                debugLog2.close();

            } catch (const std::exception& e) {
                identityFile.close();
                // File exists but corrupted
                response["exists"] = true;
                response["needsBackup"] = true;
                response["error"] = "Identity file corrupted: " + std::string(e.what());

                std::ofstream debugLog3("debug_output.log", std::ios::app);
                debugLog3 << "⚠️ Identity file corrupted: " << e.what() << std::endl;
                debugLog3.close();
            }
        } else {
            identityFile.close();
            // File doesn't exist
            response["exists"] = false;
            response["needsBackup"] = true;

            std::ofstream debugLog4("debug_output.log", std::ios::app);
            debugLog4 << "📁 Identity file does not exist" << std::endl;
            debugLog4.close();
        }

        // Send response back to frontend
        CefRefPtr<CefProcessMessage> cefResponse = CefProcessMessage::Create("identity_status_check_response");
        CefRefPtr<CefListValue> responseArgs = cefResponse->GetArgumentList();
        responseArgs->SetString(0, response.dump());

        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, cefResponse);
        std::ofstream debugLog5("debug_output.log", std::ios::app);
        debugLog5 << "📤 Identity status sent: " << response.dump() << std::endl;
        debugLog5.close();

        return true;
    }

    if (message_name == "create_identity") {
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🆕 Create identity requested" << std::endl;
        debugLog.close();

        nlohmann::json response;

        // Check if identity already exists
        const char* homeDir = std::getenv("USERPROFILE");
        std::string identityPath = std::string(homeDir) + "\\AppData\\Roaming\\BabbageBrowser\\identity.json";

        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "🔍 create_identity: Checking for existing identity at: " << identityPath << std::endl;
        debugLog2 << "🔍 create_identity: File exists check: " << std::filesystem::exists(identityPath) << std::endl;
        debugLog2.close();

        std::ifstream identityFile(identityPath);
        if (identityFile.good()) {
            try {
                // Identity already exists - return it
                nlohmann::json existingIdentity;
                identityFile >> existingIdentity;
                identityFile.close();

                response["success"] = true;
                response["identity"] = existingIdentity;

                std::cout << "📁 Identity already exists, returning existing identity" << std::endl;

            } catch (const std::exception& e) {
                identityFile.close();
                response["success"] = false;
                response["error"] = "Failed to read existing identity: " + std::string(e.what());

                std::cout << "⚠️ Failed to read existing identity: " << e.what() << std::endl;
            }
        } else {
            identityFile.close();

            // Identity doesn't exist - create new one via WalletService
            try {
                WalletService walletService;

                if (!walletService.isConnected()) {
                    response["success"] = false;
                    response["error"] = "Wallet daemon is not running. Please start the daemon manually.";

                    std::cout << "❌ Cannot create identity - daemon not running" << std::endl;
                } else {
                    // Create new identity
                    nlohmann::json newIdentity = walletService.getIdentity();

                    response["success"] = true;
                    response["identity"] = newIdentity;

                    std::cout << "✅ New identity created successfully" << std::endl;
                }

            } catch (const std::exception& e) {
                response["success"] = false;
                response["error"] = "Failed to create identity: " + std::string(e.what());

                std::cout << "💥 Error creating identity: " << e.what() << std::endl;
            }
        }

        // Send response back to frontend
        CefRefPtr<CefProcessMessage> cefResponse = CefProcessMessage::Create("create_identity_response");
        CefRefPtr<CefListValue> responseArgs = cefResponse->GetArgumentList();
        responseArgs->SetString(0, response.dump());

        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, cefResponse);
        std::cout << "📤 Create identity response sent: " << response.dump() << std::endl;

        return true;
    }

    if (message_name == "mark_identity_backed_up") {
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "✅ Mark identity as backed up requested" << std::endl;
        debugLog.close();

        nlohmann::json response;

        try {
            // Read existing identity file
            const char* homeDir = std::getenv("USERPROFILE");
            std::string identityPath = std::string(homeDir) + "\\AppData\\Roaming\\BabbageBrowser\\identity.json";
            std::ifstream identityFile(identityPath);
            if (!identityFile.good()) {
                identityFile.close();
                response["success"] = false;
                response["error"] = "Identity file not found";

                std::cout << "❌ Cannot mark as backed up - identity file not found" << std::endl;
            } else {
                nlohmann::json identity;
                identityFile >> identity;
                identityFile.close();

                // Update backedUp flag
                identity["backedUp"] = true;

                // Write back to file
                std::ofstream outFile(identityPath);
                outFile << identity.dump(4); // Pretty print with 4-space indentation
                outFile.close();

                response["success"] = true;
                response["identity"] = identity;

                std::cout << "✅ Identity marked as backed up successfully" << std::endl;
            }

        } catch (const std::exception& e) {
            response["success"] = false;
            response["error"] = "Failed to mark as backed up: " + std::string(e.what());

            std::cout << "💥 Error marking identity as backed up: " << e.what() << std::endl;
        }

        // Send response back to frontend
        CefRefPtr<CefProcessMessage> cefResponse = CefProcessMessage::Create("mark_identity_backed_up_response");
        CefRefPtr<CefListValue> responseArgs = cefResponse->GetArgumentList();
        responseArgs->SetString(0, response.dump());

        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, cefResponse);
        std::cout << "📤 Mark backed up response sent: " << response.dump() << std::endl;

        return true;
    }

    if (message_name == "overlay_close") {
        std::cout << "🧠 [SimpleHandler] overlay_close message received" << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🧠 [SimpleHandler] overlay_close message received" << std::endl;
        debugLog.close();

        // Find and destroy overlay windows based on role
        HWND target_hwnd = nullptr;
        CefRefPtr<CefBrowser> target_browser = nullptr;

        if (role_ == "settings") {
            target_hwnd = FindWindow(L"CEFSettingsOverlayWindow", L"Settings Overlay");
            target_browser = GetSettingsBrowser();
            std::cout << "✅ Found settings overlay window: " << target_hwnd << std::endl;
        } else if (role_ == "wallet") {
            target_hwnd = FindWindow(L"CEFWalletOverlayWindow", L"Wallet Overlay");
            target_browser = GetWalletBrowser();
            std::cout << "✅ Found wallet overlay window: " << target_hwnd << std::endl;
        } else if (role_ == "backup") {
            target_hwnd = FindWindow(L"CEFBackupOverlayWindow", L"Backup Overlay");
            target_browser = GetBackupBrowser();
            std::cout << "✅ Found backup overlay window: " << target_hwnd << std::endl;
        }

        if (target_hwnd && IsWindow(target_hwnd)) {
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "✅ Found " << role_ << " overlay window: " << target_hwnd << std::endl;
            debugLog2.close();

            // Close the browser first
            if (target_browser) {
                std::cout << "🔄 Closing " << role_ << " browser" << std::endl;
                target_browser->GetHost()->CloseBrowser(false);
                // Clear the appropriate browser reference
                if (role_ == "settings") settings_browser_ = nullptr;
                else if (role_ == "wallet") wallet_browser_ = nullptr;
                else if (role_ == "backup") backup_browser_ = nullptr;
            }

            // Then destroy the window
            std::cout << "🔄 Destroying " << role_ << " overlay window" << std::endl;
            SendMessage(target_hwnd, WM_CLOSE, 0, 0);
        } else {
            std::cout << "❌ " << role_ << " overlay window not found" << std::endl;
            std::ofstream debugLog3("debug_output.log", std::ios::app);
            debugLog3 << "❌ " << role_ << " overlay window not found" << std::endl;
            debugLog3.close();
        }

        return true;
    }

    if (false && message_name == "overlay_hide_NEVER_CALLED_12345") {
        std::cout << "🪟 Hiding overlay HWND" << std::endl;
        std::cout << "�� Before hide - EXSTYLE: 0x" << std::hex << GetWindowLong(nullptr, GWL_EXSTYLE) << std::endl;
        ShowWindow(nullptr, SW_HIDE);
        std::cout << "🪟 After hide - EXSTYLE: 0x" << std::hex << GetWindowLong(nullptr, GWL_EXSTYLE) << std::endl;
        return true;
    }

    if (message_name == "overlay_show_wallet") {
        std::cout << "💰 overlay_show_wallet message received from role: " << role_ << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💰 overlay_show_wallet message received from role: " << role_ << std::endl;
        debugLog.close();

        std::cout << "💰 Creating wallet overlay with separate process" << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "💰 Creating wallet overlay with separate process" << std::endl;
        debugLog2.close();
        // Create new process for wallet overlay
        extern HINSTANCE g_hInstance;
        CreateWalletOverlayWithSeparateProcess(g_hInstance);
        return true;
    }

    if (message_name == "overlay_show_backup") {
        std::cout << "💾 overlay_show_backup message received from role: " << role_ << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💾 overlay_show_backup message received from role: " << role_ << std::endl;
        debugLog.close();

        std::cout << "💾 Creating backup overlay with separate process" << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "💾 Creating backup overlay with separate process" << std::endl;
        debugLog2.close();
        // Create new process for backup overlay
        extern HINSTANCE g_hInstance;
        CreateBackupOverlayWithSeparateProcess(g_hInstance);
        return true;
    }

    if (message_name == "overlay_show_settings") {
        std::cout << "🪟 overlay_show_settings message received from role: " << role_ << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🪟 overlay_show_settings message received from role: " << role_ << std::endl;
        debugLog.close();

        std::cout << "🪟 Creating settings overlay with separate process" << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "🪟 Creating settings overlay with separate process" << std::endl;
        debugLog2.close();
        // Create new process for settings overlay
        extern HINSTANCE g_hInstance;
        CreateSettingsOverlayWithSeparateProcess(g_hInstance);
        return true;
    }

    if (message_name == "test_settings_message") {
        std::cout << "🧪 test_settings_message received from role: " << role_ << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🧪 test_settings_message received from role: " << role_ << std::endl;
        debugLog.close();
        return true;
    }

    if (false && message_name == "overlay_hide_NEVER_CALLED_67890" && role_ == "settings") {
        std::cout << "🪟 overlay_hide message received for settings overlay" << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🪟 overlay_hide message received for settings overlay" << std::endl;
        debugLog.close();

        // Close the settings overlay window
        HWND settings_hwnd = FindWindow(L"CEFSettingsOverlayWindow", L"Settings Overlay");
        if (settings_hwnd) {
            std::cout << "🪟 Closing settings overlay window" << std::endl;
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "🪟 Closing settings overlay window" << std::endl;
            debugLog2.close();
            DestroyWindow(settings_hwnd);
        }
        return true;
    }

    if (message_name == "overlay_input") {
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🪟 overlay_input message received from role: " << role_ << std::endl;
        debugLog.close();

        CefRefPtr<CefListValue> args = message->GetArgumentList();
        bool enable = args->GetBool(0);
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "🪟 Setting overlay input: " << (enable ? "enabled" : "disabled") << " for role: " << role_ << std::endl;
        debugLog2.close();

        // Handle input for the appropriate overlay based on role
        HWND target_hwnd = nullptr;
        if (role_ == "settings") {
            // Find the settings overlay window
            target_hwnd = FindWindow(L"CEFSettingsOverlayWindow", L"Settings Overlay");
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "🪟 Settings overlay HWND found: " << target_hwnd << std::endl;
            debugLog2.close();
        } else if (role_ == "wallet") {
            // Find the wallet overlay window
            target_hwnd = FindWindow(L"CEFWalletOverlayWindow", L"Wallet Overlay");
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "💰 Wallet overlay HWND found: " << target_hwnd << std::endl;
            debugLog2.close();
        } else if (role_ == "backup") {
            // Find the backup overlay window
            target_hwnd = FindWindow(L"CEFBackupOverlayWindow", L"Backup Overlay");
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "💾 Backup overlay HWND found: " << target_hwnd << std::endl;
            debugLog2.close();
        }

        if (target_hwnd) {
            LONG exStyle = GetWindowLong(target_hwnd, GWL_EXSTYLE);
            if (enable) {
                SetWindowLong(target_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                std::ofstream debugLog4("debug_output.log", std::ios::app);
                debugLog4 << "🪟 Mouse input ENABLED for HWND: " << target_hwnd << std::endl;
                debugLog4.close();
            } else {
                SetWindowLong(target_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                std::ofstream debugLog5("debug_output.log", std::ios::app);
                debugLog5 << "🪟 Mouse input DISABLED for HWND: " << target_hwnd << std::endl;
                debugLog5.close();
            }
        } else {
            std::ofstream debugLog6("debug_output.log", std::ios::app);
            debugLog6 << "❌ No target HWND found for overlay_input" << std::endl;
            debugLog6.close();
        }
        return true;
    }

    if (message_name == "address_generate") {
        std::cout << "🔑 Address generation requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "🔑 Address generation requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Call WalletService to generate address
            WalletService walletService;
            nlohmann::json addressData = walletService.generateAddress();

            std::cout << "✅ Address generated successfully: " << addressData.dump() << std::endl;
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "✅ Address generated successfully: " << addressData.dump() << std::endl;
            debugLog2.close();

            // Send result back to the requesting browser
            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("address_generate_response");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, addressData.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
            std::cout << "📤 Address data sent back to browser" << std::endl;
            std::ofstream debugLog3("debug_output.log", std::ios::app);
            debugLog3 << "📤 Address data sent back to browser" << std::endl;
            debugLog3 << "🔍 Browser ID: " << browser->GetIdentifier() << std::endl;
            debugLog3 << "🔍 Frame URL: " << browser->GetMainFrame()->GetURL().ToString() << std::endl;
            debugLog3.close();

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

    // Transaction Message Handlers

    if (message_name == "create_transaction") {
        std::cout << "💰 Create transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💰 Create transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Parse transaction data from message arguments
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            if (args->GetSize() > 0) {
                std::string transactionDataJson = args->GetString(0);
                nlohmann::json transactionData = nlohmann::json::parse(transactionDataJson);

                // Call WalletService to create transaction
                WalletService walletService;
                nlohmann::json result = walletService.createTransaction(transactionData);

                std::cout << "✅ Transaction creation result: " << result.dump() << std::endl;
                std::ofstream debugLog2("debug_output.log", std::ios::app);
                debugLog2 << "✅ Transaction creation result: " << result.dump() << std::endl;
                debugLog2.close();

                // Send result back to the requesting browser
                CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("create_transaction_response");
                CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
                responseArgs->SetString(0, result.dump());

                browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
                std::cout << "📤 Transaction creation response sent back to browser" << std::endl;
                std::ofstream debugLog3("debug_output.log", std::ios::app);
                debugLog3 << "📤 Transaction creation response sent back to browser" << std::endl;
                debugLog3.close();
            } else {
                throw std::runtime_error("No transaction data provided");
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Transaction creation failed: " << e.what() << std::endl;
            std::ofstream debugLog4("debug_output.log", std::ios::app);
            debugLog4 << "❌ Transaction creation failed: " << e.what() << std::endl;
            debugLog4.close();

            // Send error response
            nlohmann::json errorResponse;
            errorResponse["error"] = e.what();

            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("create_transaction_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, errorResponse.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }

    if (message_name == "sign_transaction") {
        std::cout << "✍️ Sign transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "✍️ Sign transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Parse transaction data from message arguments
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            if (args->GetSize() > 0) {
                std::string transactionDataJson = args->GetString(0);
                nlohmann::json transactionData = nlohmann::json::parse(transactionDataJson);

                // Call WalletService to sign transaction
                WalletService walletService;
                nlohmann::json result = walletService.signTransaction(transactionData);

                std::cout << "✅ Transaction signing result: " << result.dump() << std::endl;
                std::ofstream debugLog2("debug_output.log", std::ios::app);
                debugLog2 << "✅ Transaction signing result: " << result.dump() << std::endl;
                debugLog2.close();

                // Send result back to the requesting browser
                CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("sign_transaction_response");
                CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
                responseArgs->SetString(0, result.dump());

                browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
                std::cout << "📤 Transaction signing response sent back to browser" << std::endl;
                std::ofstream debugLog3("debug_output.log", std::ios::app);
                debugLog3 << "📤 Transaction signing response sent back to browser" << std::endl;
                debugLog3.close();
            } else {
                throw std::runtime_error("No transaction data provided");
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Transaction signing failed: " << e.what() << std::endl;
            std::ofstream debugLog4("debug_output.log", std::ios::app);
            debugLog4 << "❌ Transaction signing failed: " << e.what() << std::endl;
            debugLog4.close();

            // Send error response
            nlohmann::json errorResponse;
            errorResponse["error"] = e.what();

            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("sign_transaction_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, errorResponse.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }

    if (message_name == "broadcast_transaction") {
        std::cout << "📡 Broadcast transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "📡 Broadcast transaction requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Parse transaction data from message arguments
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            if (args->GetSize() > 0) {
                std::string transactionDataJson = args->GetString(0);
                nlohmann::json transactionData = nlohmann::json::parse(transactionDataJson);

                // Call WalletService to broadcast transaction
                WalletService walletService;
                nlohmann::json result = walletService.broadcastTransaction(transactionData);

                std::cout << "✅ Transaction broadcast result: " << result.dump() << std::endl;
                std::ofstream debugLog2("debug_output.log", std::ios::app);
                debugLog2 << "✅ Transaction broadcast result: " << result.dump() << std::endl;
                debugLog2.close();

                // Send result back to the requesting browser
                CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("broadcast_transaction_response");
                CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
                responseArgs->SetString(0, result.dump());

                browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
                std::cout << "📤 Transaction broadcast response sent back to browser" << std::endl;
                std::ofstream debugLog3("debug_output.log", std::ios::app);
                debugLog3 << "📤 Transaction broadcast response sent back to browser" << std::endl;
                debugLog3.close();
            } else {
                throw std::runtime_error("No transaction data provided");
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Transaction broadcast failed: " << e.what() << std::endl;
            std::ofstream debugLog4("debug_output.log", std::ios::app);
            debugLog4 << "❌ Transaction broadcast failed: " << e.what() << std::endl;
            debugLog4.close();

            // Send error response
            nlohmann::json errorResponse;
            errorResponse["error"] = e.what();

            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("broadcast_transaction_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, errorResponse.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }


        if (message_name == "get_balance") {
        std::cout << "💰 Get balance requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "💰 Get balance requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Parse balance data from message arguments
            CefRefPtr<CefListValue> args = message->GetArgumentList();
            std::ofstream debugLog("debug_output.log", std::ios::app);
            debugLog << "🔍 get_balance: args->GetSize() = " << args->GetSize() << std::endl;
            debugLog.close();

            if (args->GetSize() > 0) {
                std::string balanceDataJson = args->GetString(0);
                std::ofstream debugLog2("debug_output.log", std::ios::app);
                debugLog2 << "🔍 get_balance: received JSON = " << balanceDataJson << std::endl;
                debugLog2.close();

                nlohmann::json balanceData = nlohmann::json::parse(balanceDataJson);

                // Call WalletService to get balance
                WalletService walletService;
                nlohmann::json result = walletService.getBalance(balanceData);

                std::cout << "✅ Balance result: " << result.dump() << std::endl;
                std::ofstream debugLog3("debug_output.log", std::ios::app);
                debugLog3 << "✅ Balance result: " << result.dump() << std::endl;
                debugLog3.close();

                // Send result back to the requesting browser
                CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("get_balance_response");
                CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
                responseArgs->SetString(0, result.dump());

                browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
                std::cout << "📤 Balance response sent back to browser" << std::endl;
                std::ofstream debugLog6("debug_output.log", std::ios::app);
                debugLog6 << "📤 Balance response sent back to browser" << std::endl;
                debugLog6.close();
            } else {
                std::ofstream debugLog4("debug_output.log", std::ios::app);
                debugLog4 << "❌ get_balance: No arguments provided, args->GetSize() = " << args->GetSize() << std::endl;
                debugLog4.close();
                throw std::runtime_error("No balance data provided");
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Get balance failed: " << e.what() << std::endl;
            std::ofstream debugLog5("debug_output.log", std::ios::app);
            debugLog5 << "❌ Get balance failed: " << e.what() << std::endl;
            debugLog5.close();

            // Send error response
            nlohmann::json errorResponse;
            errorResponse["error"] = e.what();

            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("get_balance_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, errorResponse.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }

    if (message_name == "get_transaction_history") {
        std::cout << "📜 Get transaction history requested from browser ID: " << browser->GetIdentifier() << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "📜 Get transaction history requested from browser ID: " << browser->GetIdentifier() << std::endl;
        debugLog.close();

        try {
            // Call WalletService to get transaction history
            WalletService walletService;
            nlohmann::json result = walletService.getTransactionHistory();

            std::cout << "✅ Transaction history result: " << result.dump() << std::endl;
            std::ofstream debugLog2("debug_output.log", std::ios::app);
            debugLog2 << "✅ Transaction history result: " << result.dump() << std::endl;
            debugLog2.close();

            // Send result back to the requesting browser
            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("get_transaction_history_response");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, result.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
            std::cout << "📤 Transaction history response sent back to browser" << std::endl;
            std::ofstream debugLog3("debug_output.log", std::ios::app);
            debugLog3 << "📤 Transaction history response sent back to browser" << std::endl;
            debugLog3.close();

        } catch (const std::exception& e) {
            std::cout << "❌ Get transaction history failed: " << e.what() << std::endl;
            std::ofstream debugLog4("debug_output.log", std::ios::app);
            debugLog4 << "❌ Get transaction history failed: " << e.what() << std::endl;
            debugLog4.close();

            // Send error response
            nlohmann::json errorResponse;
            errorResponse["error"] = e.what();

            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("get_transaction_history_error");
            CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
            responseArgs->SetString(0, errorResponse.dump());

            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, response);
        }

        return true;
    }

    return false;
}

CefRefPtr<CefRequestHandler> SimpleHandler::GetRequestHandler() {
    return this;
}

CefRefPtr<CefContextMenuHandler> SimpleHandler::GetContextMenuHandler() {
    return this;
}

void SimpleHandler::SetRenderHandler(CefRefPtr<CefRenderHandler> handler) {
    render_handler_ = handler;
}

CefRefPtr<CefRenderHandler> SimpleHandler::GetRenderHandler() {
    return render_handler_;
}

void SimpleHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefRefPtr<CefContextMenuParams> params,
                                        CefRefPtr<CefMenuModel> model) {
    // Enable DevTools for overlay windows in development
    if (role_ == "settings" || role_ == "wallet" || role_ == "backup") {
        // Add Inspect Element option - use custom menu ID
        const int MENU_ID_DEV_TOOLS_INSPECT = MENU_ID_USER_FIRST + 1;
        model->AddItem(MENU_ID_DEV_TOOLS_INSPECT, "Inspect Element");
        model->AddSeparator();

        std::cout << "🔧 Context menu enabled for " << role_ << " overlay - DevTools available" << std::endl;
    }
}

bool SimpleHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefContextMenuParams> params,
                                         int command_id,
                                         EventFlags event_flags) {
    if ((role_ == "settings" || role_ == "wallet" || role_ == "backup") && command_id == (MENU_ID_USER_FIRST + 1)) {
        // Open DevTools
        browser->GetHost()->ShowDevTools(CefWindowInfo(), nullptr, CefBrowserSettings(), CefPoint());
        std::cout << "🔧 DevTools opened for " << role_ << " overlay" << std::endl;
        return true;
    }
    return false;
}
