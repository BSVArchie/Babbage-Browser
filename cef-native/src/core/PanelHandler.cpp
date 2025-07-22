#include "include/cef_v8.h"
#include "include/cef_browser.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_process_message.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"

#include "../../include/core/PanelHandler.h"

#include <iostream>

bool PanelHandler::Execute(const CefString& name,
                           CefRefPtr<CefV8Value> object,
                           const CefV8ValueList& arguments,
                           CefRefPtr<CefV8Value>& retval,
                           CefString& exception) {
    if (name == "open") {
        if (arguments.size() == 1 && arguments[0]->IsString()) {
            std::string panel = arguments[0]->GetStringValue();

            std::cout << "🧠 [PanelHandler] open() called for panel: " << panel << std::endl;

            // Create unified message to open + trigger the overlay
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("overlay_open_panel");
            CefRefPtr<CefListValue> args = msg->GetArgumentList();
            args->SetString(0, panel);

            CefV8Context::GetCurrentContext()
                ->GetFrame()
                ->SendProcessMessage(PID_BROWSER, msg);

            return true;
        } else {
            exception = "overlayPanel.open() expects one string argument.";
            return false;
        }
    }

    exception = "Unknown function: " + name.ToString();
    return false;
}
