// #define _WIN32_WINNT 0x0601

// #include "../../include/handlers/my_overlay_render_handler.h"
// #include <windows.h>
// #include <dwmapi.h>
// #include <iostream>

// MyOverlayRenderHandler::MyOverlayRenderHandler(HWND hwnd, int width, int height)
//     : hwnd_(hwnd), width_(width), height_(height),
//       hdc_mem_(nullptr), hbitmap_(nullptr), dib_data_(nullptr) {

//     // Confirm DWM composition
//     BOOL dwmEnabled = FALSE;
//     if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled))) {
//         std::cout << "→ DWM composition enabled: " << (dwmEnabled ? "true" : "false") << std::endl;
//     }

//     // Create memory DC
//     HDC screenDC = GetDC(NULL);
//     hdc_mem_ = CreateCompatibleDC(screenDC);
//     ReleaseDC(NULL, screenDC);

//     // Create simple RGB bitmap (no alpha masks)
//     BITMAPINFO bmi = {};
//     bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//     bmi.bmiHeader.biWidth = width_;
//     bmi.bmiHeader.biHeight = -height_; // top-down
//     bmi.bmiHeader.biPlanes = 1;
//     bmi.bmiHeader.biBitCount = 32;
//     bmi.bmiHeader.biCompression = BI_RGB;

//     hbitmap_ = CreateDIBSection(hdc_mem_, &bmi, DIB_RGB_COLORS, &dib_data_, nullptr, 0);
//     if (!hbitmap_ || !dib_data_) {
//         std::cout << "❌ CreateDIBSection failed." << std::endl;
//         return;
//     }

//     // Select bitmap into DC
//     if (!SelectObject(hdc_mem_, hbitmap_)) {
//         std::cout << "❌ SelectObject failed." << std::endl;
//     }

//     // uint8_t* pixels = reinterpret_cast<uint8_t*>(dib_data_);
//     // int patchSize = 50;

//     // for (int y = 0; y < patchSize; ++y) {
//     //     for (int x = 0; x < patchSize; ++x) {
//     //         int i = (y * width_ + x) * 4;
//     //         pixels[i + 0] = 50;   // B
//     //         pixels[i + 1] = 50;   // G
//     //         pixels[i + 2] = 50;   // R
//     //         pixels[i + 3] = 200;  // A — semi-opaque
//     //     }
//     // }

//     // Fill bitmap with opaque white
//     // uint32_t* pixels = static_cast<uint32_t*>(dib_data_);
//     // for (int i = 0; i < width_ * height_; ++i) {
//     //     pixels[i] = 0xCC000000;  // ARGB: semi-transparent black (alpha = 0xCC)
//     // }

//     // Log bitmap info
//     BITMAP bmp = {};
//     GetObject(hbitmap_, sizeof(BITMAP), &bmp);
//     std::cout << "→ Bitmap stride: " << bmp.bmWidthBytes << " bytes\n";
//     std::cout << "→ Bitmap size: " << bmp.bmWidth << " x " << bmp.bmHeight << std::endl;
//     std::cout << "→ bmBitsPixel: " << bmp.bmBitsPixel << ", bmPlanes: " << bmp.bmPlanes
//               << ", bmType: " << bmp.bmType << std::endl;
// }

// void MyOverlayRenderHandler::GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) {
//     rect = CefRect(0, 0, width_, height_);
// }

// void MyOverlayRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,
//                                      PaintElementType type,
//                                      const RectList& dirtyRects,
//                                      const void* buffer,
//                                      int width, int height) {
//     std::cout << "🧪 OnPaint — minimal test using constructor DC and bitmap\n";


//     bool isMostlyTransparent = true;
//     const uint8_t* alpha = reinterpret_cast<const uint8_t*>(buffer);

//     for (int i = 3; i < width * height * 4; i += 4) {
//         if (alpha[i] > 20) {  // found a visible pixel
//             isMostlyTransparent = false;
//             break;
//         }
//     }

//     if (isMostlyTransparent && dib_data_) {
//         uint8_t* pixels = reinterpret_cast<uint8_t*>(dib_data_);
//         int patchSize = 50;
//         for (int y = 0; y < patchSize; ++y) {
//             for (int x = 0; x < patchSize; ++x) {
//                 int i = (y * width + x) * 4;
//                 pixels[i + 0] = 50;  // B
//                 pixels[i + 1] = 50;  // G
//                 pixels[i + 2] = 50;  // R
//                 pixels[i + 3] = 200; // A — semi-opaque
//             }
//         }
//         std::cout << "🩹 Patch applied for early input hit-test." << std::endl;
//     } else if (buffer && dib_data_) {
//         std::memcpy(dib_data_, buffer, width * height * 4);  // ✅ Only copy React buffer if it's useful
//     }


//     RECT hwndRect;
//     GetWindowRect(hwnd_, &hwndRect);
//     std::cout << "→ HWND real size: " << (hwndRect.right - hwndRect.left)
//               << " x " << (hwndRect.bottom - hwndRect.top) << std::endl;

//     POINT ptWinPos = {0, 0};
//     SIZE sizeWin = {width_, height_};
//     POINT ptSrc = {0, 0};

//     BLENDFUNCTION blend = {};
//     blend.BlendOp = AC_SRC_OVER;
//     blend.SourceConstantAlpha = 255;
//     // blend.AlphaFormat = 0;  // Use 1 if you later re-enable per-pixel alpha
//     blend.AlphaFormat = AC_SRC_ALPHA;

//     LONG exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);
//     std::cout << "→ HWND EXSTYLE: 0x" << std::hex << exStyle << std::endl;
//     std::cout << "→ Has WS_EX_LAYERED: " << ((exStyle & WS_EX_LAYERED) != 0) << std::endl;

