/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hkdf_helper.h                                      ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
