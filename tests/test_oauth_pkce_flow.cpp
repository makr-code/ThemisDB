#include <gtest/gtest.h>
#include "auth/oauth_pkce_flow.h"
#include "auth/auth_error.h"

#include <nlohmann/json.hpp>
#include <string>
#include <cstring>

using namespace themis::auth;
using json = nlohmann::json;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

OAuthPKCEFlow::Config makeConfig() {
    OAuthPKCEFlow::Config cfg;
    cfg.authorization_endpoint = "https://auth.example.com/authorize";
    cfg.token_endpoint         = "https://auth.example.com/token";
    cfg.client_id              = "test-public-client";
    cfg.redirect_uri           = "myapp://callback";
    cfg.scopes                 = {"openid", "email"};
    return cfg;
}

std::string makeTokenJson(bool include_id_token = false) {
    json j;
    j["access_token"]  = "access-token-xyz";
    j["token_type"]    = "Bearer";
    j["expires_in"]    = 3600;
    j["refresh_token"] = "refresh-token-xyz";
    j["scope"]         = "openid email";
    if (include_id_token) {
        j["id_token"] = "header.payload.signature";
    }
    return j.dump();
}

std::string makeErrorJson(const std::string& error,
                          const std::string& description = "") {
    json j;
    j["error"] = error;
    if (!description.empty()) {
      j["error_description"] = description;
    }
    return j.dump();
}

// Deterministic random bytes injector – fills buf with 0xAB repeated.
void deterministicRand(unsigned char* buf, std::size_t len) {
    std::memset(buf, 0xAB, len);
}

} // anonymous namespace

// ===========================================================================
// Construction validation
// ===========================================================================

TEST(OAuthPKCEFlowTest, ConstructorRejectsEmptyAuthorizationEndpoint) {
    OAuthPKCEFlow::Config cfg = makeConfig();
    cfg.authorization_endpoint.clear();
    EXPECT_THROW((void)OAuthPKCEFlow{cfg}, AuthException);
}

TEST(OAuthPKCEFlowTest, ConstructorRejectsEmptyTokenEndpoint) {
    OAuthPKCEFlow::Config cfg = makeConfig();
    cfg.token_endpoint.clear();
    EXPECT_THROW((void)OAuthPKCEFlow{cfg}, AuthException);
}

TEST(OAuthPKCEFlowTest, ConstructorRejectsEmptyClientId) {
    OAuthPKCEFlow::Config cfg = makeConfig();
    cfg.client_id.clear();
    EXPECT_THROW((void)OAuthPKCEFlow{cfg}, AuthException);
}

TEST(OAuthPKCEFlowTest, ConstructorRejectsEmptyRedirectUri) {
    OAuthPKCEFlow::Config cfg = makeConfig();
    cfg.redirect_uri.clear();
    EXPECT_THROW((void)OAuthPKCEFlow{cfg}, AuthException);
}

TEST(OAuthPKCEFlowTest, ConstructorSucceedsWithValidConfig) {
    EXPECT_NO_THROW((void)OAuthPKCEFlow{makeConfig()});
}

// ===========================================================================
// generateChallenge()
// ===========================================================================

TEST(OAuthPKCEFlowTest, GenerateChallenge_ReturnsS256Method) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch = flow.generateChallenge();
    EXPECT_EQ(ch.challenge_method, "S256");
}

TEST(OAuthPKCEFlowTest, GenerateChallenge_VerifierLength) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch = flow.generateChallenge();
    // 96 bytes → 128 Base64URL characters (RFC 7636 requires 43–128)
    EXPECT_GE(ch.code_verifier.size(), 43u);
    EXPECT_LE(ch.code_verifier.size(), 128u);
}

TEST(OAuthPKCEFlowTest, GenerateChallenge_VerifierIsUrlSafe) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch = flow.generateChallenge();
    for (char c : ch.code_verifier) {
        EXPECT_TRUE(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
            << "Non-URL-safe character in verifier: " << c;
    }
}

