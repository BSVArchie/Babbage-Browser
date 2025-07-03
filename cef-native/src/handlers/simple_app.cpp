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
    CefWindowInfo shell_window_info;

    RECT clientRect;
    GetClientRect(shell_hwnd, &clientRect);  // safe to call after ShowWindow

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    shell_window_info.SetAsChild(shell_hwnd, CefRect(0, 0, clientWidth, clientHeight));

    CefRefPtr<SimpleHandler> shell_handler = new SimpleHandler();
    // std::string shell_url = "http://localhost:5137";
    std::string shell_url = "http://127.0.0.1:5137";
    std::cout << "Loading React shell at: " << shell_url << std::endl;
    bool shell_result = CefBrowserHost::CreateBrowser(
        shell_window_info, shell_handler, shell_url, CefBrowserSettings(), nullptr, nullptr
    );
    std::cout << "Shell browser created: " << (shell_result ? "true" : "false") << std::endl;

    // ⚙️ Create CEF browser for the WebView (site renderer)
    CefWindowInfo webview_window_info;

    RECT webviewRect;
    GetClientRect(webview_hwnd, &webviewRect);

    int webviewClientWidth = webviewRect.right - webviewRect.left;
    int webviewClientHeight = webviewRect.bottom - webviewRect.top;

    webview_window_info.SetAsChild(webview_hwnd, CefRect(0, 0, webviewClientWidth, webviewClientHeight));

    CefRefPtr<SimpleHandler> webview_handler = new SimpleHandler();
    // Placeholder external site (use blank or real page)
    std::string external_url = "https://www.coingeek.com";
    std::cout << "Initializing WebView surface at: " << external_url << std::endl;
    bool webview_result = CefBrowserHost::CreateBrowser(
        webview_window_info, webview_handler, external_url, CefBrowserSettings(), nullptr, nullptr
    );
    std::cout << "WebView browser created: " << (webview_result ? "true" : "false") << std::endl;
}
