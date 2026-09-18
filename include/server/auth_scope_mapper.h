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

namespace auth_scope_mapper {

/**
 * @brief Map Policy Role To Scope.
 * @param[in] required_role Input parameter.
 * @return Return value.
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
 * @brief Map Audit Role To Scope.
 * @param[in] required_role Input parameter.
 * @return Return value.
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
