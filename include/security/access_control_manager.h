/**
 * @file access_control_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SecurityContext {
    std::string user_id;                          // Authenticated user ID
    std::vector<std::string> roles;               // User's assigned roles
    std::vector<std::string> groups;              // User's groups (from JWT/LDAP)
    std::string session_id;                       // Session identifier
    std::string source_ip;                        // Request source IP
    std::optional<std::string> user_agent;        // HTTP User-Agent (used by ABAC policies)
    std::unordered_map<std::string, std::string> attributes; // Additional context
    
    /**
     * @brief Has Role.
     * @param[in] role Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasRole(const std::string& role) const;
    
    /**
     * @brief Has Group.
     * @param[in] group Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasGroup(const std::string& group) const;
};

struct AccessDecision {
    bool granted = false;                         // Was access granted?
    std::string reason;                           // Reason for decision
    std::vector<std::string> applied_permissions; // Which permissions were applied
    
    static AccessDecision Allow(const std::string& reason = "Access granted") {
        return {true, reason, {}};
    }
    
    /**
     * @brief Deny.
     * @param[in] reason Input parameter.
     * @return Return value.
     * @details Implements Deny without additional internal calls.
     */
    static AccessDecision Deny(const std::string& reason) {
        return {false, reason, {}};
    }
};

enum class AuthorizationFailureMode {
    DenyOnError,             // Default secure behavior: deny access on auth failures.
    AllowOnErrorExplicit     // Explicit fail-open override: requires an override reason.
};

struct AccessControlConfig {
    std::string rbac_config_path;                 // Path to RBAC configuration
    std::string user_role_store_path;             // Path to user-role mappings
    bool enable_audit_logging = true;             // Enable access control audit logs
    bool fail_closed = true;                      // Legacy compatibility flag; prefer failure_mode.
    AuthorizationFailureMode failure_mode = AuthorizationFailureMode::DenyOnError;
    std::string fail_open_reason;                 // Required when failure_mode is AllowOnErrorExplicit.
    bool enable_resource_wildcards = true;        // Allow wildcards in resources
    
    // ABAC configuration
    std::string abac_policy_path;                 // Path to ABAC policy file (JSON/YAML)
    bool enable_abac = false;                     // Enable ABAC evaluation alongside RBAC

    // Zero-trust configuration
    bool enable_zero_trust = false;               // Enable per-request zero-trust identity verification
    // RLS configuration
    std::string rls_policy_path;                  // Path to RLS policy file (JSON)
    bool enable_rls = false;                      // Enable RLS filtering of query results

    std::function<AccessDecision(const SecurityContext&, const std::string&, const std::string&)> 
        custom_authorizer;
};

class AccessControlManager {
public:
    /**
     * @brief Access Control Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AccessControlManager(const AccessControlConfig& config);
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    std::optional<SecurityContext> authenticate(
        const std::string& token,
        const std::string& source_ip = ""
    );
    
    /**
     * @brief Authorize an access control context.
     * @param[in] context Authorization context to evaluate.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @return True when the context is authorized.
     */
    AccessDecision authorize(
        const SecurityContext& context,
        const std::string& resource,
        const std::string& action
    );
    
    AccessDecision checkAccess(
        const std::string& token,
        const std::string& resource,
        const std::string& action,
        const std::string& source_ip = ""
    );
    
    /**
     * @brief Assign a role to a user.
     * @param[in] user_id User identifier.
     * @param[in] role Role to assign.
     */
    void assignRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Revoke a role from a user.
     * @param[in] user_id User identifier.
     * @param[in] role Role to revoke.
     */
    void revokeRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Get the roles assigned to a user.
     * @param[in] user_id User identifier.
     * @return Roles assigned to the user.
     */
    std::vector<std::string> getUserRoles(const std::string& user_id) const;
    
    /**
     * @brief Get the permissions assigned to a user.
     * @param[in] user_id User identifier.
     * @return Permissions assigned to the user.
     */
    std::vector<Permission> getUserPermissions(const std::string& user_id) const;
    
    /**
     * @brief Set Auth Middleware.
     * @param[in] auth_middleware Input parameter.
     */
    void setAuthMiddleware(std::shared_ptr<AuthMiddleware> auth_middleware);
    
    /**
     * @brief Set Zero Trust Enforcer.
     * @param[in,out] enforcer Input/output parameter.
     */
    void setZeroTrustEnforcer(ZeroTrustPolicyEnforcer* enforcer);
    
    std::shared_ptr<RBAC> getRBAC() const { return rbac_; }
    
    std::shared_ptr<UserRoleStore> getUserRoleStore() const { return user_store_; }
    
    /**
     * @brief Return the ABAC policy engine.
     * @return ABAC policy engine reference.
     * @details Implements getABACEngine without additional internal calls.
     */
    PolicyEngine& getABACEngine() { return policy_engine_; }
    const PolicyEngine& getABACEngine() const { return policy_engine_; }
    
    /**
     * @brief Add an ABAC policy.
     * @param[in] policy ABAC policy to add.
     */
    void addABACPolicy(const PolicyEngine::Policy& policy);
    
    /**
     * @brief Remove an ABAC policy.
     * @param[in] policy_id Identifier of the ABAC policy to remove.
     * @return True when the policy was removed.
     */
    bool removeABACPolicy(const std::string& policy_id);
    
    /**
     * @brief ── Row-level security (RLS) ─────────────────────────────────────────────
     * @param[in] policy Input parameter.
     */

    void addRLSPolicy(const RLSPolicy& policy);

    /**
     * @brief Remove RLSPolicy.
     * @param[in] policy_id Identifier of the policy.
     * @return True when the operation succeeds.
     */
    bool removeRLSPolicy(const std::string& policy_id);

    /**
     * @brief Get RLSManager.
     * @return Return value.
     * @details Implements getRLSManager without additional internal calls.
     */
    RLSManager& getRLSManager() { return rls_manager_; }
    const RLSManager& getRLSManager() const { return rls_manager_; }

    /**
     * @brief Filter Query Results.
     * @param[in] collection Input parameter.
     * @param[in] ctx Input parameter.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    nlohmann::json filterQueryResults(
        const std::string& collection,
        const SecurityContext& ctx,
        const nlohmann::json& rows
    ) const;

    /**
     * @brief Is RLSActive.
     * @param[in] collection Input parameter.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRLSActive(const std::string& collection, const SecurityContext& ctx) const;
    
    /**
     * @brief Reload Configuration.
     * @return True when the operation succeeds.
     */
    bool reloadConfiguration();
    
    /**
     * @brief Save Configuration.
     * @return True when the operation succeeds.
     */
    bool saveConfiguration();
    
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
    
    /**
     * @brief Audit Access Decision.
     * @param[in] context Input parameter.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] decision Input parameter.
     */
    void auditAccessDecision(
        const SecurityContext& context,
        const std::string& resource,
        const std::string& action,
        const AccessDecision& decision
    );
};

} // namespace security
} // namespace themis
