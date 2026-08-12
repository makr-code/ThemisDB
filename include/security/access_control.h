/**
 * @file access_control.h
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

/**
 * @brief Comprehensive Access Control Framework for ThemisDB
 * 
 * This class serves as the central security coordinator, integrating:
 * - Authentication (password hashing, JWT, OAuth, MFA)
 * - Authorization (RBAC, ABAC, fine-grained permissions)
 * - Audit Logging (comprehensive security event tracking)
 * - Threat Detection (rate limiting, anomaly detection, SQL injection prevention)
 * - Session Management
 * 
 * Compliance: SOC 2, GDPR, HIPAA, PCI-DSS
 * Security: Defense-in-depth, principle of least privilege
 */
class AccessControl {
public:
    /**
     * @brief Configuration for the access control system
     */
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
    
    /**
     * @brief User credentials for authentication
     */
    struct Credentials {
        std::string user_id;
        std::string password;
        std::optional<std::string> mfa_token;
        std::optional<std::string> oauth_token;
    };
    
    /**
     * @brief Authentication result
     */
    struct AuthenticationResult {
        bool authenticated = false;
        std::string user_id;
        std::string session_token;
        std::vector<std::string> roles;
        bool requires_mfa = false;
        std::string error_message;
        
        static AuthenticationResult Success(
            const std::string& user_id,
            const std::string& session_token,
            const std::vector<std::string>& roles
        ) {
            return {true, user_id, session_token, roles, false, ""};
        }
        
        static AuthenticationResult RequiresMFA(const std::string& user_id) {
            return {false, user_id, "", {}, true, "Multi-factor authentication required"};
        }
        
        static AuthenticationResult Failed(const std::string& error) {
            return {false, "", "", {}, false, error};
        }
    };
    
    /**
     * @brief Session information
     */
    struct Session {
        std::string session_id;
        std::string user_id;
        std::vector<std::string> roles;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_access;
        bool mfa_verified = false;
        std::unordered_map<std::string, std::string> attributes;
    };
    
    /**
     * @brief Authorization context for fine-grained access control
     */
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
     * @brief Constructor
     */
    explicit AccessControl(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~AccessControl();
    
    // ========================================================================
    // Authentication
    // ========================================================================
    
    /**
     * @brief Authenticate user with credentials
     * @param credentials User credentials (password, MFA token, OAuth token)
     * @return Authentication result with session token if successful
     */
    AuthenticationResult authenticate(const Credentials& credentials);
    
    /**
     * @brief Register new user with password
     * 
     * User registration is delegated to plugins (Apache Arrow, WebDAV).
     * If no plugin is specified, uses the default available plugin.
     * 
     * @param user_id User identifier
     * @param password User password
     * @param plugin_name Optional plugin name ("arrow", "webdav", or empty for default)
     * @param attributes Optional user attributes for plugin
     * @return Result indicating success or error
     */
    Result<void> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::string& plugin_name = "",
        const std::unordered_map<std::string, std::string>& attributes = {}
    );
    
    /**
     * @brief Change user password
     * @param user_id User identifier
     * @param old_password Current password
     * @param new_password New password
     * @return Result indicating success or error
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
     * @brief Enroll user in MFA
     * @param user_id User identifier
     * @return MFA secret and QR code URI for setup
     */
    Result<nlohmann::json> enrollMFA(const std::string& user_id);
    
    /**
     * @brief Verify MFA token
     * @param user_id User identifier
     * @param token MFA token (TOTP code)
     * @return true if token is valid
     */
    bool verifyMFA(const std::string& user_id, const std::string& token);
    
    /**
     * @brief Disable MFA for user
     * @param user_id User identifier
     * @return Result indicating success or error
     */
    Result<void> disableMFA(const std::string& user_id);
    
    // ========================================================================
    // Authorization
    // ========================================================================
    
    /**
     * @brief Check if user has permission to perform action on resource
     * @param context Authorization context
     * @return true if authorized
     */
    bool authorize(const AuthorizationContext& context);
    
    /**
     * @brief Check permission using session token
     * @param session_token Session token
     * @param resource Resource identifier
     * @param action Action identifier
     * @return true if authorized
     */
    bool checkPermission(
        const std::string& session_token,
        const std::string& resource,
        const std::string& action
    );
    
    /**
     * @brief Get user permissions
     * @param user_id User identifier
     * @return List of permissions
     */
    std::vector<Permission> getUserPermissions(const std::string& user_id) const;
    
    // ========================================================================
    // Role Management (RBAC)
    // ========================================================================
    
    /**
     * @brief Assign role to user
     * @param user_id User identifier
     * @param role Role name
     * @return Result indicating success or error
     */
    Result<void> assignRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Revoke role from user
     * @param user_id User identifier
     * @param role Role name
     * @return Result indicating success or error
     */
    Result<void> revokeRole(const std::string& user_id, const std::string& role);
    
    /**
     * @brief Get user roles
     * @param user_id User identifier
     * @return List of role names
     */
    std::vector<std::string> getUserRoles(const std::string& user_id) const;
    
