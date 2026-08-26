/**
 * @file federated_identity_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/oidc_provider.h"
#include "auth/jwt_validator.h"
#include "auth/auth_error.h"
#include "auth/auth_audit_logger.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <utility>

namespace themis {
namespace auth {

/**
 * @brief Result of a federated token validation
 *
 * Carries the validated claims together with the realm (issuer URL) that
 * successfully validated the token.
 */
struct FederatedValidationResult {
    JWTClaims    claims;      ///< Validated JWT claims
    std::string  realm;       ///< Issuer URL of the realm that validated the token
};

/**
 * @brief Result of an RFC 8693 OAuth 2.0 Token Exchange
 *
 * Contains the raw exchanged access token, the token type declared by the IdP,
 * the optional lifetime, and the validated claims extracted by the
 * JWTValidator pipeline.
 */
struct TokenExchangeResult {
    std::string  access_token;        ///< Exchanged access token (raw JWT)
    std::string  issued_token_type;   ///< Token type URI returned by the IdP
    std::string  token_type;          ///< Bearer token type (usually "Bearer")
    int          expires_in{0};       ///< Lifetime in seconds (0 = not provided)
    std::string  scope;               ///< Granted scopes (space-separated, may be empty)
    JWTClaims    claims;              ///< Validated claims from the exchanged token
    std::string  realm;               ///< Issuer URL of the realm that issued the token
};

/**
 * @brief Manages federated identity across multiple OIDC realms
 *
 * Allows ThemisDB to accept tokens issued by any of a set of registered OIDC
 * identity providers (realms).  When a token arrives the manager inspects its
 * @c iss claim, locates the matching realm, and delegates full JWT validation
 * to the corresponding OIDCProvider.
 *
 * Thread safety: all public methods are safe to call concurrently.
 *
 * Typical usage:
 * @code
 *   FederatedIdentityManager fed;
 *
 *   OIDCProviderConfig prod_cfg;
 *   prod_cfg.issuer_url = "https://keycloak.example.com/realms/production";
 *   prod_cfg.client_id  = "themisdb";
 *   fed.addRealm(prod_cfg);
 *
 *   OIDCProviderConfig dev_cfg;
 *   dev_cfg.issuer_url = "https://keycloak.example.com/realms/development";
 *   dev_cfg.client_id  = "themisdb";
 *   fed.addRealm(dev_cfg);
 *
 *   // Validate a bearer token without knowing which realm issued it
 *   FederatedValidationResult result = fed.validateToken(bearer_token);
 *   std::cout << "Authenticated via realm: " << result.realm << "\n";
 *   std::cout << "Subject: " << result.claims.sub << "\n";
 * @endcode
 *
 * ### Provider-degradation contract (auth_principal_contract.h §6)
 *
 * Each realm is backed by an OIDCProvider that performs network I/O (JWKS fetch,
 * token-exchange POST).  The following failure semantics apply to all network-bound
 * paths:
 *
 *   - **Realm not found**: validateToken() throws AuthException with
 *     AuthErrorCode::FEDERATION_UNKNOWN_REALM (fail-closed — not a known issuer).
 *   - **Provider network error**: if OIDCProvider::validateToken() throws a network
 *     or JWKS-fetch error, FederatedIdentityManager re-throws as
 *     AuthErrorCode::PROVIDER_DEGRADED.  The caller MUST treat this as a hard denial.
 *   - **Provider capability mismatch**: if the token exchange endpoint is absent or
 *     requires TLS that is not configured, exchangeToken() throws
 *     AuthErrorCode::PROVIDER_CAPABILITY_MISMATCH.
 *   - **Multiple realms share an issuer URL**: addRealm() throws
 *     AuthErrorCode::AUTH_CONFIG_INVALID at registration time; no duplicate realms
 *     are silently accepted.
 *   - **Token size violation**: tokens exceeding kMaxJwtTokenBytes are rejected
 *     before any realm lookup with AuthErrorCode::JWT_INVALID_FORMAT.
 *
 * All network-bound operations are synchronous.  Callers that need non-blocking
 * federation MUST dispatch to an async worker thread and handle the resulting
 * future according to the async-provider contract (auth_principal_contract.h §7).
 *
 * @see include/auth/auth_principal_contract.h — §4 Fail-closed, §6 Provider capability
 */
