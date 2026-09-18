/**
 * @file auth_scope_mapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Header-Only Utilities**: Provides inline utility functions for auth scope mapping.
 *       No .cpp implementation needed. Functions are inline for performance and simplicity.
 */


#pragma once

#include <string>

namespace themis {
namespace server {

/**
 * @brief Utility functions for mapping roles to authorization scopes
 * 
 * Provides consistent role-to-scope mapping across API handlers to reduce
 * code duplication and ensure consistent authorization behavior.
 */
namespace auth_scope_mapper {

/**
 * @brief Map a role to a policy scope
 * 
 * Maps standard roles to policy scopes:
 * - "admin" -> "policy:write"
 * - "operator" -> "policy:read"
 * - other -> "policy:<role>"
 * 
 * @param required_role The role name (e.g., "admin", "operator")
 * @return The corresponding policy scope
 * @details Implements mapPolicyRoleToScope without additional internal calls.
 */
inline std::string mapPolicyRoleToScope(const std::string& required_role) {
    if (required_role == "admin") {
        return "policy:write";
    } else if (required_role == "operator") {
        return "policy:read";
    } else {
        return "policy:" + required_role;
    }
}

/**
 * @brief Map a role to an audit scope
 * 
 * Maps standard roles to audit scopes:
 * - "admin" -> "audit:write"
 * - "operator" -> "audit:read"
 * - other -> "audit:<role>"
 * 
 * @param required_role The role name (e.g., "admin", "operator")
 * @return The corresponding audit scope
 * @details Implements mapAuditRoleToScope without additional internal calls.
 */
inline std::string mapAuditRoleToScope(const std::string& required_role) {
    if (required_role == "admin") {
        return "audit:write";
    } else if (required_role == "operator") {
        return "audit:read";
    } else {
        return "audit:" + required_role;
    }
}

} // namespace auth_scope_mapper
} // namespace server
} // namespace themis
