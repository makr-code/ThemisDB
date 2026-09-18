/**
 * @file oidc_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/jwt_validator.h"
#include "auth/oauth_device_flow.h"
#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>

namespace themis {
namespace auth {

struct OIDCDiscoveryDocument {
    std::string issuer;                              ///< Issuer identifier (must match token iss)
    std::string jwks_uri;                            ///< JWKS endpoint for public key retrieval
    std::string token_endpoint;                      ///< Token endpoint (authorization_code / device)
    std::string revocation_endpoint;                 ///< OAuth2 token revocation endpoint (RFC 7009)
    std::string authorization_endpoint;              ///< Authorization endpoint (code flow)
    std::string device_authorization_endpoint;       ///< Device authorization endpoint (RFC 8628)
    std::string userinfo_endpoint;                   ///< UserInfo endpoint (optional)
    std::vector<std::string> id_token_signing_alg_values_supported; ///< Signing algorithms
    std::vector<std::string> response_types_supported;
    std::vector<std::string> grant_types_supported;
    std::vector<std::string> scopes_supported;
};

struct OIDCProviderConfig {
    std::string issuer_url;             ///< Base issuer URL (discovery appended automatically)
    std::string client_id;             ///< OAuth 2.0 client identifier
    std::string client_secret;         ///< Client secret (empty for public clients)
    std::vector<std::string> scopes;   ///< Requested scopes (default: {"openid"})
    std::string expected_audience;     ///< Expected audience claim in tokens (optional)

    std::chrono::seconds jwks_cache_ttl{600};   ///< How long to cache JWKS responses
    std::chrono::seconds clock_skew{60};        ///< Allowed clock skew for token validation

    int http_timeout_seconds{10};               ///< Timeout for discovery and JWKS HTTP requests
};

class OIDCProvider {
public:
    /**
     * @brief OIDCProvider.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OIDCProvider(const OIDCProviderConfig& config);

    // Non-copyable (owns curl state and cached document)
    OIDCProvider(const OIDCProvider&) = delete;
    OIDCProvider& operator=(const OIDCProvider&) = delete;

    // Movable
    OIDCProvider(OIDCProvider&&) noexcept = default;
    OIDCProvider& operator=(OIDCProvider&&) noexcept = default;

    // -----------------------------------------------------------------------
    // Discovery
    // -----------------------------------------------------------------------

    /**
     * @brief Discover.
     */
    void discover();

    /**
     * @brief Discovery Document.
     * @return Return value.
     */
    const OIDCDiscoveryDocument& discoveryDocument();

    // -----------------------------------------------------------------------
    // Token validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate Token.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    JWTClaims validateToken(const std::string& token);

    /**
     * @brief Validator.
     * @return Return value.
     */
    JWTValidator& validator();

    // -----------------------------------------------------------------------
    // Device flow
    // -----------------------------------------------------------------------

    /**
     * @brief Create Device Flow.
     * @return Return value.
     */
    OAuthDeviceFlow createDeviceFlow();

    // -----------------------------------------------------------------------
    // Configuration accessors
    // -----------------------------------------------------------------------

    const std::string& clientId() const { return config_.client_id; }

    const std::string& clientSecret() const { return config_.client_secret; }

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Set Discovery Document For Testing.
     * @param[in] doc Input parameter.
     */
    void setDiscoveryDocumentForTesting(const OIDCDiscoveryDocument& doc);

    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn
    );

private:
    OIDCProviderConfig config_;

    // Discovered document (nullopt = not yet fetched)
    std::optional<OIDCDiscoveryDocument> discovery_doc_;

    // Validator is (re-)created after each successful discovery
    std::unique_ptr<JWTValidator> validator_;

    // Test injection hook
    std::function<std::string(const std::string& url)> http_get_fn_;

    /**
     * @brief Fetch a URL and return the raw body (uses libcurl unless overridden)
     * @param[in] url Input parameter.
     * @return Return value.
     */
    std::string httpGet(const std::string& url) const;

    /**
     * @brief Parse raw JSON into OIDCDiscoveryDocument
     * @param[in] json_body Input parameter.
     * @return Return value.
     */
    static OIDCDiscoveryDocument parseDiscovery(const std::string& json_body);

    /**
     * @brief Build JWTValidatorConfig from the discovery document and provider config
     * @return Return value.
     */
    JWTValidatorConfig buildValidatorConfig() const;
};

} // namespace auth
} // namespace themis
