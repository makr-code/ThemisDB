/**
 * @file auth_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/mtls_authenticator.h"

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>

namespace themis {
namespace auth {
    class JWTValidator;
    class GSSAPIAuthenticator;
    struct KerberosConfig;
    class ApiKeyAuthenticator;
    struct ApiKeyCredential;
}
namespace security {
    class USBAdminAuthenticator;
}

/// Token-based and JWT-based authorization with scopes
/// Supports both static API tokens and dynamic JWT validation (Keycloak, etc.)
class AuthMiddleware {
public:
    struct AuthContext {
        std::string user_id;
        std::string tenant_id;  // Tenant from JWT or token config
        std::vector<std::string> groups;
    };
    struct AuthResult {
        bool authorized = false;
        std::string user_id;
        std::string tenant_id;  // Tenant from JWT claim or token config
        std::vector<std::string> groups;  // JWT groups claim for encryption contexts
        std::string reason; // for audit logs
        static AuthResult OK(std::string_view uid, std::string_view tid = "", std::vector<std::string> grps = {}) { 
            return {true, std::string(uid), std::string(tid), std::move(grps), ""}; 
        }
        static AuthResult Denied(std::string msg) { return {false, "", "", {}, std::move(msg)}; }
    };

    struct TokenConfig {
        std::string token;
        std::string user_id;
        std::string tenant_id;  // Optional: if not set, extracted from request headers
        std::unordered_set<std::string> scopes;
    };

    /// JWT Configuration
    struct JWTConfig {
        std::string jwks_url;           // URL to fetch JWKS (JSON Web Key Set)
        std::string expected_issuer;     // Expected "iss" claim
        std::string expected_audience;   // Expected "aud" claim
        std::chrono::seconds jwks_cache_ttl{3600}; // Default 1 hour
        std::chrono::seconds clock_skew{60};       // Default 60 seconds tolerance
        bool require_issuer_validation = true;   // Require expected_issuer to be configured
        bool require_audience_validation = true; // Require expected_audience to be configured
        
        // Mapping of JWT claims to scopes and tenant
        std::string scope_claim = "roles";  // Which JWT claim contains scopes (e.g., "roles", "groups", "scopes")
        std::string tenant_claim = "tenant_id";  // Which JWT claim contains tenant ID
    };

    /// API Key Configuration
    struct ApiKeyConfig {
        bool check_expiry{true};        ///< Reject keys whose expiry has passed
        size_t max_key_id_length{128};  ///< Maximum allowed key_id length
        size_t max_secret_length{512};  ///< Maximum allowed secret length
        static ApiKeyConfig defaults() { return {}; }
    };

    /// Constructor (must be defined in .cpp due to unique_ptr<JWTValidator>)
    AuthMiddleware();
    
    /// Destructor (must be defined in .cpp due to unique_ptr<JWTValidator>)
    ~AuthMiddleware();
    
    /// Enable JWT validation
    void enableJWT(const JWTConfig& config);
    
    /// Enable Kerberos/GSSAPI authentication
    /// @param config Kerberos configuration
    void enableKerberos(const auth::KerberosConfig& config);

    /// Enable certificate-based mutual TLS (mTLS) authentication
    /// When enabled, PEM-encoded X.509 client certificates may be passed as
    /// tokens and will be validated against the configured CA.
    /// @param config mTLS configuration (CA certificate, subject mappings, etc.)
    void enableMTLS(const auth::MTLSAuthenticator::Config& config);

    /// Enable API key (static key + secret) authentication.
    /// Tokens should be presented in the format "<key_id>.<secret>".
    /// Use addApiKeyCredential() to register key credentials.
    /// @param config API key authenticator configuration
    void enableApiKeyAuth(const ApiKeyConfig& config = ApiKeyConfig::defaults());

    /// Add an API key credential to the store.
    /// Use auth::ApiKeyAuthenticator::createCredential() to construct the credential.
    /// Thread-safe.
    /// @param credential Credential with hashed secret (see ApiKeyAuthenticator::createCredential)
    void addApiKeyCredential(const auth::ApiKeyCredential& credential);

    /// Remove an API key credential by key_id.
    /// Thread-safe. No-op if the key_id is not found.
    /// @param key_id Key identifier to remove
    void removeApiKeyCredential(const std::string& key_id);

    /// Enable USB-based admin authentication
    /// When enabled, configured admin scopes require a valid USB device to be present
    /// @param mount_path Path where encrypted USB is mounted
    /// @param protected_scopes List of scopes requiring USB (empty = use defaults)
    void enableUSBAdminAuth(
        const std::string& mount_path = "/mnt/themis-admin",
        const std::vector<std::string>& protected_scopes = {}
    );

    /// Configure allowed tokens (typically loaded from config file)
    void addToken(const TokenConfig& config);
    void removeToken(std::string_view token);
    void clearTokens();

    /// Configure a role-to-scope mapping used by JWT and Kerberos authorization.
    ///
    /// When a JWT or Kerberos token's direct scope claims do not include the
    /// `required_scope`, the middleware falls back to checking whether any of the
    /// token's roles maps to that scope through this table.
    ///
    /// Example: `setRoleScopeMapping({{"admin", {"cache:write", "cache:read"}},
    ///                                 {"viewer", {"cache:read"}}})`
    ///
    /// Thread-safe; replaces any previously configured mapping.
    void setRoleScopeMapping(
        std::unordered_map<std::string, std::vector<std::string>> mapping);

    /// Check if token has required scope
    /// @param token Bearer token from Authorization header
    /// @param required_scope Required scope (e.g., "admin", "config:write", "cdc:read", "metrics:read")
    AuthResult authorize(std::string_view token, std::string_view required_scope) const;

    /**
     * @brief Check if a token is valid without requiring a specific scope.
     * 
     * Validates the token against configured authentication methods (JWT, Kerberos, mTLS, API Key).
     * This is useful for endpoints that don't require a specific scope but still need authentication.
     * 
     * @param token Token string (typically from Authorization header without "Bearer" prefix)
     * 
     * @return AuthResult with:
     *         - authorized=true, user_id, tenant_id, groups if valid
     *         - authorized=false with reason if validation failed
     * 
     * @note Thread-safe; concurrent calls allowed
     * @note Fail-closed: any validation failure results in Denied result
     * 
     * @see authorize() to check both token validity and scope
     */
    AuthResult validateToken(std::string_view token) const;

    /**
     * @brief Extract basic context (user_id, groups) from a valid token.
     * 
     * Parses the token and extracts user context without performing full authorization.
     * Useful for operations that need user information but skip detailed scope checking.
     * Returns std::nullopt if token is invalid or cannot be parsed.
     * 
     * @param token Token string (typically from Authorization header without "Bearer" prefix)
     * 
     * @return std::optional containing AuthContext with user_id, tenant_id, and groups
     *         if token is valid; std::nullopt if token is invalid or cannot be parsed
     * 
     * @note Thread-safe; concurrent calls allowed
     * @note Does NOT perform scope validation; use authorize() for full authorization checks
     * 
     * @see authorize() for full authorization including scope validation
     */
    std::optional<AuthContext> extractContext(std::string_view token) const;

    /// Extract token from "Bearer <token>" header value
    static std::optional<std::string> extractBearerToken(std::string_view auth_header);

    /// Get metrics (for Prometheus)
    struct Metrics {
        std::atomic<uint64_t> authz_success_total{0};
        std::atomic<uint64_t> authz_denied_total{0};
        std::atomic<uint64_t> authz_invalid_token_total{0};
        std::atomic<uint64_t> jwt_validation_success_total{0};
        std::atomic<uint64_t> jwt_validation_failed_total{0};
    };

    const Metrics& getMetrics() const { return metrics_; }

    // Returns true if at least one token is configured or JWT is enabled
    bool isEnabled() const;
    
    /// Check if USB admin authentication is enabled and USB is present
    bool isUSBAdminReady() const;

    // testing helper – injects a pre-built JWKS into the JWT validator so tests
    // can verify scope enforcement without a live JWKS endpoint.
    void setJWKSForTesting(const nlohmann::json& jwks);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TokenConfig> tokens_; // token -> config
    mutable Metrics metrics_;
    
    // JWT validation
    std::unique_ptr<auth::JWTValidator> jwt_validator_;
    JWTConfig jwt_config_;
    bool jwt_enabled_ = false;
    
    // Kerberos/GSSAPI authentication
    std::unique_ptr<auth::GSSAPIAuthenticator> kerberos_auth_;
    bool kerberos_enabled_ = false;
    
    // mTLS certificate authentication
    std::unique_ptr<auth::MTLSAuthenticator> mtls_auth_;
    bool mtls_enabled_ = false;

    // USB Admin Authentication
    std::unique_ptr<security::USBAdminAuthenticator> usb_admin_auth_;
    bool usb_admin_enabled_ = false;
    std::vector<std::string> usb_protected_scopes_;

    // API key authentication
    std::unique_ptr<auth::ApiKeyAuthenticator> api_key_auth_;
    bool api_key_enabled_ = false;

    // Role-to-scope mapping: role name → list of scopes that role grants.
    // Used as fallback in JWT and Kerberos authorization when direct scope
    // claims don't contain the required_scope.
    std::unordered_map<std::string, std::vector<std::string>> role_scope_map_;
    bool role_scope_map_loaded_ = false;  // true once a load attempt has been made

    // Helper: check if scope is an admin scope requiring USB
    bool isAdminScope(std::string_view scope) const;

    /// Returns true if @p required_scope is granted by any role in @p roles via role_scope_map_.
    bool roleGrantsScope(const std::vector<std::string>& roles,
                         std::string_view required_scope) const;

    // Helper: try to authorize via JWT
    AuthResult authorizeViaJWT(std::string_view token, std::string_view required_scope) const;
    
    // Helper: try to authorize via Kerberos
    AuthResult authorizeViaKerberos(std::string_view token, std::string_view required_scope) const;

    // Helper: try to authorize via mTLS client certificate
    AuthResult authorizeViaMTLS(std::string_view cert_pem, std::string_view required_scope) const;

    // Helper: try to authorize via API key (combined "key_id.secret" format)
    AuthResult authorizeViaApiKey(std::string_view combined_token, std::string_view required_scope) const;

    // Helper: load role-to-scope mapping from config/security/rbac_roles.yaml.
    // Must be called with mutex_ held.
    void loadRoleScopeMapping();
};

} // namespace themis

