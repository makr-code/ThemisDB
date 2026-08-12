/**
 * @file oauth_pkce_flow.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/jwt_validator.h"
#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace themis {
namespace auth {

/**
 * @brief OAuth 2.0 Authorization Code Grant with PKCE (RFC 7636) for public clients
 *
 * Implements Proof Key for Code Exchange to prevent authorization code
 * interception attacks in native/mobile/SPA applications that cannot
 * safely store a client secret.
 *
 * Flow:
 *   1. Call generateChallenge() to create a code_verifier / code_challenge pair.
 *   2. Redirect (or instruct) the user to authorization_endpoint appended with
 *      the query parameters returned by buildAuthorizationUrl().
 *   3. Receive the authorization_code from the redirect URI (out of band).
 *   4. Call exchangeCode() with the authorization_code and the code_verifier
 *      from step 1 to obtain tokens.
 *   5. Optionally call validateIdToken() to extract and verify identity claims.
 *
 * Compliance: RFC 7636 – Proof Key for Code Exchange
 *             RFC 6749 – OAuth 2.0 Authorization Code Grant
 *
 * Security considerations:
 *   - code_verifier is generated with CSPRNG (OpenSSL RAND_bytes).
 *   - Only S256 (SHA-256) challenge method is supported; "plain" is rejected.
 *   - code_verifier is never transmitted to the authorization endpoint.
 *   - Authorization server TLS certificates are always verified.
 *   - access_token and refresh_token are not cached internally.
 */
class OAuthPKCEFlow {
public:
    /**
     * @brief PKCE code verifier / challenge pair (RFC 7636 §4.1, §4.2)
     *
     * code_verifier  – cryptographically random, 43–128 URL-safe characters.
     * code_challenge – BASE64URL(SHA256(ASCII(code_verifier))).
     * challenge_method – always "S256".
     */
    struct PKCEChallenge {
        std::string code_verifier;                ///< Keep secret; used at token exchange
        std::string code_challenge;               ///< Sent to authorization endpoint
        std::string challenge_method{"S256"};     ///< Hash method (always S256)
    };

    /**
     * @brief Token response from the token endpoint
     */
    struct TokenResponse {
        std::string access_token;
        std::string token_type;
        int         expires_in{0};
        std::string refresh_token;
        std::string scope;
        std::string id_token;   ///< OIDC id_token (when "openid" scope requested)
    };

    /**
     * @brief Configuration for the PKCE flow
     */
    struct Config {
        std::string authorization_endpoint;    ///< Authorization endpoint URL
        std::string token_endpoint;            ///< Token endpoint URL
        std::string client_id;                 ///< Registered public client ID
        std::string redirect_uri;              ///< Registered redirect URI
        std::vector<std::string> scopes;       ///< Requested scopes (e.g., {"openid","email"})
        std::string jwks_url;                  ///< JWKS URL for id_token validation (optional)
        int http_timeout_seconds{10};          ///< HTTP request timeout
    };

    /**
     * @brief Construct with explicit endpoint configuration
     *
     * @param config  PKCE flow configuration
     * @throws AuthException (AUTH_CONFIG_INVALID) if required fields are missing
     */
    explicit OAuthPKCEFlow(const Config& config);

    /**
     * @brief Generate a PKCE code_verifier and compute the code_challenge (RFC 7636 §4)
     *
     * Generates 96 random bytes, encodes them as Base64URL (giving 128 URL-safe
     * characters) to obtain the code_verifier, then computes
     * code_challenge = BASE64URL(SHA256(code_verifier)).
     *
     * @return PKCEChallenge containing verifier, challenge, and method
     * @throws AuthException (AUTH_INTERNAL_ERROR) on CSPRNG or hashing failure
     */
    PKCEChallenge generateChallenge();

    /**
     * @brief Build the full authorization URL the user should be redirected to
     *
     * @param challenge   PKCEChallenge from generateChallenge()
     * @param state       Optional opaque CSRF-protection state value
     * @return Full URL with query parameters appended
     */
    std::string buildAuthorizationUrl(const PKCEChallenge& challenge,
                                      const std::string& state = "") const;

    /**
     * @brief Exchange an authorization code for tokens (RFC 7636 §4.5)
     *
     * Posts to the token_endpoint with grant_type=authorization_code,
     * code, redirect_uri, client_id, and code_verifier.
     *
     * @param authorization_code  Code received at the redirect URI
     * @param code_verifier       code_verifier from the matching PKCEChallenge
     * @return TokenResponse containing access_token (and id_token if openid scope used)
     * @throws AuthException on HTTP error, server error response, or parse failure
     */
    TokenResponse exchangeCode(const std::string& authorization_code,
                               const std::string& code_verifier);

    /**
     * @brief Validate the id_token from a TokenResponse and extract JWTClaims
     *
     * Requires jwks_url to be set in Config.
     *
     * @param token_response  Token response containing id_token
     * @return Validated JWTClaims
     * @throws AuthException if id_token is missing, invalid, or fails validation
     */
    JWTClaims validateIdToken(const TokenResponse& token_response);

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Override the HTTP POST function for unit testing
     *
     * The injected function must accept a URL and a URL-encoded body and
     * return the raw HTTP response body (or throw on transport error).
     */
    void setHttpPostForTesting(
        std::function<std::string(const std::string& url, const std::string& body)> fn
    );

    /**
     * @brief Override the random-bytes source for deterministic unit testing
     *
     * The injected function must fill exactly `len` bytes into `buf`.
     * If not set, OpenSSL RAND_bytes is used.
     */
    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn
    );

private:
    Config config_;
    std::function<std::string(const std::string& url, const std::string& body)> http_post_fn_;
    std::function<void(unsigned char* buf, std::size_t len)> rand_bytes_fn_;

    std::string httpPost(const std::string& url, const std::string& body);
    void        fillRandomBytes(unsigned char* buf, std::size_t len);

    static std::string base64UrlEncode(const unsigned char* data, std::size_t len);
    static std::string sha256(const std::string& input);
    static std::string urlEncode(const std::string& value);
    static std::string buildFormBody(
        const std::vector<std::pair<std::string, std::string>>& params
    );
};

} // namespace auth
} // namespace themis
