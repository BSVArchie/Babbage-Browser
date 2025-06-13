#pragma once

#include <string>
#include <nlohmann/json.hpp>

class WalletManager {
public:
    WalletManager();

    std::string getPublicKey() const;
    std::string getAddress() const;
    bool walletExists() const;
    bool saveIdentityToFile() const;
    bool loadIdentityFromFile();
    bool markWalletAsBackedUp();

    std::string getPrivateKey() const;
    static nlohmann::json getDecryptedIdentityJSON();

private:
    std::string privateKeyHex;
    std::string publicKeyHex;
    std::string address;

    void generateKeyPair();
};
