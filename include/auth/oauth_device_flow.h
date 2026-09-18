/**
 * @file oauth_device_flow.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class OAuthDeviceFlow {
public:
    struct DeviceCodeResponse {
        std::string device_code;              ///< Opaque device code for polling
        std::string user_code;                ///< Human-readable code (e.g., "BDWP-HQMF")
        std::string verification_uri;         ///< URL user must visit
        std::string verification_uri_complete; ///< Pre-filled URL for QR code
        int expires_in{600};                  ///< Device code lifetime in seconds
        int interval{5};                      ///< Minimum polling interval in seconds
    };

    enum class PollStatus {
        Authorized,         ///< Token granted; claims populated
        AuthorizationPending, ///< User has not yet authorized
        SlowDown,           ///< Server requests slower polling
        AccessDenied,       ///< User denied authorization
        ExpiredToken,       ///< Device code expired
        Error               ///< Unexpected error
    };

    struct TokenResponse {
        std::string access_token;
        std::string token_type;
        int expires_in{0};
        std::string refresh_token;
        std::string scope;
        std::string id_token;  ///< OIDC id_token (if requested)
    };

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
     * @brief OAuth Device Flow.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OAuthDeviceFlow(const Config& config);

    /**
     * @brief Request Device Code.
     * @return Return value.
     */
    DeviceCodeResponse requestDeviceCode();

    /**
     * @brief Poll For Token.
     * @param[in] device_code Input parameter.
     * @param[in,out] status_out Input/output parameter.
     * @return Return value.
     */
    TokenResponse pollForToken(const std::string& device_code, PollStatus& status_out);

    /**
     * @brief Validate Id Token.
     * @param[in] token_response Input parameter.
     * @return Return value.
     */
    JWTClaims validateIdToken(const TokenResponse& token_response);

    JWTClaims authenticate(
        std::function<void(const DeviceCodeResponse&)> progress_cb = nullptr
    );

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    void setHttpPostForTesting(
        std::function<std::string(const std::string& url, const std::string& body)> fn
    );

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

private:
    Config config_;
    std::function<std::string(const std::string& url, const std::string& body)> http_post_fn_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning, optional.

    /**
     * @brief Http Post.
     * @param[in] url Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    std::string httpPost(const std::string& url, const std::string& body);

    /**
     * @brief Url Encode.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static std::string urlEncode(const std::string& value);
    static std::string buildFormBody(
        const std::vector<std::pair<std::string, std::string>>& params
    );
};

} // namespace auth
} // namespace themis
