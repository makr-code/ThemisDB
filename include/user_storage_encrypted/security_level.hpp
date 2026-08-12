/**
 * @file security_level.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <stdexcept>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Security classification levels matching ThemisDB governance system
 * 
 * Maps to German Federal classification (Verschlusssache):
 * - offen: Public data, no encryption required
 * - vs-nfd: Verschlusssache - Nur für den Dienstgebrauch (Restricted)
 * - geheim: Secret, no ANN/export/cache allowed
 * - streng-geheim: Top Secret, HSM-backed encryption
 */
enum class SecurityLevel {
    OFFEN = 0,           // Public, no encryption
    VS_NFD = 1,          // Restricted, AES-256-GCM with Vault
    GEHEIM = 2,          // Secret, AES-256-GCM with Vault, no ANN
    STRENG_GEHEIM = 3    // Top Secret, AES-256-GCM with HSM
};

/**
 * @brief Convert SecurityLevel to string
 */
inline std::string securityLevelToString(SecurityLevel level) {
    switch (level) {
        case SecurityLevel::OFFEN: return "offen";
        case SecurityLevel::VS_NFD: return "vs-nfd";
        case SecurityLevel::GEHEIM: return "geheim";
        case SecurityLevel::STRENG_GEHEIM: return "streng-geheim";
        default: return "unknown";
    }
}

/**
 * @brief Parse SecurityLevel from string
 */
inline SecurityLevel stringToSecurityLevel(const std::string& str) {
    if (str == "offen") return SecurityLevel::OFFEN;
    if (str == "vs-nfd") return SecurityLevel::VS_NFD;
    if (str == "geheim") return SecurityLevel::GEHEIM;
    if (str == "streng-geheim") return SecurityLevel::STRENG_GEHEIM;
    throw std::invalid_argument("Invalid security level: " + str);
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
