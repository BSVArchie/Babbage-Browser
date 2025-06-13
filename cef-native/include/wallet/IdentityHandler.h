// IdentityHandler.h
#pragma once

#include "include/cef_v8.h"
#include "WalletManager.h"  // ✅ Include your native wallet logic
#include <nlohmann/json.hpp>
#include <iostream>

CefRefPtr<CefV8Value> jsonToV8(const nlohmann::json& j) {
    if (j.is_object()) {
        CefRefPtr<CefV8Value> obj = CefV8Value::CreateObject(nullptr, nullptr);
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            const auto& value = it.value();
            if (value.is_string()) {
                obj->SetValue(key, CefV8Value::CreateString(value.get<std::string>()), V8_PROPERTY_ATTRIBUTE_NONE);
            } else if (value.is_boolean()) {
                obj->SetValue(key, CefV8Value::CreateBool(value), V8_PROPERTY_ATTRIBUTE_NONE);
            } else if (value.is_number_integer()) {
                obj->SetValue(key, CefV8Value::CreateInt(value), V8_PROPERTY_ATTRIBUTE_NONE);
            } else if (value.is_number_float()) {
                obj->SetValue(key, CefV8Value::CreateDouble(value), V8_PROPERTY_ATTRIBUTE_NONE);
            } else {
                obj->SetValue(key, CefV8Value::CreateString(value.dump()), V8_PROPERTY_ATTRIBUTE_NONE);
            }
        }
        return obj;
    }
    return CefV8Value::CreateUndefined();
}

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

        if (name == "markBackedUp") {
            std::cout << "✅ Marking wallet as backed up" << std::endl;

            WalletManager manager;
            if (manager.markWalletAsBackedUp()) {
                retval = CefV8Value::CreateString("success");
            } else {
                retval = CefV8Value::CreateString("error");
            }

            return true;
        }

        try {
            nlohmann::json identity = manager.getDecryptedIdentityJSON();
            std::cout << "📦 Decrypted identity JSON: " << identity.dump() << std::endl;

            bool backedUp = identity.value("backedUp", false);

            if (backedUp) {
                std::cout << "✅ Wallet is backed up" << std::endl;
                nlohmann::json response = { {"backedUp", true} };
                // retval = CefV8Value::CreateString(response.dump());
                CefRefPtr<CefV8Value> identityObject = jsonToV8(identity);
                retval = identityObject;

            } else {
                std::cout << "📝 Wallet not backed up" << std::endl;
                // retval = CefV8Value::CreateString(identity.dump());
                CefRefPtr<CefV8Value> identityObject = jsonToV8(identity);
                retval = identityObject;

            }

            return true;

        } catch (const std::exception& e) {
            std::cerr << "💥 Error in IdentityHandler: " << e.what() << std::endl;
            exception = "Exception in IdentityHandler: " + std::string(e.what());
            return false;
        }
    }

    IMPLEMENT_REFCOUNTING(IdentityHandler);
};
