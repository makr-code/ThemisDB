/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_level.hpp                                 ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:10:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
