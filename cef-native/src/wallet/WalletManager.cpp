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
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>

using json = nlohmann::json;

// === Utility Functions ===
static std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return oss.str();
}

const unsigned char AES_KEY[32] = "0123456789012345678901234567890"; // Example 256-bit key
const int AES_BLOCK_SIZE = 16;

std::string encryptAES(const std::string& plaintext) {
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, sizeof(iv));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
    int len = 0, ciphertext_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, AES_KEY, iv);
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const unsigned char*)plaintext.data(), plaintext.length());
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    std::string result((char*)iv, AES_BLOCK_SIZE); // prepend IV
    result.append((char*)ciphertext.data(), ciphertext_len);
    return toHex((const unsigned char*)result.data(), result.size());
}

std::string decryptAES(const std::string& hexCiphertext) {
    std::vector<unsigned char> raw;
    for (size_t i = 0; i < hexCiphertext.length(); i += 2) {
        std::string byteString = hexCiphertext.substr(i, 2);
        raw.push_back((unsigned char)std::stoi(byteString, nullptr, 16));
    }

    const unsigned char* iv = raw.data();
    const unsigned char* ciphertext = raw.data() + AES_BLOCK_SIZE;
    int ciphertext_len = raw.size() - AES_BLOCK_SIZE;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> plaintext(ciphertext_len + AES_BLOCK_SIZE);
    int len = 0, plaintext_len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, AES_KEY, iv);
    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)plaintext.data(), plaintext_len);
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
        std::cout << "🔐 No wallet found. Generating new key pair..." << std::endl;
        generateKeyPair();
        saveIdentityToFile();
        std::cout << "💾 Identity saved to AppData." << std::endl;

        std::cout << "⚠️  IMPORTANT: Back up your private key now!" << std::endl;
        std::cout << "⚠️  If you lose this key, your funds will be permanently inaccessible." << std::endl;
        std::cout << "⚠️  You can find it in: %APPDATA%/BabbageBrowser/identity.json" << std::endl;
    } else {
        std::cout << "✅ Wallet found!" << std::endl;
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
            {"privateKey", encryptAES(privateKeyHex)},
            {"backedUp", false}
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
        std::filesystem::path filePath = std::filesystem::path(appData) / "BabbageBrowser" / "identity.json";

        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        json identity;
        file >> identity;

        publicKeyHex = identity["publicKey"].get<std::string>();
        address = identity["address"].get<std::string>();
        privateKeyHex = decryptAES(identity["privateKey"].get<std::string>());

        return true;
    } catch (...) {
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

nlohmann::json WalletManager::getDecryptedIdentityJSON() {
    try {
        std::string appData = std::getenv("APPDATA");
        std::filesystem::path filePath = std::filesystem::path(appData) / "BabbageBrowser" / "identity.json";

        std::ifstream file(filePath);
        if (!file.is_open()) return "{}";

        nlohmann::json j;
        file >> j;
        file.close();

        std::string decryptedPrivateKey = decryptAES(j["privateKey"]);
        j["privateKey"] = decryptedPrivateKey;

        return j;  // Return full JSON with decrypted key
    } catch (...) {
        return "{}";
    }
}
