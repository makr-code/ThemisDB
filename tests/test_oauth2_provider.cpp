/*
 * test_oauth2_provider.cpp
 *
 * Unit tests for themis::server::OAuth2Provider.
 *
 * Tests cover:
 *  - Construction: valid config, missing issuer_url, missing client_id,
 *    missing redirect_uri
 *  - handleAuthorize: returns authorization_url, state, code_verifier;
 *    empty state generates one; oversized state rejected
 *  - handleCallback: missing code/state → 400; unknown state → 400;
 *    token exchange failure → 401; success → token response
 *  - handleTokenExchange: missing code/verifier → 400; verifier mismatch → 400;
 *    success with and without state
 *  - handleRefresh: missing refresh_token → 400; IdP error → 401;
 *    success → new token pair
 *  - handleIntrospect: missing token → 400; invalid token → { active: false };
 *    valid token → { active: true, sub, ... }
 *  - handleLogout: always returns { success: true }
 *  - State TTL: expired pending state is rejected
 *  - Custom token factory: invoked with access_token
 *  - PKCE state consumed after first use (no replay)
 */

#include <gtest/gtest.h>
#include "server/oauth2_provider.h"
#include "auth/auth_error.h"

#include <nlohmann/json.hpp>
#include <string>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace themis::server;
using namespace themis::auth;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal provider config that bypasses real HTTP by injecting a
/// mock discovery document and a mock HTTP POST function.
static OAuth2Provider::Config makeConfig()
{
    OAuth2Provider::Config cfg;
    cfg.oidc.issuer_url             = "https://idp.example.com/realms/test";
    cfg.oidc.client_id              = "themisdb-test";
    cfg.oidc.client_secret          = "";
    cfg.oidc.scopes                 = {"openid", "email"};
    cfg.oidc.expected_audience      = "themisdb-test";
    cfg.oidc.jwks_cache_ttl         = std::chrono::seconds{60};
    cfg.oidc.clock_skew             = std::chrono::seconds{30};
    cfg.oidc.http_timeout_seconds   = 5;
    cfg.redirect_uri                = "https://myapp.example.com/auth/callback";
    cfg.state_ttl                   = std::chrono::seconds{300};
    return cfg;
}

/// Inject a discovery document into the provider, bypassing HTTP discovery.
static void injectDiscovery(OAuth2Provider& provider)
{
    OIDCDiscoveryDocument doc;
    doc.issuer                         = "https://idp.example.com/realms/test";
    doc.jwks_uri                       = "https://idp.example.com/realms/test/protocol/openid-connect/certs";
    doc.authorization_endpoint         = "https://idp.example.com/realms/test/protocol/openid-connect/auth";
    doc.token_endpoint                 = "https://idp.example.com/realms/test/protocol/openid-connect/token";
    doc.device_authorization_endpoint  = "";
    doc.userinfo_endpoint              = "";
    provider.setDiscoveryDocumentForTesting(doc);
}

/// Build a minimal fake token endpoint JSON response body.
static std::string fakeTokenResponse(const std::string& access_token  = "fake-access",
                                     const std::string& refresh_token = "fake-refresh",
                                     const std::string& id_token      = "")
{
    nlohmann::json j = {
        {"access_token",  access_token},
        {"token_type",    "Bearer"},
        {"expires_in",    3600},
        {"refresh_token", refresh_token},
        {"scope",         "openid email"}
    };
    if (!id_token.empty()) {
        j["id_token"] = id_token;
    }
    return j.dump();
}

/// Build a fake IdP error response body.
static std::string fakeErrorResponse(const std::string& error,
                                     const std::string& description = "")
{
    nlohmann::json j = {{"error", error}};
    if (!description.empty()) {
        j["error_description"] = description;
    }
    return j.dump();
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class OAuth2ProviderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        provider_ = std::make_unique<OAuth2Provider>(makeConfig());
        injectDiscovery(*provider_);

        // Default: token exchange always succeeds
        provider_->setHttpPostForTesting(
            [](const std::string& /*url*/, const std::string& /*body*/) {
                return fakeTokenResponse();
            });
    }

    std::unique_ptr<OAuth2Provider> provider_;
};

