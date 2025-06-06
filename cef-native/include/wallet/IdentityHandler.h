// IdentityHandler.h
#pragma once

#include "include/cef_v8.h"
#include "WalletManager.h"  // ✅ Include your native wallet logic
#include <nlohmann/json.hpp>
#include <iostream>

class IdentityHandler : public CefV8Handler {
public:

    bool IdentityHandler::Execute(const CefString& name,
                                CefRefPtr<CefV8Value> object,
                                const CefV8ValueList& arguments,
                                CefRefPtr<CefV8Value>& retval,
                                CefString& exception) {
        std::cout << "💡 IdentityHandler started" << std::endl;

        WalletManager manager;

        if (!manager.walletExists()) {
            std::cout << "🆕 No wallet found. Creating new identity..." << std::endl;
            manager.saveIdentityToFile();
        } else {
            std::cout << "📁 Wallet file exists" << std::endl;
        }

        try {
            nlohmann::json identity = manager.getDecryptedIdentityJSON();
            std::cout << "📦 Decrypted identity JSON: " << identity.dump() << std::endl;

            bool backedUp = identity.value("backedUp", false);

            if (backedUp) {
                std::cout << "✅ Wallet is backed up" << std::endl;
                nlohmann::json response = { {"backedUp", true} };
                retval = CefV8Value::CreateString(response.dump());
            } else {
                std::cout << "📝 Wallet not backed up" << std::endl;
                retval = CefV8Value::CreateString(identity.dump());
            }

            return true;

        } catch (const std::exception& e) {
            std::cerr << "💥 Error in IdentityHandler: " << e.what() << std::endl;
            exception = "Exception in IdentityHandler: " + std::string(e.what());
            return false;
        }
    }


    // bool IdentityHandler::Execute(const CefString& name,
    //                             CefRefPtr<CefV8Value> object,
    //                             const CefV8ValueList& arguments,
    //                             CefRefPtr<CefV8Value>& retval,
    //                             CefString& exception) {
    //     std::cout << "🧠 IdentityHandler Execute() called!" << std::endl;

    //     CefRefPtr<CefV8Value> result = CefV8Value::CreateObject(nullptr, nullptr);
    //     result->SetValue("test", CefV8Value::CreateString("hello world"), V8_PROPERTY_ATTRIBUTE_NONE);
    //     retval = result;

    //     return true;
    // }


    IMPLEMENT_REFCOUNTING(IdentityHandler);
};
