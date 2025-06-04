#include "wallet/WalletManager.h"
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// === Utility Functions ===

static std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return oss.str();
}

static const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static std::string base58Encode(const std::vector<unsigned char>& input) {
    std::vector<unsigned char> digits((input.size() * 138 / 100) + 1);
    size_t digits_len = 1;

    for (unsigned char byte : input) {
        int carry = byte;
        for (size_t j = 0; j < digits_len; ++j) {
            carry += digits[j] << 8;
            digits[j] = carry % 58;
            carry /= 58;
        }
        while (carry) {
            digits[digits_len++] = carry % 58;
            carry /= 58;
        }
    }

    std::string result;
    for (unsigned char byte : input) {
        if (byte == 0x00) result += '1';
        else break;
    }
    for (size_t i = 0; i < digits_len; ++i) {
        result += BASE58_ALPHABET[digits[digits_len - 1 - i]];
    }
    return result;
}

// === WalletManager Methods ===

WalletManager::WalletManager() {
    if (!walletExists()) {
        std::cout << "🔐 No wallet found. Generating new key pair...\n";
        generateKeyPair();
        saveIdentityToFile();
        std::cout << "💾 Identity saved to AppData.\n";
    } else {
        std::cout << "✅ Wallet found!\n";
        loadIdentityFromFile();
    }
}

bool WalletManager::walletExists() const {
    std::string appData = std::getenv("APPDATA");
    std::filesystem::path file = std::filesystem::path(appData) / "BabbageBrowser" / "identity.json";
    return std::filesystem::exists(file);
}

void WalletManager::generateKeyPair() {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key || !EC_KEY_generate_key(key)) {
        std::cerr << "Failed to generate ECC key.\n";
        return;
    }

    // === Private Key (hex) ===
    const BIGNUM* priv_key_bn = EC_KEY_get0_private_key(key);
    if (priv_key_bn) {
        std::vector<unsigned char> priv(BN_num_bytes(priv_key_bn));
        BN_bn2bin(priv_key_bn, priv.data());
        privateKeyHex = toHex(priv.data(), priv.size());
    }

    // === Public Key (hex) ===
    int pub_len = i2o_ECPublicKey(key, nullptr);
    std::vector<unsigned char> pub(pub_len);
    unsigned char* pub_ptr = pub.data();
    i2o_ECPublicKey(key, &pub_ptr);
    publicKeyHex = toHex(pub.data(), pub.size());

    // === Address ===
    unsigned char sha256[SHA256_DIGEST_LENGTH];
    SHA256(pub.data(), pub.size(), sha256);

    unsigned char ripemd160[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160(sha256, SHA256_DIGEST_LENGTH, ripemd160);

    std::vector<unsigned char> address_data;
    address_data.push_back(0x00); // Mainnet version byte
    address_data.insert(address_data.end(), ripemd160, ripemd160 + RIPEMD160_DIGEST_LENGTH);

    unsigned char checksum[SHA256_DIGEST_LENGTH];
    SHA256(address_data.data(), address_data.size(), checksum);
    SHA256(checksum, SHA256_DIGEST_LENGTH, checksum);
    address_data.insert(address_data.end(), checksum, checksum + 4);

    address = base58Encode(address_data);

    EC_KEY_free(key);
}

bool WalletManager::saveIdentityToFile() const {
    try {
        std::string appData = std::getenv("APPDATA");
        std::filesystem::path dir = std::filesystem::path(appData) / "BabbageBrowser";
        std::filesystem::create_directories(dir);

        json identity = {
            {"publicKey", publicKeyHex},
            {"address", address},
            {"privateKey", privateKeyHex}
        };

        std::ofstream file(dir / "identity.json");
        file << identity.dump(4);
        file.close();

        return true;
    } catch (...) {
        return false;
    }
}

bool WalletManager::loadIdentityFromFile() {
    try {
        std::string appData = std::getenv("APPDATA");
        std::filesystem::path path = std::filesystem::path(appData) / "BabbageBrowser" / "identity.json";

        if (!std::filesystem::exists(path)) {
            std::cerr << "⚠️ Identity file not found at: " << path << std::endl;
            return false;
        }

        std::ifstream file(path);
        json identity;
        file >> identity;

        privateKeyHex = identity.at("privateKey").get<std::string>();
        publicKeyHex = identity.at("publicKey").get<std::string>();
        address = identity.at("address").get<std::string>();

        std::cout << "🔑 Identity loaded from file." << std::endl;
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "❌ Failed to load identity: " << ex.what() << std::endl;
        return false;
    }
}

std::string WalletManager::getPublicKey() const {
    return publicKeyHex;
}

std::string WalletManager::getAddress() const {
    return address;
}

std::string WalletManager::getPrivateKey() const {
    return privateKeyHex;
}
