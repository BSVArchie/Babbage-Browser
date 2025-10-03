#pragma once

#include "include/cef_resource_request_handler.h"
#include "include/cef_resource_handler.h"
#include "include/cef_request.h"
#include "include/cef_response.h"
#include "include/cef_callback.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_urlrequest.h"
#include <string>

class HttpRequestInterceptor : public CefResourceRequestHandler {
public:
    HttpRequestInterceptor();
    virtual ~HttpRequestInterceptor();

    // CefResourceRequestHandler methods
    CefRefPtr<CefResourceHandler> GetResourceHandler(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request) override;

    void OnResourceRedirect(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefRequest> request,
                           CefRefPtr<CefResponse> response,
                           CefString& new_url) override;

    bool OnResourceResponse(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefRequest> request,
                           CefRefPtr<CefResponse> response) override;

private:
    // Helper methods
    bool isWalletEndpoint(const std::string& url);

    IMPLEMENT_REFCOUNTING(HttpRequestInterceptor);
    DISALLOW_COPY_AND_ASSIGN(HttpRequestInterceptor);
};
