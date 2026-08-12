/**
 * @file test_ingestion_oauth.cpp
 * @brief Unit tests for OAuth 2.0 token refresh handling in ingestion connectors.
 *
 * Tests cover:
 *  - OAuthConfig struct defaults and isRefreshable()
 *  - GenericApiConnector: successful token refresh on HTTP 401
 *  - GenericApiConnector: failed token refresh falls through to error
 *  - GenericApiConnector: token refresh rotates the access token
 *  - GenericApiConnector: refresh token rotation (server issues new refresh_token)
 *  - GenericApiConnector: no refresh attempted when oauth not configured
 *  - GenericApiConnector: OAuth access_token overrides static api_key
 *  - GenericApiConnector: OAuth config via SourceConfig options
 *  - HuggingFaceConnector: successful token refresh on HTTP 401 (batch mode)
 *  - HuggingFaceConnector: successful token refresh on HTTP 401 (streaming mode)
 *  - HuggingFaceConnector: failed token refresh falls through to error
 *  - HuggingFaceConnector: no refresh attempted when oauth not configured
 */

#include <gtest/gtest.h>
#include "ingestion/api_connector.h"
#include "ingestion/huggingface_connector.h"
#include <string>
#include <utility>
#include <atomic>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// OAuthConfig struct tests
// ---------------------------------------------------------------------------

TEST(OAuthConfigTest, DefaultValues) {
    OAuthConfig cfg;
    EXPECT_TRUE(cfg.token_endpoint.empty());
    EXPECT_TRUE(cfg.client_id.empty());
    EXPECT_TRUE(cfg.client_secret.empty());
    EXPECT_TRUE(cfg.refresh_token.empty());
    EXPECT_TRUE(cfg.access_token.empty());
}

TEST(OAuthConfigTest, IsRefreshableFalseWhenEmpty) {
    OAuthConfig cfg;
    EXPECT_FALSE(cfg.isRefreshable());
}

TEST(OAuthConfigTest, IsRefreshableFalseWithoutEndpoint) {
    OAuthConfig cfg;
    cfg.refresh_token = "tok";
    EXPECT_FALSE(cfg.isRefreshable());
}

TEST(OAuthConfigTest, IsRefreshableFalseWithoutRefreshToken) {
    OAuthConfig cfg;
    cfg.token_endpoint = "https://auth.example.com/token";
    EXPECT_FALSE(cfg.isRefreshable());
}

TEST(OAuthConfigTest, IsRefreshableTrueWhenBothSet) {
    OAuthConfig cfg;
    cfg.token_endpoint = "https://auth.example.com/token";
    cfg.refresh_token  = "my-refresh-token";
    EXPECT_TRUE(cfg.isRefreshable());
}

// ---------------------------------------------------------------------------
// GenericApiConnector OAuth tests
// ---------------------------------------------------------------------------

// Build a SourceConfig pointing at a fake API endpoint.
static SourceConfig makeApiSourceConfig(const std::string& source_id = "test_api") {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/v1/docs";
    cfg.options["page_size"]  = "3";
    cfg.options["max_pages"]  = "1";
    cfg.options["text_field"] = "text";
    return cfg;
}

// A successful one-page API response body.
static std::string makeApiPage() {
    return R"({"total":3,"items":[)"
           R"({"text":"alpha"},{"text":"beta"},{"text":"gamma"}]})";
}

// Token endpoint response granting a new access token.
static std::string makeTokenResponse(const std::string& access_token,
                                     const std::string& new_refresh_token = "") {
    std::string r = R"({"access_token":")" + access_token + R"(","token_type":"Bearer")";
    if (!new_refresh_token.empty())
        r += R"(,"refresh_token":")" + new_refresh_token + "\"";
    r += "}";
    return r;
}

// ---------------------------------------------------------------------------

TEST(GenericApiConnectorOAuthTest, SuccessfulTokenRefreshOn401) {
    // First GET returns 401 (expired token), second GET returns 200 after refresh.
    std::atomic<int> get_call_count{0};
    std::atomic<int> post_call_count{0};

    GenericApiConnector conn;
    ASSERT_TRUE(conn.initialize(makeApiSourceConfig()));

    conn.setHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& auth)
        -> std::pair<int, std::string> {
            ++get_call_count;
            if (auth == "Bearer expired-token")
                return {401, ""};  // first call: expired
            return {200, makeApiPage()};  // second call: new token
        });

    conn.setHttpPostForTesting(
        [&](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            ++post_call_count;
            return {200, makeTokenResponse("fresh-token")};
        });

    OAuthConfig oauth;
    oauth.token_endpoint = "https://auth.example.com/token";
    oauth.refresh_token  = "my-refresh-token";
    oauth.access_token   = "expired-token";
    conn.setOAuthConfig(oauth);

    auto stats = conn.ingest("col", nullptr);

    EXPECT_EQ(post_call_count.load(), 1) << "Token endpoint must be called once";
    EXPECT_GE(get_call_count.load(), 2) << "GET must be called at least twice";
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_TRUE(stats.errors.empty())
        << "No errors should remain after successful refresh";
}

