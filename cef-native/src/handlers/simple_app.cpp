// src/simple_app.cpp
#include "../../include/handlers/simple_app.h"
#include "../../include/handlers/simple_handler.h"
#include "../../include/handlers/simple_render_process_handler.h"
#include "../../include/handlers/my_overlay_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_process_message.h"
#include <iostream>
#include <fstream>

// External global HWND declarations for shutdown cleanup
extern HWND g_settings_overlay_hwnd;
extern HWND g_wallet_overlay_hwnd;
extern HWND g_backup_overlay_hwnd;

SimpleApp::SimpleApp()
    : render_process_handler_(new SimpleRenderProcessHandler()) {
    std::cout << "🔧 SimpleApp constructor called!" << std::endl;
    std::cout << "🔧 Render process handler created: " << (render_process_handler_ ? "true" : "false") << std::endl;
}

CefRefPtr<CefBrowserProcessHandler> SimpleApp::GetBrowserProcessHandler() {
    std::cout << "✅ SimpleApp::GetBrowserProcessHandler CALLED" << std::endl;
    return this;
}

CefRefPtr<CefRenderProcessHandler> SimpleApp::GetRenderProcessHandler() {
    std::cout << "🔧 SimpleApp::GetRenderProcessHandler CALLED" << std::endl;
    std::cout << "🔧 Returning render process handler: " << (render_process_handler_ ? "true" : "false") << std::endl;
    return render_process_handler_;
}

void SimpleApp::OnBeforeCommandLineProcessing(const CefString& process_type,
                                               CefRefPtr<CefCommandLine> command_line) {
    std::wcout << L"OnBeforeCommandLineProcessing for type: " << std::wstring(process_type) << std::endl;

    if (!command_line->HasSwitch("lang")) {
        std::wcout << L"Appending --lang=en-US" << std::endl;
        command_line->AppendSwitchWithValue("lang", "en-US");
    } else {
        std::wcout << L"--lang already present" << std::endl;
    }

    command_line->AppendSwitchWithValue("remote-allow-origins", "*");

    // command_line->AppendSwitch("disable-gpu");
    // command_line->AppendSwitch("disable-gpu-compositing");
    // command_line->AppendSwitch("disable-gpu-shader-disk-cache");
    // command_line->AppendSwitchWithValue("use-gl", "disabled");
    // command_line->AppendSwitchWithValue("use-angle", "none");

    // command_line->AppendSwitch("allow-running-insecure-content");
    // command_line->AppendSwitch("disable-web-security");
    // command_line->AppendSwitch("disable-site-isolation-trials");
    // command_line->AppendSwitch("no-sandbox");
    // command_line->AppendSwitch("disable-features=RendererCodeIntegrity");

}

void SimpleApp::SetWindowHandles(HWND hwnd, HWND header, HWND webview) {
    hwnd_ = hwnd;
    header_hwnd_ = header;
    webview_hwnd_ = webview;
}

void SimpleApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    std::cout << "✅ OnContextInitialized CALLED" << std::endl;
    // Sleep(500);

    std::ofstream log("startup_log.txt", std::ios::app);
    log << "🚀 OnContextInitialized entered\n";
    log << "→ header_hwnd_: " << header_hwnd_ << "\n";
    log << "→ IsWindow(header_hwnd_): " << IsWindow(header_hwnd_) << "\n";
    log << "→ webview_hwnd_: " << webview_hwnd_ << "\n";
    log << "→ IsWindow(webview_hwnd_): " << IsWindow(webview_hwnd_) << "\n";

    log.close();

    // ───── header Browser Setup ─────
    RECT headerRect;
    GetClientRect(g_header_hwnd, &headerRect);
    int headerWidth = headerRect.right - headerRect.left;
    int headerHeight = headerRect.bottom - headerRect.top;

    CefWindowInfo header_window_info;
    header_window_info.SetAsChild(g_header_hwnd, CefRect(0, 0, headerWidth, headerHeight));

    CefRefPtr<SimpleHandler> header_handler = new SimpleHandler("header");
    CefBrowserSettings header_settings;
    std::string header_url = "http://127.0.0.1:5137";
    std::cout << "Loading React header at: " << header_url << std::endl;

    try{
        bool header_result = CefBrowserHost::CreateBrowser(
        header_window_info,
        header_handler,
        header_url,
        header_settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );
    std::cout << "header browser created: " << (header_result ? "true" : "false") << std::endl;
    } catch (...) {
        log << "❌ header browser creation threw an exception!\n";
    }

    // ───── WebView Browser Setup ─────
    RECT webviewRect;
    GetClientRect(g_webview_hwnd, &webviewRect);
    int webviewWidth = webviewRect.right - webviewRect.left;
    int webviewHeight = webviewRect.bottom - webviewRect.top;

    CefWindowInfo webview_window_info;
    webview_window_info.SetAsChild(g_webview_hwnd, CefRect(0, 0, webviewWidth, webviewHeight));

    CefRefPtr<SimpleHandler> webview_handler = new SimpleHandler("webview");
    CefBrowserSettings webview_settings;

    try {
        bool webview_result = CefBrowserHost::CreateBrowser(
        webview_window_info,
        webview_handler,
        "https://www.coingeek.com",
        webview_settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );
    std::cout << "WebView browser created: " << (webview_result ? "true" : "false") << std::endl;
    } catch (...) {
        log << "❌ webview browser creation threw an exception!\n";
    }
}