class FederatedIdentityManager {
public:
    FederatedIdentityManager() = default;

    // Non-copyable (owns OIDCProvider instances)
    FederatedIdentityManager(const FederatedIdentityManager&) = delete;
    FederatedIdentityManager& operator=(const FederatedIdentityManager&) = delete;

    // Movable
    FederatedIdentityManager(FederatedIdentityManager&&) noexcept = default;
    FederatedIdentityManager& operator=(FederatedIdentityManager&&) noexcept = default;

    // -----------------------------------------------------------------------
    // Realm registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new OIDC realm.
     *
     * The trailing slash of @p config.issuer_url (if any) is stripped before
     * registration so that "https://idp.example.com/realms/x" and
     * "https://idp.example.com/realms/x/" are treated as the same realm.
     *
     * @param config  Provider configuration for the realm.
     * @throws AuthException(AUTH_CONFIG_INVALID) if @p config.issuer_url is
     *         empty or if a realm with the same normalized issuer URL is
     *         already registered.
     */
    void addRealm(const OIDCProviderConfig& config);

    /**
     * @brief Remove a previously registered realm.
     *
     * @param issuer_url  Issuer URL of the realm to remove (trailing slash
     *                    normalized automatically).
     * @return true if the realm was found and removed, false otherwise.
     */
    bool removeRealm(const std::string& issuer_url);

    /**
     * @brief Check whether a realm is registered.
     *
     * @param issuer_url  Issuer URL (trailing slash normalized automatically).
     */
    bool hasRealm(const std::string& issuer_url) const;

    /**
     * @brief Return the normalized issuer URLs of all registered realms.
     */
    std::vector<std::string> realmIssuers() const;

    /**
     * @brief Return the number of registered realms.
     */
    size_t realmCount() const;

    // -----------------------------------------------------------------------
    // Token validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate a bearer token against the realm that issued it.
     *
     * The method peeks at the token's @c iss claim (without full validation)
     * to identify the responsible realm, then delegates to that realm's
     * OIDCProvider::validateToken().
     *
     * @param token  JWT bearer token (with or without "Bearer " prefix).
     * @return FederatedValidationResult containing the validated claims and
     *         the matched realm's issuer URL.
     * @throws AuthException(JWT_ISSUER_MISMATCH) if the token's issuer does
     *         not match any registered realm.
     * @throws AuthException on any other validation failure (expired,
     *         invalid signature, audience mismatch, …).
     */
    FederatedValidationResult validateToken(const std::string& token);

    // -----------------------------------------------------------------------
    // RFC 8693 Token Exchange
    // -----------------------------------------------------------------------

