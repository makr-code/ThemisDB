/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            access_control_manager.h                           ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     187                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include "security/rbac.h"

namespace themis {

// Forward declarations
class AuthMiddleware;

namespace security {

/// Security context for a request
struct SecurityContext {
    std::string user_id;                          // Authenticated user ID
    std::vector<std::string> roles;               // User's assigned roles
    std::vector<std::string> groups;              // User's groups (from JWT/LDAP)
    std::string session_id;                       // Session identifier
    std::string source_ip;                        // Request source IP
    std::unordered_map<std::string, std::string> attributes; // Additional context
    
    /// Check if context has a specific role
    bool hasRole(const std::string& role) const;
    
    /// Check if context has a specific group
    bool hasGroup(const std::string& group) const;
};

/// Access decision result
struct AccessDecision {
    bool granted = false;                         // Was access granted?
    std::string reason;                           // Reason for decision
    std::vector<std::string> applied_permissions; // Which permissions were applied
    
    static AccessDecision Allow(const std::string& reason = "Access granted") {
        return {true, reason, {}};
    }
    
    static AccessDecision Deny(const std::string& reason) {
        return {false, reason, {}};
    }
};

/// Access control policy configuration
struct AccessControlConfig {
    std::string rbac_config_path;                 // Path to RBAC configuration
    std::string user_role_store_path;             // Path to user-role mappings
    bool enable_audit_logging = true;             // Enable access control audit logs
    bool fail_closed = true;                      // Deny access on errors (fail-safe)
    bool enable_resource_wildcards = true;        // Allow wildcards in resources
    
    /// Custom authorization hook (optional)
    /// Can be used to implement custom authorization logic
    std::function<AccessDecision(const SecurityContext&, const std::string&, const std::string&)> 
        custom_authorizer;
};

/// Central access control manager
/// Integrates RBAC, authentication, and authorization
class AccessControlManager {
public:
    explicit AccessControlManager(const AccessControlConfig& config);
    
    /// Initialize the access control system
    /// Loads RBAC configuration and user-role mappings
    bool initialize();
    
    /// Create security context from authentication token
    /// @param token Bearer token or API key
    /// @param source_ip Request source IP address
    /// @return Security context if authentication succeeds
    std::optional<SecurityContext> authenticate(
        const std::string& token,
        const std::string& source_ip = ""
    );
    
    /// Check if user has permission to perform action on resource
    /// @param context Security context from authenticate()
    /// @param resource Resource identifier (e.g., "data", "config", "keys")
    /// @param action Action identifier (e.g., "read", "write", "delete")
    /// @return Access decision with details
    AccessDecision authorize(
        const SecurityContext& context,
        const std::string& resource,
        const std::string& action
    );
    
    /// Combined authenticate + authorize operation
    /// @param token Authentication token
    /// @param resource Resource identifier
    /// @param action Action identifier
    /// @param source_ip Request source IP (optional)
    /// @return Access decision
    AccessDecision checkAccess(
        const std::string& token,
        const std::string& resource,
        const std::string& action,
        const std::string& source_ip = ""
    );
    
    /// Assign role to user
    void assignRole(const std::string& user_id, const std::string& role);
    
    /// Revoke role from user
    void revokeRole(const std::string& user_id, const std::string& role);
    
    /// Get all roles assigned to user
    std::vector<std::string> getUserRoles(const std::string& user_id) const;
    
    /// Get all effective permissions for user
    std::vector<Permission> getUserPermissions(const std::string& user_id) const;
    
    /// Set authentication middleware (for token validation)
    void setAuthMiddleware(std::shared_ptr<AuthMiddleware> auth_middleware);
    
    /// Get RBAC instance (for advanced operations)
    std::shared_ptr<RBAC> getRBAC() const { return rbac_; }
    
    /// Get user-role store (for advanced operations)
    std::shared_ptr<UserRoleStore> getUserRoleStore() const { return user_store_; }
    
    /// Reload configuration from disk
    bool reloadConfiguration();
    
    /// Save current configuration to disk
    bool saveConfiguration();
    
    /// Get access control metrics
    struct Metrics {
        std::atomic<uint64_t> authentication_success{0};
        std::atomic<uint64_t> authentication_failure{0};
        std::atomic<uint64_t> authorization_success{0};
        std::atomic<uint64_t> authorization_failure{0};
        std::atomic<uint64_t> access_denied{0};
    };
    
    const Metrics& getMetrics() const { return metrics_; }
    
private:
    AccessControlConfig config_;
    std::shared_ptr<RBAC> rbac_;
    std::shared_ptr<UserRoleStore> user_store_;
    std::shared_ptr<AuthMiddleware> auth_middleware_;
    mutable Metrics metrics_;
    
    /// Helper: log access decision for audit
    void auditAccessDecision(
        const SecurityContext& context,
        const std::string& resource,
        const std::string& action,
        const AccessDecision& decision
    );
};

} // namespace security
} // namespace themis
