/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_middleware.cpp                                ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     353                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/auth_middleware.h"
#include "auth/jwt_validator.h"
#include "auth/gssapi_authenticator.h"
#include "security/usb_admin_authenticator.h"
#include "utils/logger.h"
#include <sstream>

namespace themis {

AuthMiddleware::AuthMiddleware() = default;
AuthMiddleware::~AuthMiddleware() = default;

void AuthMiddleware::enableJWT(const JWTConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auth::JWTValidatorConfig jwt_cfg;
    jwt_cfg.jwks_url = config.jwks_url;
    jwt_cfg.expected_issuer = config.expected_issuer;
    jwt_cfg.expected_audience = config.expected_audience;
    jwt_cfg.cache_ttl = config.jwks_cache_ttl;
    jwt_cfg.clock_skew = config.clock_skew;
    
    jwt_validator_ = std::make_unique<auth::JWTValidator>(jwt_cfg);
    jwt_config_ = config;
    jwt_enabled_ = true;
    
    THEMIS_INFO("JWT validation enabled: issuer='{}', audience='{}', scope_claim='{}'",
                config.expected_issuer, config.expected_audience, config.scope_claim);
}

void AuthMiddleware::enableKerberos(const auth::KerberosConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    kerberos_auth_ = std::make_unique<auth::GSSAPIAuthenticator>();
    
    if (!kerberos_auth_->initialize(config)) {
        THEMIS_ERROR("Failed to initialize Kerberos authentication");
        kerberos_auth_.reset();
        return;
    }
    
    kerberos_enabled_ = true;
    
    THEMIS_INFO("Kerberos/GSSAPI authentication enabled: service_principal='{}', fallback={}",
                config.service_principal, config.fallback_to_basic);
}

void AuthMiddleware::enableUSBAdminAuth(const std::string& mount_path, const std::vector<std::string>& protected_scopes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    security::USBAdminConfig usb_cfg;
    usb_cfg.mount_path = mount_path;
    usb_cfg.require_usb_for_admin = true;
    usb_cfg.silent_failure = true;
    
    // Use provided scopes or defaults from config
    if (!protected_scopes.empty()) {
        usb_cfg.usb_protected_scopes = protected_scopes;
    }
    // else: use defaults from USBAdminConfig
    
    usb_admin_auth_ = std::make_unique<security::USBAdminAuthenticator>(usb_cfg);
    usb_admin_auth_->initialize();
    usb_admin_enabled_ = true;
    usb_protected_scopes_ = usb_cfg.usb_protected_scopes;
    
    THEMIS_INFO("USB Admin Authentication enabled with mount_path='{}', {} protected scopes", 
                mount_path, usb_protected_scopes_.size());
}

void AuthMiddleware::addToken(const TokenConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_[config.token] = config;
    THEMIS_INFO("Added API token for user '{}' with {} scopes", config.user_id, config.scopes.size());
}

void AuthMiddleware::removeToken(std::string_view token) {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_.erase(std::string(token));
}

void AuthMiddleware::clearTokens() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_.clear();
}

AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Mask token for logging (show first/last 4 chars)
    auto mask = [](std::string_view t) {
        std::string s(t);
        if (s.size() <= 8) return s;
        return s.substr(0,4) + "..." + s.substr(s.size()-4);
    };
    THEMIS_INFO("AuthMiddleware::authorize called for token='{}' required_scope='{}'", mask(token), required_scope);
    
    // First try API token lookup
    auto it = tokens_.find(std::string(token));
    if (it != tokens_.end()) {
        const auto& config = it->second;
        // Build scopes string for diagnostics
        std::string scopes_list;
        for (const auto& s : config.scopes) {
            if (!scopes_list.empty()) scopes_list += ",";
            scopes_list += s;
        }
        THEMIS_INFO("Auth token matched for user='{}' tenant='{}' scopes='{}'", 
                    config.user_id, config.tenant_id, scopes_list);
        
        // Check if token has required scope
        if (config.scopes.count(std::string(required_scope)) == 0) {
            metrics_.authz_denied_total++;
            std::ostringstream oss;
            oss << "Missing required scope: " << required_scope;
            THEMIS_WARN("Authorization denied for user '{}': {}", config.user_id, oss.str());
            return AuthResult::Denied(oss.str());
        }
        
        // If USB admin authentication is enabled and this is an admin scope, check USB
        if (usb_admin_enabled_ && isAdminScope(required_scope)) {
            if (!usb_admin_auth_->validateAdminOperation(std::string(required_scope), config.user_id)) {
                metrics_.authz_denied_total++;
                // Silent failure mode - return generic denial without revealing USB requirement
                return AuthResult::Denied("Insufficient privileges");
            }
        }

        metrics_.authz_success_total++;
        return AuthResult::OK(config.user_id, config.tenant_id);
    }
    
    // If JWT is enabled, try JWT validation as fallback
    if (jwt_enabled_) {
        return authorizeViaJWT(token, required_scope);
    }
    
    // If Kerberos is enabled, try Kerberos authentication
    if (kerberos_enabled_) {
        return authorizeViaKerberos(token, required_scope);
    }
    
    // No match found
    metrics_.authz_invalid_token_total++;
    return AuthResult::Denied("Invalid or missing token");
}

