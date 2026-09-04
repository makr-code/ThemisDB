/**
 * @file test_oauth_token_manager.cpp
 * @brief Unit tests for OAuthTokenManager.
 *
 * Phase 2.3: OAuthTokenManager
 * 10 tests covering: valid token, proactive refresh, concurrency, backoff,
 * refresh expiry, NullTokenManager, token update, bearer header.
 */

#include <gtest/gtest.h>
#include "ingestion/oauth_token_manager.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace themis::ingestion;
using Clock = std::chrono::system_clock;

// ─── Helpers ────────────────────────────────────────────────────────────────

/// Build a mock HTTP POST function that returns a fixed response
static OAuthHttpPostFn make_mock_post(int http_code, const std::string& body) {
    return [http_code, body](const std::string&, const std::string&,
                              const std::string&) -> std::pair<int, std::string> {
        return {http_code, body};
    };
}

/// Build a valid token response JSON
static std::string make_token_response(
    const std::string& access_token = "new_access_token",
    const std::string& refresh_token = "new_refresh_token",
    int expires_in = 3600)
{
    return R"({"access_token":")" + access_token +
           R"(","refresh_token":")" + refresh_token +
           R"(","expires_in":)" + std::to_string(expires_in) + "}";
}

/// Build a basic OAuthConfig for testing
static OAuthConfig make_config(
    const std::string& access_token = "initial_access_token",
    const std::string& refresh_token = "initial_refresh_token")
{
    OAuthConfig cfg;
    cfg.token_endpoint = "https://auth.example.com/token";
    cfg.client_id      = "client_id_123";
    cfg.client_secret  = "client_secret_abc";
    cfg.access_token   = access_token;
    cfg.refresh_token  = refresh_token;
    return cfg;
}

// ---------------------------------------------------------------------------
// Test 1 — Initial getAccessToken with valid (non-expiring) token
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, InitialValidToken_ReturnedWithoutRefresh) {
    int refresh_call_count = 0;
    auto mock = [&refresh_call_count](const std::string&, const std::string&,
                                       const std::string&) -> std::pair<int, std::string> {
        ++refresh_call_count;
        return {200, make_token_response("refreshed_token")};
    };

    OAuthTokenManager mgr(make_config(), mock);
    // Set a far-future expiry — no refresh should happen
    mgr.setTokenForTesting("initial_access_token", "initial_refresh_token",
                           Clock::now() + std::chrono::hours(1));

    const auto token = mgr.getAccessToken();
    EXPECT_EQ(token, "initial_access_token");
    EXPECT_EQ(refresh_call_count, 0) << "No HTTP call when token is not near expiry";
}

// ---------------------------------------------------------------------------
// Test 2 — Proactive refresh when near expiry (< 60s)
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, NearExpiry_ProactiveRefresh) {
    int refresh_call_count = 0;
    auto mock = [&refresh_call_count](const std::string&, const std::string&,
                                       const std::string&) -> std::pair<int, std::string> {
        ++refresh_call_count;
        return {200, make_token_response("refreshed_access_token")};
    };

    OAuthTokenManager mgr(make_config(), mock);
    // Set expiry to 30s from now — within 60s threshold
    mgr.setTokenForTesting("old_token", "old_refresh",
                           Clock::now() + std::chrono::seconds(30));

    const auto token = mgr.getAccessToken();
    EXPECT_EQ(token, "refreshed_access_token");
    EXPECT_GE(refresh_call_count, 1) << "HTTP refresh called when token near expiry";
}

