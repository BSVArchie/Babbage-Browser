#define CEF_ENABLE_SANDBOX 0

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#undef ERROR  // 💥 Avoid conflict with wingdi.h macro

#include "cef_app.h"
#include "cef_client.h"
#include "cef_browser.h"
#include "cef_command_line.h"
#include "cef_life_span_handler.h"
#include "wrapper/cef_helpers.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_v8.h"
#include "include/wallet/IdentityHandler.h"
#include "include/simple_handler.h"
#include "include/simple_render_process_handler.h"
#include <shellapi.h>
#include <windows.h>
#include <filesystem>
#include <iostream>


class SimpleApp : public CefApp,
                  public CefBrowserProcessHandler,
                  public CefRenderProcessHandler {
public:
    SimpleApp() : render_process_handler_(new SimpleRenderProcessHandler()) {}

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
        return render_process_handler_;
    }

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                    CefRefPtr<CefCommandLine> command_line) override {
        std::wcout << L"OnBeforeCommandLineProcessing for type: " << std::wstring(process_type) << std::endl;

        if (!command_line->HasSwitch("lang")) {
            std::wcout << L"Appending --lang=en-US" << std::endl;
            command_line->AppendSwitchWithValue("lang", "en-US");
        } else {
            std::wcout << L"--lang already present" << std::endl;
        }

        // Disable GPU and related features
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-gpu-compositing");
        command_line->AppendSwitch("disable-gpu-shader-disk-cache");
        command_line->AppendSwitchWithValue("use-gl", "disabled");
        command_line->AppendSwitchWithValue("use-angle", "none");

    }

    void OnContextInitialized() override {
        CEF_REQUIRE_UI_THREAD();

        // Register window class
        HINSTANCE instance = GetModuleHandle(nullptr);

        WNDCLASS wc = {};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"BitcoinBrowserWndClass";
        RegisterClass(&wc);

        // Create native Win32 window
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

        HWND hwnd = CreateWindow(
            L"BitcoinBrowserWndClass",    // Class name
            L"Bitcoin Browser/ Babbage Browser",           // Window title
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,                      // No parent
            nullptr,                      // No menu
            instance,                     // ✅ Use instance here
            nullptr
        );

        // Embed Chromium browser into that window
        CefWindowInfo window_info;
        // window_info.SetAsPopup(nullptr, "Bitcoin Browser");
        window_info.SetAsChild(hwnd, CefRect(0, 0, rect.right - rect.left, rect.bottom - rect.top));

        CefBrowserSettings browser_settings;
        CefRefPtr<SimpleHandler> handler = new SimpleHandler();

        std::string url = "http://localhost:5137";
        // std::string url = "data:text/html,<html><body><h1>Hello from CEF!</h1></body></html>";
        // std::string url = "https://www.google.com";
        std::cout << "Attempting to load URL: " << url << std::endl;
        bool result = CefBrowserHost::CreateBrowser(
            window_info, handler, url, browser_settings, nullptr, nullptr);
        std::cout << "CreateBrowser returned: " << (result ? "true" : "false") << std::endl;
    }



private:
    CefRefPtr<SimpleRenderProcessHandler> render_process_handler_;

    IMPLEMENT_REFCOUNTING(SimpleApp);
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    std::cout << "Shell starting..." << std::endl;

    CefMainArgs main_args(hInstance);
    CefRefPtr<SimpleApp> app(new SimpleApp());

    // ✅ Reconstruct command line and inject --lang if needed
    LPWSTR* argv;
    int argc;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::wstring final_cmd;
    bool has_lang = false;

    for (int i = 0; i < argc; ++i) {
        final_cmd += argv[i];
        final_cmd += L" ";

        if (std::wstring(argv[i]).find(L"--lang=") != std::wstring::npos) {
            has_lang = true;
        }
    }

    if (!has_lang) {
        final_cmd += L"--lang=en-US";
    }

    CefCommandLine::CreateCommandLine()->InitFromString(final_cmd);

    // ✅ Subprocess handling: ALWAYS include this block
    std::cout << "Running CefExecuteProcess..." << std::endl;
    // MessageBox(nullptr, L"About to run CefExecuteProcess", L"Debug", MB_OK);
    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    // MessageBox(nullptr, L"CefExecuteProcess finished", L"Debug", MB_OK);
    std::cout << "CefExecuteProcess returned: " << exit_code << std::endl;
    if (exit_code >= 0) {
        std::cout << "Exiting because this is a subprocess." << std::endl;
        return exit_code;
    }

    // ✅ Configure CEF settings
    CefSettings settings;
    settings.no_sandbox = true;
    settings.command_line_args_disabled = false;
    CefString(&settings.log_file).FromASCII("debug.log");
    settings.log_severity = LOGSEVERITY_INFO;
    std::wstring absPath = std::filesystem::absolute(".").wstring();
    CefString(&settings.resources_dir_path) = absPath;
    settings.remote_debugging_port = 9222;

    CefString(&settings.resources_dir_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release");
    CefString(&settings.locales_dir_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release\\locales");
    CefString(&settings.browser_subprocess_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release\\BitcoinBrowserShell.exe");


    std::cout << "Initializing CEF..." << std::endl;
    bool success = CefInitialize(main_args, settings, app, nullptr);
    std::cout << "CefInitialize success: " << (success ? "true" : "false") << std::endl;

    // ✅ Message loop
    CefRunMessageLoop();
    // MSG msg;
    // while (GetMessage(&msg, nullptr, 0, 0)) {
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }

    CefShutdown();
    return 0;
}
