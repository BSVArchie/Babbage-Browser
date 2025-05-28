#pragma once

#include <string>

class WalletManager {
public:
    WalletManager();

    std::string getPublicKey() const;
    std::string getAddress() const;
    bool walletExists() const;

private:
    std::string privateKeyHex;
    std::string publicKeyHex;
    std::string address;

    void generateKeyPair();
};