AuthMiddleware::AuthResult AuthMiddleware::authorizeViaJWT(std::string_view token, std::string_view required_scope) const {
    (void)required_scope;
    // Note: mutex is already locked by caller (authorize)
    
    if (!jwt_validator_) {
        return AuthResult::Denied("JWT validation not configured");
    }
    
    try {
        // Parse and validate JWT
        auto claims = jwt_validator_->parseAndValidate(std::string(token));
        
        metrics_.jwt_validation_success_total++;
        
        // Extract scopes from configured claim (e.g., "roles", "groups")
        std::unordered_set<std::string> scopes;
        
        // Check if claim exists and is array
        // For now, we'll use a simple approach: derive scope from user_id if no scope claim
        // In production, you'd parse claims.roles or claims.groups properly
        
        // Simple mapping: if user has valid JWT, grant basic access
        // TODO: Enhance with proper scope extraction from JWT claims
        
        // For now: check if required_scope is in a hardcoded allowed list or derive from sub
        // Better: parse jwt_config_.scope_claim from the JWT payload
        
        // Placeholder: grant access if JWT is valid (you should enhance this)
        THEMIS_INFO("JWT validated for user '{}' (sub: {}), tenant='{}', groups: {}", 
                    claims.email, claims.sub, claims.tenant_id, claims.groups.size());
        
        metrics_.authz_success_total++;
        return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);  // Pass user_id, tenant_id, and groups from JWT
        
    } catch (const std::exception& e) {
        metrics_.jwt_validation_failed_total++;
        THEMIS_WARN("JWT validation failed: {}", e.what());
        return AuthResult::Denied(std::string("JWT validation failed: ") + e.what());
    }
}

AuthMiddleware::AuthResult AuthMiddleware::validateToken(std::string_view token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Try API token first
    auto it = tokens_.find(std::string(token));
    if (it != tokens_.end()) {
        return AuthResult::OK(it->second.user_id, it->second.tenant_id);
    }
    
    // Try JWT validation
    if (jwt_enabled_ && jwt_validator_) {
        try {
            auto claims = jwt_validator_->parseAndValidate(std::string(token));
            metrics_.jwt_validation_success_total++;
            return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);
        } catch (const std::exception& e) {
            metrics_.jwt_validation_failed_total++;
            THEMIS_DEBUG("JWT validation failed during validateToken: {}", e.what());
        }
    }
    
    metrics_.authz_invalid_token_total++;
    return AuthResult::Denied("Invalid token");
}

bool AuthMiddleware::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !tokens_.empty() || jwt_enabled_ || kerberos_enabled_;
}

std::optional<std::string> AuthMiddleware::extractBearerToken(std::string_view auth_header) {
    // Expected format: "Bearer <token>"
    constexpr std::string_view prefix = "Bearer ";
    
    if (auth_header.size() <= prefix.size()) {
        return std::nullopt;
    }

    if (auth_header.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }

    std::string token(auth_header.substr(prefix.size()));
    
    // Trim whitespace
    auto start = token.find_first_not_of(" \t");
    auto end = token.find_last_not_of(" \t");
    
    if (start == std::string::npos) {
        return std::nullopt;
    }

    return token.substr(start, end - start + 1);
}

std::optional<AuthMiddleware::AuthContext> AuthMiddleware::extractContext(std::string_view token) const {
    // Do not lock validateToken again; it already locks internally.
    auto res = validateToken(token);
    if (res.authorized) {
        AuthContext ctx;
        ctx.user_id = std::move(res.user_id);
        ctx.tenant_id = std::move(res.tenant_id);
        ctx.groups = std::move(res.groups);
        return ctx;
    }
    return std::nullopt;
}

bool AuthMiddleware::isUSBAdminReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!usb_admin_enabled_ || !usb_admin_auth_) {
        return false;
    }
    return usb_admin_auth_->isAdminUSBPresent();
}

bool AuthMiddleware::isAdminScope(std::string_view scope) const {
    // Check if scope is in the configured list of USB-protected scopes
    // Note: mutex already locked by caller (authorize method)
    for (const auto& protected_scope : usb_protected_scopes_) {
        if (protected_scope == scope) {
            return true;
        }
        // Support wildcard matching for "admin:*" pattern
        if (protected_scope == "admin:*" && scope.find("admin:") == 0) {
            return true;
        }
    }
    return false;
}

AuthMiddleware::AuthResult AuthMiddleware::authorizeViaKerberos(
    std::string_view token,
    std::string_view required_scope) const {
    
    (void)required_scope;  // For now, Kerberos auth grants access if principal is valid
    
    // Note: mutex is already locked by caller (authorize)
    
    if (!kerberos_auth_) {
        return AuthResult::Denied("Kerberos authentication not configured");
    }
    
    try {
        // Authenticate the Kerberos token
        auto result = kerberos_auth_->authenticateToken(std::string(token));
        
        if (!result.success) {
            THEMIS_WARN("Kerberos authentication failed: {}", result.error_message);
            return AuthResult::Denied("Kerberos authentication failed: " + result.error_message);
        }
        
        // Build roles string manually (fmt::join not available in fmt 11.0.2)
        std::string roles_str;
        for (size_t i = 0; i < result.roles.size(); ++i) {
            if (i > 0) roles_str += ", ";
            roles_str += result.roles[i];
        }
        THEMIS_INFO("Kerberos authentication successful for principal '{}' with roles: [{}]",
                   result.principal_name, roles_str);
        
        // TODO: Check if any of the roles provide the required_scope
        // For now, we grant access if authentication succeeds
        // 
        // IMPORTANT: Kerberos tickets do not include tenant information.
        // Clients using Kerberos authentication MUST provide tenant_id via:
        // - X-Tenant-ID header
        // - Path parameter (/tenants/{tenant_id}/...)
        // The tenant_id will be extracted from the request in the API handler.
        
        metrics_.authz_success_total++;
        return AuthResult::OK(result.principal_name, "", {});  // Empty tenant_id - must be provided via header
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Kerberos authentication error: {}", e.what());
        return AuthResult::Denied(std::string("Kerberos authentication error: ") + e.what());
    }
}

} // namespace themis
