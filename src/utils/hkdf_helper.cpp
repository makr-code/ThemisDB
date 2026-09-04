/**
 * @file hkdf_helper.cpp
 * @brief HKDF implementation details and memory management.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Last Updated: 2026-08-08
 * @note Source: Level 1 - Implementation Details
 * @note SOT Domain: crypto-key-management
 * 
 * ## Implementation Notes
 * 
 * **Memory Safety:**
 * - All key material is stored in std::vector (heap allocated)
 * - OPENSSL_cleanse() is NOT used here (only in cache/storage layers)
 * - Callers responsible for cleanup via OPENSSL_cleanse() or volatile_free()
 * - No automatic wiping on destruction to allow key passing between layers
 * 
 * **OpenSSL Compatibility:**
 * - Compile-time detection of OpenSSL 3.0+ vs 1.1.1
 * - OPENSSL_VERSION_NUMBER >= 0x30000000L indicates 3.0+
 * - OpenSSL 3.0 uses EVP_KDF API (modern, thread-safe)
 * - OpenSSL 1.1.1 uses EVP_PKEY_CTX API (legacy fallback)
 * - Both paths produce identical RFC 5869 output
 * 
 * **RFC 5869 Compliance:**
 * - HKDF-Extract: PRK = HMAC-SHA256(salt, IKM)
 * - HKDF-Expand: OKM = PRF(PRK, info || 0x01)
 * - Output limit: 255 * hash_length = 255 * 32 = 8160 bytes for SHA256
 * 
 * **Timing Considerations:**
 * - SHA256 hash is not constant-time (leaks key length via timing)
 * - Caller responsible for constant-time comparisons if needed
 * - HKDF itself has no length-based side channels beyond output length
 * 
 * **Error Handling:**
 * - EVP_KDF_fetch() can fail on OpenSSL 3.0+ if HKDF provider missing
 * - EVP_PKEY_CTX_new() can fail on resource exhaustion
 * - EVP_KDF_derive() can fail if params invalid or buffer too small
 * - All failures throw std::runtime_error with descriptive message
 * 
 * @warning This file contains sensitive key derivation code.
 *          Do not modify without security review.
 * @note Security audit: [LINK TO AUDIT FINDINGS]
 */

#include "utils/hkdf_helper.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <stdexcept>
#include <cstring>
#include "utils/error_contracts.h"

namespace themis {
namespace utils {

std::vector<uint8_t> HKDFHelper::derive(
    const std::vector<uint8_t>& ikm,
    const std::vector<uint8_t>& salt,
    const std::string& info,
    size_t output_length) {
    
    std::vector<uint8_t> output(output_length);
    
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    // OpenSSL 3.0+ API
    EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "HKDF", nullptr);
    if (!kdf) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::CRYPTO_KEY_DERIVATION_FAILED,
            "EVP_KDF_fetch failed – OpenSSL HKDF provider unavailable (fail-closed); "
            "output_length=" + std::to_string(output_length),
            "HKDFHelper::derive",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw std::runtime_error("HKDFHelper::derive: EVP_KDF_fetch failed – HKDF provider unavailable");
    }
    
    EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    
    if (!kctx) {
        throw std::runtime_error("EVP_KDF_CTX_new failed");
    }
    
    // Build params for HKDF
    OSSL_PARAM params[5];
    const char *digest = "SHA256";
    
    params[0] = OSSL_PARAM_construct_utf8_string("digest", (char*)digest, 0);
    params[1] = OSSL_PARAM_construct_octet_string("key", (void*)ikm.data(),static_cast<int>(ikm.size()));
    
    if (!salt.empty()) {
        params[2] = OSSL_PARAM_construct_octet_string("salt", (void*)salt.data(),static_cast<int>(salt.size()));
    } else {
        params[2] = OSSL_PARAM_construct_end();
    }
    
    if (!info.empty()) {
        size_t param_idx = salt.empty() ? 2 : 3;
        params[param_idx] = OSSL_PARAM_construct_octet_string("info", (void*)info.data(),static_cast<int>(info.size()));
        params[param_idx + 1] = OSSL_PARAM_construct_end();
    } else {
        size_t param_idx = salt.empty() ? 2 : 3;
        params[param_idx] = OSSL_PARAM_construct_end();
    }
    
    if (EVP_KDF_derive(kctx, output.data(),static_cast<int>(output.size()), params) <= 0) {
        EVP_KDF_CTX_free(kctx);
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::CRYPTO_KEY_DERIVATION_FAILED,
            "EVP_KDF_derive failed – key derivation unsuccessful (fail-closed); "
            "output_length=" + std::to_string(output_length) +
            "; ikm_size=" + std::to_string(ikm.size()),
            "HKDFHelper::derive",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw std::runtime_error("HKDFHelper::derive: EVP_KDF_derive failed");
    }
    
    EVP_KDF_CTX_free(kctx);
    
#else
    // OpenSSL 1.1 fallback API
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!pctx) {
        throw std::runtime_error("EVP_PKEY_CTX_new_id failed");
    }
    
    if (EVP_PKEY_derive_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("EVP_PKEY_derive_init failed");
    }
    
    if (EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("EVP_PKEY_CTX_set_hkdf_md failed");
    }
    
    if (!salt.empty()) {
        if (EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt.data(),static_cast<int>(salt.size())) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            throw std::runtime_error("EVP_PKEY_CTX_set1_hkdf_salt failed");
        }
    }
    
    if (EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm.data(),static_cast<int>(ikm.size())) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("EVP_PKEY_CTX_set1_hkdf_key failed");
    }
    
    if (!info.empty()) {
        if (EVP_PKEY_CTX_add1_hkdf_info(pctx, 
                                         reinterpret_cast<const unsigned char*>(info.data()),
                                         info.size()) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            throw std::runtime_error("EVP_PKEY_CTX_add1_hkdf_info failed");
        }
    }
    
    size_t outlen = output.size();
    if (EVP_PKEY_derive(pctx, output.data(), &outlen) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("EVP_PKEY_derive failed");
    }
    
    EVP_PKEY_CTX_free(pctx);
#endif
    
    return output;
}

std::vector<uint8_t> HKDFHelper::deriveFromString(
    const std::string& ikm_str,
    const std::string& info,
    size_t output_length) {
    
    std::vector<uint8_t> ikm(ikm_str.begin(), ikm_str.end());
    std::vector<uint8_t> salt;  // Empty salt
    
    return derive(ikm, salt, info, output_length);
}

} // namespace utils
} // namespace themis
