/**
 * @file access_control.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/access_control.h"
#include "security/user_registration_plugin.h"
#include "server/auth_middleware.h"
#include "auth/mfa_authenticator.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "utils/tracing.h"

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace themis {
namespace security {

// ============================================================================
// Constructor & Destructor
// ============================================================================

AccessControl::AccessControl(const Config& config)
    : config_(config)
    , rbac_(std::make_unique<RBAC>(config.rbac_config))
    , user_role_store_(std::make_unique<UserRoleStore>())
    , auth_middleware_(std::make_unique<AuthMiddleware>())
    , mfa_authenticator_(std::make_unique<auth::MFAAuthenticator>())
    , audit_logger_(std::make_unique<utils::AuditLogger>(
        nullptr, 
        nullptr, 
        utils::AuditLoggerConfig{
            .enabled = config.audit_config.enable_audit_logging,
            .encrypt_then_sign = false,
            .log_path = config.audit_config.audit_log_path,
            .key_id = "access_control",
            .enable_hash_chain = false,
            .splunk_token = {},
            .siem_ca_bundle_path = {},
            .secondary_log_path = {}
        }
    ))
    , user_registration_plugin_manager_(std::make_unique<UserRegistrationPluginManager>())
{
    THEMIS_INFO("Initializing Access Control Framework");
    
    // Configure audit logger
    if (config_.audit_config.enable_audit_logging) {
        THEMIS_INFO("Audit logging enabled: {}", config_.audit_config.audit_log_path);
    }
    
    // Load ABAC policies if configured
    if (config_.abac_config.enable_abac && !config_.abac_config.abac_policy_path.empty()) {
        std::string err = {};
        if (!policy_engine_.loadFromFile(config_.abac_config.abac_policy_path, &err)) {
            THEMIS_WARN("Failed to load ABAC policies from {}: {}",
                config_.abac_config.abac_policy_path, err);
        } else {
            THEMIS_INFO("Loaded ABAC policies from {}", config_.abac_config.abac_policy_path);
        }
    }
    
    // Log initialization
    logSecurityEvent(
        utils::SecurityEventType::SERVER_STARTED,
        "system",
        "access_control",
        {{"message", "Access Control Framework initialized"}}
    );
    
    THEMIS_INFO("User registration will be handled via plugins (Apache Arrow, WebDAV)");

    // Phase 2.2: apply THEMIS_MFA_REQUIRED_ROLES env override if set.
    // Format: comma-separated role names, e.g. "admin,operator,superuser".
    if (const char* env_roles = std::getenv("THEMIS_MFA_REQUIRED_ROLES")) {
        std::vector<std::string> roles;
        std::istringstream ss(env_roles);
        std::string token = {};
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) {
              roles.push_back(token);
            }
        }
        if (!roles.empty()) {
            config_.session_config.mfa_required_roles = std::move(roles);
            THEMIS_INFO("AccessControl: THEMIS_MFA_REQUIRED_ROLES override applied ({} roles)",
                        config_.session_config.mfa_required_roles.size());
        }
    }
}

AccessControl::~AccessControl() {
    THEMIS_INFO("Shutting down Access Control Framework");
    
    logSecurityEvent(
        utils::SecurityEventType::SERVER_STOPPED,
        "system",
        "access_control",
        {{"message", "Access Control Framework shutdown"}}
    );
}

// ============================================================================
// Authentication
// ============================================================================

AccessControl::AuthenticationResult AccessControl::authenticate(const Credentials& credentials) {
    TracedSpan span("AccessControl.authenticate");
    span.setAttribute("security.user_id", credentials.user_id);
    span.setAttribute("security.auth_type", credentials.oauth_token.has_value() ? "oauth" : "password"); // NOPII: value is a literal enum-string, not the token
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.total_authentications++;
    
    // Check if user is locked out
    if (isLockedOut(credentials.user_id)) {
        stats_.failed_authentications++;
        logSecurityEvent(
            utils::SecurityEventType::LOGIN_FAILED,
            credentials.user_id,
            "authentication",
            {{"reason", "Account locked due to failed login attempts"}}
        );
        span.setStatus(false, "Account locked");
        return AuthenticationResult::Failed("Account locked. Please try again later.");
    }
    
    // OAuth authentication
    if (config_.oauth_config.enabled && credentials.oauth_token.has_value()) {
        // OAuth token validation would be handled by auth_middleware_
        auto result = auth_middleware_->validateToken(credentials.oauth_token.value());
        if (result.authorized) {
            stats_.successful_authentications++;
            auto roles = getUserRolesLocked(result.user_id);
            auto session_token = createSessionLocked(result.user_id, roles, false);
            
            logSecurityEvent(
                utils::SecurityEventType::LOGIN_SUCCESS,
                result.user_id,
                "authentication",
                {{"method", "oauth"}}
            );
            
            return AuthenticationResult::Success(result.user_id, session_token, roles);
        }
    }
    
    // Delegate password authentication to plugin
    // ThemisDB does NOT store passwords - all authentication is plugin-based
    auto plugin = user_registration_plugin_manager_->getDefaultPlugin();
    if (!plugin) {
        stats_.failed_authentications++;
        logSecurityEvent(
            utils::SecurityEventType::LOGIN_FAILED,
            credentials.user_id,
            "authentication",
            {{"reason", "No authentication plugin available"}}
        );
        return AuthenticationResult::Failed("Authentication system not configured");
    }
    
    // Authenticate via plugin (WebDAV, Apache, Arrow, or embedded)
    auto auth_result = plugin->authenticateUser(credentials.user_id, credentials.password);
    if (!auth_result.has_value()) {
        stats_.failed_authentications++;
        recordFailedLogin(credentials.user_id, "unknown");
        logSecurityEvent(
            utils::SecurityEventType::LOGIN_FAILED,
            credentials.user_id,
            "authentication",
            {{"reason", "Authentication failed"}, {"plugin", plugin->getName()}}
        );
        return AuthenticationResult::Failed("Invalid credentials");
    }
    
    // Check if MFA is required — Phase 2.2: also check per-role requirements.
    // The global require_mfa flag OR the role being in mfa_required_roles both
    // trigger mandatory MFA.  At this point we have the user's roles from the
    // auth_result.
    {
        auto& user_data_local = auth_result.value();
        const auto& required_roles = config_.session_config.mfa_required_roles;

        bool role_requires_mfa = false;
        if (!required_roles.empty()) {
            for (const auto& role : user_data_local.roles) {
                for (const auto& req_role : required_roles) {
                    if (role == req_role) {
                        role_requires_mfa = true;
                        break;
                    }
                }
                if (role_requires_mfa) {
                  break;
                }
            }
        }

        bool mfa_needed = config_.session_config.require_mfa || role_requires_mfa;

        if (mfa_needed) {
            if (!credentials.mfa_token.has_value()) {
                if (role_requires_mfa) {
                    logSecurityEvent(
                        utils::SecurityEventType::LOGIN_FAILED,
                        credentials.user_id,
                        "authentication",
                        {{"reason", "MFA required for privileged role"},
                         {"event", "MFA_SKIPPED_ADMIN"}}
                    );
                }
                return AuthenticationResult::RequiresMFA(credentials.user_id);
            }

            if (!verifyMFA(credentials.user_id, credentials.mfa_token.value())) {
                stats_.failed_authentications++;
                logSecurityEvent(
                    utils::SecurityEventType::MFA_TOTP_FAILED,
                    credentials.user_id,
                    "authentication",
                    {{"reason", "Invalid MFA token"}}
                );
                return AuthenticationResult::Failed("Invalid MFA token");
            }
        }
    }
    
    // Authentication successful
    stats_.successful_authentications++;
    auto user_data = auth_result.value();
    auto roles = user_data.roles;
    auto session_token = createSessionLocked(credentials.user_id, roles, credentials.mfa_token.has_value());
    
    logSecurityEvent(
        utils::SecurityEventType::LOGIN_SUCCESS,
        credentials.user_id,
        "authentication",
        {{"method", "plugin"}, {"plugin", plugin->getName()}, {"mfa", credentials.mfa_token.has_value()}}
    );
    
    span.setStatus(true);
    return AuthenticationResult::Success(credentials.user_id, session_token, roles);
}

Result<void> AccessControl::registerUser(
    const std::string& user_id,
    const std::string& password,
    const std::string& plugin_name,
    const std::unordered_map<std::string, std::string>& attributes
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Delegate registration to plugin
    // ThemisDB does NOT store user credentials locally
    auto plugin_result = user_registration_plugin_manager_->registerUser(
        plugin_name,
        user_id,
        password,
        attributes
    );
    
    if (!plugin_result.has_value()) {
        return themis::ErrVoid(
            themis::errors::ErrorCode::ERR_API_UNAUTHORIZED,
            "User registration via plugin failed: " + plugin_result.error().message()
        );
    }
    
    // Get registration data from plugin
    auto reg_data = plugin_result.value();
    
    // Assign roles from plugin
    for (const auto& role : reg_data.roles) {
        user_role_store_->assignRole(user_id, role);
    }
    
    logSecurityEvent(
        utils::SecurityEventType::TOKEN_CREATED,
        user_id,
        "user_management",
        {
            {"action", "User registered"},
            {"source", reg_data.source},
            {"plugin", plugin_name.empty() ? "default" : plugin_name}
        }
    );
    
    THEMIS_INFO("User registered via plugin '{}': {}", reg_data.source, user_id);
    return themis::OkVoid();
}

Result<void> AccessControl::changePassword(
    const std::string& user_id,
    const std::string& old_password,
    const std::string& /*new_password*/
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Password changes are delegated to plugins
    // ThemisDB does NOT store passwords locally
    
    // For embedded plugin, we need to cast to access changePassword method
    auto plugin = user_registration_plugin_manager_->getDefaultPlugin();
    if (!plugin) {
        return themis::ErrVoid(
            themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "No authentication plugin available"
        );
    }
    
    // First verify old password
    auto auth_result = plugin->authenticateUser(user_id, old_password);
    if (!auth_result.has_value()) {
        return themis::ErrVoid(
            themis::errors::ErrorCode::ERR_API_UNAUTHORIZED,
            "Invalid old password"
        );
    }
    
    // Note: Only the embedded plugin supports password changes directly.
    // For WebDAV/Apache, users must change passwords through their respective systems.
    if (plugin->getName() != "embedded") {
        return themis::ErrVoid(
            themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
            "Password changes not supported for " + plugin->getName() + 
            " plugin. Please use your identity provider's password change process."
        );
    }
    
    // For embedded plugin, attempt to change password
    // This requires knowledge of plugin-specific API
    THEMIS_WARN("Password change for embedded plugin - consider using external identity provider"); // NOPII: static advisory string, no PII value
    
    // Invalidate all existing sessions
    invalidateUserSessionsLocked(user_id);
    
    logSecurityEvent(
        utils::SecurityEventType::CONFIG_CHANGED,
        user_id,
        "user_management",
        {{"action", "Password change requested"}, {"plugin", plugin->getName()}}
    );
    
    return themis::ErrVoid(
        themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
        "Password changes must be done through the plugin's management interface. "
        "For embedded plugin, use plugin API directly. "
        "For WebDAV/Apache, use your identity provider."
    );
}

