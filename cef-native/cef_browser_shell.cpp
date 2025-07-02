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
// #include "include/core/IdentityHandler.h"
#include "include/handlers/simple_handler.h"
#include "include/handlers/simple_render_process_handler.h"
#include "include/handlers/simple_app.h"
#include <shellapi.h>
#include <windows.h>
#include <filesystem>
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);
    std::cout << "Shell starting..." << std::endl;

    CefMainArgs main_args(hInstance);
    CefRefPtr<SimpleApp> app(new SimpleApp());

    // Handle subprocess logic
    std::cout << "Running CefExecuteProcess..." << std::endl;
    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    std::cout << "CefExecuteProcess returned: " << exit_code << std::endl;
    std::cout << "💡 WinMain starting browser process." << std::endl;
    if (exit_code >= 0) {
        std::cout << "Exiting because this is a subprocess." << std::endl;
        return exit_code;
    }

    // CEF global settings
    CefSettings settings;
    settings.no_sandbox = true;
    settings.command_line_args_disabled = false;
    CefString(&settings.log_file).FromASCII("debug.log");
    settings.log_severity = LOGSEVERITY_INFO;
    settings.remote_debugging_port = 9222;

    // ✅ Hardcoded resource paths for now
    CefString(&settings.resources_dir_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release");
    CefString(&settings.locales_dir_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release\\locales");
    CefString(&settings.browser_subprocess_path).FromWString(L"D:\\BSVProjects\\Browser-Project\\Babbage-Browser\\cef-native\\build\\bin\\Release\\BitcoinBrowserShell.exe");

    std::cout << "Initializing CEF..." << std::endl;
    bool success = CefInitialize(main_args, settings, app, nullptr);
    std::cout << "CefInitialize success: " << (success ? "true" : "false") << std::endl;

    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
