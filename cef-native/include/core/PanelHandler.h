// handlers/PanelHandler.h
#pragma once

#include "include/cef_browser.h"
#include "include/cef_process_message.h"
#include "include/cef_v8.h"
#include <iostream>

class PanelHandler : public CefV8Handler {

public:
    PanelHandler() {}
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override;

private:
    IMPLEMENT_REFCOUNTING(PanelHandler);
};
