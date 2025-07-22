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
// #include "include/core/IdentityHandler.h"
#include "include/handlers/simple_handler.h"
#include "include/handlers/simple_render_process_handler.h"
#include "include/handlers/simple_app.h"
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>
#include <filesystem>
#include <iostream>

HWND g_hwnd = nullptr;
HWND g_header_hwnd = nullptr;
HWND g_webview_hwnd = nullptr;
HWND g_overlay_hwnd = nullptr;
HINSTANCE g_hInstance = nullptr;



LRESULT CALLBACK ShellWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOVE:
        case WM_SIZE:
            if (g_hwnd && g_overlay_hwnd) {
                RECT shellRect;
                GetWindowRect(g_hwnd, &shellRect);
                int width  = shellRect.right - shellRect.left;
                int height = shellRect.bottom - shellRect.top;

                SetWindowPos(g_overlay_hwnd, HWND_TOPMOST,
                             shellRect.left, shellRect.top,
                             width, height,
                             SWP_NOOWNERZORDER);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEACTIVATE:
            std::cout << "👆 Overlay HWND received WM_MOUSEACTIVATE\n";
            return MA_ACTIVATE;

        case WM_LBUTTONDOWN: {
            std::cout << "🖱️ Overlay received WM_LBUTTONDOWN\n";
            SetFocus(hwnd);

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // Translate to CEF MouseEvent
            CefMouseEvent mouse_event;
            mouse_event.x = pt.x;
            mouse_event.y = pt.y;
            mouse_event.modifiers = 0;

            CefRefPtr<CefBrowser> overlay = SimpleHandler::GetOverlayBrowser();
            if (overlay) {
                overlay->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, false, 1);  // mouse down
                overlay->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, true, 1);   // mouse up
                std::cout << "🧠 Mouse click sent to overlay browser\n";
            } else {
                std::cout << "⚠️ No overlay browser to send click\n";
            }

            return 0;
        }

        case WM_ACTIVATE:
            std::cout << "⚡ HWND activated with state: " << LOWORD(wParam) << "\n";
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
    std::cout << "Shell starting..." << std::endl;

    CefSettings settings;
    settings.command_line_args_disabled = false;
    CefString(&settings.log_file).FromASCII("debug.log");
    settings.log_severity = LOGSEVERITY_INFO;
    settings.remote_debugging_port = 9222;
    settings.windowless_rendering_enabled = true;

    CefString(&settings.resources_dir_path).FromWString(L"...");
    CefString(&settings.locales_dir_path).FromWString(L"...");
    CefString(&settings.browser_subprocess_path).FromWString(L"...");

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

    // WNDCLASS overlayClass = {}; overlayClass.lpfnWndProc = DefWindowProc; overlayClass.hInstance = hInstance;
    // overlayClass.lpszClassName = L"CEFOverlayWindow"; RegisterClass(&overlayClass);

    WNDCLASS overlayClass = {};
    overlayClass.lpfnWndProc = OverlayWndProc;  // ✅ Correct static function
    overlayClass.hInstance = hInstance;
    overlayClass.lpszClassName = L"CEFOverlayWindow";

    if (!RegisterClass(&overlayClass)) {
        std::cout << "❌ Failed to register overlay window class. Error: " << GetLastError() << std::endl;
    }

    // WNDCLASS overlayClass = {}; overlayClass.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
    //     switch (msg) {
    //         case WM_MOUSEACTIVATE:
    //             std::cout << "👆 Overlay HWND received WM_MOUSEACTIVATE\n";
    //             return MA_ACTIVATE;

    //         case WM_LBUTTONDOWN:
    //             std::cout << "🖱️ Overlay received WM_LBUTTONDOWN\n";
    //             SetFocus(hwnd);  // Optional: give focus
    //             return 0;

    //         case WM_ACTIVATE:
    //             std::cout << "⚡ HWND activated with state: " << LOWORD(wParam) << "\n";
    //             break;
    //     }
    //     return DefWindowProc(hwnd, msg, wParam, lParam);
    // };

    HWND hwnd = CreateWindow(L"BitcoinBrowserWndClass", L"Bitcoin Browser / Babbage Browser",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        rect.left, rect.top, width, height, nullptr, nullptr, hInstance, nullptr);

    HWND header_hwnd = CreateWindow(L"CEFHostWindow", nullptr,
        WS_CHILD | WS_VISIBLE, 0, 0, width, shellHeight, hwnd, nullptr, hInstance, nullptr);

    HWND webview_hwnd = CreateWindow(L"CEFHostWindow", nullptr,
        WS_CHILD | WS_VISIBLE, 0, shellHeight, width, webviewHeight, hwnd, nullptr, hInstance, nullptr);

    // HWND overlay_hwnd = CreateWindowEx(
    //     WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    //     L"CEFOverlayWindow", nullptr, WS_POPUP | WS_VISIBLE,
    //     0, 0, width, height, hwnd, nullptr, hInstance, nullptr);

    // LONG exStyle = GetWindowLong(overlay_hwnd, GWL_EXSTYLE);
    // std::cout << "🧠 HWND EXSTYLE AFTER CREATE: " << std::hex << exStyle << std::endl;

    // 🌍 Assign to globals
    g_hwnd = hwnd;
    g_header_hwnd = header_hwnd;
    g_webview_hwnd = webview_hwnd;
    // g_overlay_hwnd = overlay_hwnd;

    ShowWindow(hwnd, SW_SHOW);        UpdateWindow(hwnd);
    ShowWindow(header_hwnd, SW_SHOW); UpdateWindow(header_hwnd);
    ShowWindow(webview_hwnd, SW_SHOW); UpdateWindow(webview_hwnd);
    // ShowWindow(overlay_hwnd, SW_SHOW); UpdateWindow(overlay_hwnd);

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
