/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_middleware.h                                  ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 92608937d  2026-02-26  fix: GCC default-arg error in 18 headers - add ::defaults... ║
    • 33a346e4e  2026-02-25  Refactor code structure and remove redundant code blocks ... ║
    • ce63cc36d  2026-02-24  feat(auth): integrate ApiKeyAuthenticator into AuthMiddle... ║
    • 5cc90b16b  2026-02-24  feat(auth): implement mTLS certificate-based authentication ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/mtls_authenticator.h"

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
        bool require_issuer_validation = true;   // Fail at construction if expected_issuer is empty
        bool require_audience_validation = true; // Fail at construction if expected_audience is empty
        
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

    /// Check if token has required scope
    /// @param token Bearer token from Authorization header
    /// @param required_scope Required scope (e.g., "admin", "config:write", "cdc:read", "metrics:read")
    AuthResult authorize(std::string_view token, std::string_view required_scope) const;

    /// Check if token is valid (any scope)
    AuthResult validateToken(std::string_view token) const;

    /// Extract basic context (user_id, groups) from token if valid
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
    
    // Helper: check if scope is an admin scope requiring USB
    bool isAdminScope(std::string_view scope) const;
    
    // Helper: try to authorize via JWT
    AuthResult authorizeViaJWT(std::string_view token, std::string_view required_scope) const;
    
    // Helper: try to authorize via Kerberos
    AuthResult authorizeViaKerberos(std::string_view token, std::string_view required_scope) const;

    // Helper: try to authorize via mTLS client certificate
    AuthResult authorizeViaMTLS(std::string_view cert_pem, std::string_view required_scope) const;

    // Helper: try to authorize via API key (combined "key_id.secret" format)
    AuthResult authorizeViaApiKey(std::string_view combined_token, std::string_view required_scope) const;
};

} // namespace themis