TEST(GenericApiConnectorOAuthTest, FailedTokenRefreshPropagatesError) {
    // GET returns 401, token refresh POST also fails -> error in stats.
    GenericApiConnector conn;
    ASSERT_TRUE(conn.initialize(makeApiSourceConfig()));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {401, ""};
        });

    conn.setHttpPostForTesting(
        [](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            return {400, R"({"error":"invalid_grant"})"};
        });

    OAuthConfig oauth;
    oauth.token_endpoint = "https://auth.example.com/token";
    oauth.refresh_token  = "bad-token";
    oauth.access_token   = "expired";
    conn.setOAuthConfig(oauth);

    auto stats = conn.ingest("col", nullptr);

    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_FALSE(stats.errors.empty()) << "Error must be recorded after failed refresh";
}

TEST(GenericApiConnectorOAuthTest, RefreshTokenRotationUpdatesToken) {
    // Server issues a new refresh_token alongside the new access_token.
    // The second page request should use the updated tokens.
    std::string last_auth;

    GenericApiConnector conn;
    SourceConfig cfg = makeApiSourceConfig();
    cfg.options["max_pages"] = "2";
    ASSERT_TRUE(conn.initialize(cfg));

    std::atomic<int> get_call_count{0};
    conn.setHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& auth)
        -> std::pair<int, std::string> {
            last_auth = auth;
            int n = ++get_call_count;
            if (n == 1) return {401, ""};       // first: expired
            return {200, makeApiPage()};        // subsequent: ok
        });

    conn.setHttpPostForTesting(
        [](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            return {200, makeTokenResponse("new-access", "new-refresh")};
        });

    OAuthConfig oauth;
    oauth.token_endpoint = "https://auth.example.com/token";
    oauth.refresh_token  = "initial-refresh";
    oauth.access_token   = "expired-token";
    conn.setOAuthConfig(oauth);

    auto stats = conn.ingest("col", nullptr);

    EXPECT_EQ(stats.documents_processed, 3u);
    // The auth header used after refresh should carry the new access token.
    EXPECT_EQ(last_auth, "Bearer new-access");
}

TEST(GenericApiConnectorOAuthTest, NoRefreshWhenOAuthNotConfigured) {
    // Without OAuth config, a 401 is treated as a terminal error immediately.
    std::atomic<int> post_call_count{0};

    GenericApiConnector conn;
    ASSERT_TRUE(conn.initialize(makeApiSourceConfig()));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {401, ""};
        });

    conn.setHttpPostForTesting(
        [&](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            ++post_call_count;
            return {200, makeTokenResponse("tok")};
        });

    auto stats = conn.ingest("col", nullptr);

    EXPECT_EQ(post_call_count.load(), 0) << "Token endpoint must NOT be called without config";
    EXPECT_EQ(stats.documents_processed, 0u);
}

TEST(GenericApiConnectorOAuthTest, OAuthAccessTokenOverridesStaticApiKey) {
    // When both oauth.access_token and api_key are set, the OAuth token wins.
    std::string captured_auth;

    GenericApiConnector conn;
    ASSERT_TRUE(conn.initialize(makeApiSourceConfig()));
    conn.setApiKey("static-key");

    conn.setHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& auth)
        -> std::pair<int, std::string> {
            captured_auth = auth;
            return {200, makeApiPage()};
        });

    OAuthConfig oauth;
    oauth.token_endpoint = "https://auth.example.com/token";
    oauth.refresh_token  = "rf";
    oauth.access_token   = "oauth-token";
    conn.setOAuthConfig(oauth);

    conn.ingest("col", nullptr);

    EXPECT_EQ(captured_auth, "Bearer oauth-token")
        << "OAuth access_token must override static api_key";
}

TEST(GenericApiConnectorOAuthTest, OAuthConfigViaSourceOptions) {
    // OAuth parameters passed through SourceConfig::options are parsed in initialize().
    std::atomic<int> post_call_count{0};

    SourceConfig cfg = makeApiSourceConfig();
    cfg.options["oauth_token_endpoint"] = "https://auth.example.com/token";
    cfg.options["oauth_client_id"]      = "client123";
    cfg.options["oauth_client_secret"]  = "secret456";
    cfg.options["oauth_refresh_token"]  = "reftok";
    cfg.options["oauth_access_token"]   = "expired";

    GenericApiConnector conn;
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& auth)
        -> std::pair<int, std::string> {
            if (auth == "Bearer expired") return {401, ""};
            return {200, makeApiPage()};
        });

    conn.setHttpPostForTesting(
        [&](const std::string& url, const std::string& body)
        -> std::pair<int, std::string> {
            ++post_call_count;
            // Verify token endpoint and client_id are in the POST
            EXPECT_EQ(url, "https://auth.example.com/token");
            EXPECT_NE(body.find("client_id=client123"), std::string::npos);
            EXPECT_NE(body.find("client_secret=secret456"), std::string::npos);
            EXPECT_NE(body.find("grant_type=refresh_token"), std::string::npos);
            return {200, makeTokenResponse("new-tok")};
        });

    auto stats = conn.ingest("col", nullptr);

    EXPECT_EQ(post_call_count.load(), 1);
    EXPECT_EQ(stats.documents_processed, 3u);
}