// ============================================================================
// Multi-Factor Authentication
// ============================================================================

Result<nlohmann::json> AccessControl::enrollMFA(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // [A-3] Guard: prevent silent overwrite of an active MFA enrollment.
    // An attacker who can call enrollMFA() with an arbitrary user_id must not be
    // able to replace a live TOTP secret without first disabling the existing
    // enrollment. Production callers are expected to enforce RBAC / session checks
    // before reaching this point; this check is a defence-in-depth backstop.
    auto existing_it = mfa_enrollments_.find(user_id);
    if (existing_it != mfa_enrollments_.end() && existing_it->second.enabled) {
        logSecurityEvent(
            utils::SecurityEventType::MFA_ENROLLED,
            user_id,
            "mfa",
            {{"action", "MFA enrollment rejected — existing enrollment active"}}
        );
        return themis::Err<nlohmann::json>(
            themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
            "MFA is already enrolled for this user. Disable existing MFA before re-enrolling."
        );
    }

    auto enrollment = mfa_authenticator_->generateEnrollment(user_id);
    auto uri = mfa_authenticator_->generateProvisioningURI(enrollment);
    
    nlohmann::json result = {
        {"user_id", user_id},
        {"secret", enrollment.secret_base32},
        {"qr_code_uri", uri},
        {"recovery_codes", enrollment.recovery_codes}
    };

    // Persist enrollment so verifyMFA() can look up the secret.
    enrollment.enabled = true;
    mfa_enrollments_[user_id] = std::move(enrollment);

    logSecurityEvent(
        utils::SecurityEventType::MFA_ENROLLED,
        user_id,
        "mfa",
        {{"action", "MFA enrollment"}}
    );
    
    THEMIS_INFO("MFA enrolled for user: {}", user_id);
    return themis::Ok(std::move(result));
}

