#include "wallet/WalletManager.h"
#include <iostream>

int main() {
    WalletManager wallet;

    if (wallet.walletExists()) {
        std::cout << "✅ Wallet found!" << std::endl;
        std::cout << "Address: " << wallet.getAddress() << std::endl;
        std::cout << "Public Key: " << wallet.getPublicKey() << std::endl;
    } else {
        std::cout << "❌ Wallet not found." << std::endl;
    }

    return 0;
}
