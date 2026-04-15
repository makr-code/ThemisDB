/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_scope_mapper.h                                ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
