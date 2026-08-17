/**
 * @file oauth2_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/oidc_provider.h"
#include "auth/oauth_pkce_flow.h"
#include "auth/jwt_validator.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>
#include <optional>

namespace themis {
namespace server {

/**
 * @brief OAuth2/OIDC Provider HTTP handler (server-side)
 *
 * Bridges the auth-layer OIDCProvider and OAuthPKCEFlow with the HTTP server
 * routing layer.  Implements the OAuth 2.0 Authorization Code Grant with PKCE
 * (RFC 7636 / RFC 6749) as the primary flow and provides a JWT token
 * introspection endpoint (RFC 7662).
 *
 * Endpoints exposed:
 *
 *   GET  /api/v1/auth/oauth2/authorize
 *     Initiates the authorization code + PKCE flow.
 *     Query param: state (optional, CSRF token), redirect_uri (optional override)
 *     Response: { "authorization_url": "...", "state": "...", "code_verifier": "..." }
 *     Note: The code_verifier must be stored client-side and used at the
 *     /token endpoint; it is never sent to the authorization server.
 *
 *   GET  /api/v1/auth/oauth2/callback
 *     Handles the redirect callback from the authorization server.
 *     Query params: code, state
 *     Response: { "access_token": "...", "token_type": "Bearer", "expires_in": ...,
 *                 "refresh_token": "...", "id_token": "..." }
 *     Error: 400 on missing code/state or unknown/expired state,
 *            401 on token exchange failure with the IdP.
 *
 *   POST /api/v1/auth/oauth2/token
 *     Explicit token exchange (server-side / CLI clients).
 *     Body (JSON): { "code": "...", "code_verifier": "...", "state": "..." }
 *     Response: same as /callback on success.
 *     Error: 400 on missing/mismatched fields, 401 on IdP rejection.
 *
 *   POST /api/v1/auth/oauth2/refresh
 *     Refresh token rotation: exchange a refresh_token for a new token pair.
 *     Body (JSON): { "refresh_token": "..." }
 *     Response: { "access_token": "...", "token_type": "Bearer", "expires_in": ...,
 *                 "refresh_token": "..." }
 *     Error: 400 on missing refresh_token, 401 on IdP rejection.
 *
 *   POST /api/v1/auth/token/introspect
 *     RFC 7662 token introspection.
 *     Body (JSON): { "token": "..." }
 *     Response (active):   { "active": true, "sub": "...", "exp": ..., "iss": "...", ... }
 *     Response (inactive): { "active": false }
 *     Error: 400 on missing token field.
 *
 *   POST /api/v1/auth/oauth2/logout
 *     End-session: best-effort revocation of the refresh_token at the IdP.
 *     Body (JSON): { "refresh_token": "..." } (optional)
 *     Response: { "success": true }
 *
 * Thread-safe after construction.
 */
class OAuth2Provider {
public:
    using RefreshTokenRevocationFn = std::function<bool(const std::string& refresh_token)>;
    /**
     * @brief Configuration for the server-layer OAuth2/OIDC provider
     */
    struct Config {
        /// Underlying OIDC provider configuration (issuer_url, client_id, scopes …)
        auth::OIDCProviderConfig oidc;

        /// Redirect URI registered at the authorization server.
        /// Used in the authorization code exchange at the token endpoint.
        std::string redirect_uri;

        /// Maximum length of the opaque "state" parameter (default: 256).
        /// Requests with a longer state value are rejected with 400.
        std::size_t max_state_length{256};

        /// How long a pending authorization request (state → code_verifier) may
        /// live before it is considered expired (default: 10 minutes).
        std::chrono::seconds state_ttl{600};

        /// Token factory: given a validated access token string, produce an
        /// internal session token.  If not set, the raw access_token is returned.
        ///
        /// @note Thread-safety: the factory may be called from multiple threads
        /// concurrently (each handleCallback / handleTokenExchange call may invoke
        /// it).  Implementations must either be stateless or protect shared state
        /// with their own synchronization.
        std::function<std::string(const std::string& access_token)> token_factory;
    };