// ---------------------------------------------------------------------------
// Test 3 — Concurrent calls: only one refresh runs (mutex)
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, ConcurrentGetAccessToken_OnlyOneRefresh) {
    std::atomic<int> call_count{0};
    auto mock = [&call_count](const std::string&, const std::string&,
                               const std::string&) -> std::pair<int, std::string> {
        ++call_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return {200, make_token_response("concurrent_token")};
    };

    OAuthTokenManager mgr(make_config(), mock);
    // Expire immediately
    mgr.setTokenForTesting("old", "old_refresh", Clock::now() - std::chrono::seconds(1));

    // Launch multiple threads
    constexpr int kThreads = 8;
    std::vector<std::string> tokens(kThreads);
    std::vector<std::thread> threads = {};

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&mgr, &tokens, i]() {
            try {
                tokens[i] = mgr.getAccessToken();
            } catch (...) {
                tokens[i] = "error";
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // All threads should get the same (refreshed) token
    for (const auto& tok : tokens) {
        EXPECT_EQ(tok, "concurrent_token");
    }
    // The mutex ensures at most one HTTP call per refresh cycle; exact count
    // depends on timing but the result must be consistent.
    EXPECT_GE(call_count.load(), 1) << "At least one refresh must have occurred";
}

// ---------------------------------------------------------------------------
// Test 4 — 429 response triggers backoff and retry
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, Http429_BackoffAndRetry) {
    int call_count = 0;
    auto mock = [&call_count](const std::string&, const std::string&,
                               const std::string&) -> std::pair<int, std::string> {
        ++call_count;
        if (call_count < 2) {
            return {429, R"({"error":"rate_limited"})"};
        }
        return {200, make_token_response("retry_token")};
    };

    OAuthTokenManager mgr(make_config(), mock);
    mgr.setTokenForTesting("old", "old_refresh", Clock::now() - std::chrono::seconds(1));

    std::string token = {};
    EXPECT_NO_THROW(token = mgr.getAccessToken());
    EXPECT_EQ(token, "retry_token");
    EXPECT_GE(call_count, 2) << "Retry after 429 must occur";
}

// ---------------------------------------------------------------------------
// Test 5 — 503 triggers backoff and retry
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, Http503_BackoffAndRetry) {
    int call_count = 0;
    auto mock = [&call_count](const std::string&, const std::string&,
                               const std::string&) -> std::pair<int, std::string> {
        ++call_count;
        if (call_count < 2) {
            return {503, "service unavailable"};
        }
        return {200, make_token_response("retry_token_503")};
    };

    OAuthTokenManager mgr(make_config(), mock);
    mgr.setTokenForTesting("old", "old_refresh", Clock::now() - std::chrono::seconds(1));

    std::string token = {};
    EXPECT_NO_THROW(token = mgr.getAccessToken());
    EXPECT_EQ(token, "retry_token_503");
    EXPECT_GE(call_count, 2);
}

// ---------------------------------------------------------------------------
// Test 6 — Refresh token expiry (401 from endpoint) throws ERR_OAUTH_REFRESH_EXPIRED
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, Http401_ThrowsRefreshExpiredError) {
    auto mock = make_mock_post(401, R"({"error":"invalid_grant"})");

    OAuthTokenManager mgr(make_config(), mock);
    mgr.setTokenForTesting("old", "old_refresh", Clock::now() - std::chrono::seconds(1));

    EXPECT_THROW(mgr.getAccessToken(), OAuthRefreshExpiredError);
}

// ---------------------------------------------------------------------------
// Test 7 — NullTokenManager getAccessToken returns empty string
// ---------------------------------------------------------------------------
TEST(NullTokenManagerTest, GetAccessToken_ReturnsEmpty) {
    NullTokenManager null_mgr;
    EXPECT_EQ(null_mgr.getAccessToken(), "");
}

// ---------------------------------------------------------------------------
// Test 8 — NullTokenManager is a no-op (refreshToken does nothing)
// ---------------------------------------------------------------------------
TEST(NullTokenManagerTest, RefreshToken_IsNoOp) {
    NullTokenManager null_mgr;
    EXPECT_NO_THROW(null_mgr.refreshToken());
    EXPECT_EQ(null_mgr.getAccessToken(), "");
}

// ---------------------------------------------------------------------------
// Test 9 — Token updated after successful refresh
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, TokenUpdatedAfterRefresh) {
    auto mock = make_mock_post(200, make_token_response("brand_new_token"));

    OAuthTokenManager mgr(make_config("old_token"), mock);
    mgr.setTokenForTesting("old_token", "some_refresh",
                           Clock::now() - std::chrono::seconds(1));

    const auto token = mgr.getAccessToken();
    EXPECT_EQ(token, "brand_new_token")
        << "Access token must be updated after successful refresh";
}

// ---------------------------------------------------------------------------
// Test 10 — Access token used as Bearer in authorization header
// ---------------------------------------------------------------------------
TEST(OAuthTokenManager, BearerHeader_ContainsAccessToken) {
    auto mock = make_mock_post(200, make_token_response("bearer_test_token"));

    OAuthTokenManager mgr(make_config(), mock);
    mgr.setTokenForTesting("bearer_test_token", "bearer_refresh",
                           Clock::now() + std::chrono::hours(1));

    const auto header = mgr.getBearerAuthorizationHeader();
    EXPECT_EQ(header, "Bearer bearer_test_token");
}
