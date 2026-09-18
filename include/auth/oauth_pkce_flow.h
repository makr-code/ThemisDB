/**
 * @file oauth_pkce_flow.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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
#include <functional>

namespace themis {
namespace auth {

class OAuthPKCEFlow {
public:
    struct PKCEChallenge {
        std::string code_verifier;                ///< Keep secret; used at token exchange
        std::string code_challenge;               ///< Sent to authorization endpoint
        std::string challenge_method{"S256"};     ///< Hash method (always S256)
    };

    struct TokenResponse {
        std::string access_token;
        std::string token_type;
        int         expires_in{0};
        std::string refresh_token;
        std::string scope = {};
        std::string id_token;   ///< OIDC id_token (when "openid" scope requested)
    };

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
     * @brief OAuth PKCEFlow.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OAuthPKCEFlow(const Config& config);

    /**
     * @brief Generate Challenge.
     * @return Return value.
     */
    PKCEChallenge generateChallenge();

    std::string buildAuthorizationUrl(const PKCEChallenge& challenge,
                                      const std::string& state = "") const;

    /**
     * @brief Exchange Code.
     * @param[in] authorization_code Input parameter.
     * @param[in] code_verifier Input parameter.
     * @return Return value.
     */
    TokenResponse exchangeCode(const std::string& authorization_code,
                               const std::string& code_verifier);

    /**
     * @brief Validate Id Token.
     * @param[in] token_response Input parameter.
     * @return Return value.
     */
    JWTClaims validateIdToken(const TokenResponse& token_response);

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    void setHttpPostForTesting(
        std::function<std::string(const std::string& url, const std::string& body)> fn
    );

    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn
    );

private:
    Config config_;
    std::function<std::string(const std::string& url, const std::string& body)> http_post_fn_;
    std::function<void(unsigned char* buf, std::size_t len)> rand_bytes_fn_;

    /**
     * @brief Http Post.
     * @param[in] url Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    std::string httpPost(const std::string& url, const std::string& body);
    /**
     * @brief Fill Random Bytes.
     * @param[in,out] buf Input/output parameter.
     * @param[in] len Input parameter.
     */
    void        fillRandomBytes(unsigned char* buf, std::size_t len);

    /**
     * @brief Base64 Url Encode.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     */
    static std::string base64UrlEncode(const unsigned char* data, std::size_t len);
    /**
     * @brief Sha256.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string sha256(const std::string& input);
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