bool AccessControl::verifyMFA(const std::string& user_id, const std::string& token) {
    if (token.empty()) {
        return false;
    }

    auto it = mfa_enrollments_.find(user_id);
    if (it == mfa_enrollments_.end() || !it->second.enabled) {
        THEMIS_WARN("verifyMFA: no active MFA enrollment found for user '{}'", user_id);
        return false;
    }

    const auto& enrollment = it->second;

    // Primary path: TOTP code validation.
    if (mfa_authenticator_->validateTOTP(enrollment.secret_base32, token,
                                          std::nullopt, user_id)) {
        return true;
    }

    // Fallback path: single-use recovery code.
    auto& mutable_enrollment = it->second;
    if (mfa_authenticator_->validateRecoveryCode(mutable_enrollment, token)) {
        THEMIS_INFO("verifyMFA: recovery code used for user '{}'", user_id);
        return true;
    }

    return false;
}

Result<void> AccessControl::disableMFA(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto erased = mfa_enrollments_.erase(user_id);
    if (erased == 0) {
        THEMIS_WARN("disableMFA: no active MFA enrollment found for user '{}'", user_id);
    }

    logSecurityEvent(
        utils::SecurityEventType::MFA_DISABLED,
        user_id,
        "mfa",
        {{"action", "MFA disabled"}}
    );

    THEMIS_INFO("MFA disabled for user: {}", user_id);
    return themis::OkVoid();
}

