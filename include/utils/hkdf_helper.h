/**
 * @file hkdf_helper.h
 * @brief HKDF (HMAC-based Key Derivation Function) helper for OpenSSL integration.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Last Updated: 2026-08-08
 * @note Source: Level 0 - API Contract
 * @note SOT Domain: crypto-key-management
 * 
 * @mainpage HKDF Helper API
 *
 * ## Purpose
 * HKDFHelper provides HKDF-SHA256 key derivation with OpenSSL 3.0+ and 1.1.1+
 * compatibility. It implements RFC 5869 (HKDF specification) for deterministic
 * key derivation from input key material.
 *
 * ## Key Derivation Process (Per RFC 5869)
 * ```
 * PRK = HMAC-Hash(salt, IKM)           // Extract: compress IKM
 * OKM = HKDF-Expand(PRK, info, L)      // Expand: stretch to desired length
 * ```
 *
 * ## Security Properties
 * - **Deterministic**: Same (ikm, salt, info, length) always produces same output
 * - **Pseudo-Random**: Output is indistinguishable from random to attackers
 * - **Non-Invertible**: Cannot recover IKM from output
 * - **Domain Separation**: Different info strings produce different keys
 *
 * ## Thread Safety
 * - All methods are thread-safe (no static mutable state)
 * - Each derivation is independent
 * - Suitable for multi-threaded key generation
 *
 * ## OpenSSL Compatibility
 * - OpenSSL 3.0+: Uses EVP_KDF API (modern, preferred)
 * - OpenSSL 1.1.1+: Uses EVP_PKEY_CTX API (legacy fallback)
 * - Automatic version detection at compile time
 *
 * @see HKDFCache for cached key derivation
 * @see LEKManager for daily key rotation using HKDF
 * @see RFC 5869 for HKDF specification details
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace themis {
namespace utils {

/**
 * @brief HKDF (HMAC-based Key Derivation Function) helper
 * 
 * Provides OpenSSL 3.0 compatible HKDF implementation.
 * Falls back to OpenSSL 1.1 EVP_PKEY_CTX API if 3.0 is not available.
 */
class HKDFHelper {
public:
    /**
     * @brief Derive key using HKDF-SHA256
     * 
     * @param ikm Input key material
     * @param salt Salt value (can be empty)
     * @param info Context and application specific information
     * @param output_length Desired output length in bytes
     * @return Derived key
     */
    static std::vector<uint8_t> derive(
        const std::vector<uint8_t>& ikm,
        const std::vector<uint8_t>& salt,
        const std::string& info,
        size_t output_length);
    
    /**
     * @brief Derive key using HKDF-SHA256 (simplified interface)
     * 
     * @param ikm_str Input key material as string
     * @param info Context string
     * @param output_length Desired output length in bytes
     * @return Derived key
     */
    static std::vector<uint8_t> deriveFromString(
        const std::string& ikm_str,
        const std::string& info,
        size_t output_length = 32);
};

} // namespace utils
} // namespace themis