TEST(OAuthPKCEFlowTest, GenerateChallenge_ChallengeIsUrlSafeNoPadding) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch = flow.generateChallenge();
    EXPECT_EQ(ch.code_challenge.find('+'), std::string::npos);
    EXPECT_EQ(ch.code_challenge.find('/'), std::string::npos);
    EXPECT_EQ(ch.code_challenge.find('='), std::string::npos);
}

TEST(OAuthPKCEFlowTest, GenerateChallenge_DeterministicWithFixedRand) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch1 = flow.generateChallenge();
    const auto ch2 = flow.generateChallenge();

    // Same seed → same output
    EXPECT_EQ(ch1.code_verifier,  ch2.code_verifier);
    EXPECT_EQ(ch1.code_challenge, ch2.code_challenge);
}

TEST(OAuthPKCEFlowTest, GenerateChallenge_DifferentRandProducesDifferentChallenge) {
    OAuthPKCEFlow flow(makeConfig());

    int seed = 0;
    auto seededRand = [&seed](unsigned char* buf, std::size_t len) {
        std::memset(buf, seed++, len);
    };
    flow.setRandBytesForTesting(seededRand);

    const auto ch1 = flow.generateChallenge();
    const auto ch2 = flow.generateChallenge();

    EXPECT_NE(ch1.code_verifier,  ch2.code_verifier);
    EXPECT_NE(ch1.code_challenge, ch2.code_challenge);
}

// ===========================================================================
// buildAuthorizationUrl()
// ===========================================================================

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_ContainsRequiredParams) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch);

    EXPECT_NE(url.find("response_type=code"),        std::string::npos);
    EXPECT_NE(url.find("client_id="),                std::string::npos);
    EXPECT_NE(url.find("redirect_uri="),             std::string::npos);
    EXPECT_NE(url.find("code_challenge="),           std::string::npos);
    EXPECT_NE(url.find("code_challenge_method=S256"), std::string::npos);
}

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_ContainsScopeWhenSet) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch);
    EXPECT_NE(url.find("scope="), std::string::npos);
    EXPECT_NE(url.find("openid"), std::string::npos);
    EXPECT_NE(url.find("email"),  std::string::npos);
}

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_ContainsStateWhenProvided) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch, "csrf-state-123");
    EXPECT_NE(url.find("state="), std::string::npos);
    EXPECT_NE(url.find("csrf-state-123"), std::string::npos);
}

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_NoStateWhenEmpty) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch);
    EXPECT_EQ(url.find("state="), std::string::npos);
}

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_StartsWithAuthorizationEndpoint) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch);
    EXPECT_EQ(url.substr(0, 35), "https://auth.example.com/authorize?");
}

TEST(OAuthPKCEFlowTest, BuildAuthorizationUrl_NoTrailingAmpersand) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);
    const auto ch = flow.generateChallenge();

    const std::string url = flow.buildAuthorizationUrl(ch);
    EXPECT_NE(url.back(), '&');
}

// ===========================================================================
// exchangeCode()
// ===========================================================================

TEST(OAuthPKCEFlowTest, ExchangeCode_Success) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeTokenJson();
    });

    const auto token = flow.exchangeCode("auth-code-123", "verifier-xyz");

    EXPECT_EQ(token.access_token,  "access-token-xyz");
    EXPECT_EQ(token.token_type,    "Bearer");
    EXPECT_EQ(token.expires_in,    3600);
    EXPECT_EQ(token.refresh_token, "refresh-token-xyz");
    EXPECT_EQ(token.scope,         "openid email");
}

TEST(OAuthPKCEFlowTest, ExchangeCode_PostsToTokenEndpoint) {
    std::string captured_url;
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([&captured_url](const std::string& url,
                                               const std::string&) {
        captured_url = url;
        return makeTokenJson();
    });

    flow.exchangeCode("code", "verifier");
    EXPECT_EQ(captured_url, "https://auth.example.com/token");
}

