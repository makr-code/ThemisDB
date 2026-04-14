/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            user_models.hpp                                    ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:30:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "security_level.hpp"

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief User data model for encrypted storage
 */
struct User {
    std::string user_id;           // Unique user identifier
    std::string username;          // Login username
    std::string email;             // Email address (PII)
    std::string full_name;         // Full name (PII)
    std::vector<std::string> roles; // User roles
    SecurityLevel classification;  // Data classification level
    int64_t created_at_ms;         // Creation timestamp
    int64_t updated_at_ms;         // Last update timestamp
    
    User() 
        : classification(SecurityLevel::OFFEN)
        , created_at_ms(0)
        , updated_at_ms(0) 
    {}
};

/**
 * @brief Group data model for encrypted storage
 */
struct Group {
    std::string group_id;          // Unique group identifier
    std::string name;              // Group name
    std::string description;       // Group description
    std::vector<std::string> member_ids; // User IDs in group
    SecurityLevel classification;  // Data classification level
    int64_t created_at_ms;         // Creation timestamp
    
    Group() 
        : classification(SecurityLevel::OFFEN)
        , created_at_ms(0) 
    {}
};

/**
 * @brief Health status for storage containers
 */
struct HealthStatus {
    bool healthy;                  // Overall health status
    std::string message;           // Status message
    std::vector<std::string> errors; // Error details
    int64_t checked_at_ms;         // Health check timestamp
    
    HealthStatus() : healthy(true), checked_at_ms(0) {}
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
