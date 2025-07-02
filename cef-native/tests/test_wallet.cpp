#include "../include/core/WalletManager.h"
#include <iostream>

int main() {
    WalletManager wallet;
    if (wallet.walletExists()) {
        std::cout << "✅ Wallet found!" << std::endl;
        std::cout << "Address: " << wallet.getAddress() << std::endl;
        std::cout << "Public Key: " << wallet.getPublicKey() << std::endl;
        std::cout << "Private Key: " << wallet.getPrivateKey() << std::endl;
        if (wallet.saveIdentityToFile()) {
            std::cout << "💾 Identity saved to AppData." << std::endl;
        } else {
            std::cout << "❌ Failed to save identity." << std::endl;
        }
    }

    return 0;
}