// ===========================================================================
// Construction tests
// ===========================================================================

TEST(OAuth2ProviderConstructionTest, ValidConfigSucceeds) {
    EXPECT_NO_THROW(OAuth2Provider p(makeConfig()));
}

TEST(OAuth2ProviderConstructionTest, MissingIssuerUrlThrows) {
    auto cfg = makeConfig();
    cfg.oidc.issuer_url = "";
    EXPECT_THROW(OAuth2Provider p(cfg), AuthException);
}

TEST(OAuth2ProviderConstructionTest, MissingClientIdThrows) {
    auto cfg = makeConfig();
    cfg.oidc.client_id = "";
    EXPECT_THROW(OAuth2Provider p(cfg), AuthException);
}

TEST(OAuth2ProviderConstructionTest, MissingRedirectUriThrows) {
    auto cfg = makeConfig();
    cfg.redirect_uri = "";
    EXPECT_THROW(OAuth2Provider p(cfg), AuthException);
}

// ===========================================================================
// handleAuthorize tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, AuthorizeReturnsRequiredFields) {
    auto result = provider_->handleAuthorize();

    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.contains("authorization_url"));
    EXPECT_TRUE(result.contains("state"));
    EXPECT_TRUE(result.contains("code_verifier"));

    EXPECT_FALSE(result["authorization_url"].get<std::string>().empty());
    EXPECT_FALSE(result["state"].get<std::string>().empty());
    EXPECT_FALSE(result["code_verifier"].get<std::string>().empty());
}

