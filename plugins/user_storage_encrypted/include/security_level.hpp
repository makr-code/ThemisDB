/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_level.hpp                                 ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     77                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>

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