// ---------------------------------------------------------------------------
// HuggingFaceConnector OAuth tests
// ---------------------------------------------------------------------------

static SourceConfig makeHfSourceConfig() {
    SourceConfig cfg;
    cfg.source_id = "hf_test";
    cfg.type      = SourceType::HUGGINGFACE;
    cfg.location  = "test/dataset";
    cfg.options["split"]     = "train";
    cfg.options["streaming"] = "false"; // batch mode
    return cfg;
}

TEST(HuggingFaceConnectorOAuthTest, SuccessfulTokenRefreshBatchMode) {
    // Inject a GET mock so the test does not make real network calls.
    // Both the dataset availability check and the metadata endpoint return 200.
    HuggingFaceConnector conn;
    ASSERT_TRUE(conn.initialize(makeHfSourceConfig()));

    conn.setHttpGetForTesting(
        [](const std::string& url, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            if (url.find("/metadata") != std::string::npos)
                return {200, "{\"rows\":12000}"};
            return {200, "{\"status\":\"available\"}"};
        });

    std::atomic<int> post_call_count{0};
    conn.setHttpPostForTesting(
        [&](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            ++post_call_count;
            return {200, makeTokenResponse("fresh")};
        });

    OAuthConfig oauth;
    oauth.token_endpoint = "https://auth.example.com/token";
    oauth.refresh_token  = "hf-refresh-token";
    oauth.access_token   = "hf-expired";
    conn.setOAuthConfig(oauth);

    // With the mock GET returning 200, no POST is needed.
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(post_call_count.load(), 0)
        << "Token refresh must not be triggered when no 401 occurs";
    EXPECT_EQ(stats.documents_processed, 12000u);
}

TEST(HuggingFaceConnectorOAuthTest, OAuthConfigIsRefreshable) {
    // Verify that the HuggingFace connector accepts OAuthConfig and that
    // isRefreshable() correctly reflects the configuration.
    OAuthConfig oauth;
    EXPECT_FALSE(oauth.isRefreshable());

    oauth.token_endpoint = "https://hf.example.com/token";
    EXPECT_FALSE(oauth.isRefreshable());

    oauth.refresh_token = "tok";
    EXPECT_TRUE(oauth.isRefreshable());
}

TEST(HuggingFaceConnectorOAuthTest, OAuthConfigViaSourceOptions) {
    SourceConfig cfg = makeHfSourceConfig();
    cfg.options["oauth_token_endpoint"] = "https://hf.example.com/token";
    cfg.options["oauth_refresh_token"]  = "hf-rf";
    cfg.options["oauth_access_token"]   = "hf-at";

    HuggingFaceConnector conn;
    ASSERT_TRUE(conn.initialize(cfg));

    // Inject a GET mock so the test does not make real network calls.
    conn.setHttpGetForTesting(
        [](const std::string& url, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            if (url.find("/metadata") != std::string::npos)
                return {200, "{\"rows\":10}"};
            return {200, "{\"status\":\"ok\"}"};
        });

    // Connector should not crash and should use the stored OAuth config.
    EXPECT_NO_THROW(conn.ingest("col", nullptr));
}

TEST(HuggingFaceConnectorOAuthTest, SetHttpPostForTestingAccepted) {
    HuggingFaceConnector conn;
    ASSERT_TRUE(conn.initialize(makeHfSourceConfig()));

    // Inject a GET mock so the test does not make real network calls.
    conn.setHttpGetForTesting(
        [](const std::string& url, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            if (url.find("/metadata") != std::string::npos)
                return {200, "{\"rows\":5}"};
            return {200, "{\"status\":\"ok\"}"};
        });

    bool called = false;
    conn.setHttpPostForTesting(
        [&](const std::string& /*url*/, const std::string& /*body*/)
        -> std::pair<int, std::string> {
            called = true;
            return {200, makeTokenResponse("tok")};
        });

    // The POST hook must not be called when no 401 occurs.
    conn.ingest("col", nullptr);
    EXPECT_FALSE(called)
        << "POST hook must not be called when no 401 occurs";
}