    /**
     * @brief Construct from configuration.
     *
     * @throws auth::AuthException (AUTH_CONFIG_INVALID) if required fields are empty.
     */
    explicit OAuth2Provider(const Config& config);

    ~OAuth2Provider() = default;

    // Non-copyable (owns OIDCProvider state)
    OAuth2Provider(const OAuth2Provider&) = delete;
    OAuth2Provider& operator=(const OAuth2Provider&) = delete;

    // Movable
    OAuth2Provider(OAuth2Provider&&) noexcept noexcept = default;
    OAuth2Provider& operator=(OAuth2Provider&&) noexcept noexcept = default;

    // -----------------------------------------------------------------------
    // HTTP handlers – each returns a JSON result.
    // On error the JSON contains a "status_code" field with the HTTP status.
    // -----------------------------------------------------------------------

    /**
     * @brief Handle GET /api/v1/auth/oauth2/authorize
     *
     * Generates a PKCE challenge and returns the authorization URL together
     * with the state and code_verifier (for the client to store).
     *
     * @param state         Optional CSRF-protection opaque value (generated if empty).
     * @param redirect_uri  Optional per-request redirect URI override.
     * @return JSON: { "authorization_url": "...", "state": "...", "code_verifier": "..." }
     *         or error JSON with "status_code" key.
     */
    nlohmann::json handleAuthorize(const std::string& state = "",
                                   const std::string& redirect_uri = "");

    /**
     * @brief Handle GET /api/v1/auth/oauth2/callback
     *
     * Receives the authorization server redirect with code and state, looks up
     * the stored code_verifier for that state, and exchanges the code for tokens.
     *
     * @param code   Authorization code from the IdP redirect.
     * @param state  Opaque state value from the IdP redirect.
     * @return JSON token response or error JSON with "status_code" key.
     */
    nlohmann::json handleCallback(const std::string& code, const std::string& state);

    /**
     * @brief Handle POST /api/v1/auth/oauth2/token
     *
     * Explicit code-exchange endpoint for server-side / CLI clients that
     * cannot be redirected.  The caller supplies both the authorization code
     * and the code_verifier from the matching authorize request.
     *
     * @param code           Authorization code.
     * @param code_verifier  PKCE code verifier matching the challenge.
     * @param state          State value (optional; used to validate the pending entry).
     * @return JSON token response or error JSON with "status_code" key.
     */
    nlohmann::json handleTokenExchange(const std::string& code,
                                       const std::string& code_verifier,
                                       const std::string& state = "");

    /**
     * @brief Handle POST /api/v1/auth/oauth2/refresh
     *
     * Exchanges a refresh_token for a new token pair via the IdP's token
     * endpoint using the RFC 6749 refresh_token grant.
     *
     * @param refresh_token  Refresh token previously issued by the IdP.
     * @return JSON: { "access_token": "...", "token_type": "Bearer",
     *                 "expires_in": ..., "refresh_token": "..." }
     *         or error JSON with "status_code" key.
     */
    nlohmann::json handleRefresh(const std::string& refresh_token);

    /**
     * @brief Handle POST /api/v1/auth/token/introspect
     *
     * Validates a JWT/access-token locally (using the provider's JWKS) and
     * returns its claims if active.  Conforms to RFC 7662 response format.
     *
     * @param token  Bearer access token or id_token to introspect.
     * @return JSON: { "active": true, "sub": "...", "exp": ..., "iss": "...",
     *                 "aud": [...], "jti": "...", "email": "..." }
     *         or { "active": false } when the token is invalid/expired.
     *         or error JSON with "status_code" key on malformed request.
     */
    nlohmann::json handleIntrospect(const std::string& token);

