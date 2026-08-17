/**
 * @file oidc_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief OIDC Discovery Document (OpenID Connect Discovery 1.0)
 *
 * Parsed from the provider's /.well-known/openid-configuration endpoint.
 * Only the fields relevant to ThemisDB are captured; all others are ignored.
 */
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

/**
 * @brief Configuration for an OIDC federated identity provider
 */
struct OIDCProviderConfig {
    std::string issuer_url;             ///< Base issuer URL (discovery appended automatically)
    std::string client_id;             ///< OAuth 2.0 client identifier
    std::string client_secret;         ///< Client secret (empty for public clients)
    std::vector<std::string> scopes;   ///< Requested scopes (default: {"openid"})
    std::string expected_audience;     ///< Expected audience claim in tokens (optional)

    /// JWTValidator tuning
    std::chrono::seconds jwks_cache_ttl{600};   ///< How long to cache JWKS responses
    std::chrono::seconds clock_skew{60};        ///< Allowed clock skew for token validation

    /// HTTP timeouts
    int http_timeout_seconds{10};               ///< Timeout for discovery and JWKS HTTP requests
};

/**
 * @brief Federated identity provider using OpenID Connect (OIDC)
 *
 * Implements OpenID Connect Discovery 1.0 to automatically configure JWT
 * validation and OAuth 2.0 device-flow authentication from any standards-
 * compliant provider (Keycloak, Auth0, Microsoft Entra, Google, Okta, …).
 *
 * Responsibilities:
 *  1. Fetch and cache the provider's discovery document.
 *  2. Expose a ready-to-use JWTValidator configured from that document.
 *  3. Create an OAuthDeviceFlow (RFC 8628) using discovered endpoints.
 *  4. Validate bearer tokens via the integrated JWTValidator.
 *
 * Usage:
 * @code
 *   OIDCProviderConfig cfg;
 *   cfg.issuer_url  = "https://keycloak.example.com/realms/production";
 *   cfg.client_id   = "themisdb";
 *   cfg.scopes      = {"openid", "email", "groups"};
 *
 *   OIDCProvider provider(cfg);
 *   JWTClaims claims = provider.validateToken(bearer_token);
 * @endcode
 */
class OIDCProvider {
public:
    /**
     * @brief Construct and discover the provider's OIDC metadata.
     *
     * Fetches `{issuer_url}/.well-known/openid-configuration` on first use
     * (lazy discovery) unless explicitly called via discover().
     *
     * @param config Provider configuration
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
     * @brief Fetch (or refresh) the OIDC discovery document.
     *
     * Normally called lazily by validateToken() / createDeviceFlow().
     * Call explicitly to pre-warm the cache or to detect configuration errors
     * at startup.
     *
     * @throws AuthException(AUTH_CONFIG_INVALID) if the discovery URL is malformed
     * @throws AuthException(AUTH_INTERNAL_ERROR) on HTTP or JSON parse failure
     */
    void discover();

    /**
     * @brief Return the cached discovery document.
     *
     * Calls discover() if not yet fetched.
     *
     * @return Parsed OIDC discovery document
     */
    const OIDCDiscoveryDocument& discoveryDocument();

    // -----------------------------------------------------------------------
    // Token validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate a JWT / OIDC token issued by this provider.
     *
     * Performs:
     *  - JWKS-based signature verification (RS256, ES256, EdDSA)
     *  - Issuer matching against the discovered issuer
     *  - Audience matching (if expected_audience is set)
     *  - Expiration and not-before checks with clock-skew tolerance
     *
     * @param token  Bearer token (with or without "Bearer " prefix)
     * @return Validated JWTClaims
     * @throws AuthException on any validation failure
     */
    JWTClaims validateToken(const std::string& token);

    /**
     * @brief Access the underlying JWTValidator for advanced use.
     *
     * Calls discover() if not yet fetched.
     */
    JWTValidator& validator();

    // -----------------------------------------------------------------------
    // Device flow
    // -----------------------------------------------------------------------

    /**
     * @brief Create an OAuthDeviceFlow configured from the discovery document.
     *
     * Calls discover() automatically if not yet fetched.
     * Requires the provider to support the device_authorization_endpoint.
     *
     * @throws AuthException(AUTH_CONFIG_INVALID) if the provider does not
     *         advertise a device_authorization_endpoint
     * @throws AuthException(AUTH_INTERNAL_ERROR) if discovery fails
     */
    OAuthDeviceFlow createDeviceFlow();

    // -----------------------------------------------------------------------
    // Configuration accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Return the OAuth 2.0 client identifier for this provider.
     */
    const std::string& clientId() const { return config_.client_id; }

    /**
     * @brief Return the OAuth 2.0 client secret for this provider.
     *
     * Empty for public clients.
     */
    const std::string& clientSecret() const { return config_.client_secret; }

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Inject a mock discovery document (bypasses HTTP fetch).
     *
     * For unit tests only.  Subsequent calls to discover() are no-ops until
     * clearDiscoveryDocumentForTesting() is called.
     */
    void setDiscoveryDocumentForTesting(const OIDCDiscoveryDocument& doc);

    /**
     * @brief Override the HTTP GET function for unit testing.
     *
     * The injected function receives a URL and must return the raw response
     * body, or throw std::runtime_error on transport failure.
     */
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

    // Fetch a URL and return the raw body (uses libcurl unless overridden)
    std::string httpGet(const std::string& url) const;

    // Parse raw JSON into OIDCDiscoveryDocument
    static OIDCDiscoveryDocument parseDiscovery(const std::string& json_body);

    // Build JWTValidatorConfig from the discovery document and provider config
    JWTValidatorConfig buildValidatorConfig() const;
};

} // namespace auth
} // namespace themis
