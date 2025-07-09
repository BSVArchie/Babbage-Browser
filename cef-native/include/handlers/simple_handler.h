// cef_native/src/simple_handler.h
#pragma once

#include "include/cef_client.h"
#include "include/cef_display_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"

class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefDisplayHandler,
                      public CefLoadHandler {
public:
    explicit SimpleHandler(const std::string& role);

    // CefClient methods
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;
    static CefRefPtr<CefBrowser> webview_browser_;


    // CefDisplayHandler methods
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

    // CefLoadHandler methods
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override;

    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                               bool isLoading,
                               bool canGoBack,
                               bool canGoForward) override;

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefProcessId source_process,
                              CefRefPtr<CefProcessMessage> message) override;

private:
    std::string role_;
    IMPLEMENT_REFCOUNTING(SimpleHandler);
};
