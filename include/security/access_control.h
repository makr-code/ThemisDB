/**
 * @file access_control.h
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
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <nlohmann/json.hpp>

#include "security/rbac.h"
#include "security/user_registration_plugin.h"
#include "server/policy_engine.h"
#include "utils/expected.h"
#include "auth/mfa_authenticator.h"

// Forward declarations
namespace themis {
    class AuthMiddleware;
    namespace auth {
        class MFAAuthenticator;
    }
    namespace utils {
        class AuditLogger;
        enum class SecurityEventType;
    }
}

namespace themis {
namespace security {

class AccessControl {
public:
    struct Config {
        // RBAC Configuration
        RBACConfig rbac_config;
        
        // Password Policy
        struct PasswordPolicy {
            int min_length = 12;
            bool require_uppercase = true;
            bool require_lowercase = true;
            bool require_digit = true;
            bool require_special = true;
            int max_age_days = 90;
            int history_count = 5; // Prevent reusing last N passwords
        } password_policy;
        
        // Session Configuration
        struct SessionConfig {
            std::chrono::seconds timeout = std::chrono::hours(8);
            std::chrono::seconds idle_timeout = std::chrono::minutes(30);
            bool require_mfa = false;
            int max_concurrent_sessions = 5;

            // Phase 2.2: per-role MFA enforcement.
            // Users whose RBAC role is in this list MUST provide a valid TOTP
            // code at login, regardless of the global require_mfa flag.
            // Default: admin and operator roles require MFA.
            // Override at runtime via THEMIS_MFA_REQUIRED_ROLES env variable
            // (comma-separated list).
            std::vector<std::string> mfa_required_roles = {"admin", "operator"};
        } session_config;
        
        // Rate Limiting
        struct RateLimitConfig {
            int max_requests_per_minute = 60;
            int max_failed_logins = 5;
            std::chrono::seconds lockout_duration = std::chrono::minutes(15);
            bool enable_adaptive_limiting = true;
        } rate_limit_config;
        
        // Threat Detection
        struct ThreatDetectionConfig {
            bool enable_anomaly_detection = true;
            bool enable_sql_injection_detection = true;
            bool enable_suspicious_query_detection = true;
            double anomaly_threshold = 0.8; // 0.0 - 1.0
        } threat_detection_config;
        
        // Audit Logging
        struct AuditConfig {
            bool enable_audit_logging = true;
            bool log_read_operations = true;
            bool log_write_operations = true;
            bool log_admin_operations = true;
            bool log_failed_access = true;
            std::string audit_log_path = "/var/log/themisdb/audit.log";
        } audit_config;
        
        // OAuth Configuration
        struct OAuthConfig {
            bool enabled = false;
            std::string provider_url;
            std::string client_id;
            std::string client_secret;
            std::vector<std::string> scopes;
        } oauth_config;
        
        // ABAC Configuration
        struct ABACConfig {
            bool enable_abac = false;
            std::string abac_policy_path;
        } abac_config;
    };
    
    struct Credentials {
        std::string user_id;
        std::string password;
        std::optional<std::string> mfa_token;
        std::optional<std::string> oauth_token;
    };
    
    struct AuthenticationResult {
        bool authenticated = false;
        std::string user_id;
        std::string session_token;
        std::vector<std::string> roles;
        bool requires_mfa = false;
        std::string error_message;
        
        /**
         * @brief Success.
         * @param[in] user_id Identifier of the user.
         * @param[in] session_token Input parameter.
         * @param[in] roles Input parameter.
         * @return Return value.
         * @details Implements Success without additional internal calls.
         */
        static AuthenticationResult Success(
            const std::string& user_id,
            const std::string& session_token,
            const std::vector<std::string>& roles
        ) {
            return {true, user_id, session_token, roles, false, ""};
        }
        
        /**
         * @brief Requires MFA.
         * @param[in] user_id Identifier of the user.
         * @return Return value.
         * @details Implements RequiresMFA without additional internal calls.
         */
        static AuthenticationResult RequiresMFA(const std::string& user_id) {
            return {false, user_id, "", {}, true, "Multi-factor authentication required"};
        }
        
        /**
         * @brief Failed.
         * @param[in] error Input parameter.
         * @return Return value.
         * @details Implements Failed without additional internal calls.
         */
        static AuthenticationResult Failed(const std::string& error) {
            return {false, "", "", {}, false, error};
        }
    };
    
    struct Session {
        std::string session_id;
        std::string user_id;
        std::vector<std::string> roles;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_access;
        bool mfa_verified = false;
        std::unordered_map<std::string, std::string> attributes;
    };
    
    struct AuthorizationContext {
        std::string user_id;
        std::vector<std::string> roles;
        std::string resource;
        std::string action;
        std::unordered_map<std::string, std::string> attributes;
        std::string ip_address;
        std::optional<std::string> user_agent; // HTTP User-Agent (used by ABAC UA conditions)
        std::chrono::system_clock::time_point timestamp;
    };
    
    /**
     * @brief Construct the access control subsystem.
     * @param[in] config Access control configuration.
     * @return Access control subsystem instance.
     */
    explicit AccessControl(const Config& config);
    
    ~AccessControl();
    
    // ========================================================================
    // Authentication
    // ========================================================================
    
    /**
     * @brief Authenticate.
     * @param[in] credentials User credentials to authenticate.
     * @return Authentication result.
     */
    AuthenticationResult authenticate(const Credentials& credentials);
    
    Result<void> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::string& plugin_name = "",
        const std::unordered_map<std::string, std::string>& attributes = {}
    );
    
    /**
     * @brief Change a user's password.
     * @param[in] user_id User identifier.
     * @param[in] old_password Current password.
     * @param[in] new_password Replacement password.
     * @return Result indicating whether the password changed.
     */
    Result<void> changePassword(
        const std::string& user_id,
        const std::string& old_password,
        const std::string& new_password
    );
    
    // NOTE: Password validation, hashing, and verification are delegated to plugins.
    // AccessControl does NOT handle password management directly.
    
    // ========================================================================
    // Multi-Factor Authentication (MFA)
    // ========================================================================
    
    /**
     * @brief Enroll multi-factor authentication for a user.
     * @param[in] user_id User identifier.
     * @return Enrollment result as JSON.
     */
    Result<nlohmann::json> enrollMFA(const std::string& user_id);
    
    /**
     * @brief Verify a multi-factor authentication token.
     * @param[in] user_id User identifier.
     * @param[in] token MFA token to verify.
     * @return True when the token is valid.
     */
    bool verifyMFA(const std::string& user_id, const std::string& token);
    
    /**
     * @brief Disable multi-factor authentication for a user.
     * @param[in] user_id User identifier.
     * @return Result indicating whether MFA was disabled.
     */
    Result<void> disableMFA(const std::string& user_id);
    
    // ========================================================================
    // Authorization
    // ========================================================================
    
    /**
     * @brief Authorize an access control context.
     * @param[in] context Authorization context to evaluate.
     * @return True when the context is authorized.
     */
    bool authorize(const AuthorizationContext& context);
    
    /**
     * @brief Check whether a role grants permission for an action.
     * @param[in] session_token Input parameter.
     * @param[in] resource Protected resource identifier.
     * @param[in] action Requested action.
     * @return True when the permission is granted.
     */
    bool checkPermission(
        const std::string& session_token,
        const std::string& resource,
        const std::string& action
    );
    
    /**
     * @brief Get the permissions assigned to a user.
     * @param[in] user_id User identifier.
     * @return Permissions assigned to the user.
     */
    std::vector<Permission> getUserPermissions(const std::string& user_id) const;
    
    // ========================================================================
    // Role Management (RBAC)
    // ========================================================================
    
    /**
     * @brief Assign a role to a user.
     * @param[in] user_id User identifier.
     * @param[in] role Role to assign.
     * @return Result indicating whether the role was assigned.
     */
    Result<void> assignRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Revoke a role from a user.
     * @param[in] user_id User identifier.
     * @param[in] role Role to revoke.
     * @return Result indicating whether the role was revoked.
     */
    Result<void> revokeRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Get the roles assigned to a user.
     * @param[in] user_id User identifier.
     * @return Roles assigned to the user.
     */
    std::vector<std::string> getUserRoles(const std::string& user_id) const;
    
    // ========================================================================
    // Session Management
    // ========================================================================
    
    std::string createSession(
        const std::string& user_id,
        const std::vector<std::string>& roles,
        bool mfa_verified = false
    );
    
    /**
     * @brief Validate a session token.
     * @param[in] session_token Session token to validate.
     * @return Validated session on success.
     */
    std::optional<Session> validateSession(const std::string& session_token);
    
    /**
     * @brief Invalidate a session token.
     * @param[in] session_token Session token to invalidate.
     */
    void invalidateSession(const std::string& session_token);
    
    /**
     * @brief Invalidate all sessions for a user.
     * @param[in] user_id User identifier.
     */
    void invalidateUserSessions(const std::string& user_id);
    
    // ========================================================================
    // Threat Detection
    // ========================================================================
    
    /**
     * @brief Check whether a user is rate limited.
     * @param[in] user_id User identifier.
     * @param[in] resource Resource being accessed.
     * @return True when the user is rate limited.
     */
    bool isRateLimited(const std::string& user_id, const std::string& resource);
    
    /**
     * @brief Detect SQL injection patterns in a query.
     * @param[in] query Query string to inspect.
     * @return True when an injection pattern is detected.
     */
    bool detectSQLInjection(const std::string& query) const;
    
    /**
     * @brief Detect suspicious query patterns.
     * @param[in] query Query string to inspect.
     * @param[in] user_id User identifier.
     * @return True when the query is suspicious.
     */
    bool detectSuspiciousQuery(const std::string& query, const std::string& user_id);
    
    /**
     * @brief Record a failed login attempt.
     * @param[in] user_id User identifier.
     * @param[in] ip_address Source IP address.
     */
    void recordFailedLogin(const std::string& user_id, const std::string& ip_address);
    
    /**
     * @brief Check whether a user is locked out.
     * @param[in] user_id User identifier.
     * @return True when the user is locked out.
     */
    bool isLockedOut(const std::string& user_id) const;
    
    // ========================================================================
    // Audit Logging
    // ========================================================================
    
    void logSecurityEvent(
        utils::SecurityEventType event_type,
        const std::string& user_id,
        const std::string& resource,
        const nlohmann::json& details = {}
    );
    
    nlohmann::json getAuditLogs(
        const std::string& user_id,
        std::optional<std::chrono::system_clock::time_point> since = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> until = std::nullopt
    ) const;
    
    // ========================================================================
    // Configuration & Administration
    // ========================================================================
    
    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const Config& config);
    
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Return the RBAC subsystem.
     * @return RBAC subsystem reference.
     * @details Implements getRBAC without additional internal calls.
     */
    RBAC& getRBAC() { return *rbac_; }
    const RBAC& getRBAC() const { return *rbac_; }
    
    /**
     * @brief Return the user role store.
     * @return Role store reference.
     * @details Implements getUserRoleStore without additional internal calls.
     */
    UserRoleStore& getUserRoleStore() { return *user_role_store_; }
    const UserRoleStore& getUserRoleStore() const { return *user_role_store_; }
    
    /**
     * @brief Return the user registration plugin manager.
     * @return Plugin manager reference.
     * @details Implements getUserRegistrationPluginManager without additional internal calls.
     */
    UserRegistrationPluginManager& getUserRegistrationPluginManager() { 
        return *user_registration_plugin_manager_; 
    }
    const UserRegistrationPluginManager& getUserRegistrationPluginManager() const { 
        return *user_registration_plugin_manager_; 
    }

    // ========================================================================
    // ABAC Policy Management
    // ========================================================================

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