// ============================================================================
// Authorization
// ============================================================================

bool AccessControl::authorize(const AuthorizationContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.total_authorizations++;
    
    // Check if user is rate limited
    if (isRateLimited(context.user_id, context.resource)) {
        stats_.rate_limited_requests++;
        stats_.denied_authorizations++;
        
        logSecurityEvent(
            utils::SecurityEventType::RATE_LIMIT_EXCEEDED,
            context.user_id,
            context.resource,
            {{"action", context.action}, {"ip", context.ip_address}}
        );
        
        return false;
    }
    
    // RBAC check
    bool authorized = rbac_->checkPermission(context.roles, context.resource, context.action);
    
    if (authorized && config_.abac_config.enable_abac) {
        // ABAC check: evaluate policies alongside RBAC
        std::optional<std::string> client_ip = context.ip_address.empty()
            ? std::nullopt
            : std::make_optional(context.ip_address);
        
        auto abac_decision = policy_engine_.authorize(
            context.user_id, context.action, context.resource,
            client_ip, context.user_agent);
        
        if (!abac_decision.allowed) {
            authorized = false;
            
            logSecurityEvent(
                utils::SecurityEventType::PERMISSION_DENIED,
                context.user_id,
                context.resource,
                {{"action", context.action}, {"reason", "ABAC policy denied"},
                 {"policy_id", abac_decision.policy_id}}
            );
            
            stats_.denied_authorizations++;
            return false;
        }
    }
    
    if (authorized) {
        stats_.successful_authorizations++;
        
        if (config_.audit_config.log_read_operations || 
            config_.audit_config.log_write_operations) {
            logSecurityEvent(
                utils::SecurityEventType::DATA_READ,
                context.user_id,
                context.resource,
                {{"action", context.action}}
            );
        }
    } else {
        stats_.denied_authorizations++;
        
        if (config_.audit_config.log_failed_access) {
            logSecurityEvent(
                utils::SecurityEventType::PERMISSION_DENIED,
                context.user_id,
                context.resource,
                {{"action", context.action}, {"roles", context.roles}}
            );
        }
    }
    
    return authorized;
}

