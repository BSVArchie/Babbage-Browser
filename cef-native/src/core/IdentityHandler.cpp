#include "../../include/core/IdentityHandler.h"

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

bool IdentityHandler::Execute(const CefString& name,
                               CefRefPtr<CefV8Value> object,
                               const CefV8ValueList& arguments,
                               CefRefPtr<CefV8Value>& retval,
                               CefString& exception) {
    std::cout << "💡 IdentityHandler started - Function: " << name.ToString() << std::endl;
    std::cout.flush(); // Force flush

    // Also try OutputDebugString for Windows
    std::string debugMsg = "💡 IdentityHandler started - Function: " + name.ToString();
    OutputDebugStringA(debugMsg.c_str());
    OutputDebugStringA("\n");

    WalletService walletService;

    // Check if Go daemon is running
    if (!walletService.isConnected()) {
        std::cerr << "❌ Cannot connect to Go wallet daemon. Make sure it's running on port 8080." << std::endl;
        exception = "Go wallet daemon is not running. Please start the wallet daemon first.";
        return false;
    }

    // Check daemon health
    if (!walletService.isHealthy()) {
        std::cerr << "❌ Go wallet daemon is not healthy" << std::endl;
        exception = "Go wallet daemon is not responding properly.";
        return false;
    }

    if (name == "markBackedUp") {
        std::cout << "✅ Marking wallet as backed up via Go daemon" << std::endl;

        if (walletService.markBackedUp()) {
            retval = CefV8Value::CreateString("success");
        } else {
            retval = CefV8Value::CreateString("error");
        }

        return true;
    }

    try {
        // Get identity from Go daemon
        nlohmann::json identity = walletService.getIdentity();

        if (identity.empty()) {
            std::cerr << "❌ Failed to get identity from Go daemon" << std::endl;
            exception = "Failed to retrieve identity from Go wallet daemon.";
            return false;
        }

        std::cout << "📦 Identity from Go daemon: " << identity.dump() << std::endl;

        CefRefPtr<CefV8Value> identityObject = jsonToV8(identity);
        retval = identityObject;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "💥 Error in IdentityHandler: " << e.what() << std::endl;
        exception = "Exception in IdentityHandler: " + std::string(e.what());
        return false;
    }
}
