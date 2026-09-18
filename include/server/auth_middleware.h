/**
 * @file auth_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
        /**
         * @brief Denied.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static AuthResult Denied(std::string msg) { return {false, "", "", {}, std::move(msg)}; }
    };

    struct TokenConfig {
        std::string token;
        std::string user_id;
        std::string tenant_id;  // Optional: if not set, extracted from request headers
        std::unordered_set<std::string> scopes;
    };

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

    struct ApiKeyConfig {
        bool check_expiry{true};        ///< Reject keys whose expiry has passed
        size_t max_key_id_length{128};  ///< Maximum allowed key_id length
        size_t max_secret_length{512};  ///< Maximum allowed secret length
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static ApiKeyConfig defaults() { return {}; }
    };

    AuthMiddleware();
    
    ~AuthMiddleware();
    
    /**
     * @brief Enable JWT.
     * @param[in] config Input parameter.
     */
    void enableJWT(const JWTConfig& config);
    
    /**
     * @brief Enable Kerberos.
     * @param[in] config Input parameter.
     */
    void enableKerberos(const auth::KerberosConfig& config);

    /**
     * @brief Enable MTLS.
     * @param[in] config Input parameter.
     */
    void enableMTLS(const auth::MTLSAuthenticator::Config& config);

    void enableApiKeyAuth(const ApiKeyConfig& config = ApiKeyConfig::defaults());

    /**
     * @brief Add Api Key Credential.
     * @param[in] credential Input parameter.
     */
    void addApiKeyCredential(const auth::ApiKeyCredential& credential);

    /**
     * @brief Remove Api Key Credential.
     * @param[in] key_id Identifier of the key.
     */
    void removeApiKeyCredential(const std::string& key_id);

    void enableUSBAdminAuth(
        const std::string& mount_path = "/mnt/themis-admin",
        const std::vector<std::string>& protected_scopes = {}
    );

    /**
     * @brief Add Token.
     * @param[in] config Input parameter.
     */
    void addToken(const TokenConfig& config);
    /**
     * @brief Remove Token.
     * @param[in] token Input parameter.
     */
    void removeToken(std::string_view token);
    /**
     * @brief Clear Tokens.
     */
    void clearTokens();

    void setRoleScopeMapping(
        std::unordered_map<std::string, std::vector<std::string>> mapping);

    /**
     * @brief Authorize an access control context.
     * @param[in] token Input parameter.
     * @param[in] required_scope Input parameter.
     * @return True when the context is authorized.
     */
    AuthResult authorize(std::string_view token, std::string_view required_scope) const;

    /**
     * @brief Validate Token.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    AuthResult validateToken(std::string_view token) const;

    /**
     * @brief Extract Context.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    std::optional<AuthContext> extractContext(std::string_view token) const;

    /**
     * @brief Extract Bearer Token.
     * @param[in] auth_header Input parameter.
     * @return Return value.
     */
    static std::optional<std::string> extractBearerToken(std::string_view auth_header);

    struct Metrics {
        std::atomic<uint64_t> authz_success_total{0};
        std::atomic<uint64_t> authz_denied_total{0};
        std::atomic<uint64_t> authz_invalid_token_total{0};
        std::atomic<uint64_t> jwt_validation_success_total{0};
        std::atomic<uint64_t> jwt_validation_failed_total{0};
    };

    const Metrics& getMetrics() const { return metrics_; }

    /**
     * @brief Is Enabled.
     * @return True when the operation succeeds.
     */
    bool isEnabled() const;
    
    /**
     * @brief Is USBAdmin Ready.
     * @return True when the operation succeeds.
     */
    bool isUSBAdminReady() const;

    /**
     * @brief Set JWKSFor Testing.
     * @param[in] jwks Input parameter.
     */
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

    /**
     * @brief Is Admin Scope.
     * @param[in] scope Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAdminScope(std::string_view scope) const;

    /**
     * @brief Role Grants Scope.
     * @param[in] roles Input parameter.
     * @param[in] required_scope Input parameter.
     * @return True when the operation succeeds.
     */
    bool roleGrantsScope(const std::vector<std::string>& roles,
                         std::string_view required_scope) const;

    /**
     * @brief Authorize Via JWT.
     * @param[in] token Input parameter.
     * @param[in] required_scope Input parameter.
     * @return Return value.
     */
    AuthResult authorizeViaJWT(std::string_view token, std::string_view required_scope) const;
    
    /**
     * @brief Authorize Via Kerberos.
     * @param[in] token Input parameter.
     * @param[in] required_scope Input parameter.
     * @return Return value.
     */
    AuthResult authorizeViaKerberos(std::string_view token, std::string_view required_scope) const;

    /**
     * @brief Authorize Via MTLS.
     * @param[in] cert_pem Input parameter.
     * @param[in] required_scope Input parameter.
     * @return Return value.
     */
    AuthResult authorizeViaMTLS(std::string_view cert_pem, std::string_view required_scope) const;

    /**
     * @brief Authorize Via Api Key.
     * @param[in] combined_token Input parameter.
     * @param[in] required_scope Input parameter.
     * @return Return value.
     */
    AuthResult authorizeViaApiKey(std::string_view combined_token, std::string_view required_scope) const;

    /**
     * @brief Load Role Scope Mapping.
     */
    void loadRoleScopeMapping();
};

} // namespace themis