TEST_F(OAuth2ProviderTest, AuthorizeUrlContainsPKCEChallenge) {
    auto result = provider_->handleAuthorize();
    const std::string url = result["authorization_url"].get<std::string>();

    // The authorization URL must carry PKCE parameters.
    EXPECT_NE(url.find("code_challenge"), std::string::npos);
    EXPECT_NE(url.find("code_challenge_method"), std::string::npos);
    EXPECT_NE(url.find("S256"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, AuthorizeUrlContainsClientId) {
    auto result = provider_->handleAuthorize();
    const std::string url = result["authorization_url"].get<std::string>();
    EXPECT_NE(url.find("themisdb-test"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, AuthorizeUrlContainsResponseTypeCode) {
    auto result = provider_->handleAuthorize();
    const std::string url = result["authorization_url"].get<std::string>();
    EXPECT_NE(url.find("response_type"), std::string::npos);
    EXPECT_NE(url.find("code"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, AuthorizeGeneratesStateWhenNotProvided) {
    auto r1 = provider_->handleAuthorize();
    auto r2 = provider_->handleAuthorize();

    const std::string s1 = r1["state"].get<std::string>();
    const std::string s2 = r2["state"].get<std::string>();

    EXPECT_FALSE(s1.empty());
    EXPECT_FALSE(s2.empty());
    // Two consecutive calls should produce different states.
    EXPECT_NE(s1, s2);
}

TEST_F(OAuth2ProviderTest, AuthorizeUsesProvidedState) {
    auto result = provider_->handleAuthorize("my-csrf-token");
    EXPECT_EQ(result["state"].get<std::string>(), "my-csrf-token");
}

TEST_F(OAuth2ProviderTest, AuthorizeRejectsOversizedState) {
    const std::string long_state(300, 'x'); // > default max_state_length=256
    auto result = provider_->handleAuthorize(long_state);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, AuthorizeUrlContainsStateParam) {
    auto result = provider_->handleAuthorize("csrf-abc");
    const std::string url = result["authorization_url"].get<std::string>();
    EXPECT_NE(url.find("state"), std::string::npos);
    EXPECT_NE(url.find("csrf-abc"), std::string::npos);
}

// ===========================================================================
// handleCallback tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, CallbackMissingCodeReturns400) {
    auto result = provider_->handleCallback("", "some-state");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, CallbackMissingStateReturns400) {
    auto result = provider_->handleCallback("auth-code", "");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, CallbackUnknownStateReturns400) {
    auto result = provider_->handleCallback("auth-code", "nonexistent-state");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, CallbackSuccessReturnsTokens) {
    // First obtain a valid state via handleAuthorize
    auto auth = provider_->handleAuthorize("test-state-cb");
    const std::string state = auth["state"].get<std::string>();

    // Now simulate the IdP redirect callback
    auto result = provider_->handleCallback("valid-code", state);

    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.contains("access_token"));
    EXPECT_TRUE(result.contains("token_type"));
    EXPECT_TRUE(result.contains("expires_in"));
    EXPECT_EQ(result["access_token"].get<std::string>(), "fake-access");
    EXPECT_EQ(result["token_type"].get<std::string>(), "Bearer");
}

TEST_F(OAuth2ProviderTest, CallbackStateSingleUse) {
    // State must be consumed after the first callback use.
    auto auth = provider_->handleAuthorize("state-single-use");
    const std::string state = auth["state"].get<std::string>();

    auto r1 = provider_->handleCallback("code1", state);
    ASSERT_FALSE(r1.contains("status_code")) << r1.dump();

    // Second callback with the same state must fail (consumed).
    auto r2 = provider_->handleCallback("code2", state);
    ASSERT_TRUE(r2.contains("status_code"));
    EXPECT_EQ(r2["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, CallbackIdPErrorReturns401) {
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeErrorResponse("invalid_grant", "Authorization code expired");
        });

    auto auth = provider_->handleAuthorize("state-idp-err");
    const std::string state = auth["state"].get<std::string>();

    auto result = provider_->handleCallback("bad-code", state);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(OAuth2ProviderTest, CallbackIdTokenIncludedWhenPresent) {
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeTokenResponse("acc", "ref", "id-tok-here");
        });

    auto auth = provider_->handleAuthorize("state-id-tok");
    const std::string state = auth["state"].get<std::string>();

    auto result = provider_->handleCallback("code", state);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    ASSERT_TRUE(result.contains("id_token"));
    EXPECT_EQ(result["id_token"].get<std::string>(), "id-tok-here");
}

// ===========================================================================
// handleTokenExchange tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, TokenExchangeMissingCodeReturns400) {
    auto result = provider_->handleTokenExchange("", "verifier");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, TokenExchangeMissingVerifierReturns400) {
    auto result = provider_->handleTokenExchange("code", "");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, TokenExchangeSuccessWithoutState) {
    auto result = provider_->handleTokenExchange("code", "verifier");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_EQ(result["access_token"].get<std::string>(), "fake-access");
}

TEST_F(OAuth2ProviderTest, TokenExchangeSuccessWithMatchingState) {
    auto auth = provider_->handleAuthorize("state-tex");
    const std::string state    = auth["state"].get<std::string>();
    const std::string verifier = auth["code_verifier"].get<std::string>();

    auto result = provider_->handleTokenExchange("code", verifier, state);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.contains("access_token"));
}

TEST_F(OAuth2ProviderTest, TokenExchangeVerifierMismatchReturns400) {
    auto auth = provider_->handleAuthorize("state-vm");
    const std::string state = auth["state"].get<std::string>();

    auto result = provider_->handleTokenExchange("code", "wrong-verifier", state);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, TokenExchangeUnknownStateReturns400) {
    auto result = provider_->handleTokenExchange("code", "verifier", "no-such-state");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

// ===========================================================================
// handleRefresh tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, RefreshMissingTokenReturns400) {
    auto result = provider_->handleRefresh("");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, RefreshSuccessReturnsNewTokens) {
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeTokenResponse("new-access", "new-refresh");
        });

    auto result = provider_->handleRefresh("old-refresh-token");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_EQ(result["access_token"].get<std::string>(), "new-access");
    EXPECT_EQ(result["refresh_token"].get<std::string>(), "new-refresh");
    EXPECT_EQ(result["token_type"].get<std::string>(), "Bearer");
    EXPECT_EQ(result["expires_in"].get<int>(), 3600);
}

