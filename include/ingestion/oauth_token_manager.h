#pragma once

/**
 * @file oauth_token_manager.h
 * @brief OAuth 2.0 token manager with proactive refresh and backoff.
 *
 * Phase 2.3: OAuthTokenManager
 *
 * Provides:
 *   - IOAuthTokenManager interface
 *   - OAuthTokenManager: automatic proactive refresh, mutex-guarded, backoff on 429/503
 *   - NullTokenManager: no-op implementation for testing / disabled auth
 */

#include "ingestion/ingestion_manager.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include &lt;optional&gt;
#include <stdexcept>
#include <string>

namespace themis {
namespace ingestion {

// ─── Error codes ──────────────────────────────────────────────────────────

/**
 * @brief Thrown when the refresh token has expired (HTTP 401 from token endpoint).
 */
class OAuthRefreshExpiredError : public std::runtime_error {
public:
    explicit OAuthRefreshExpiredError(const std::string& msg = "OAuth refresh token expired")
        : std::runtime_error(msg) {}
};

// ─── Interface ────────────────────────────────────────────────────────────

/**
 * @brief Interface for OAuth access-token providers.
 */
class IOAuthTokenManager {
public:
    virtual ~IOAuthTokenManager() = default;

    /**
     * @brief Returns the current access token, refreshing if necessary.
     *
     * If the token expires within 60 seconds, a refresh is triggered first.
     *
     * @throws OAuthRefreshExpiredError when the refresh token has expired (401).
     * @throws std::runtime_error on persistent refresh failures.
     */
    virtual std::string getAccessToken() = 0;

    /**
     * @brief Force an immediate token refresh.
     *
     * @throws OAuthRefreshExpiredError on 401 from the token endpoint.
     * @throws std::runtime_error on persistent failures after retries.
     */
    virtual void refreshToken() = 0;

    /**
     * @brief Returns the current access token as a Bearer authorization header value.
     *
     * Format: "Bearer <access_token>"
     */
    virtual std::string getBearerAuthorizationHeader() = 0;
};

// ─── HttpPostFn (injectable for testing) ──────────────────────────────────

/**
 * @brief Function signature for HTTP POST requests to the token endpoint.
 *
 * Returns {http_status_code, response_body}.
 * Injectable for testing without real network calls.
 */
using OAuthHttpPostFn = std::function<std::pair<int, std::string>(
    const std::string& url,
    const std::string& body,
    const std::string& content_type)>;

// ─── OAuthTokenManager ────────────────────────────────────────────────────

/**
 * @brief Production OAuth 2.0 token manager.
 *
 * Features:
 *   - Proactive refresh: triggers refresh when token expires within 60 seconds.
 *   - Mutex-guarded: only one concurrent refresh runs.
 *   - Exponential backoff with jitter on HTTP 429/503 (2 retries).
 *   - ERR_OAUTH_REFRESH_EXPIRED exception on HTTP 401 from token endpoint.
 *   - Injectable HTTP POST function for unit testing.
 */
class OAuthTokenManager final : public IOAuthTokenManager {
public:
    /// Seconds before expiry at which proactive refresh is triggered.
    static constexpr int kRefreshThresholdSecs = 60;
    /// Maximum retries on 429/503 responses.
    static constexpr int kMaxRetries = 2;
    /// Base backoff in milliseconds (doubled each retry, plus jitter).
    static constexpr int kBaseBackoffMs = 500;

    /**
     * @brief Construct with OAuth configuration.
     *
     * @param config     OAuth endpoint / credentials configuration.
     * @param http_post  Injectable HTTP POST function (defaults to real libcurl).
     */
    explicit OAuthTokenManager(
        OAuthConfig                     config,
        std::optional<OAuthHttpPostFn>  http_post = std::nullopt);

    ~OAuthTokenManager() override = default;

    OAuthTokenManager(const OAuthTokenManager&)            = delete;
    OAuthTokenManager& operator=(const OAuthTokenManager&) = delete;

    std::string getAccessToken() override;
    void        refreshToken()   override;
    std::string getBearerAuthorizationHeader() override;

    /// Returns the timestamp when the current access token expires.
    std::chrono::system_clock::time_point getExpiryTime() const;

    /// Direct token injection (for testing).
    void setTokenForTesting(const std::string& access_token,
                            const std::string& refresh_token,
                            std::chrono::system_clock::time_point expiry);

private:
    mutable std::mutex mutex_;
    OAuthConfig        config_;
    OAuthHttpPostFn    http_post_;

    std::chrono::system_clock::time_point expiry_time_;

    bool isNearExpiry() const;
    std::pair<int, std::string> doHttpPost(const std::string& body);
    void parseTokenResponse(const std::string& body);
};

// ─── NullTokenManager ─────────────────────────────────────────────────────

/**
 * @brief No-op token manager for testing and disabled-auth scenarios.
 *
 * `getAccessToken()` always returns an empty string.
 * `refreshToken()` is a no-op.
 */
class NullTokenManager final : public IOAuthTokenManager {
public:
    NullTokenManager() = default;
    ~NullTokenManager() override = default;

    std::string getAccessToken() override { return ""; }
    void        refreshToken()   override {}
    std::string getBearerAuthorizationHeader() override { return "Bearer "; }
};

} // namespace ingestion
} // namespace themis