    // ========================================================================
    // Session Management
    // ========================================================================
    
    /**
     * @brief Create new session for user
     * @param user_id User identifier
     * @param roles User roles
     * @param mfa_verified Whether MFA was verified
     * @return Session token
     */
    std::string createSession(
        const std::string& user_id,
        const std::vector<std::string>& roles,
        bool mfa_verified = false
    );
    
    /**
     * @brief Validate session token
     * @param session_token Session token
     * @return Session if valid, nullopt otherwise
     */
    std::optional<Session> validateSession(const std::string& session_token);
    
    /**
     * @brief Invalidate session
     * @param session_token Session token
     */
    void invalidateSession(const std::string& session_token);
    
    /**
     * @brief Invalidate all sessions for user
     * @param user_id User identifier
     */
    void invalidateUserSessions(const std::string& user_id);
    
    // ========================================================================
    // Threat Detection
    // ========================================================================
    
    /**
     * @brief Check if request should be rate limited
     * @param user_id User identifier
     * @param resource Resource being accessed
     * @return true if rate limit exceeded
     */
    bool isRateLimited(const std::string& user_id, const std::string& resource);
    
    /**
     * @brief Detect SQL injection attempt in query
     * @param query SQL query
     * @return true if injection detected
     */
    bool detectSQLInjection(const std::string& query) const;
    
    /**
     * @brief Detect suspicious query patterns
     * @param query SQL query
     * @param user_id User identifier
     * @return true if suspicious
     */
    bool detectSuspiciousQuery(const std::string& query, const std::string& user_id);
    
    /**
     * @brief Record failed login attempt
     * @param user_id User identifier
     * @param ip_address IP address
     */
    void recordFailedLogin(const std::string& user_id, const std::string& ip_address);
    
    /**
     * @brief Check if user is locked out due to failed attempts
     * @param user_id User identifier
     * @return true if locked out
     */
    bool isLockedOut(const std::string& user_id) const;
    
    // ========================================================================
    // Audit Logging
    // ========================================================================
    
    /**
     * @brief Log security event
     * @param event_type Type of security event
     * @param user_id User identifier
     * @param resource Resource accessed
     * @param details Additional details
     */
    void logSecurityEvent(
        utils::SecurityEventType event_type,
        const std::string& user_id,
        const std::string& resource,
        const nlohmann::json& details = {}
    );
    
    /**
     * @brief Get audit logs for user
     * @param user_id User identifier
     * @param since Optional start time
     * @param until Optional end time
     * @return Audit logs as JSON array
     */
    nlohmann::json getAuditLogs(
        const std::string& user_id,
        std::optional<std::chrono::system_clock::time_point> since = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> until = std::nullopt
    ) const;
    
    // ========================================================================
    // Configuration & Administration
    // ========================================================================
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void updateConfig(const Config& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Get security statistics
     * @return Statistics as JSON
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Get RBAC system
     * @return RBAC instance
     */
    RBAC& getRBAC() { return *rbac_; }
    const RBAC& getRBAC() const { return *rbac_; }
    
    /**
     * @brief Get user role store
     * @return UserRoleStore instance
     */
    UserRoleStore& getUserRoleStore() { return *user_role_store_; }
    const UserRoleStore& getUserRoleStore() const { return *user_role_store_; }
    
    /**
     * @brief Get user registration plugin manager
     * @return UserRegistrationPluginManager instance
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
     * @brief Get the ABAC policy engine
     * @return PolicyEngine instance
     * @note PolicyEngine is internally thread-safe via its own mutex.
     *       Do not modify the engine concurrently with ongoing authorization calls
     *       unless relying solely on PolicyEngine's own synchronization.
     */
    PolicyEngine& getABACEngine() { return policy_engine_; }
    const PolicyEngine& getABACEngine() const { return policy_engine_; }

    /**
     * @brief Add an ABAC policy at runtime
     * @param policy PolicyEngine policy to add
     * @note Thread-safe: PolicyEngine serialises all policy mutations internally.
     */
    void addABACPolicy(const PolicyEngine::Policy& policy);

    /**
     * @brief Remove an ABAC policy by id
     * @param policy_id Policy identifier
     * @return true if the policy was removed
     * @note Thread-safe: PolicyEngine serialises all policy mutations internally.
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
    
    // Helper methods
    bool isSessionExpired(const Session& session) const;
    void cleanupExpiredSessions();
    std::string generateSessionToken() const;
    void updateRateLimit(const std::string& user_id);
    bool checkRateLimit(const std::string& user_id);

    // Mutex-free variants for internal use when mutex_ is already held by the caller.
    std::vector<std::string> getUserRolesLocked(const std::string& user_id) const;
    std::string createSessionLocked(const std::string& user_id,
                                    const std::vector<std::string>& roles,
                                    bool mfa_verified);
    void invalidateSessionLocked(const std::string& session_token);
    void invalidateUserSessionsLocked(const std::string& user_id);
};

} // namespace security
} // namespace themis