TEST(OAuthPKCEFlowTest, ExchangeCode_BodyContainsRequiredParams) {
    std::string captured_body;
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([&captured_body](const std::string&,
                                                const std::string& body) {
        captured_body = body;
        return makeTokenJson();
    });

    flow.exchangeCode("the-code", "the-verifier");

    EXPECT_NE(captured_body.find("grant_type=authorization_code"), std::string::npos);
    EXPECT_NE(captured_body.find("code=the-code"),                 std::string::npos);
    EXPECT_NE(captured_body.find("code_verifier=the-verifier"),    std::string::npos);
    EXPECT_NE(captured_body.find("client_id="),                    std::string::npos);
    EXPECT_NE(captured_body.find("redirect_uri="),                 std::string::npos);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_EmptyCodeThrows) {
    OAuthPKCEFlow flow(makeConfig());
    EXPECT_THROW(flow.exchangeCode("", "verifier"), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_EmptyVerifierThrows) {
    OAuthPKCEFlow flow(makeConfig());
    EXPECT_THROW(flow.exchangeCode("code", ""), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_ServerErrorThrows) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("invalid_grant", "Code already used");
    });

    EXPECT_THROW(flow.exchangeCode("used-code", "verifier"), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_UnknownErrorThrows) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("server_error", "Internal server error");
    });

    EXPECT_THROW(flow.exchangeCode("code", "verifier"), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_InvalidJsonThrows) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return "not-valid-json{{{";
    });

    EXPECT_THROW(flow.exchangeCode("code", "verifier"), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_MissingAccessTokenThrows) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        json j;
        j["token_type"] = "Bearer";
        j["expires_in"] = 3600;
        // access_token intentionally missing
        return j.dump();
    });

    EXPECT_THROW(flow.exchangeCode("code", "verifier"), AuthException);
}

TEST(OAuthPKCEFlowTest, ExchangeCode_TransportErrorThrows) {
    OAuthPKCEFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) -> std::string {
        throw std::runtime_error("Connection refused");
    });

    EXPECT_THROW(flow.exchangeCode("code", "verifier"), AuthException);
}

// ===========================================================================
// validateIdToken()
// ===========================================================================

TEST(OAuthPKCEFlowTest, ValidateIdToken_EmptyIdTokenThrows) {
    OAuthPKCEFlow flow(makeConfig());

    OAuthPKCEFlow::TokenResponse token;
    token.access_token = "at";
    // id_token intentionally empty

    EXPECT_THROW(flow.validateIdToken(token), AuthException);
}

TEST(OAuthPKCEFlowTest, ValidateIdToken_MissingJwksUrlThrows) {
    OAuthPKCEFlow::Config cfg = makeConfig();
    cfg.jwks_url.clear();
    OAuthPKCEFlow flow(cfg);

    OAuthPKCEFlow::TokenResponse token;
    token.id_token = "header.payload.signature";

    EXPECT_THROW(flow.validateIdToken(token), AuthException);
}

// ===========================================================================
// End-to-end flow coherence
// ===========================================================================

TEST(OAuthPKCEFlowTest, FullFlow_ChallengeVerifierConsistency) {
    // Verify that generateChallenge produces a valid PKCEChallenge:
    // the challenge must differ from the verifier (it is a SHA-256 hash),
    // both must be non-empty Base64URL strings without padding.
    OAuthPKCEFlow flow(makeConfig());
    flow.setRandBytesForTesting(deterministicRand);

    const auto ch = flow.generateChallenge();

    // The challenge is the SHA-256 hash of the verifier – must be different
    EXPECT_NE(ch.code_verifier, ch.code_challenge);
    EXPECT_FALSE(ch.code_verifier.empty());
    EXPECT_FALSE(ch.code_challenge.empty());

    // SHA-256 produces 32 bytes → 43 Base64URL chars (no padding)
    // 32 bytes: ceil(32 * 4 / 3) = 43 chars (no padding)
    EXPECT_EQ(ch.code_challenge.size(), 43u);

    // code_challenge must be URL-safe Base64 without padding
    for (char c : ch.code_challenge) {
        EXPECT_TRUE(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
            << "Non-URL-safe character in challenge: " << c;
    }
}
