/**
 * @file oauth2_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class OAuth2Provider {
public:
    using RefreshTokenRevocationFn = std::function<bool(const std::string& refresh_token)>;
    struct Config {
        auth::OIDCProviderConfig oidc;

        std::string redirect_uri;

        std::size_t max_state_length{256};

        std::chrono::seconds state_ttl{600};

        std::function<std::string(const std::string& access_token)> token_factory;
    };

    /**
     * @brief OAuth2 Provider.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OAuth2Provider(const Config& config);

    ~OAuth2Provider() = default;

    // Non-copyable (owns OIDCProvider state)
    OAuth2Provider(const OAuth2Provider&) = delete;
    OAuth2Provider& operator=(const OAuth2Provider&) = delete;

    // Movable
    OAuth2Provider(OAuth2Provider&&) noexcept = default;
    OAuth2Provider& operator=(OAuth2Provider&&) noexcept = default;

    // -----------------------------------------------------------------------
    // HTTP handlers – each returns a JSON result.
    // On error the JSON contains a "status_code" field with the HTTP status.
    // -----------------------------------------------------------------------

    nlohmann::json handleAuthorize(const std::string& state = "",
                                   const std::string& redirect_uri = "");

    /**
     * @brief Handle Callback.
     * @param[in] code Input parameter.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    nlohmann::json handleCallback(const std::string& code, const std::string& state);

    nlohmann::json handleTokenExchange(const std::string& code,
                                       const std::string& code_verifier,
                                       const std::string& state = "");

    /**
     * @brief Handle Refresh.
     * @param[in] refresh_token Input parameter.
     * @return Return value.
     */
    nlohmann::json handleRefresh(const std::string& refresh_token);

    /**
     * @brief Handle Introspect.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    nlohmann::json handleIntrospect(const std::string& token);

    nlohmann::json handleLogout(const std::string& refresh_token = "");

    // -----------------------------------------------------------------------
    // Testing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Set Discovery Document For Testing.
     * @param[in] doc Input parameter.
     */
    void setDiscoveryDocumentForTesting(const auth::OIDCDiscoveryDocument& doc);

    void setHttpGetForTesting(
        std::function<std::string(const std::string& url)> fn);

    void setHttpPostForTesting(
        std::function<std::string(const std::string& url, const std::string& body)> fn);

    void setRandBytesForTesting(
        std::function<void(unsigned char* buf, std::size_t len)> fn);

    /**
     * @brief Set Refresh Token Revocation Fn.
     * @param[in] fn Input parameter.
     */
    void setRefreshTokenRevocationFn(RefreshTokenRevocationFn fn);

private:
    Config config_;
    std::unique_ptr<auth::OIDCProvider>   oidc_provider_;
    std::unique_ptr<auth::OAuthPKCEFlow>  pkce_flow_;

    std::function<std::string(const std::string& url, const std::string& body)>
        http_post_fn_;

    std::function<void(unsigned char* buf, std::size_t len)>
        rand_bytes_fn_;

    RefreshTokenRevocationFn refresh_token_revocation_fn_;

    // -----------------------------------------------------------------------
    // Pending-state map (state → {code_verifier, expiry})
    // -----------------------------------------------------------------------
    struct PendingEntry {
        std::string code_verifier = {};
        std::chrono::system_clock::time_point expires_at;
    };
    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, PendingEntry> pending_states_;

    /**
     * @brief Store Pending State.
     * @param[in] code_verifier Input parameter.
     * @param[in] requested_state Input parameter.
     * @return Return value.
     */
    std::string storePendingState(const std::string& code_verifier,
                                  const std::string& requested_state);

    /**
     * @brief Consume Pending State.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    std::optional<std::string> consumePendingState(const std::string& state);

    /**
     * @brief Evict Expired States.
     */
    void evictExpiredStates();

    // -----------------------------------------------------------------------
    // PKCE flow helpers
    // -----------------------------------------------------------------------

    auth::OAuthPKCEFlow::Config buildPKCEConfig(
        const std::string& redirect_uri_override = "") const;

    void ensurePKCEFlow(const std::string& redirect_uri_override = "");

    // -----------------------------------------------------------------------
    // Token response helpers
    // -----------------------------------------------------------------------

    nlohmann::json doTokenExchange(const std::string& code,
                                   const std::string& code_verifier,
                                   const std::string& redirect_uri_override = "");

    /**
     * @brief Http Post.
     * @param[in] url Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    std::string httpPost(const std::string& url, const std::string& body) const;

    // -----------------------------------------------------------------------
    // Static utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Make Error.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    static nlohmann::json makeError(int status_code, const std::string& message);

    /**
     * @brief Url Encode.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string urlEncode(const std::string& input);

    /**
     * @brief Generate State.
     * @return Return value.
     */
    static std::string generateState();
};

} // namespace server
} // namespace themis
