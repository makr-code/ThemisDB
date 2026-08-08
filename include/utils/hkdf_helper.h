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
     * @brief Derive key using HKDF-SHA256 with full RFC 5869 parameters.
     * 
     * Implements HKDF-SHA256 key derivation per RFC 5869. The derivation
     * process is deterministic: identical inputs always produce identical outputs.
     * 
     * Process:
     * 1. **Extract phase**: Compress IKM using HMAC-SHA256(salt, IKM) → PRK
     * 2. **Expand phase**: Stretch PRK to desired length using info context → OKM
     * 
     * Parameters:
     * - **ikm** (Input Key Material): Cryptographic secret material
     *   - Can be output from another KDF, random entropy, or master key
     *   - Should be at least 16 bytes; longer is better (e.g., 32 bytes)
     * 
     * - **salt**: Random value to prevent rainbow table attacks
     *   - Optional (can be empty), but strongly recommended
     *   - Should be at least 16 bytes if provided (per RFC 5869)
     *   - Different salts with same IKM produce different outputs
     * 
     * - **info**: Application-specific domain separation string
     *   - Example: "encryption.session.v1", "auth.token.refresh"
     *   - Different info with same (ikm, salt) produces different outputs
     *   - Allows same key material for multiple purposes
     * 
     * - **output_length**: Desired output key length in bytes
     *   - Valid range: 1 to 255*32 (8160 bytes max for SHA256)
     *   - Typical values: 16 (AES-128), 32 (AES-256), 64 (SHA-512 equivalent)
     * 
     * @param ikm Input Key Material – must be cryptographically strong
     * @param salt Salt value; at least 16 bytes recommended, empty is allowed
     * @param info Application-specific context string; can be empty
     * @param output_length Desired output length in bytes
     * 
     * @return Derived key material (OKM) of length output_length
     * 
     * @throws std::invalid_argument if output_length exceeds RFC 5869 limit
     * @throws std::runtime_error if OpenSSL HKDF fails
     * 
     * @note Thread-Safe: Fully thread-safe, stateless operation
     * @note Deterministic: Same inputs → same output
     * @note Memory-Safe: Return value must be cleaned up by caller
     * 
     * @see RFC 5869 for specification
     * @see HKDFCache for cached key derivation (better performance)
     * 
     * @example
     * @code{.cpp}
     * // Derive session key from master key
     * std::vector<uint8_t> master_key = ...;
     * std::vector<uint8_t> salt = generateRandomBytes(32);
     * std::string info = "auth.session.v1";
     * 
     * auto session_key = HKDFHelper::derive(master_key, salt, info, 32);
     * // Use session_key...
     * OPENSSL_cleanse(session_key.data(), session_key.size());
     * @endcode
     */
    static std::vector<uint8_t> derive(
        const std::vector<uint8_t>& ikm,
        const std::vector<uint8_t>& salt,
        const std::string& info,
        size_t output_length);
    
    /**
     * @brief Derive key using HKDF-SHA256 (simplified string-based interface).
     * 
     * Convenience overload for string-based IKM input. Internally converts
     * the string to bytes and calls derive() with empty salt.
     * 
     * Useful for:
     * - Deriving keys from string passwords/passphrases
     * - Quick prototyping
     * - Scenarios where IKM is naturally string-typed
     * 
     * @param ikm_str Input Key Material as string
     * @param info Application-specific context string
     * @param output_length Desired output length (default: 32 bytes for AES-256)
     * 
     * @return Derived key material
     * 
     * @note Empty salt: Uses empty vector for salt (less secure than random salt)
     * @warning For production: Use derive() with random salt instead
     * 
     * @example
     * @code
     * // Simple example: derive from password
     * auto key = HKDFHelper::deriveFromString(
     *     "my-secret-passphrase",
     *     "encryption.data.v1",
     *     32
     * );
     * @endcode
     */
    static std::vector<uint8_t> deriveFromString(
        const std::string& ikm_str,
        const std::string& info,
        size_t output_length = 32);
};

} // namespace utils
} // namespace themis
