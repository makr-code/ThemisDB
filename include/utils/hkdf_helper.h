/**
 * @file hkdf_helper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: hkdf_helper.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