//     HDC screenDC = GetDC(NULL);

//     if (buffer && dib_data_) {
//         std::memcpy(dib_data_, buffer, width * height * 4);  // Assuming 4 bytes per pixel
//     }

//     BOOL result = UpdateLayeredWindow(hwnd_, screenDC, &ptWinPos, &sizeWin,
//                                       hdc_mem_, &ptSrc, 0, &blend, ULW_ALPHA);
//     ReleaseDC(NULL, screenDC);

//     DWORD err = GetLastError();
//     std::cout << "→ UpdateLayeredWindow result: " << (result ? "success" : "fail")
//               << ", error: " << err << std::endl;
//     std::cout << "→ IsWindowVisible(hwnd_): " << IsWindowVisible(hwnd_) << std::endl;
// }

#define _WIN32_WINNT 0x0601

#include "../../include/handlers/my_overlay_render_handler.h"
#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include "include/wrapper/cef_helpers.h"  // ✅ required for CEF_REQUIRE_UI_THREAD()

MyOverlayRenderHandler::MyOverlayRenderHandler(HWND hwnd, int width, int height)
    : hwnd_(hwnd), width_(width), height_(height),
      hdc_mem_(nullptr), hbitmap_(nullptr), dib_data_(nullptr) {

    // Confirm DWM composition
    BOOL dwmEnabled = FALSE;
    if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled))) {
        std::cout << "→ DWM composition enabled: " << (dwmEnabled ? "true" : "false") << std::endl;
    }

    // Create memory DC
    HDC screenDC = GetDC(NULL);
    hdc_mem_ = CreateCompatibleDC(screenDC);
    ReleaseDC(NULL, screenDC);

    // Create simple RGB bitmap (no alpha masks)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width_;
    bmi.bmiHeader.biHeight = -height_; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbitmap_ = CreateDIBSection(hdc_mem_, &bmi, DIB_RGB_COLORS, &dib_data_, nullptr, 0);
    if (!hbitmap_ || !dib_data_) {
        std::cout << "❌ CreateDIBSection failed." << std::endl;
        return;
    }

    // Select bitmap into DC
    if (!SelectObject(hdc_mem_, hbitmap_)) {
        std::cout << "❌ SelectObject failed." << std::endl;
    }

    // Prime layered HWND with dummy pixel for early hit-test
    UpdateLayeredWindow(hwnd_, hdc_mem_, nullptr, nullptr, hdc_mem_, nullptr, 0, nullptr, ULW_ALPHA);
    DWORD err = GetLastError();
    std::cout << "🧪 Dummy layered update error: " << err << std::endl;

    // Log bitmap info
    BITMAP bmp = {};
    GetObject(hbitmap_, sizeof(BITMAP), &bmp);
    std::cout << "→ Bitmap stride: " << bmp.bmWidthBytes << " bytes\n";
    std::cout << "→ Bitmap size: " << bmp.bmWidth << " x " << bmp.bmHeight << std::endl;
    std::cout << "→ bmBitsPixel: " << bmp.bmBitsPixel << ", bmPlanes: " << bmp.bmPlanes
              << ", bmType: " << bmp.bmType << std::endl;
}

void MyOverlayRenderHandler::GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) {
    rect = CefRect(0, 0, width_, height_);
}

void MyOverlayRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                                     PaintElementType type,
                                     const RectList& dirtyRects,
                                     const void* buffer,
                                     int width, int height) {
    CEF_REQUIRE_UI_THREAD();  // ✅ Confirm we're on the UI thread

    std::cout << "🧪 OnPaint — minimal test using constructor DC and bitmap\n";

    bool isMostlyTransparent = true;
    const uint8_t* alpha = reinterpret_cast<const uint8_t*>(buffer);

    for (int i = 3; i < width * height * 4; i += 4) {
        if (alpha[i] > 20) {  // found a visible pixel
            isMostlyTransparent = false;
            break;
        }
    }

    if (isMostlyTransparent && dib_data_) {
        uint8_t* pixels = reinterpret_cast<uint8_t*>(dib_data_);
        int patchSize = 50;
        for (int y = 0; y < patchSize; ++y) {
            for (int x = 0; x < patchSize; ++x) {
                int i = (y * width + x) * 4;
                pixels[i + 0] = 50;  // B
                pixels[i + 1] = 50;  // G
                pixels[i + 2] = 50;  // R
                pixels[i + 3] = 200; // A — semi-opaque
            }
        }
        std::cout << "🩹 Patch applied for early input hit-test." << std::endl;
    } else if (buffer && dib_data_) {
        std::memcpy(dib_data_, buffer, width * height * 4);
    }

    RECT hwndRect;
    GetWindowRect(hwnd_, &hwndRect);
    std::cout << "→ HWND real size: " << (hwndRect.right - hwndRect.left)
              << " x " << (hwndRect.bottom - hwndRect.top) << std::endl;

    POINT ptWinPos = {0, 0};
    SIZE sizeWin = {width_, height_};
    POINT ptSrc = {0, 0};

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    LONG exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);
    std::cout << "→ HWND EXSTYLE: 0x" << std::hex << exStyle << std::endl;
    std::cout << "→ Has WS_EX_LAYERED: " << ((exStyle & WS_EX_LAYERED) != 0) << std::endl;

    HDC screenDC = GetDC(NULL);

    BOOL result = UpdateLayeredWindow(hwnd_, screenDC, &ptWinPos, &sizeWin,
                                      hdc_mem_, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, screenDC);

    DWORD err = GetLastError();
    std::cout << "→ UpdateLayeredWindow result: " << (result ? "success" : "fail")
              << ", error: " << err << std::endl;
    std::cout << "→ IsWindowVisible(hwnd_): " << IsWindowVisible(hwnd_) << std::endl;
}