bool AccessControl::checkPermission(
    const std::string& session_token,
    const std::string& resource,
    const std::string& action
) {
    auto session_opt = validateSession(session_token);
    if (!session_opt.has_value()) {
        return false;
    }
    
    auto& session = session_opt.value();
    
    AuthorizationContext context;
    context.user_id = session.user_id;
    context.roles = session.roles;
    context.resource = resource;
    context.action = action;
    context.timestamp = std::chrono::system_clock::now();
    
    return authorize(context);
}

std::vector<Permission> AccessControl::getUserPermissions(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto roles = user_role_store_->getUserRoles(user_id);
    return rbac_->getUserPermissions(roles);
}

// ============================================================================
// Role Management
// ============================================================================

Result<void> AccessControl::assignRole(const std::string& user_id, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if role exists
    auto role_opt = rbac_->getRole(role);
    if (!role_opt.has_value()) {
        return themis::ErrVoid(
            themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
            "Role not found: " + role
        );
    }
    
    user_role_store_->assignRole(user_id, role);
    
    logSecurityEvent(
        utils::SecurityEventType::ROLE_CHANGED,
        user_id,
        "role_management",
        {{"action", "Role assigned"}, {"role", role}}
    );
    
    THEMIS_INFO("Role '{}' assigned to user '{}'", role, user_id);
    return themis::OkVoid();
}

Result<void> AccessControl::revokeRole(const std::string& user_id, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    user_role_store_->revokeRole(user_id, role);
    
    logSecurityEvent(
        utils::SecurityEventType::ROLE_CHANGED,
        user_id,
        "role_management",
        {{"action", "Role revoked"}, {"role", role}}
    );
    
    THEMIS_INFO("Role '{}' revoked from user '{}'", role, user_id);
    return themis::OkVoid();
}

std::vector<std::string> AccessControl::getUserRolesLocked(const std::string& user_id) const {
    return user_role_store_->getUserRoles(user_id);
}

std::vector<std::string> AccessControl::getUserRoles(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getUserRolesLocked(user_id);
}

// ============================================================================
// ABAC Policy Management
// ============================================================================

void AccessControl::addABACPolicy(const PolicyEngine::Policy& policy) {
    policy_engine_.addPolicy(policy);
    THEMIS_INFO("Added ABAC policy '{}' to AccessControl", policy.id);
}

bool AccessControl::removeABACPolicy(const std::string& policy_id) {
    bool removed = policy_engine_.removePolicy(policy_id);
    if (removed) {
        THEMIS_INFO("Removed ABAC policy '{}' from AccessControl", policy_id);
    }
    return removed;
}

// ============================================================================
// Session Management
// ============================================================================

std::string AccessControl::createSessionLocked(
    const std::string& user_id,
    const std::vector<std::string>& roles,
    bool mfa_verified
) {
    // Generate session token
    auto session_token = generateSessionToken();

    Session session;
    session.session_id = session_token;
    session.user_id = user_id;
    session.roles = roles;
    session.created_at = std::chrono::system_clock::now();
    session.last_access = session.created_at;
    session.mfa_verified = mfa_verified;

    sessions_[session_token] = session;
    user_sessions_[user_id].push_back(session_token);

    // Check max concurrent sessions
    auto& user_sess = user_sessions_[user_id];
    if (static_cast<int>(user_sess.size()) > static_cast<size_t>(config_.session_config.max_concurrent_sessions)) {
        // Remove oldest session without re-acquiring the mutex.
        invalidateSessionLocked(user_sess.front());
        user_sess.erase(user_sess.begin());
    }

    THEMIS_DEBUG("Session created for user: {}", user_id);
    return session_token;
}

std::string AccessControl::createSession(
    const std::string& user_id,
    const std::vector<std::string>& roles,
    bool mfa_verified
) {
    std::lock_guard<std::mutex> lock(mutex_);
    return createSessionLocked(user_id, roles, mfa_verified);
}

std::optional<AccessControl::Session> AccessControl::validateSession(const std::string& session_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(session_token);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    
    auto& session = it->second;
    
    // Check if session is expired
    if (isSessionExpired(session)) {
        sessions_.erase(it);
        return std::nullopt;
    }
    
    // Update last access time
    session.last_access = std::chrono::system_clock::now();
    
    return session;
}

