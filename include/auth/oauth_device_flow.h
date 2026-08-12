/**
 * @file oauth_device_flow.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
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
#include <chrono>
#include <functional>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

/**
 * @brief OAuth 2.0 Device Authorization Grant (RFC 8628) authenticator
 *
 * Implements the Device Authorization Grant flow for headless devices,
 * CLI tools, and IoT clients that cannot open a browser.
 *
 * Flow:
 *   1. Call requestDeviceCode() to obtain a device code and user code.
 *   2. Display verification_uri and user_code to the user.
 *   3. Call pollForToken() repeatedly (at `interval` second intervals)
 *      until authorization is granted, denied, or expires.
 *
 * Compliance: RFC 8628 – OAuth 2.0 Device Authorization Grant
 *
 * Security considerations:
 *   - Device codes must be treated as short-lived secrets.
 *   - Polling interval is enforced; slow_down errors increase it.
 *   - Authorization server TLS certificates are always verified.
 *   - access_token and refresh_token are not cached internally.
 */
class OAuthDeviceFlow {
public:
    /**
     * @brief Response from the device authorization endpoint (RFC 8628 §3.2)
     */
    struct DeviceCodeResponse {
        std::string device_code;              ///< Opaque device code for polling
        std::string user_code;                ///< Human-readable code (e.g., "BDWP-HQMF")
        std::string verification_uri;         ///< URL user must visit
        std::string verification_uri_complete; ///< Pre-filled URL for QR code
        int expires_in{600};                  ///< Device code lifetime in seconds
        int interval{5};                      ///< Minimum polling interval in seconds
    };

    /**
     * @brief Result of a token poll attempt
     */
    enum class PollStatus {
        Authorized,         ///< Token granted; claims populated
        AuthorizationPending, ///< User has not yet authorized
        SlowDown,           ///< Server requests slower polling
        AccessDenied,       ///< User denied authorization
        ExpiredToken,       ///< Device code expired
        Error               ///< Unexpected error
    };

    /**
     * @brief Token response from the token endpoint
     */
    struct TokenResponse {
        std::string access_token;
        std::string token_type;
        int expires_in{0};
        std::string refresh_token;
        std::string scope;
        std::string id_token;  ///< OIDC id_token (if requested)
    };

    /**
     * @brief Configuration for the device flow
     */
    struct Config {
        std::string device_authorization_endpoint; ///< RFC 8628 device auth endpoint
        std::string token_endpoint;                ///< OAuth token endpoint
        std::string client_id;                     ///< Registered OAuth client ID
        std::string client_secret;                 ///< Client secret (empty for public clients)
        std::vector<std::string> scopes;           ///< Requested scopes (e.g., {"openid","email"})
        std::string jwks_url;                      ///< JWKS URL for id_token validation (optional)
        int http_timeout_seconds{10};              ///< HTTP request timeout
        int max_poll_interval_seconds{30};         ///< Cap on poll interval after slow_down
    };

    /**
     * @brief Construct with explicit endpoint configuration
     *
     * @param config  Device flow configuration
     */
    explicit OAuthDeviceFlow(const Config& config);

    /**
     * @brief Request a device code from the authorization server (RFC 8628 §3.1)
     *
     * Sends a POST to device_authorization_endpoint with client_id and scope,
     * then parses and returns the DeviceCodeResponse.
     *
     * @return DeviceCodeResponse with user_code and verification_uri
     * @throws AuthException (AUTH_INTERNAL_ERROR) on HTTP or parse failure
     */
    DeviceCodeResponse requestDeviceCode();

    /**
     * @brief Poll the token endpoint for an access token (RFC 8628 §3.4)
     *
     * @param device_code  Device code from requestDeviceCode()
     * @param status_out   Set to the PollStatus result
     * @return TokenResponse if status_out == Authorized, empty otherwise
     * @throws AuthException on authorization_denied or expired_token
     */
    TokenResponse pollForToken(const std::string& device_code, PollStatus& status_out);

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

    /**
     * @brief Run the complete device flow with optional progress callback
     *
     * Calls requestDeviceCode(), then polls until authorized, denied, or expired.
     * Calls progress_cb after each requestDeviceCode() so the caller can display
     * the user code and verification URI.
     *
     * @param progress_cb  Called once with the DeviceCodeResponse (may be nullptr)
     * @return Validated JWTClaims on success
     * @throws AuthException on denial, expiry, or error
     */
    JWTClaims authenticate(
        std::function<void(const DeviceCodeResponse&)> progress_cb = nullptr
    );

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
     * @brief Attach an AuditLogger to receive TOKEN_CREATED / UNAUTHORIZED_ACCESS events.
     * Pass nullptr to detach.  The flow does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

private:
    Config config_;
    std::function<std::string(const std::string& url, const std::string& body)> http_post_fn_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning, optional.

    std::string httpPost(const std::string& url, const std::string& body);

    static std::string urlEncode(const std::string& value);
    static std::string buildFormBody(
        const std::vector<std::pair<std::string, std::string>>& params
    );
};

} // namespace auth
} // namespace themis
