/**
 * @file access_control_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include "security/rbac.h"
#include "security/zero_trust_policy_enforcer.h"
#include "security/row_level_security.h"
#include "server/policy_engine.h"

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
    std::optional<std::string> user_agent;        // HTTP User-Agent (used by ABAC policies)
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
    
    // ABAC configuration
    std::string abac_policy_path;                 // Path to ABAC policy file (JSON/YAML)
    bool enable_abac = false;                     // Enable ABAC evaluation alongside RBAC

    // Zero-trust configuration
    bool enable_zero_trust = false;               // Enable per-request zero-trust identity verification
    // RLS configuration
    std::string rls_policy_path;                  // Path to RLS policy file (JSON)
    bool enable_rls = false;                      // Enable RLS filtering of query results

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
    
    /// Set zero-trust policy enforcer for per-request identity verification.
    /// When set (and enable_zero_trust is true in config), checkAccess() runs
    /// zero-trust verification between authentication and RBAC/ABAC evaluation.
    /// Pass nullptr to disable. The manager does NOT take ownership.
    void setZeroTrustEnforcer(ZeroTrustPolicyEnforcer* enforcer);
    
    /// Get RBAC instance (for advanced operations)
    std::shared_ptr<RBAC> getRBAC() const { return rbac_; }
    
    /// Get user-role store (for advanced operations)
    std::shared_ptr<UserRoleStore> getUserRoleStore() const { return user_store_; }
    
    /// Get ABAC policy engine (for advanced operations)
    PolicyEngine& getABACEngine() { return policy_engine_; }
    const PolicyEngine& getABACEngine() const { return policy_engine_; }
    
    /// Add an ABAC policy at runtime
    void addABACPolicy(const PolicyEngine::Policy& policy);
    
    /// Remove an ABAC policy by id
    bool removeABACPolicy(const std::string& policy_id);
    
    // ── Row-level security (RLS) ─────────────────────────────────────────────

    /// Register an RLS policy.
    /// Policies are keyed by policy.id; an existing policy with the same id is replaced.
    void addRLSPolicy(const RLSPolicy& policy);

    /// Remove an RLS policy by id.
    /// @return true if the policy existed and was removed.
    bool removeRLSPolicy(const std::string& policy_id);

    /// Access the underlying RLS manager (for advanced operations).
    RLSManager& getRLSManager() { return rls_manager_; }
    const RLSManager& getRLSManager() const { return rls_manager_; }

    /// Filter a JSON array of query-result rows through applicable RLS policies.
    ///
    /// When RLS is active for the collection/user combination, rows that do not
    /// satisfy any matching policy predicate are silently excluded.  If no RLS
    /// policies apply the array is returned unchanged.
    ///
    /// @param collection  Name of the queried collection.
    /// @param ctx         Security context of the requesting user.
    /// @param rows        JSON array returned by the query engine.
    /// @return            Filtered JSON array (subset of @p rows visible to the user).
    nlohmann::json filterQueryResults(
        const std::string& collection,
        const SecurityContext& ctx,
        const nlohmann::json& rows
    ) const;

    /// Returns true when at least one enabled RLS policy matches the collection
    /// and the security context.
    bool isRLSActive(const std::string& collection, const SecurityContext& ctx) const;
    
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
    ZeroTrustPolicyEnforcer* zero_trust_enforcer_ = nullptr; ///< Non-owning; may be nullptr.
    mutable Metrics metrics_;
    PolicyEngine policy_engine_;    ///< ABAC policy engine (evaluated alongside RBAC)
    RLSManager rls_manager_;        ///< Row-level security policy registry
    
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