private:
    Config config_;
    mutable std::mutex mutex_;
    
    // Core components
    std::unique_ptr<RBAC> rbac_;
    std::unique_ptr<UserRoleStore> user_role_store_;
    std::unique_ptr<AuthMiddleware> auth_middleware_;
    std::unique_ptr<auth::MFAAuthenticator> mfa_authenticator_;
    // In-memory MFA enrollment store: user_id → EnrollmentData (secret + recovery codes).
    // Populated by enrollMFA(); cleared by disableMFA().
    // Production: replace with encrypted persistent store (see src/security/ROADMAP.md).
    std::unordered_map<std::string, auth::MFAAuthenticator::EnrollmentData> mfa_enrollments_;
    std::unique_ptr<utils::AuditLogger> audit_logger_;
    std::unique_ptr<UserRegistrationPluginManager> user_registration_plugin_manager_;
    PolicyEngine policy_engine_;  ///< ABAC policy engine (evaluated alongside RBAC)
    
    // NOTE: ThemisDB does NOT store user passwords locally.
    // All user authentication is delegated to plugins:
    // - WebDAV plugin (Active Directory, SharePoint)
    // - Apache authentication
    // - Arrow plugin (data warehouse integration)
    // - Embedded plugin (for standalone/embedded deployments only)
    
    // Session management
    std::unordered_map<std::string, Session> sessions_;
    std::unordered_map<std::string, std::vector<std::string>> user_sessions_;
    
    // Rate limiting
    struct RateLimitEntry {
        int request_count = 0;
        std::chrono::system_clock::time_point window_start;
        int failed_login_count = 0;
        std::chrono::system_clock::time_point lockout_until;
    };
    std::unordered_map<std::string, RateLimitEntry> rate_limits_;
    
    // Statistics
    struct Statistics {
        std::atomic<uint64_t> total_authentications{0};
        std::atomic<uint64_t> successful_authentications{0};
        std::atomic<uint64_t> failed_authentications{0};
        std::atomic<uint64_t> total_authorizations{0};
        std::atomic<uint64_t> successful_authorizations{0};
        std::atomic<uint64_t> denied_authorizations{0};
        std::atomic<uint64_t> rate_limited_requests{0};
        std::atomic<uint64_t> sql_injection_attempts{0};
        std::atomic<uint64_t> suspicious_queries{0};
    } stats_;
    
    /**
     * @brief Check whether a session is expired.
     * @param[in] session Session to check.
     * @return True when the session is expired.
     */
    bool isSessionExpired(const Session& session) const;
    /**
     * @brief Remove expired sessions from the cache.
     */
    void cleanupExpiredSessions();
    /**
     * @brief Generate a new session token.
     * @return Generated session token.
     */
    std::string generateSessionToken() const;
    /**
     * @brief Update rate limit state for a user.
     * @param[in] user_id User identifier.
     */
    void updateRateLimit(const std::string& user_id);
    /**
     * @brief Check whether a user exceeds the current rate limit.
     * @param[in] user_id User identifier.
     * @return True when the user remains within the configured limit.
     */
    bool checkRateLimit(const std::string& user_id);

    /**
     * @brief Get roles for a user while holding the lock.
     * @param[in] user_id User identifier.
     * @return Roles assigned to the user.
     */
    std::vector<std::string> getUserRolesLocked(const std::string& user_id) const;
    /**
     * @brief Create a session while holding the lock.
     * @param[in] user_id User identifier.
     * @param[in] roles Roles to attach to the session.
     * @param[in] mfa_verified True if MFA was verified for the session.
     * @return Session token.
     */
    std::string createSessionLocked(const std::string& user_id,
                                    const std::vector<std::string>& roles,
                                    bool mfa_verified);
    /**
     * @brief Invalidate a session while holding the lock.
     * @param[in] session_token Session token to invalidate.
     */
    void invalidateSessionLocked(const std::string& session_token);
    /**
     * @brief Invalidate all user sessions while holding the lock.
     * @param[in] user_id User identifier.
     */
    void invalidateUserSessionsLocked(const std::string& user_id);
};

} // namespace security
} // namespace themis
