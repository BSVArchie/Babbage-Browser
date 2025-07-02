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

    //  command_line->AppendSwitch("disable-gpu");
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

    HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"BitcoinBrowserWndClass";
    RegisterClass(&wc);

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    HWND hwnd = CreateWindow(
        L"BitcoinBrowserWndClass",
        L"Bitcoin Browser / Babbage Browser",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    CefWindowInfo window_info;
    window_info.SetAsChild(hwnd, CefRect(0, 0, rect.right - rect.left, rect.bottom - rect.top));
    CefBrowserSettings browser_settings;
    CefRefPtr<SimpleHandler> handler = new SimpleHandler();

    // std::string url = "https://example.com";
    // std::string url = "about:blank";

    // std::string url = "http://localhost:5137";
    std::string url = "http://127.0.0.1:5137";
    std::cout << "Attempting to load URL: " << url << std::endl;
    bool result = CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr, nullptr);
    std::cout << "CreateBrowser returned: " << (result ? "true" : "false") << std::endl;
}
