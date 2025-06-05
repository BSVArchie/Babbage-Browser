// IdentityHandler.h
#pragma once

#include "include/cef_v8.h"
#include "WalletManager.h"  // ✅ Include your native wallet logic

class IdentityHandler : public CefV8Handler {
public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override {
        if (name == "get") {
            try {
                std::string json = WalletManager::getDecryptedIdentityJSON();
                retval = CefV8Value::CreateString(json);
                return true;
            } catch (const std::exception& e) {
                exception = e.what();
                return false;
            }
        }
        return false;
    }

    IMPLEMENT_REFCOUNTING(IdentityHandler);
};