// Chrome-style approach: Inject JavaScript directly into the overlay browser
void InjectBitcoinBrowserAPI(CefRefPtr<CefBrowser> browser) {
    if (!browser || !browser->GetMainFrame()) {
        std::cout << "❌ Cannot inject API - browser or frame not available" << std::endl;
        std::ofstream debugLog("debug_output.log", std::ios::app);
        debugLog << "❌ Cannot inject API - browser or frame not available" << std::endl;
        debugLog.close();
        return;
    }

    std::cout << "🔧 Injecting bitcoinBrowser API into browser ID: " << browser->GetIdentifier() << std::endl;
    std::ofstream debugLog1("debug_output.log", std::ios::app);
    debugLog1 << "🔧 Injecting bitcoinBrowser API into browser ID: " << browser->GetIdentifier() << std::endl;
    debugLog1.close();

    std::string jsCode = R"(
                 // Create bitcoinBrowser object using CEF's built-in V8 integration
                 window.bitcoinBrowser = {
                     address: {
                         generate: function() {
                             console.log('🔑 Address generation requested via injected JavaScript');

                             // Also try to log to a visible element for debugging
                             var debugDiv = document.getElementById('debug-log');
                             if (!debugDiv) {
                                 debugDiv = document.createElement('div');
                                 debugDiv.id = 'debug-log';
                                 debugDiv.style.position = 'fixed';
                                 debugDiv.style.top = '10px';
                                 debugDiv.style.left = '10px';
                                 debugDiv.style.background = 'black';
                                 debugDiv.style.color = 'white';
                                 debugDiv.style.padding = '10px';
                                 debugDiv.style.zIndex = '9999';
                                 debugDiv.style.fontSize = '12px';
                                 document.body.appendChild(debugDiv);
                             }
                             debugDiv.innerHTML += '🔑 Address generation requested<br>';

                             // Return a Promise for async operation
                             return new Promise((resolve, reject) => {
                                 try {
                                     // Use CEF's process message system
                                     if (window.chrome && window.chrome.runtime && window.chrome.runtime.sendMessage) {
                                         debugDiv.innerHTML += '📤 Sending process message<br>';
                                         window.chrome.runtime.sendMessage({
                                             type: 'address_generate'
                                         }, function(response) {
                                             debugDiv.innerHTML += '📥 Response received<br>';
                                             console.log('🔍 Response received:', JSON.stringify(response));
                                             if (response && response.success) {
                                                 debugDiv.innerHTML += '✅ Address generated successfully<br>';
                                                 console.log('✅ Address generated:', response.data);
                                                 console.log('🔍 Address field:', response.data.address);
                                                 console.log('🔍 PublicKey field:', response.data.publicKey);
                                                 console.log('🔍 PrivateKey field:', response.data.privateKey);
                                                 resolve(response.data);
                                             } else {
                                                 debugDiv.innerHTML += '❌ Address generation failed<br>';
                                                 console.error('❌ Address generation failed:', response ? response.error : 'Unknown error');
                                                 reject(new Error(response ? response.error : 'Unknown error'));
                                             }
                                         });
                                     } else {
                                         debugDiv.innerHTML += '❌ CEF runtime not available<br>';
                                         console.error('❌ CEF runtime not available, trying alternative method');
                                         // Fallback: try to call a global function
                                         if (window.generateAddress) {
                                             try {
                                                 var result = window.generateAddress();
                                                 debugDiv.innerHTML += '✅ Address generated via fallback<br>';
                                                 console.log('✅ Address generated via fallback:', result);
                                                 resolve(result);
                                             } catch (e) {
                                                 debugDiv.innerHTML += '❌ Error in fallback<br>';
                                                 console.error('❌ Error in fallback address generation:', e);
                                                 reject(e);
                                             }
                                         } else {
                                             debugDiv.innerHTML += '❌ No address generation method available<br>';
                                             console.error('❌ No address generation method available');
                                             reject(new Error('No address generation method available'));
                                         }
                                     }
                                 } catch (e) {
                                     debugDiv.innerHTML += '❌ Error in address generation<br>';
                                     console.error('❌ Error in address generation:', e);
                                     reject(e);
                                 }
                             });
                         }
                     },
                     overlay: {
                         show: function() {
                             console.log('🧪 Test overlay requested via bitcoinBrowser API');
                             // Send process message for test overlay
                             if (window.chrome && window.chrome.runtime && window.chrome.runtime.sendMessage) {
                                 window.chrome.runtime.sendMessage({
                                     type: 'test_overlay'
                                 }, function(response) {
                                     console.log('🧪 Test overlay response:', response);
                                 });
                             } else {
                                 console.error('❌ CEF runtime not available for test overlay');
                             }
                         }
                     }
                 };

                // cefMessage is now implemented in the render process handler
                // No need to set it up here as a stub


        console.log('✅ bitcoinBrowser API injected successfully');
    )";

    browser->GetMainFrame()->ExecuteJavaScript(jsCode, "", 0);
    std::cout << "🔧 Injected bitcoinBrowser API into browser ID: " << browser->GetIdentifier() << std::endl;

    // Also log to file
    std::ofstream debugLog2("debug_output.log", std::ios::app);
    debugLog2 << "🔧 Injected bitcoinBrowser API into browser ID: " << browser->GetIdentifier() << std::endl;
    debugLog2.close();
}