    /**
     * @brief Exchange a token via RFC 8693 (OAuth 2.0 Token Exchange).
     *
     * Implements the token exchange grant type for service-to-service
     * impersonation and delegation in federated scenarios.
     *
     * The method:
     *  1. Validates @p subject_token through the registered realm's
     *     JWTValidator pipeline to ensure the caller holds a valid credential.
     *  2. Posts a token-exchange request to the realm's @c token_endpoint
     *     using @c grant_type=urn:ietf:params:oauth:grant-type:token-exchange.
     *  3. Validates the returned token through the same JWTValidator pipeline.
     *  4. Scopes the exchanged token to @p target_scopes (minimum required
     *     permissions) if provided.
     *
     * @param subject_token        The token being exchanged (JWT bearer token).
     * @param subject_token_type   URI identifying the type of @p subject_token,
     *                             e.g. @c urn:ietf:params:oauth:token-type:access_token.
     * @param requested_token_type URI identifying the desired token type,
     *                             e.g. @c urn:ietf:params:oauth:token-type:access_token.
     * @param target_scopes        Optional list of scopes to request; the IdP
     *                             will scope the exchanged token to the
     *                             minimum required permissions.  When empty,
     *                             no explicit scope restriction is sent.
     * @return TokenExchangeResult with the raw exchanged token and its
     *         validated JWTClaims.
     * @throws AuthException(JWT_ISSUER_MISMATCH) if @p subject_token's issuer
     *         does not match any registered realm.
     * @throws AuthException(AUTH_CONFIG_INVALID) if the realm's
     *         @c token_endpoint is absent or not HTTPS.
     * @throws AuthException(AUTH_INTERNAL_ERROR) on HTTP or JSON parse failure.
     * @throws AuthException(AUTH_INVALID_CREDENTIALS) if the IdP returns an
     *         OAuth error response.
     * @throws AuthException(AUTH_INSUFFICIENT_PERMISSIONS) if the IdP grants
     *         fewer scopes than requested via @p target_scopes.
     * @throws std::runtime_error if the subject token or the returned token
     *         fail JWTValidator signature/expiry/audience validation.
     */
    TokenExchangeResult exchangeToken(
        const std::string& subject_token,
        const std::string& subject_token_type,
        const std::string& requested_token_type,
        const std::vector<std::string>& target_scopes = {});

    /**
     * @brief Access a specific realm's OIDCProvider.
     *
     * Calls discover() lazily if the provider has not yet fetched its
     * discovery document.
     *
     * @param issuer_url  Issuer URL (trailing slash normalized automatically).
     * @return Reference to the OIDCProvider for that realm.
     * @throws AuthException(AUTH_CONFIG_INVALID) if no realm with the given
     *         issuer URL is registered.
     */
    OIDCProvider& realmProvider(const std::string& issuer_url);

    /**
     * @brief Attach an AuthAuditLogger that receives JWT success/failure events.
     * @param logger Non-owning pointer; may be nullptr (disables audit logging).
     */
    void setAuditLogger(AuthAuditLogger* logger) { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Override the HTTP GET function injected into every realm's
     *        OIDCProvider (for unit tests only).
     *
     * Must be called *before* addRealm() for the hook to apply, or use
     * realmProvider() to inject per-realm after registration.
     */
    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn);

    /**
     * @brief Override the HTTP POST function used by exchangeToken()
     *        (for unit tests only).
     *
     * When set, exchangeToken() calls this function instead of libcurl to
     * submit the token-exchange form POST.  The function receives the target
     * URL and the URL-encoded form body, and must return the raw JSON response
     * body or throw std::runtime_error on failure.
     */
    void setHttpPostForTesting(
        std::function<std::string(const std::string& url,
                                  const std::string& body)> fn);

private:
    /// Normalize an issuer URL by stripping trailing slashes.
    static std::string normalize(const std::string& url);

    /// Peek at the JWT payload and extract the "iss" claim without
    /// performing any cryptographic verification.
    static std::string extractIssuer(const std::string& token);

    /// Build an application/x-www-form-urlencoded request body from a list
    /// of key–value pairs.
    static std::string buildFormBody(
        const std::vector<std::pair<std::string, std::string>>& params);

    /// Perform an HTTP POST and return the raw response body.
    /// Uses the mock function if setHttpPostForTesting() was called.
    std::string httpPost(const std::string& url, const std::string& body) const;

    mutable std::mutex mutex_;

    /// issuer_url (normalized) -> OIDCProvider
    std::unordered_map<std::string, std::shared_ptr<OIDCProvider>> realms_;

    /// Optional HTTP GET mock injected for testing; applied to all new realms
    std::function<std::string(const std::string& url)> http_get_fn_;

    /// Optional HTTP POST mock injected for testing; used by exchangeToken()
    std::function<std::string(const std::string& url,
                               const std::string& body)> http_post_fn_;

    AuthAuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.
};

} // namespace auth
} // namespace themis