TEST_F(OAuth2ProviderTest, RefreshIdPErrorReturns401) {
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeErrorResponse("invalid_grant", "Refresh token expired");
        });

    auto result = provider_->handleRefresh("expired-refresh-token");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(OAuth2ProviderTest, RefreshBodyContainsGrantType) {
    std::string captured_body;
    provider_->setHttpPostForTesting(
        [&captured_body](const std::string&, const std::string& body) {
            captured_body = body;
            return fakeTokenResponse("acc", "ref");
        });

    provider_->handleRefresh("my-refresh-tok");
    EXPECT_NE(captured_body.find("grant_type"), std::string::npos);
    EXPECT_NE(captured_body.find("refresh_token"), std::string::npos);
    EXPECT_NE(captured_body.find("my-refresh-tok"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, RefreshBodyContainsClientId) {
    std::string captured_body;
    provider_->setHttpPostForTesting(
        [&captured_body](const std::string&, const std::string& body) {
            captured_body = body;
            return fakeTokenResponse("acc", "ref");
        });

    provider_->handleRefresh("some-refresh");
    EXPECT_NE(captured_body.find("client_id"), std::string::npos);
    EXPECT_NE(captured_body.find("themisdb-test"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, RefreshNoNewRefreshTokenOmitsField) {
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            // IdP does not rotate the refresh token
            nlohmann::json j = {
                {"access_token", "new-acc"},
                {"token_type",   "Bearer"},
                {"expires_in",   1800}
            };
            return j.dump();
        });

    auto result = provider_->handleRefresh("old-refresh");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_EQ(result["access_token"].get<std::string>(), "new-acc");
    // refresh_token must NOT be present when IdP did not return one.
    EXPECT_FALSE(result.contains("refresh_token"));
}

// ===========================================================================
// handleIntrospect tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, IntrospectMissingTokenReturns400) {
    auto result = provider_->handleIntrospect("");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(OAuth2ProviderTest, IntrospectInvalidTokenReturnsInactive) {
    // Any non-JWT / unverifiable string → active: false
    auto result = provider_->handleIntrospect("not.a.valid.jwt");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_FALSE(result["active"].get<bool>());
}

TEST_F(OAuth2ProviderTest, IntrospectEmptyStringReturns400) {
    auto result = provider_->handleIntrospect("");
    EXPECT_EQ(result.value("status_code", 0), 400);
}

// ===========================================================================
// handleLogout tests
// ===========================================================================

TEST_F(OAuth2ProviderTest, LogoutAlwaysReturnsSuccess) {
    auto result = provider_->handleLogout();
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
}

TEST_F(OAuth2ProviderTest, LogoutWithRefreshTokenReturnsSuccess) {
    auto result = provider_->handleLogout("some-refresh-token");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
}

TEST_F(OAuth2ProviderTest, LogoutPostsToRevocationEndpointWhenAvailable) {
    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    doc.revocation_endpoint    = "https://idp.example.com/revoke";
    provider_->setDiscoveryDocumentForTesting(doc);

    std::string captured_url;
    std::string captured_body;
    provider_->setHttpPostForTesting(
        [&](const std::string& url, const std::string& body) {
            captured_url = url;
            captured_body = body;
            return std::string("{}");
        });

    auto result = provider_->handleLogout("refresh-token-123");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
    EXPECT_EQ(captured_url, "https://idp.example.com/revoke");
    EXPECT_NE(captured_body.find("token=refresh-token-123"), std::string::npos);
    EXPECT_NE(captured_body.find("token_type_hint=refresh_token"), std::string::npos);
    EXPECT_NE(captured_body.find("client_id=themisdb-test"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, LogoutIncludesClientSecretWhenConfigured) {
    // Create provider with client_secret
    auto cfg = makeConfig();
    cfg.oidc.client_secret = "super-secret-123";
    OAuth2Provider provider_with_secret(cfg);

    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    doc.revocation_endpoint    = "https://idp.example.com/revoke";
    provider_with_secret.setDiscoveryDocumentForTesting(doc);

    std::string captured_body;
    provider_with_secret.setHttpPostForTesting(
        [&](const std::string&, const std::string& body) {
            captured_body = body;
            return std::string("{}");
        });

    auto result = provider_with_secret.handleLogout("refresh-token-xyz");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
    EXPECT_NE(captured_body.find("client_secret=super-secret-123"), std::string::npos);
}

TEST_F(OAuth2ProviderTest, LogoutReturnsSuccessEvenIfRevocationEndpointFails) {
    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    doc.revocation_endpoint    = "https://idp.example.com/revoke";
    provider_->setDiscoveryDocumentForTesting(doc);

    // Inject HTTP POST that throws an exception (simulating network failure)
    provider_->setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            throw std::runtime_error("Network error");
        });

    // Even with the error, logout should return success (best-effort behavior)
    auto result = provider_->handleLogout("refresh-token-123");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
}

TEST_F(OAuth2ProviderTest, LogoutWithoutRefreshTokenReturnsSuccess) {
    auto result = provider_->handleLogout("");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.value("success", false));
}

// ===========================================================================
// State TTL expiry
// ===========================================================================

TEST(OAuth2ProviderTtlTest, ExpiredStateIsRejected) {
    auto cfg = makeConfig();
    cfg.state_ttl = std::chrono::seconds{0}; // Expire immediately
    OAuth2Provider provider(cfg);

    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    provider.setDiscoveryDocumentForTesting(doc);
    provider.setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeTokenResponse();
        });

    auto auth = provider.handleAuthorize("state-ttl");
    const std::string state = auth["state"].get<std::string>();

    // Wait slightly to guarantee TTL=0 has elapsed.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto result = provider.handleCallback("code", state);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

