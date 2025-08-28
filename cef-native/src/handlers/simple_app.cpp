// src/simple_app.cpp
#include "../../include/handlers/simple_app.h"
#include "../../include/handlers/simple_handler.h"
#include "../../include/handlers/my_overlay_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_process_message.h"
#include <iostream>
#include <fstream>

SimpleApp::SimpleApp()
    : render_process_handler_(new SimpleRenderProcessHandler()) {}

CefRefPtr<CefBrowserProcessHandler> SimpleApp::GetBrowserProcessHandler() {
    std::cout << "✅ SimpleApp::GetBrowserProcessHandler CALLED" << std::endl;
    return this;
}

CefRefPtr<CefRenderProcessHandler> SimpleApp::GetRenderProcessHandler() {
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

void CreateOverlayBrowserIfNeeded(HINSTANCE hInstance) {
    std::cout << "🚀 Called CreateOverlayBrowserIfNeeded()" << std::endl;

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HWND overlay_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"CEFOverlayWindow", nullptr, WS_POPUP | WS_VISIBLE,
        0, 0, width, height, nullptr, nullptr, hInstance, nullptr);

    if (overlay_hwnd) {
        // Force it to be truly topmost
        SetWindowPos(overlay_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

        // Bring to front and activate
        BringWindowToTop(overlay_hwnd);
        SetForegroundWindow(overlay_hwnd);

        // Force redraw
        UpdateWindow(overlay_hwnd);
    }

    LONG exStyle = GetWindowLong(overlay_hwnd, GWL_EXSTYLE);
    std::cout << "🧠 HWND EXSTYLE AFTER CREATE: " << std::hex << exStyle << std::endl;

    g_overlay_hwnd = overlay_hwnd;

    RECT overlayRect;

    SystemParametersInfo(SPI_GETWORKAREA, 0, &overlayRect, 0);
    int prepWidth  = overlayRect.right - overlayRect.left;
    int prepHeight = overlayRect.bottom - overlayRect.top;

    std::cout << "[Overlay Setup] g_overlay_hwnd BEFORE passing to render handler: " << g_overlay_hwnd << std::endl;

    SetWindowPos(g_overlay_hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW);
    UpdateWindow(g_overlay_hwnd);

    SetForegroundWindow(g_overlay_hwnd);
    SetFocus(g_overlay_hwnd);
    SetActiveWindow(g_overlay_hwnd);

    int overlayWidth = prepWidth;
    int overlayHeight = prepHeight;

    CefWindowInfo overlay_window_info;
    overlay_window_info.windowless_rendering_enabled = true;
    overlay_window_info.SetAsPopup(g_overlay_hwnd, "OverlayWindow");
 
    CefRefPtr<MyOverlayRenderHandler> render_handler =
        new MyOverlayRenderHandler(g_overlay_hwnd, overlayWidth, overlayHeight);


    CefRefPtr<SimpleHandler> overlay_handler = new SimpleHandler("overlay");
    overlay_handler->SetRenderHandler(render_handler);

    std::cout << "[OverlayRenderHandler] HWND: " << g_overlay_hwnd << ", Width: " << overlayWidth << ", Height: " << overlayHeight << std::endl;

    CefBrowserSettings overlay_settings;
    overlay_settings.windowless_frame_rate = 30;
    overlay_settings.background_color = CefColorSetARGB(0, 0, 0, 0); // fully transparent

    bool overlay_result = CefBrowserHost::CreateBrowser(
        overlay_window_info,
        overlay_handler,
        "http://127.0.0.1:5137/overlay",
        overlay_settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );
}