void AccessControl::invalidateSessionLocked(const std::string& session_token) {
    auto it = sessions_.find(session_token);
    if (it != sessions_.end()) {
        auto user_id = it->second.user_id;
        sessions_.erase(it);

        auto& user_sess = user_sessions_[user_id];
        user_sess.erase(
            std::remove(user_sess.begin(), user_sess.end(), session_token),
            user_sess.end()
        );

        logSecurityEvent(
            utils::SecurityEventType::LOGOUT,
            user_id,
            "session",
            {{"action", "Session invalidated"}}
        );

        THEMIS_DEBUG("Session invalidated: {}", session_token);
    }
}

void AccessControl::invalidateSession(const std::string& session_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    invalidateSessionLocked(session_token);
}

void AccessControl::invalidateUserSessionsLocked(const std::string& user_id) {
    auto it = user_sessions_.find(user_id);
    if (it != user_sessions_.end()) {
        for (const auto& session_token : it->second) {
            sessions_.erase(session_token);
        }
        user_sessions_.erase(it);

        logSecurityEvent(
            utils::SecurityEventType::LOGOUT,
            user_id,
            "session",
            {{"action", "All sessions invalidated"}}
        );

        THEMIS_INFO("All sessions invalidated for user: {}", user_id);
    }
}

void AccessControl::invalidateUserSessions(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    invalidateUserSessionsLocked(user_id);
}

// ============================================================================
// Threat Detection
// ============================================================================

bool AccessControl::isRateLimited(const std::string& user_id, [[maybe_unused]] const std::string& resource) {
    // unused for now
    return !checkRateLimit(user_id);
}