void CreateSettingsOverlayWithSeparateProcess(HINSTANCE hInstance) {
    std::cout << "🪟 Creating settings overlay with separate process" << std::endl;
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "🪟 Creating settings overlay with separate process" << std::endl;
    debugLog.close();

    // Get main window dimensions for positioning
    RECT mainRect;
    GetWindowRect(g_hwnd, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;

    // Create new HWND for settings overlay
    HWND settings_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"CEFSettingsOverlayWindow",
        L"Settings Overlay",
        WS_POPUP | WS_VISIBLE,
        mainRect.left, mainRect.top, width, height,
        g_hwnd, nullptr, hInstance, nullptr);

    if (!settings_hwnd) {
        std::cout << "❌ Failed to create settings overlay HWND. Error: " << GetLastError() << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "❌ Failed to create settings overlay HWND. Error: " << GetLastError() << std::endl;
        debugLog2.close();
        return;
    }

    std::cout << "✅ Settings overlay HWND created: " << settings_hwnd << std::endl;

    // Store HWND for shutdown cleanup
    g_settings_overlay_hwnd = settings_hwnd;
    std::ofstream debugLog3("debug_output.log", std::ios::app);
    debugLog3 << "✅ Settings overlay HWND created: " << settings_hwnd << std::endl;
    debugLog3.close();

    // Create new CEF browser with subprocess
    CefWindowInfo window_info;
    window_info.windowless_rendering_enabled = true;
    window_info.SetAsPopup(settings_hwnd, "SettingsOverlay");

    CefBrowserSettings settings;
    settings.windowless_frame_rate = 30;
    settings.background_color = CefColorSetARGB(0, 0, 0, 0); // fully transparent
    settings.javascript = STATE_ENABLED;
    settings.javascript_access_clipboard = STATE_ENABLED;
    settings.javascript_dom_paste = STATE_ENABLED;

    // Note: DevTools is enabled through context menu handler, not browser settings

    // Create new handler for settings overlay
    CefRefPtr<SimpleHandler> settings_handler(new SimpleHandler("settings"));

    // Set render handler for settings overlay (same as wallet overlay)
    CefRefPtr<MyOverlayRenderHandler> render_handler = new MyOverlayRenderHandler(settings_hwnd, width, height);
    settings_handler->SetRenderHandler(render_handler);

    // Create new browser with subprocess
    bool result = CefBrowserHost::CreateBrowser(
        window_info,
        settings_handler,
        "http://127.0.0.1:5137/settings",
        settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );

    if (result) {
        std::cout << "✅ Settings overlay browser created with subprocess" << std::endl;
        std::ofstream debugLog4("debug_output.log", std::ios::app);
        debugLog4 << "✅ Settings overlay browser created with subprocess" << std::endl;
        debugLog4.close();

        // Enable mouse input for settings overlay
        LONG exStyle = GetWindowLong(settings_hwnd, GWL_EXSTYLE);
        SetWindowLong(settings_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        std::ofstream debugLog6("debug_output.log", std::ios::app);
        debugLog6 << "🪟 Mouse input ENABLED for settings overlay HWND: " << settings_hwnd << std::endl;
        debugLog6.close();
    } else {
        std::cout << "❌ Failed to create settings overlay browser" << std::endl;
        std::ofstream debugLog5("debug_output.log", std::ios::app);
        debugLog5 << "❌ Failed to create settings overlay browser" << std::endl;
        debugLog5.close();
    }
}

void CreateWalletOverlayWithSeparateProcess(HINSTANCE hInstance) {
    std::cout << "💰 Creating wallet overlay with separate process" << std::endl;
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "💰 Creating wallet overlay with separate process" << std::endl;
    debugLog.close();

    // Get main window dimensions for positioning
    RECT mainRect;
    GetWindowRect(g_hwnd, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;

    // Create new HWND for wallet overlay
    HWND wallet_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"CEFWalletOverlayWindow",
        L"Wallet Overlay",
        WS_POPUP | WS_VISIBLE,
        mainRect.left, mainRect.top, width, height,
        g_hwnd, nullptr, hInstance, nullptr);

    if (!wallet_hwnd) {
        std::cout << "❌ Failed to create wallet overlay HWND. Error: " << GetLastError() << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "❌ Failed to create wallet overlay HWND. Error: " << GetLastError() << std::endl;
        debugLog2.close();
        return;
    }

    std::cout << "✅ Wallet overlay HWND created: " << wallet_hwnd << std::endl;

    // Store HWND for shutdown cleanup
    g_wallet_overlay_hwnd = wallet_hwnd;

    std::ofstream debugLog3("debug_output.log", std::ios::app);
    debugLog3 << "✅ Wallet overlay HWND created: " << wallet_hwnd << std::endl;
    debugLog3.close();

    // Create new CEF browser with subprocess
    CefWindowInfo window_info;
    window_info.windowless_rendering_enabled = true;
    window_info.SetAsPopup(wallet_hwnd, "WalletOverlay");

    CefBrowserSettings settings;
    settings.windowless_frame_rate = 30;
    settings.background_color = CefColorSetARGB(0, 0, 0, 0); // fully transparent
    settings.javascript = STATE_ENABLED;
    settings.javascript_access_clipboard = STATE_ENABLED;
    settings.javascript_dom_paste = STATE_ENABLED;

    // Note: DevTools is enabled through context menu handler, not browser settings

    // Create new handler for wallet overlay
    CefRefPtr<SimpleHandler> wallet_handler(new SimpleHandler("wallet"));

    // Set render handler for wallet overlay (same as settings overlay)
    CefRefPtr<MyOverlayRenderHandler> render_handler = new MyOverlayRenderHandler(wallet_hwnd, width, height);
    wallet_handler->SetRenderHandler(render_handler);

    // Create new browser with subprocess
    bool result = CefBrowserHost::CreateBrowser(
        window_info,
        wallet_handler,
        "http://127.0.0.1:5137/wallet",
        settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );

    if (result) {
        std::cout << "✅ Wallet overlay browser created with subprocess" << std::endl;
        std::ofstream debugLog4("debug_output.log", std::ios::app);
        debugLog4 << "✅ Wallet overlay browser created with subprocess" << std::endl;
        debugLog4.close();

        // Enable mouse input for wallet overlay
        LONG exStyle = GetWindowLong(wallet_hwnd, GWL_EXSTYLE);
        SetWindowLong(wallet_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        std::ofstream debugLog6("debug_output.log", std::ios::app);
        debugLog6 << "💰 Mouse input ENABLED for wallet overlay HWND: " << wallet_hwnd << std::endl;
        debugLog6.close();
    } else {
        std::cout << "❌ Failed to create wallet overlay browser" << std::endl;
        std::ofstream debugLog5("debug_output.log", std::ios::app);
        debugLog5 << "❌ Failed to create wallet overlay browser" << std::endl;
        debugLog5.close();
    }
}

void CreateBackupOverlayWithSeparateProcess(HINSTANCE hInstance) {
    std::cout << "💾 Creating backup overlay with separate process" << std::endl;
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "💾 Creating backup overlay with separate process" << std::endl;
    debugLog.close();

    RECT mainRect;
    GetWindowRect(g_hwnd, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;

    HWND backup_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"CEFBackupOverlayWindow",
        L"Backup Overlay",
        WS_POPUP | WS_VISIBLE,
        mainRect.left, mainRect.top, width, height,
        g_hwnd, nullptr, hInstance, nullptr);

    if (!backup_hwnd) {
        std::cout << "❌ Failed to create backup overlay HWND. Error: " << GetLastError() << std::endl;
        std::ofstream debugLog2("debug_output.log", std::ios::app);
        debugLog2 << "❌ Failed to create backup overlay HWND. Error: " << GetLastError() << std::endl;
        debugLog2.close();
        return;
    }

    std::cout << "✅ Backup overlay HWND created: " << backup_hwnd << std::endl;

    // Store HWND for shutdown cleanup
    g_backup_overlay_hwnd = backup_hwnd;

    std::ofstream debugLog3("debug_output.log", std::ios::app);
    debugLog3 << "✅ Backup overlay HWND created: " << backup_hwnd << std::endl;
    debugLog3.close();

    CefWindowInfo window_info;
    window_info.windowless_rendering_enabled = true;
    window_info.SetAsPopup(backup_hwnd, "BackupOverlay");

    CefBrowserSettings settings;
    settings.windowless_frame_rate = 30;
    settings.background_color = CefColorSetARGB(0, 0, 0, 0);
    settings.javascript = STATE_ENABLED;
    settings.javascript_access_clipboard = STATE_ENABLED;
    settings.javascript_dom_paste = STATE_ENABLED;

    CefRefPtr<SimpleHandler> backup_handler(new SimpleHandler("backup"));
    CefRefPtr<MyOverlayRenderHandler> render_handler = new MyOverlayRenderHandler(backup_hwnd, width, height);
    backup_handler->SetRenderHandler(render_handler);

    std::ofstream debugLog4("debug_output.log", std::ios::app);
    debugLog4 << "💾 Backup overlay render handler set for HWND: " << backup_hwnd << std::endl;
    debugLog4.close();

    bool result = CefBrowserHost::CreateBrowser(
        window_info,
        backup_handler,
        "http://127.0.0.1:5137/backup",
        settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );

    if (result) {
        std::cout << "✅ Backup overlay browser created with subprocess" << std::endl;
        std::ofstream debugLog4("debug_output.log", std::ios::app);
        debugLog4 << "✅ Backup overlay browser created with subprocess" << std::endl;
        debugLog4.close();

        LONG exStyle = GetWindowLong(backup_hwnd, GWL_EXSTYLE);
        SetWindowLong(backup_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        std::ofstream debugLog6("debug_output.log", std::ios::app);
        debugLog6 << "💾 Mouse input ENABLED for backup overlay HWND: " << backup_hwnd << std::endl;
        debugLog6.close();

    } else {
        std::cout << "❌ Failed to create backup overlay browser" << std::endl;
        std::ofstream debugLog5("debug_output.log", std::ios::app);
        debugLog5 << "❌ Failed to create backup overlay browser" << std::endl;
        debugLog5.close();
    }
}
