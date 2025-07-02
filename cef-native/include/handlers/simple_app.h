// include/core/simple_app.h
#pragma once

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_browser_process_handler.h"
#include "simple_render_process_handler.h"
#include "simple_handler.h"

class SimpleApp : public CefApp,
                  public CefBrowserProcessHandler,
                  public CefRenderProcessHandler {
public:
    SimpleApp();

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> command_line) override;

    void OnContextInitialized() override;

private:
    CefRefPtr<SimpleRenderProcessHandler> render_process_handler_;

    IMPLEMENT_REFCOUNTING(SimpleApp);
};
