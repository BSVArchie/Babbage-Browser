#pragma once

#include <string>

class WalletManager {
public:
    WalletManager();

    std::string getPublicKey() const;
    std::string getAddress() const;
    bool walletExists() const;

    bool saveIdentityToFile() const;
    bool loadIdentityFromFile();

    std::string getPrivateKey() const;
    static std::string getDecryptedIdentityJSON();

private:
    std::string privateKeyHex;
    std::string publicKeyHex;
    std::string address;

    void generateKeyPair();
};
