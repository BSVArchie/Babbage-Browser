#include "wallet/WalletManager.h"
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <sstream>
#include <iomanip>

// Helper: Convert bytes to hex string
std::string toHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return ss.str();
}

// Helper: Base58Check (simplified, not full libbitcoin base58 impl)
std::string base58Encode(const unsigned char* input, size_t len); // stub for now

WalletManager::WalletManager() {
    generateKeyPair();
}

bool WalletManager::walletExists() const {
    return true;  // will add real file check later
}

std::string WalletManager::getPublicKey() const {
    return publicKeyHex;
}

std::string WalletManager::getAddress() const {
    return address;
}

void WalletManager::generateKeyPair() {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_generate_key(key);

    const BIGNUM* privKeyBN = EC_KEY_get0_private_key(key);
    unsigned char privKeyBuf[32];
    BN_bn2binpad(privKeyBN, privKeyBuf, 32);
    privateKeyHex = toHex(privKeyBuf, 32);

    const EC_POINT* pubKeyPoint = EC_KEY_get0_public_key(key);
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    unsigned char pubKeyBuf[65];
    size_t pubKeyLen = EC_POINT_point2oct(group, pubKeyPoint, POINT_CONVERSION_UNCOMPRESSED, pubKeyBuf, sizeof(pubKeyBuf), nullptr);
    publicKeyHex = toHex(pubKeyBuf, pubKeyLen);

    // SHA256 -> RIPEMD160 of public key
    unsigned char sha256Digest[SHA256_DIGEST_LENGTH];
    SHA256(pubKeyBuf, pubKeyLen, sha256Digest);

    unsigned char ripemdDigest[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160(sha256Digest, SHA256_DIGEST_LENGTH, ripemdDigest);

    // Prepend version byte (0x00 for P2PKH)
    unsigned char addressBytes[21];
    addressBytes[0] = 0x00;
    memcpy(addressBytes + 1, ripemdDigest, RIPEMD160_DIGEST_LENGTH);

    // Base58Check (TODO: add checksum and base58 encoding)
    address = "1DummyBSVAddress"; // placeholder

    EC_GROUP_free(group);
    EC_KEY_free(key);
}
