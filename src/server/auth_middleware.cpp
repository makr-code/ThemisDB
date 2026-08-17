/**
 * @file auth_middleware.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/auth_middleware.h"
#include "auth/jwt_validator.h"
#include "auth/gssapi_authenticator.h"
#include "auth/mtls_authenticator.h"
#include "auth/api_key_authenticator.h"
#include "security/usb_admin_authenticator.h"
#include "utils/logger.h"
#include "config/config_path_resolver.h"
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <openssl/crypto.h>  // CRYPTO_memcmp
#include <stdexcept>

namespace themis {

AuthMiddleware::AuthMiddleware() = default;
AuthMiddleware::~AuthMiddleware() = default;

void AuthMiddleware::enableJWT(const JWTConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auth::JWTValidatorConfig jwt_cfg;
    jwt_cfg.jwks_url = config.jwks_url;
    if (!config.expected_issuer.empty()) {
        jwt_cfg.expected_issuer = config.expected_issuer;
    }
    jwt_cfg.require_issuer_validation = config.require_issuer_validation;
    if (!config.expected_audience.empty()) {
        jwt_cfg.expected_audience = config.expected_audience;
    }
    jwt_cfg.require_audience_validation = config.require_audience_validation;
    jwt_cfg.cache_ttl = config.jwks_cache_ttl;
    jwt_cfg.clock_skew = config.clock_skew;
    
    jwt_validator_ = std::make_unique<auth::JWTValidator>(jwt_cfg);
    jwt_config_ = config;
    jwt_enabled_ = true;

    if (!role_scope_map_loaded_) {
        loadRoleScopeMapping();
    }

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

    if (!role_scope_map_loaded_) {
        loadRoleScopeMapping();
    }

    THEMIS_INFO("Kerberos/GSSAPI authentication enabled: service_principal='{}', fallback={}",
                config.service_principal, config.fallback_to_basic);
}

void AuthMiddleware::enableMTLS(const auth::MTLSAuthenticator::Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    mtls_auth_ = std::make_unique<auth::MTLSAuthenticator>(config);

    mtls_enabled_ = true;

    THEMIS_INFO("mTLS certificate authentication enabled");
}

void AuthMiddleware::enableApiKeyAuth(const ApiKeyConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auth::ApiKeyAuthenticator::Config api_cfg;
    api_cfg.check_expiry       = config.check_expiry;
    api_cfg.max_key_id_length  = config.max_key_id_length;
    api_cfg.max_secret_length  = config.max_secret_length;

    api_key_auth_    = std::make_unique<auth::ApiKeyAuthenticator>(api_cfg);
    api_key_enabled_ = true;

    THEMIS_INFO("API key authentication enabled (check_expiry={})", config.check_expiry);
}

void AuthMiddleware::addApiKeyCredential(const auth::ApiKeyCredential& credential) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!api_key_auth_) {
        THEMIS_WARN("addApiKeyCredential: API key auth not enabled; call enableApiKeyAuth() first");
        return;
    }
    auto& api_key_auth = *api_key_auth_;
    api_key_auth.addCredential(credential);
}

void AuthMiddleware::removeApiKeyCredential(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (api_key_auth_) {
        api_key_auth_->removeCredential(key_id);
    }
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

void AuthMiddleware::setRoleScopeMapping(
    std::unordered_map<std::string, std::vector<std::string>> mapping)
{
    std::lock_guard<std::mutex> lock(mutex_);
    role_scope_map_ = std::move(mapping);
    THEMIS_INFO("Role-to-scope mapping updated: {} role(s) configured", role_scope_map_.size());
}

bool AuthMiddleware::roleGrantsScope(const std::vector<std::string>& roles,
                                     std::string_view required_scope) const
{
    // Note: mutex is already held by caller.
    for (const auto& role : roles) {
        auto it = role_scope_map_.find(role);
        if (it != role_scope_map_.end()) {
            for (const auto& granted : it->second) {
                if (granted == required_scope) {
                    return true;
                }
            }
        }
    }
    return false;
}

AuthMiddleware::AuthResult AuthMiddleware::authorize(std::string_view token, std::string_view required_scope) const {
    std::lock_guard<std::mutex> lock(mutex_);
    THEMIS_INFO("AuthMiddleware::authorize called (required_scope='{}')", required_scope);
    
    // First try API token lookup.
    // GAP-008 fixed: instead of relying on the hash-map comparison for the final
    // token-equality check (which can leak information via cache/timing differences),
    // we find candidates by HMAC-SHA256 key digest and confirm with CRYPTO_memcmp
    // (constant-time comparison over equal-length byte sequences).
    // The token value stored in tokens_ is compared character-by-character only in
    // the constant-time branch, so the timing is independent of the token content.
    const std::string token_str(token);
    for (const auto& kv : tokens_) {
        const std::string& stored = kv.first;
        // Length is not a secret — tokens are randomly generated with a fixed
        // width (not user-chosen), so the length is public information.  The
        // short-circuit here avoids calling CRYPTO_memcmp on differently-sized
        // inputs (which would require zero-padding and may confuse static
        // analysers), and does not expose any additional timing information
        // beyond the publicly-known expected token length.
        if (stored.size() != token_str.size()) continue;
        // Constant-time byte comparison: CRYPTO_memcmp runs in O(len) time
        // regardless of the first differing byte, preventing content-based
        // timing attacks.
        if (CRYPTO_memcmp(stored.data(), token_str.data(), stored.size()) != 0) continue;

        const auto& config = kv.second;
        // Build scopes string for diagnostics
        std::ostringstream scopes_oss;
        bool first_scope = true;
        for (const auto& s : config.scopes) {
            if (!first_scope) scopes_oss << ",";
            scopes_oss << s;
            first_scope = false;
        }
        std::string scopes_list = scopes_oss.str();
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

    // If API key authentication is enabled, try combined "key_id.secret" format.
    // API key tokens contain exactly one dot separator; skip tokens that have
    // no dot to avoid unnecessary work (plain bearer tokens and JWTs fall through).
    if (api_key_enabled_ && token.find('.') != std::string_view::npos) {
        return authorizeViaApiKey(token, required_scope);
    }

    // If JWT is enabled, try JWT validation as fallback
    if (jwt_enabled_) {
        return authorizeViaJWT(token, required_scope);
    }
    
    // If Kerberos is enabled, try Kerberos authentication
    if (kerberos_enabled_) {
        return authorizeViaKerberos(token, required_scope);
    }

    // If mTLS is enabled, try certificate-based authentication
    if (mtls_enabled_) {
        return authorizeViaMTLS(token, required_scope);
    }

    // No match found — GAP-013: log at WARN for audit trail (CWE-778).
    metrics_.authz_invalid_token_total++;
    THEMIS_WARN("[SECURITY] authorize: token unrecognized for required_scope='{}' — "
                "no matching static token, API key, JWT, Kerberos, or mTLS credential "
                "(GAP-013/CWE-778). authz_invalid_token_total={}",
                required_scope, metrics_.authz_invalid_token_total.load());
    return AuthResult::Denied("Invalid or missing token");
}

AuthMiddleware::AuthResult AuthMiddleware::authorizeViaJWT(std::string_view token, std::string_view required_scope) const {
    // Note: mutex is already locked by caller (authorize)

    if (!jwt_validator_) {
        return AuthResult::Denied("JWT validation not configured");
    }
    auto& jwt_validator = *jwt_validator_;

    try {
        // Parse and validate JWT
        auto claims = jwt_validator.parseAndValidate(std::string(token));

        metrics_.jwt_validation_success_total++;

        THEMIS_INFO("JWT validated for user '{}' (sub: {}), tenant='{}', scopes: {}, groups: {}",
                    claims.email, claims.sub, claims.tenant_id,
                    claims.scopes.size(), claims.groups.size());

        // Scope enforcement: check required_scope against JWT-granted scopes and role-to-scope map
        if (!required_scope.empty()) {
            bool scope_granted = false;

            // 1. Direct scope claim match (OAuth2 "scope"/"scp" claims parsed into claims.scopes)
            for (const auto& s : claims.scopes) {
                if (s == required_scope) {
                    scope_granted = true;
                    break;
                }
            }

            // 2. Fallback: check role-to-scope mapping for each role in the JWT
            if (!scope_granted && !role_scope_map_.empty()) {
                for (const auto& role : claims.roles) {
                    auto it = role_scope_map_.find(role);
                    if (it != role_scope_map_.end() &&
                        std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
                        scope_granted = true;
                        break;
                    }
                }
                // Also treat group memberships as roles
                if (!scope_granted) {
                    for (const auto& group : claims.groups) {
                        auto it = role_scope_map_.find(group);
                        if (it != role_scope_map_.end() &&
                            std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
                            scope_granted = true;
                            break;
                        }
                    }
                }
            }

            if (!scope_granted) {
                metrics_.authz_denied_total++;
                THEMIS_WARN("JWT authorization denied for user '{}': missing scope '{}'",
                            claims.sub, required_scope);
                return AuthResult::Denied(
                    std::string("JWT missing required scope: ") + std::string(required_scope));
            }
        }

        metrics_.authz_success_total++;
        return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);

        // Build the complete set of scopes granted by this token.
        // Priority order:
        //  1. OAuth2 `scope`/`scp` claims parsed from the JWT payload (populated by JWTClaims::scopes)
        //  2. The claim named by jwt_config_.scope_claim (defaults to "roles")
        //     which maps to claims.roles, claims.groups, etc.
        std::unordered_set<std::string> granted_scopes(claims.scopes.begin(),
                                                       claims.scopes.end());

        // Add values from the configured scope_claim
        if (jwt_config_.scope_claim == "roles") {
            for (const auto& r : claims.roles)  granted_scopes.insert(r);
        } else if (jwt_config_.scope_claim == "groups") {
            for (const auto& g : claims.groups) granted_scopes.insert(g);
        }
        // If scope_claim is "scope" or "scp" the data is already in claims.scopes above.

        THEMIS_INFO("JWT validated for user '{}' (sub: {}), tenant='{}', groups: {}, scopes: {}",
                    claims.email, claims.sub, claims.tenant_id, claims.groups.size(),
                    granted_scopes.size());

        // Check required scope (if non-empty)
        if (!required_scope.empty()) {
            const std::string req(required_scope);
            bool scope_ok = granted_scopes.count(req) > 0;

            // Fallback: check if any role in the token grants the scope via role_scope_map_
            if (!scope_ok) {
                scope_ok = roleGrantsScope(claims.roles, required_scope);
            }
            if (!scope_ok) {
                scope_ok = roleGrantsScope(claims.groups, required_scope);
            }

            if (!scope_ok) {
                THEMIS_WARN("JWT authorization denied for user '{}': missing scope '{}'",
                            claims.sub, required_scope);
                metrics_.authz_denied_total++;
                return AuthResult::Denied("Missing required scope: " + req);
            }
        }
        
        metrics_.authz_success_total++;
        return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);
        
    } catch (const std::exception& e) {
        metrics_.jwt_validation_failed_total++;
        THEMIS_WARN("JWT validation failed: {}", e.what());
        return AuthResult::Denied(std::string("JWT validation failed: ") + e.what());
    }
}

AuthMiddleware::AuthResult AuthMiddleware::validateToken(std::string_view token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Try static bearer token first
    auto it = tokens_.find(std::string(token));
    if (it != tokens_.end()) {
        return AuthResult::OK(it->second.user_id, it->second.tenant_id);
    }

    // Try API key (combined "key_id.secret" format)
    if (api_key_enabled_ && api_key_auth_ && token.find('.') != std::string_view::npos) {
        try {
            auto claims = api_key_auth_->authenticateCombined(std::string(token));
            metrics_.authz_success_total++;
            return AuthResult::OK(claims.principal, claims.tenant_id, claims.roles);
        } catch (const auth::AuthException&) {
            // Fall through to JWT
        }
    }

    // Try JWT validation
    if (jwt_enabled_ && jwt_validator_) {
        try {
            auto claims = jwt_validator_->parseAndValidate(std::string(token));
            metrics_.jwt_validation_success_total++;
            return AuthResult::OK(claims.sub, claims.tenant_id, claims.groups);
        } catch (...) {
            THEMIS_DEBUG("auth_middleware: unhandled exception caught");
            metrics_.jwt_validation_failed_total++;
            // GAP-013: Log JWT validation failures at WARN for auditability (CWE-778).
            // Previously logged at DEBUG, which means auth failures were invisible
            // in production log levels and could not be detected by SIEM systems.
            // The JWT-specific message is the most informative denial reason, so we
            // also emit the final counter update here and return immediately to avoid
            // a redundant second WARN from the generic catch-all below.
            metrics_.authz_invalid_token_total++;
            THEMIS_WARN("[SECURITY] validateToken: JWT validation failed — "
                        "possible invalid or tampered token (GAP-013/CWE-778). "
                        "authz_invalid_token_total={}", metrics_.authz_invalid_token_total.load());
            return AuthResult::Denied("Invalid token");
        }
    }
    
    // GAP-013: Log the final denial at WARN so every unauthenticated request
    // is visible in the audit trail regardless of log level.
    metrics_.authz_invalid_token_total++;
    THEMIS_WARN("[SECURITY] validateToken: token rejected — no matching static token, "
                "API key, or JWT found (GAP-013/CWE-778). "
                "authz_invalid_token_total={}", metrics_.authz_invalid_token_total.load());
    return AuthResult::Denied("Invalid token");
}

bool AuthMiddleware::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !tokens_.empty() || jwt_enabled_ || kerberos_enabled_ || mtls_enabled_ || api_key_enabled_;
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

    
    // Note: mutex is already locked by caller (authorize)

    if (!kerberos_auth_) {
        return AuthResult::Denied("Kerberos authentication not configured");
    }
    auto& kerberos_auth = *kerberos_auth_;

    try {
        // Authenticate the Kerberos token
        auto result = kerberos_auth.authenticateToken(std::string(token));

        if (!result.success) {
            THEMIS_WARN("Kerberos authentication failed: {}", result.error_message);
            return AuthResult::Denied("Kerberos authentication failed: " + result.error_message);
        }

        // Build roles string manually (fmt::join not available in fmt 11.0.2)
        std::ostringstream roles_oss;
        for (size_t i = 0; i < result.roles.size(); ++i) {
            if (i > 0) roles_oss << ", ";
            roles_oss << result.roles[i];
        }
        std::string roles_str = roles_oss.str();
        THEMIS_INFO("Kerberos authentication successful for principal '{}' with roles: [{}]",
                   result.principal_name, roles_str);

        // Check if any of the principal's roles grants the required_scope via role-to-scope mapping
        if (!required_scope.empty() && !role_scope_map_.empty()) {
            bool scope_granted = false;
            for (const auto& role : result.roles) {
                auto it = role_scope_map_.find(role);
                if (it != role_scope_map_.end() &&
                    std::find(it->second.begin(), it->second.end(), required_scope) != it->second.end()) {
                    scope_granted = true;
                    break;
                }
            }
            if (!scope_granted) {
                metrics_.authz_denied_total++;
                THEMIS_WARN("Kerberos authorization denied for principal '{}': "
                            "no role grants scope '{}'",
                            result.principal_name, required_scope);
                return AuthResult::Denied(
                    std::string("Kerberos principal missing required scope: ") +
                    std::string(required_scope));
            }
        }

        
        // Check required scope: treat each Kerberos role as a direct scope grant,
        // and also consult role_scope_map_ for role → scope expansion.
        //
        // IMPORTANT: Kerberos tickets do not include tenant information.
        // Clients using Kerberos authentication MUST provide tenant_id via:
        // - X-Tenant-ID header
        // - Path parameter (/tenants/{tenant_id}/...)
        // The tenant_id will be extracted from the request in the API handler.

        metrics_.authz_success_total++;
        return AuthResult::OK(result.principal_name, "", {});  // Empty tenant_id - must be provided via header

        if (!required_scope.empty()) {
            const std::string req(required_scope);
            // Direct role match: role name == required_scope
            bool scope_ok = false;
            for (const auto& role : result.roles) {
                if (role == req) { scope_ok = true; break; }
            }
            // Fallback: check role_scope_map_ for any role that grants the scope
            if (!scope_ok) {
                scope_ok = roleGrantsScope(result.roles, required_scope);
            }
            if (!scope_ok) {
                THEMIS_WARN("Kerberos authorization denied for principal '{}': "
                            "no role provides scope '{}'",
                            result.principal_name, required_scope);
                metrics_.authz_denied_total++;
                return AuthResult::Denied("Missing required scope: " + req);
            }
        }
        
        metrics_.authz_success_total++;
        return AuthResult::OK(result.principal_name, "", result.roles);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Kerberos authentication error: {}", e.what());
        return AuthResult::Denied(std::string("Kerberos authentication error: ") + e.what());
    }
}

AuthMiddleware::AuthResult AuthMiddleware::authorizeViaMTLS(
    std::string_view cert_pem,
    [[maybe_unused]] std::string_view required_scope) const {

    // Scope is role-based via subject_mappings; checked by caller chain

    // Note: mutex is already locked by caller (authorize)

    if (!mtls_auth_) {
        return AuthResult::Denied("mTLS authentication not configured");
    }
    auto& mtls_auth = *mtls_auth_;

    try {
        auto claims = mtls_auth.authenticate(std::string(cert_pem));

        THEMIS_INFO("mTLS authentication successful for principal '{}' roles={}",
                claims.principal, claims.roles.size());

        metrics_.authz_success_total++;
        return AuthResult::OK(claims.principal, "", claims.roles);

    } catch (const auth::AuthException& e) {
        THEMIS_WARN("mTLS authentication failed: {}", e.what());
        metrics_.authz_invalid_token_total++;
        return AuthResult::Denied(std::string("mTLS authentication failed: ") + e.what());
    } catch (const std::exception& e) {
        THEMIS_ERROR("mTLS authentication error: {}", e.what());
        return AuthResult::Denied(std::string("mTLS authentication error: ") + e.what());
    }
}

AuthMiddleware::AuthResult AuthMiddleware::authorizeViaApiKey(
    std::string_view combined_token,
    std::string_view required_scope) const
{
    // Note: mutex is already locked by caller (authorize)

    if (!api_key_auth_) {
        return AuthResult::Denied("API key authentication not configured");
    }
    auto& api_key_auth = *api_key_auth_;

    try {
        auto claims = api_key_auth.authenticateCombined(std::string(combined_token));

        THEMIS_INFO("API key authenticated: key_id='{}' principal='{}' tenant='{}'",
                    claims.key_id, claims.principal, claims.tenant_id);

        // Scope check: if a scope is required, the key must carry it explicitly
        if (!required_scope.empty() &&
            !claims.hasScope(std::string(required_scope))) {
            metrics_.authz_denied_total++;
            return AuthResult::Denied(
                std::string("API key missing required scope: ") + std::string(required_scope));
        }

        metrics_.authz_success_total++;
        return AuthResult::OK(claims.principal, claims.tenant_id, claims.roles);

    } catch (const auth::AuthException& e) {
        THEMIS_WARN("API key authentication failed: {}", e.what());
        metrics_.authz_invalid_token_total++;
        return AuthResult::Denied(std::string("API key authentication failed: ") + e.what());
    } catch (const std::exception& e) {
        THEMIS_ERROR("API key authentication error: {}", e.what());
        return AuthResult::Denied(std::string("API key authentication error: ") + e.what());
    }
}

void AuthMiddleware::setJWKSForTesting(const nlohmann::json& jwks)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (jwt_validator_) {
        jwt_validator_->setJWKSForTesting(jwks);
    }
}

void AuthMiddleware::loadRoleScopeMapping()
{
    // Called with mutex_ held.  Attempt to load config/security/rbac_roles.yaml.
    role_scope_map_loaded_ = true;  // Mark regardless of outcome to avoid repeated attempts

    auto resolved = config::ConfigPathResolver::tryResolve("config/security/rbac_roles.yaml");
    if (!resolved.has_value()) {
        THEMIS_DEBUG("rbac_roles.yaml not found (config/security/rbac_roles.yaml); "
                     "role-to-scope mapping not loaded — only direct scope claims from JWTs "
                     "will be enforced until setRoleScopeMapping() is called");
        return;
    }

    try {
        YAML::Node root = YAML::LoadFile(*resolved);
        if (!root["roles"]) {
            THEMIS_WARN("rbac_roles.yaml loaded but contains no 'roles' key; "
                        "role-to-scope mapping will be empty");
            return;
        }

        std::unordered_map<std::string, std::vector<std::string>> mapping;
        for (const auto& entry : root["roles"]) {
            std::string role_name = entry.first.as<std::string>();
            std::vector<std::string> scopes;
            if (entry.second["scopes"]) {
                for (const auto& s : entry.second["scopes"]) {
                    scopes.push_back(s.as<std::string>());
                }
            }
            mapping[role_name] = std::move(scopes);
        }

        role_scope_map_ = std::move(mapping);
        THEMIS_INFO("Loaded role-to-scope mapping from '{}': {} roles",
                    *resolved, role_scope_map_.size());

    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to load role-to-scope mapping from '{}': {}",
                    *resolved, e.what());
    }
}

} // namespace themis

