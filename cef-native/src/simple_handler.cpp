// cef_native/src/simple_handler.cpp
#include "../include/simple_handler.h"
#include <windows.h>
#include <iostream>
#include <string>

SimpleHandler::SimpleHandler() {}

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() {
    return this;
}

CefRefPtr<CefDisplayHandler> SimpleHandler::GetDisplayHandler() {
    return this;
}

CefRefPtr<CefLoadHandler> SimpleHandler::GetLoadHandler() {
    return this;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
#if defined(OS_WIN)
    SetWindowText(browser->GetHost()->GetWindowHandle(), std::wstring(title).c_str());
#endif
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
    std::wcout << L"❌ Load error: " << std::wstring(failedUrl)
               << L" - " << std::wstring(errorText) << std::endl;

    if (frame->IsMain()) {
        std::string html = "<html><body><h1>Failed to load</h1><p>URL: " +
                           failedUrl.ToString() + "</p><p>Error: " +
                           errorText.ToString() + "</p></body></html>";

        std::string encoded_html;
        for (char c : html) {
            if (isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '.' || c == '-' || c == '_' || c == ':')
                encoded_html += c;
            else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
                encoded_html += buf;
            }
        }

        std::string data_url = "data:text/html," + encoded_html;
        frame->LoadURL(data_url);
    }
}

void SimpleHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
    std::cout << "📡 Loading state: " << (isLoading ? "loading..." : "done") << std::endl;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    std::cout << "🛠️ Browser created, opening DevTools..." << std::endl;
    // You can uncomment this later if you want to show DevTools
}
