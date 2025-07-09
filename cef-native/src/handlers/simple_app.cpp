// src/simple_app.cpp
#include "../../include/handlers/simple_app.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_process_message.h"
#include <iostream>

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

void SimpleApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    std::cout << "✅ OnAfterCreated CALLED for Shell Browser" << std::endl;

    HINSTANCE instance = GetModuleHandle(nullptr);

    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"BitcoinBrowserWndClass";
    RegisterClass(&wc);

    WNDCLASS browserClass = {};
    browserClass.lpfnWndProc = DefWindowProc;
    browserClass.hInstance = instance;
    browserClass.lpszClassName = L"CEFHostWindow";
    RegisterClass(&browserClass);

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // Create main application window (parent HWND)
    HWND hwnd = CreateWindow(
        L"BitcoinBrowserWndClass",
        L"Bitcoin Browser / Babbage Browser",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Layout config
    const int shellHeight = 100;  // Space for React-based browser shell UI
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int webviewHeight = height - shellHeight;

    HWND shell_hwnd = CreateWindow(
        L"CEFHostWindow",
        nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, width, shellHeight,
        hwnd, nullptr, instance, nullptr
    );

    ShowWindow(shell_hwnd, SW_SHOW);
    UpdateWindow(shell_hwnd);

    // 2️⃣ Create bottom child window for external site view (WebView browser)
    HWND webview_hwnd = CreateWindow(
        L"CEFHostWindow", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, shellHeight, width, webviewHeight,
        hwnd, nullptr, instance, nullptr
    );

    ShowWindow(webview_hwnd, SW_SHOW);
    UpdateWindow(webview_hwnd);

   // ⚙️ Create CEF browser for the React UI shell
    RECT clientRect;
    GetClientRect(shell_hwnd, &clientRect);  // safe to call after ShowWindow

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    CefWindowInfo shell_window_info;
    shell_window_info.SetAsChild(shell_hwnd, CefRect(0, 0, clientWidth, clientHeight));

    // 🔧 Create handler with shell role
    CefRefPtr<SimpleHandler> shell_handler = new SimpleHandler("shell");

    // 🌐 Set initial shell URL
    std::string shell_url = "http://127.0.0.1:5137";
    std::cout << "Loading React shell at: " << shell_url << std::endl;

    // 📋 Browser settings and context
    CefBrowserSettings shell_settings;
    CefRefPtr<CefRequestContext> shell_context = CefRequestContext::GetGlobalContext();

    // 🌍 Launch the shell browser
    bool shell_result = CefBrowserHost::CreateBrowser(
        shell_window_info,
        shell_handler,
        shell_url,
        shell_settings,
        nullptr,
        CefRequestContext::GetGlobalContext()
    );

    std::cout << "Shell browser created: " << (shell_result ? "true" : "false") << std::endl;

    // ⚙️ Create CEF browser for the WebView (site renderer)
    // Setup child window size
    RECT webviewRect;
    GetClientRect(webview_hwnd, &webviewRect);
    int webviewClientWidth = webviewRect.right - webviewRect.left;
    int webviewClientHeight = webviewRect.bottom - webviewRect.top;

    CefWindowInfo webview_window_info;
    webview_window_info.SetAsChild(webview_hwnd, CefRect(0, 0, webviewClientWidth, webviewClientHeight));

    // Create handler with role
    CefRefPtr<SimpleHandler> webview_handler = new SimpleHandler("webview");

    // Setup browser settings
    CefBrowserSettings webview_settings;

    // (Optional but safe) use global context
    CefRefPtr<CefRequestContext> webview_context = CefRequestContext::GetGlobalContext();

    // Create the browser (5 parameters only)
    bool webview_result = CefBrowserHost::CreateBrowser(
        webview_window_info,
        webview_handler,
        "https://www.coingeek.com",
        webview_settings,
        nullptr,              // extra_info (you can use this if needed later)
        CefRequestContext::GetGlobalContext() // request_context
    );

    std::cout << "WebView browser created: " << (webview_result ? "true" : "false") << std::endl;
}