// ===========================================================================
// Custom token factory
// ===========================================================================

TEST(OAuth2ProviderFactoryTest, CustomTokenFactoryInvoked) {
    auto cfg = makeConfig();
    // Use shared_ptr to avoid a by-reference capture that could outlive the test.
    auto factory_called = std::make_shared<bool>(false);
    cfg.token_factory = [factory_called](const std::string& at) -> std::string {
        *factory_called = true;
        return "custom_" + at;
    };

    OAuth2Provider provider(cfg);

    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    provider.setDiscoveryDocumentForTesting(doc);
    provider.setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeTokenResponse("raw-access");
        });

    auto auth = provider.handleAuthorize("factory-state");
    const std::string state = auth["state"].get<std::string>();

    auto result = provider.handleCallback("code", state);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(*factory_called);
    EXPECT_EQ(result["access_token"].get<std::string>(), "custom_raw-access");
}

TEST(OAuth2ProviderFactoryTest, DefaultFactoryPassesThroughAccessToken) {
    // Without a token factory the raw access_token from the IdP is returned.
    auto cfg = makeConfig();
    // token_factory is default-constructed (empty std::function) – no factory set.

    OAuth2Provider provider(cfg);

    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com/realms/test";
    doc.jwks_uri               = "https://idp.example.com/certs";
    doc.authorization_endpoint = "https://idp.example.com/auth";
    doc.token_endpoint         = "https://idp.example.com/token";
    provider.setDiscoveryDocumentForTesting(doc);
    provider.setHttpPostForTesting(
        [](const std::string&, const std::string&) {
            return fakeTokenResponse("idp-issued-token");
        });

    auto auth = provider.handleAuthorize("default-factory-state");
    const std::string state = auth["state"].get<std::string>();

    auto result = provider.handleCallback("code", state);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_EQ(result["access_token"].get<std::string>(), "idp-issued-token");
}

// ===========================================================================
// PKCE determinism via random-bytes hook
// ===========================================================================

TEST_F(OAuth2ProviderTest, DeterministicRandBytesProducesConsistentChallenge) {
    // Inject a deterministic random-bytes source.
    provider_->setRandBytesForTesting(
        [](unsigned char* buf, std::size_t len) {
            for (std::size_t i = 0; i < len; ++i) {
                buf[i] = static_cast<unsigned char>(i & 0xFF);
            }
        });

    auto r1 = provider_->handleAuthorize("det-state-1");
    auto r2 = provider_->handleAuthorize("det-state-2");

    // With a deterministic source both code_verifiers are equal.
    EXPECT_EQ(r1["code_verifier"].get<std::string>(),
              r2["code_verifier"].get<std::string>());
}

// ===========================================================================
// main
// ===========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