    /**
     * @brief Handle POST /api/v1/auth/oauth2/logout
     *
     * Best-effort revocation of the supplied refresh_token at the IdP.
     * Always returns success from the perspective of the local session.
     *
     * @param refresh_token  Refresh token to revoke (optional).
     * @return JSON: { "success": true }
     */
    nlohmann::json handleLogout(const std::string& refresh_token = "");

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Inject a mock OIDC discovery document (bypasses HTTP fetch).
     *
     * Forwarded to the underlying OIDCProvider; for unit tests only.
     */
    void setDiscoveryDocumentForTesting(const auth::OIDCDiscoveryDocument& doc);

    /**
     * @brief Override the HTTP GET function on the underlying OIDCProvider.
     */
    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn);

    /**
     * @brief Override the HTTP POST function used by the PKCE flow and
     *        the refresh/revocation endpoints.
     *
     * Allows tests to mock token endpoint responses without real network calls.
     */
    void setHttpPostForTesting(
        std::function<std::string(const std::string& url, const std::string& body)> fn);

    /**
     * @brief Override the random-bytes source on the PKCE flow (deterministic tests).
     */
    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn);

    /**
     * @brief Override refresh-token revocation dispatch used by handleLogout().
     *
     * Enables wiring a concrete revocation endpoint integration without changing
     * the logout endpoint contract.
     */
    void setRefreshTokenRevocationFn(RefreshTokenRevocationFn fn);

private:
    Config config_;
    std::unique_ptr<auth::OIDCProvider>   oidc_provider_;
    std::unique_ptr<auth::OAuthPKCEFlow>  pkce_flow_;

    /// Injected HTTP POST hook (for testing).
    std::function<std::string(const std::string& url, const std::string& body)>
        http_post_fn_;

    /// Injected random-bytes hook (for testing).
    std::function<void(unsigned char* buf, std::size_t len)>
        rand_bytes_fn_;

    /// Optional refresh-token revocation bridge for logout.
    RefreshTokenRevocationFn refresh_token_revocation_fn_;

    // -----------------------------------------------------------------------
    // Pending-state map (state → {code_verifier, expiry})
    // -----------------------------------------------------------------------
    struct PendingEntry {
        std::string code_verifier;
        std::chrono::system_clock::time_point expires_at;
    };
    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, PendingEntry> pending_states_;

    /// Build and persist a new pending entry.  Returns the state value used.
    std::string storePendingState(const std::string& code_verifier,
                                  const std::string& requested_state);

    /// Look up and consume a pending entry.  Returns nullopt if not found/expired.
    std::optional<std::string> consumePendingState(const std::string& state);

    /// Remove entries whose TTL has passed (must be called under pending_mutex_).
    void evictExpiredStates();

    // -----------------------------------------------------------------------
    // PKCE flow helpers
    // -----------------------------------------------------------------------

    /// Build an OAuthPKCEFlow::Config from the discovered OIDC document.
    auth::OAuthPKCEFlow::Config buildPKCEConfig(
        const std::string& redirect_uri_override = "") const;

    /// (Re-)create pkce_flow_ from the latest discovery document, applying
    /// any registered test hooks.
    void ensurePKCEFlow(const std::string& redirect_uri_override = "");

    // -----------------------------------------------------------------------
    // Token response helpers
    // -----------------------------------------------------------------------

    /// Issue the token exchange call and return a normalised JSON response.
    /// Used by handleCallback() and handleTokenExchange().
    nlohmann::json doTokenExchange(const std::string& code,
                                   const std::string& code_verifier,
                                   const std::string& redirect_uri_override = "");

    /// Perform a raw HTTP POST to the token endpoint with the given
    /// URL-encoded form body.  Uses http_post_fn_ if set, otherwise real curl.
    std::string httpPost(const std::string& url, const std::string& body) const;

    // -----------------------------------------------------------------------
    // Static utilities
    // -----------------------------------------------------------------------

    /// Build a standardised error JSON object.
    static nlohmann::json makeError(int status_code, const std::string& message);

    /// URL-encode a string (RFC 3986 unreserved chars pass through).
    static std::string urlEncode(const std::string& input);

    /// Generate a cryptographically random, URL-safe state value.
    static std::string generateState();
};

} // namespace server
} // namespace themis
