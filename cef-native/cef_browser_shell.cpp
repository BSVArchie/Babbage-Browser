// #define CEF_ENABLE_SANDBOX 0

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
#include "include/cef_browser.h"
#include "include/internal/cef_types.h"
#include "include/handlers/simple_handler.h"
#include "include/handlers/simple_render_process_handler.h"
#include "include/handlers/simple_app.h"
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>
#include <filesystem>
#include <iostream>
#include <fstream>

HWND g_hwnd = nullptr;
HWND g_header_hwnd = nullptr;
HWND g_webview_hwnd = nullptr;
HINSTANCE g_hInstance = nullptr;

// Debug logging function
void DebugLog(const std::string& message) {
    std::cout << message << std::endl;
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << message << std::endl;
    debugLog.close();
}

LRESULT CALLBACK ShellWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOVE:
        case WM_SIZE:
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


LRESULT CALLBACK SettingsOverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEACTIVATE:
            std::cout << "👆 Settings Overlay HWND received WM_MOUSEACTIVATE\n";
            // Allow normal activation without forcing z-order
            return MA_ACTIVATE;

        case WM_LBUTTONDOWN: {
            std::cout << "🖱️ Settings Overlay received WM_LBUTTONDOWN\n";
            SetFocus(hwnd);

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Translate to CEF MouseEvent
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;

            // Find the settings browser
            CefRefPtr<CefBrowser> settings_browser = SimpleHandler::GetSettingsBrowser();
            if (settings_browser) {
                settings_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, false, 1);  // mouse down
                settings_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, true, 1);   // mouse up
                std::cout << "🧠 Left-click sent to settings overlay browser\n";
            } else {
                std::cout << "⚠️ No settings overlay browser to send left-click\n";
            }

            return 0;
        }

        case WM_RBUTTONDOWN: {
            std::cout << "🖱️ Settings Overlay received WM_RBUTTONDOWN\n";
            SetFocus(hwnd);

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Translate to CEF MouseEvent
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;

            // Find the settings browser
            CefRefPtr<CefBrowser> settings_browser = SimpleHandler::GetSettingsBrowser();
            if (settings_browser) {
                settings_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, false, 1);  // mouse down
                settings_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, true, 1);   // mouse up
                std::cout << "🧠 Right-click sent to settings overlay browser\n";
            } else {
                std::cout << "⚠️ No settings overlay browser to send right-click\n";
            }

            return 0;
        }

        case WM_CLOSE:
            std::cout << "❌ Settings Overlay received WM_CLOSE - destroying window\n";
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            std::cout << "❌ Settings Overlay received WM_DESTROY - cleaning up\n";
            // Clean up any resources if needed
            return 0;

        case WM_ACTIVATE:
            std::cout << "⚡ Settings HWND activated with state: " << LOWORD(wParam) << "\n";
            break;

        case WM_WINDOWPOSCHANGING:
            // Allow normal z-order changes for better window management
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WalletOverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEACTIVATE:
            std::cout << "👆 Wallet Overlay HWND received WM_MOUSEACTIVATE\n";
            // Allow normal activation without forcing z-order
            return MA_ACTIVATE;

        case WM_LBUTTONDOWN: {
            std::cout << "🖱️ Wallet Overlay received WM_LBUTTONDOWN\n";
            SetFocus(hwnd);

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Translate to CEF MouseEvent
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;

            // Find the wallet browser
            CefRefPtr<CefBrowser> wallet_browser = SimpleHandler::GetWalletBrowser();
            if (wallet_browser) {
                wallet_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, false, 1);  // mouse down
                wallet_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, true, 1);   // mouse up
                std::cout << "🧠 Left-click sent to wallet overlay browser\n";
            } else {
                std::cout << "⚠️ No wallet overlay browser to send left-click\n";
            }

            return 0;
        }

        case WM_RBUTTONDOWN: {
            std::cout << "🖱️ Wallet Overlay received WM_RBUTTONDOWN\n";
            SetFocus(hwnd);

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Translate to CEF MouseEvent
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;

            // Find the wallet browser
            CefRefPtr<CefBrowser> wallet_browser = SimpleHandler::GetWalletBrowser();
            if (wallet_browser) {
                wallet_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, false, 1);  // mouse down
                wallet_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, true, 1);   // mouse up
                std::cout << "🧠 Right-click sent to wallet overlay browser\n";
            } else {
                std::cout << "⚠️ No wallet overlay browser to send right-click\n";
            }

            return 0;
        }

        case WM_CLOSE:
            std::cout << "❌ Wallet Overlay received WM_CLOSE - destroying window\n";
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            std::cout << "❌ Wallet Overlay received WM_DESTROY - cleaning up\n";
            // Clean up any resources if needed
            return 0;

        case WM_ACTIVATE:
            std::cout << "⚡ Wallet HWND activated with state: " << LOWORD(wParam) << "\n";
            break;

        case WM_WINDOWPOSCHANGING:
            // Allow normal z-order changes for better window management
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK BackupOverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEACTIVATE:
            std::cout << "👆 Backup Overlay HWND received WM_MOUSEACTIVATE\n";
            return MA_ACTIVATE;

        case WM_LBUTTONDOWN: {
            std::cout << "🖱️ Backup Overlay received WM_LBUTTONDOWN\n";
            SetFocus(hwnd);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;
            CefRefPtr<CefBrowser> backup_browser = SimpleHandler::GetBackupBrowser();
            if (backup_browser) {
                backup_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, false, 1);
                backup_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, true, 1);
                std::cout << "🧠 Left-click sent to backup overlay browser\n";
            } else {
                std::cout << "⚠️ No backup overlay browser to send left-click\n";
            }
            return 0;
        }

        case WM_RBUTTONDOWN: {
            std::cout << "🖱️ Backup Overlay received WM_RBUTTONDOWN\n";
            SetFocus(hwnd);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;
            CefRefPtr<CefBrowser> backup_browser = SimpleHandler::GetBackupBrowser();
            if (backup_browser) {
                backup_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, false, 1);
                backup_browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_RIGHT, true, 1);
                std::cout << "🧠 Right-click sent to backup overlay browser\n";
            } else {
                std::cout << "⚠️ No backup overlay browser to send right-click\n";
            }
            return 0;
        }

        case WM_CLOSE:
            std::cout << "❌ Backup Overlay received WM_CLOSE - destroying window\n";
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            std::cout << "❌ Backup Overlay received WM_DESTROY - cleaning up\n";
            return 0;

        case WM_ACTIVATE:
            std::cout << "⚡ Backup HWND activated with state: " << LOWORD(wParam) << "\n";
            break;

        case WM_WINDOWPOSCHANGING:
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInstance = hInstance;
    CefMainArgs main_args(hInstance);
    CefRefPtr<SimpleApp> app(new SimpleApp());

    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    if (exit_code >= 0) return exit_code;

    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);

    // Create a separate log file for our debug messages
    std::ofstream debugLog("debug_output.log", std::ios::app);
    debugLog << "=== NEW SESSION STARTED ===" << std::endl;
    debugLog.close();

    DebugLog("Shell starting...");
    debugLog << "=== NEW SESSION STARTED ===" << std::endl;
    debugLog.close();

    CefSettings settings;
    settings.command_line_args_disabled = false;
    CefString(&settings.log_file).FromASCII("debug.log");
    settings.log_severity = LOGSEVERITY_INFO;
    settings.remote_debugging_port = 9222;
    settings.windowless_rendering_enabled = true;

    // Enable CEF's runtime API for JavaScript communication
    CefString(&settings.javascript_flags).FromASCII("--expose-gc --allow-running-insecure-content");

    // Get the executable path for subprocess
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

    // Set CEF paths - use relative paths from the executable
    CefString(&settings.resources_dir_path).FromWString(L"cef-binaries\\Resources");
    CefString(&settings.locales_dir_path).FromWString(L"cef-binaries\\Resources\\locales");
    CefString(&settings.browser_subprocess_path).FromWString(exe_path);

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int shellHeight = 80;
    int webviewHeight = height - shellHeight;

    WNDCLASS wc = {}; wc.lpfnWndProc = ShellWindowProc; wc.hInstance = hInstance;
    wc.lpszClassName = L"BitcoinBrowserWndClass"; RegisterClass(&wc);

    WNDCLASS browserClass = {}; browserClass.lpfnWndProc = DefWindowProc; browserClass.hInstance = hInstance;
    browserClass.lpszClassName = L"CEFHostWindow"; RegisterClass(&browserClass);


    WNDCLASS settingsOverlayClass = {};
    settingsOverlayClass.lpfnWndProc = SettingsOverlayWndProc;  // ✅ Settings-specific message handler
    settingsOverlayClass.hInstance = hInstance;
    settingsOverlayClass.lpszClassName = L"CEFSettingsOverlayWindow";

    if (!RegisterClass(&settingsOverlayClass)) {
        std::cout << "❌ Failed to register settings overlay window class. Error: " << GetLastError() << std::endl;
    }

    WNDCLASS walletOverlayClass = {};
    walletOverlayClass.lpfnWndProc = WalletOverlayWndProc;  // ✅ Wallet-specific message handler
    walletOverlayClass.hInstance = hInstance;
    walletOverlayClass.lpszClassName = L"CEFWalletOverlayWindow";

    if (!RegisterClass(&walletOverlayClass)) {
        std::cout << "❌ Failed to register wallet overlay window class. Error: " << GetLastError() << std::endl;
    }

    // Register backup overlay window class
    WNDCLASS backupOverlayClass = {};
    backupOverlayClass.lpfnWndProc = BackupOverlayWndProc;  // ✅ Backup-specific message handler
    backupOverlayClass.hInstance = hInstance;
    backupOverlayClass.lpszClassName = L"CEFBackupOverlayWindow";

    if (!RegisterClass(&backupOverlayClass)) {
        std::cout << "❌ Failed to register backup overlay window class. Error: " << GetLastError() << std::endl;
    }

    HWND hwnd = CreateWindow(L"BitcoinBrowserWndClass", L"Bitcoin Browser / Babbage Browser",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        rect.left, rect.top, width, height, nullptr, nullptr, hInstance, nullptr);

    HWND header_hwnd = CreateWindow(L"CEFHostWindow", nullptr,
        WS_CHILD | WS_VISIBLE, 0, 0, width, shellHeight, hwnd, nullptr, hInstance, nullptr);

    HWND webview_hwnd = CreateWindow(L"CEFHostWindow", nullptr,
        WS_CHILD | WS_VISIBLE, 0, shellHeight, width, webviewHeight, hwnd, nullptr, hInstance, nullptr);

    // 🌍 Assign to globals
    g_hwnd = hwnd;
    g_header_hwnd = header_hwnd;
    g_webview_hwnd = webview_hwnd;

    ShowWindow(hwnd, SW_SHOW);        UpdateWindow(hwnd);
    ShowWindow(header_hwnd, SW_SHOW); UpdateWindow(header_hwnd);
    ShowWindow(webview_hwnd, SW_SHOW); UpdateWindow(webview_hwnd);

    std::cout << "Initializing CEF..." << std::endl;
    bool success = CefInitialize(main_args, settings, app, nullptr);
    std::cout << "CefInitialize success: " << (success ? "true" : "false") << std::endl;

    if (!success) return 1;

    // 💡 Optionally pass handles to app instance
    app->SetWindowHandles(hwnd, header_hwnd, webview_hwnd);

    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
