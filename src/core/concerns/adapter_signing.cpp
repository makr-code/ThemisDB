/**
 * @file adapter_signing.cpp
 * @brief SHA-256 adapter signing validator implementation.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#include "core/concerns/adapter_signing.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SignedAdapterValidator::SignedAdapterValidator(AdapterSignature expected_sig)
    : expected_sig_(std::move(expected_sig)) {}

// ---------------------------------------------------------------------------
// validate()
// ---------------------------------------------------------------------------

bool SignedAdapterValidator::validate(const AdapterMetadata& m) {
    // Reject absent or unsupported algorithm
    if (!expected_sig_.present()) {
        return false;
    }
    if (expected_sig_.algorithm != "sha256") {
        return false;
    }

    // Compute digest of canonical representation
    const std::string canonical = canonicalString(m);
    const std::string computed  = sha256Hex(canonical);
    if (computed.empty()) {
        // Internal OpenSSL failure — fail closed
        return false;
    }

    // Constant-time comparison to resist timing side-channels.
    // Both strings must be the same length (64 hex chars for sha256).
    const std::string& expected = expected_sig_.digest;
    if (static_cast<int>(computed.size()) != expected.size()) {
        return false;
    }

    // CRYPTO_memcmp is a constant-time byte comparison from OpenSSL.
    return CRYPTO_memcmp(computed.data(), expected.data(),static_cast<int>(computed.size())) == 0;
}

// ---------------------------------------------------------------------------
// canonicalString()
// ---------------------------------------------------------------------------

std::string SignedAdapterValidator::canonicalString(const AdapterMetadata& m) {
    std::string result = {};
    result.reserve(m.id.size() + 12 + m.description.size());
    result += m.id;
    result += ':';
    result += std::to_string(m.apiVersion);
    result += ':';
    result += m.description;
    return result;
}

// ---------------------------------------------------------------------------
// sha256Hex()
// ---------------------------------------------------------------------------

std::string SignedAdapterValidator::sha256Hex(std::string_view data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return {};
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    if (EVP_DigestUpdate(ctx, data.data(),static_cast<int>(data.size())) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return oss.str();
}

} // namespace concerns
} // namespace core
} // namespace themis
