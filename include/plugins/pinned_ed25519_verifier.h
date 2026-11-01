/**
 * @file pinned_ed25519_verifier.h
 * @brief Shared pinned-key Ed25519 verification primitive for ThemisDB artifacts.
 */

#pragma once

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::plugins::pinned_ed25519 {

/**
 * @brief Compute the canonical lowercase SHA-256 fingerprint of an Ed25519 key.
 * @param public_key Raw Ed25519 public key; must contain exactly 32 bytes.
 * @return 64-character lowercase hexadecimal fingerprint, or empty on failure.
 */
inline std::string fingerprint(const std::vector<uint8_t>& public_key) {
    if (public_key.size() != 32) {
        return {};
    }
    uint8_t digest[SHA256_DIGEST_LENGTH]{};
    if (SHA256(public_key.data(), public_key.size(), digest) == nullptr) {
        return {};
    }
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.resize(SHA256_DIGEST_LENGTH * 2);
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        result[i * 2] = kHex[digest[i] >> 4];
        result[i * 2 + 1] = kHex[digest[i] & 0x0f];
    }
    return result;
}

/**
 * @brief Verify a detached Base64 Ed25519 signature using an explicitly pinned key.
 * @param pinned_fingerprint Expected lowercase SHA-256 fingerprint of public_key.
 * @param public_key Raw 32-byte Ed25519 public key.
 * @param message Exact signed byte sequence.
 * @param signature_base64 Standard Base64 encoding of a 64-byte Ed25519 signature.
 * @return true only when the fingerprint matches and the cryptographic check succeeds.
 */
inline bool verifyRaw(
    const std::vector<uint8_t>& public_key,
    const std::string& message,
    const std::vector<uint8_t>& signature) {
    if (public_key.size() != 32 || message.empty() || signature.size() != 64) {
        return false;
    }

    struct PkeyDeleter {
        void operator()(EVP_PKEY* key) const noexcept {
            EVP_PKEY_free(key);
        }
    };
    struct ContextDeleter {
        void operator()(EVP_MD_CTX* context) const noexcept {
            EVP_MD_CTX_free(context);
        }
    };

    std::unique_ptr<EVP_PKEY, PkeyDeleter> key(EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr, public_key.data(), public_key.size()));
    std::unique_ptr<EVP_MD_CTX, ContextDeleter> context(EVP_MD_CTX_new());
    if (!key || !context ||
        EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, key.get()) != 1) {
        return false;
    }
    return EVP_DigestVerify(
               context.get(), signature.data(), signature.size(),
               reinterpret_cast<const unsigned char*>(message.data()),
               message.size()) == 1;
}

/**
 * @brief Verify a detached Base64 Ed25519 signature using an explicitly pinned key.
 * @param pinned_fingerprint Expected lowercase SHA-256 fingerprint of public_key.
 * @param public_key Raw 32-byte Ed25519 public key.
 * @param message Exact signed byte sequence.
 * @param signature_base64 Standard Base64 encoding of a 64-byte Ed25519 signature.
 * @return true only when the fingerprint matches and the cryptographic check succeeds.
 */
inline bool verify(
    const std::string& pinned_fingerprint,
    const std::vector<uint8_t>& public_key,
    const std::string& message,
    const std::string& signature_base64) {
    if (pinned_fingerprint.empty() || message.empty() ||
        fingerprint(public_key) != pinned_fingerprint ||
        signature_base64.size() != 88 ||
        signature_base64.compare(86, 2, "==") != 0) {
        return false;
    }

    for (size_t i = 0; i < signature_base64.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(signature_base64[i]);
        const bool alphabet =
            (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '+' || c == '/';
        if (i < 86 && !alphabet) {
            return false;
        }
    }

    std::vector<uint8_t> signature(66);
    const int decoded_size = EVP_DecodeBlock(
        signature.data(),
        reinterpret_cast<const unsigned char*>(signature_base64.data()),
        static_cast<int>(signature_base64.size()));
    if (decoded_size != 66) {
        return false;
    }
    signature.resize(64);
    return verifyRaw(public_key, message, signature);
}

}  // namespace themis::plugins::pinned_ed25519
