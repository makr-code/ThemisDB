/**
 * @file user_models.hpp
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