bool AccessControl::detectSQLInjection(const std::string& query) const {
    if (!config_.threat_detection_config.enable_sql_injection_detection) {
        return false;
    }

    // SECURITY NOTE: This is a heuristic/defense-in-depth layer, NOT a substitute
    // for parameterized queries. Always use parameterized queries as the primary
    // defense against SQL injection.

    // Case-fold for detection — prevents trivial bypass via lowercase/mixed-case input
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    static const std::array<std::string_view, 17> kSqlPatterns = {
        "union", "select", "insert", "update", "delete", "drop", "exec",
        "execute", "xp_", "--", "/*", "*/", ";", "or 1=1", "' or '",
        "1=1", "1 = 1"
    };
    for (const auto& pat : kSqlPatterns) {
        if (lower_query.find(pat) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool AccessControl::detectSuspiciousQuery(const std::string& query, const std::string& user_id) {
    if (!config_.threat_detection_config.enable_suspicious_query_detection) {
        return false;
    }
    
    // Detect SQL injection
    if (detectSQLInjection(query)) {
        stats_.sql_injection_attempts++;
        
        logSecurityEvent(
            utils::SecurityEventType::SUSPICIOUS_ACTIVITY,
            user_id,
            "query",
            {{"type", "sql_injection"}, {"query", query}}
        );
        
        return true;
    }
    
    // Check for excessively long queries
    if (query.length() > 10000) {
        stats_.suspicious_queries++;
        
        logSecurityEvent(
            utils::SecurityEventType::SUSPICIOUS_ACTIVITY,
            user_id,
            "query",
            {{"type", "oversized_query"}, {"length", query.length()}}
        );
        
        return true;
    }
    
    return false;
}

void AccessControl::recordFailedLogin(const std::string& user_id, [[maybe_unused]] const std::string& ip_address) {
    // SECURITY NOTE: Lockout state is in-memory only. On process restart, counters reset.
    // Persistent lockout requires an external store (Redis, DB). Log lockout events for SIEM.
    auto& entry = rate_limits_[user_id];
    entry.failed_login_count++;
    
    if (entry.failed_login_count >= config_.rate_limit_config.max_failed_logins) {
        entry.lockout_until = std::chrono::system_clock::now() + 
                              config_.rate_limit_config.lockout_duration;
        
        logSecurityEvent(
            utils::SecurityEventType::BRUTE_FORCE_DETECTED,
            user_id,
            "authentication",
            {{"failed_attempts", entry.failed_login_count}}
        );
        
        THEMIS_WARN("[SECURITY] Account lockout activated for user '{}' after {} failed attempts. "
                    "Note: lockout state is not persistent across restarts.", user_id, entry.failed_login_count);
    }
}

bool AccessControl::isLockedOut(const std::string& user_id) const {
    auto it = rate_limits_.find(user_id);
    if (it == rate_limits_.end()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return now < it->second.lockout_until;
}

// ============================================================================
// Audit Logging
// ============================================================================

void AccessControl::logSecurityEvent(
    utils::SecurityEventType event_type,
    const std::string& user_id,
    const std::string& resource,
    const nlohmann::json& details
) {
    if (!config_.audit_config.enable_audit_logging) {
        return;
    }
    
    // Log to audit logger
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(event_type, user_id, resource, details);
    }
}

nlohmann::json AccessControl::getAuditLogs(
    const std::string& user_id,
    std::optional<std::chrono::system_clock::time_point> since,
    std::optional<std::chrono::system_clock::time_point> until
) const {
    auto result = nlohmann::json::array();

    if (!audit_logger_) {
        return result;
    }

    const auto entries = audit_logger_->enumerateEntries();
    for (const auto& entry : entries) {
        // Apply time-range filters
        if (since.has_value() && entry.timestamp < *since) {
            continue;
        }
        if (until.has_value() && entry.timestamp > *until) {
            continue;
        }
        // Filter by user_id when provided (empty means "all users")
        if (!user_id.empty()) {
            const auto& rec = entry.record;
            if (!rec.contains("user_id") ||
                rec["user_id"].get<std::string>() != user_id) {
                continue;
            }
        }
        result.push_back(entry.record);
    }

    return result;
}

// ============================================================================
// Configuration & Administration
// ============================================================================

void AccessControl::updateConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    logSecurityEvent(
        utils::SecurityEventType::CONFIG_CHANGED,
        "system",
        "access_control",
        {{"action", "Configuration updated"}}
    );
    
    THEMIS_INFO("Access Control configuration updated");
}

nlohmann::json AccessControl::getStatistics() const {
    return {
        {"total_authentications", stats_.total_authentications.load()},
        {"successful_authentications", stats_.successful_authentications.load()},
        {"failed_authentications", stats_.failed_authentications.load()},
        {"total_authorizations", stats_.total_authorizations.load()},
        {"successful_authorizations", stats_.successful_authorizations.load()},
        {"denied_authorizations", stats_.denied_authorizations.load()},
        {"rate_limited_requests", stats_.rate_limited_requests.load()},
        {"sql_injection_attempts", stats_.sql_injection_attempts.load()},
        {"suspicious_queries", stats_.suspicious_queries.load()},
        {"active_sessions", sessions_.size()}
    };
}

// ============================================================================
// Helper Methods
// ============================================================================

bool AccessControl::isSessionExpired(const Session& session) const {
    auto now = std::chrono::system_clock::now();
    
    // Check absolute timeout
    auto age = now - session.created_at;
    if (age > config_.session_config.timeout) {
        return true;
    }
    
    // Check idle timeout
    auto idle = now - session.last_access;
    if (idle > config_.session_config.idle_timeout) {
        return true;
    }
    
    return false;
}

void AccessControl::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> expired_sessions = {};

    expired_sessions.reserve(sessions_.size());
    
    for (const auto& [token, session] : sessions_) {
        if (isSessionExpired(session)) {
            expired_sessions.push_back(token);
        }
    }
    
    for (const auto& token : expired_sessions) {
        invalidateSessionLocked(token);
    }
}

std::string AccessControl::generateSessionToken() const {
    // Generate random token
    unsigned char buffer[32];
    RAND_bytes(buffer, sizeof(buffer));
    
    std::stringstream ss = {};
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    
    return ss.str();
}

void AccessControl::updateRateLimit(const std::string& user_id) {
    auto now = std::chrono::system_clock::now();
    auto& entry = rate_limits_[user_id];
    
    // Reset window if expired
    if (now - entry.window_start > std::chrono::minutes(1)) {
        entry.window_start = now;
        entry.request_count = 0;
    }
    
    entry.request_count++;
}

bool AccessControl::checkRateLimit(const std::string& user_id) {
    auto now = std::chrono::system_clock::now();
    auto& entry = rate_limits_[user_id];
    
    // Reset window if expired
    if (now - entry.window_start > std::chrono::minutes(1)) {
        entry.window_start = now;
        entry.request_count = 0;
    }
    
    return entry.request_count < config_.rate_limit_config.max_requests_per_minute;
}

} // namespace security
} // namespace themis

