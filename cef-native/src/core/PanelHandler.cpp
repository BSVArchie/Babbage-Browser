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
    } else if (name == "close") {
        std::cout << "🧠 [PanelHandler] close() called" << std::endl;

        // Create message to close the overlay
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("overlay_close");
        CefRefPtr<CefListValue> args = msg->GetArgumentList();
        // No arguments needed for close

        CefV8Context::GetCurrentContext()
            ->GetFrame()
            ->SendProcessMessage(PID_BROWSER, msg);

        return true;
    } else if (name == "toggleInput") {
        if (arguments.size() == 1 && arguments[0]->IsBool()) {
            bool enable = arguments[0]->GetBoolValue();
            std::cout << "🧠 [PanelHandler] toggleInput() called with enable: " << (enable ? "true" : "false") << std::endl;

            // Create message to toggle input
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("overlay_input");
            CefRefPtr<CefListValue> args = msg->GetArgumentList();
            args->SetBool(0, enable);

            CefV8Context::GetCurrentContext()
                ->GetFrame()
                ->SendProcessMessage(PID_BROWSER, msg);

            return true;
        } else {
            exception = "overlayPanel.toggleInput() expects one boolean argument.";
            return false;
        }
    }

    exception = "Unknown function: " + name.ToString();
    return false;
}
