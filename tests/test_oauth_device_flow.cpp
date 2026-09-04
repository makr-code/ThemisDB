#include <gtest/gtest.h>
#include "auth/oauth_device_flow.h"
#include "auth/auth_error.h"

#include <nlohmann/json.hpp>
#include <string>

using namespace themis::auth;
using json = nlohmann::json;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

OAuthDeviceFlow::Config makeConfig() {
    OAuthDeviceFlow::Config cfg;
    cfg.device_authorization_endpoint = "https://auth.example.com/device_authorization";
    cfg.token_endpoint                 = "https://auth.example.com/token";
    cfg.client_id                      = "test-client";
    cfg.scopes                         = {"openid", "email"};
    return cfg;
}

std::string makeDeviceCodeJson(int interval = 1) {
    json j;
    j["device_code"]               = "device-code-abc";
    j["user_code"]                 = "BDWP-HQMF";
    j["verification_uri"]          = "https://auth.example.com/activate";
    j["verification_uri_complete"] = "https://auth.example.com/activate?user_code=BDWP-HQMF";
    j["expires_in"]                = 600;
    j["interval"]                  = interval;
    return j.dump();
}

std::string makeTokenJson() {
    json j;
    j["access_token"]  = "access-token-xyz";
    j["token_type"]    = "Bearer";
    j["expires_in"]    = 3600;
    j["refresh_token"] = "refresh-token-xyz";
    j["scope"]         = "openid email";
    // id_token is intentionally omitted in some tests
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

} // anonymous namespace

// ===========================================================================
// Construction validation
// ===========================================================================

TEST(OAuthDeviceFlowTest, ConstructorRejectsEmptyEndpoint) {
    OAuthDeviceFlow::Config cfg = makeConfig();
    cfg.device_authorization_endpoint.clear();
    EXPECT_THROW((void)OAuthDeviceFlow{cfg}, AuthException);
}

TEST(OAuthDeviceFlowTest, ConstructorRejectsEmptyTokenEndpoint) {
    OAuthDeviceFlow::Config cfg = makeConfig();
    cfg.token_endpoint.clear();
    EXPECT_THROW((void)OAuthDeviceFlow{cfg}, AuthException);
}

TEST(OAuthDeviceFlowTest, ConstructorRejectsEmptyClientId) {
    OAuthDeviceFlow::Config cfg = makeConfig();
    cfg.client_id.clear();
    EXPECT_THROW((void)OAuthDeviceFlow{cfg}, AuthException);
}

// ===========================================================================
// requestDeviceCode()
// ===========================================================================

TEST(OAuthDeviceFlowTest, RequestDeviceCode_Success) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string& /*url*/,
                                  const std::string& /*body*/) {
        return makeDeviceCodeJson();
    });

    const auto resp = flow.requestDeviceCode();

    EXPECT_EQ(resp.device_code, "device-code-abc");
    EXPECT_EQ(resp.user_code,   "BDWP-HQMF");
    EXPECT_EQ(resp.verification_uri, "https://auth.example.com/activate");
    EXPECT_EQ(resp.verification_uri_complete,
              "https://auth.example.com/activate?user_code=BDWP-HQMF");
    EXPECT_EQ(resp.expires_in, 600);
    EXPECT_EQ(resp.interval,   1);
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_MissingVerificationUri) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        json j;
        j["device_code"] = "x";
        j["user_code"]   = "X";
        // verification_uri intentionally missing
        return j.dump();
    });

    EXPECT_THROW(flow.requestDeviceCode(), AuthException);
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_ServerError) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("invalid_client", "Unknown client_id");
    });

    EXPECT_THROW(flow.requestDeviceCode(), AuthException);
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_InvalidJson) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return "not-valid-json{{{";
    });

    EXPECT_THROW(flow.requestDeviceCode(), AuthException);
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_TransportError) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) -> std::string {
        throw std::runtime_error("Connection refused");
    });

    EXPECT_THROW(flow.requestDeviceCode(), AuthException);
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_MissingVerificationUriComplete_DefaultsToVerificationUri) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        json j;
        j["device_code"]      = "dc";
        j["user_code"]        = "UC";
        j["verification_uri"] = "https://auth.example.com/activate";
        j["expires_in"]       = 300;
        j["interval"]         = 5;
        // No verification_uri_complete
        return j.dump();
    });

    const auto resp = flow.requestDeviceCode();
    EXPECT_EQ(resp.verification_uri_complete, resp.verification_uri);
}

// ===========================================================================
// pollForToken()
// ===========================================================================

TEST(OAuthDeviceFlowTest, PollForToken_Authorized) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeTokenJson();
    });

    OAuthDeviceFlow::PollStatus status;
    const auto token = flow.pollForToken("dc-xyz", status);

    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::Authorized);
    EXPECT_EQ(token.access_token,  "access-token-xyz");
    EXPECT_EQ(token.token_type,    "Bearer");
    EXPECT_EQ(token.refresh_token, "refresh-token-xyz");
    EXPECT_EQ(token.expires_in,    3600);
}

TEST(OAuthDeviceFlowTest, PollForToken_AuthorizationPending) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("authorization_pending");
    });

    OAuthDeviceFlow::PollStatus status;
    flow.pollForToken("dc", status);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::AuthorizationPending);
}

TEST(OAuthDeviceFlowTest, PollForToken_SlowDown) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("slow_down");
    });

    OAuthDeviceFlow::PollStatus status;
    flow.pollForToken("dc", status);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::SlowDown);
}

TEST(OAuthDeviceFlowTest, PollForToken_AccessDenied_Throws) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("access_denied");
    });

    OAuthDeviceFlow::PollStatus status;
    EXPECT_THROW(flow.pollForToken("dc", status), AuthException);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::AccessDenied);
}

TEST(OAuthDeviceFlowTest, PollForToken_ExpiredToken_Throws) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("expired_token");
    });

    OAuthDeviceFlow::PollStatus status;
    EXPECT_THROW(flow.pollForToken("dc", status), AuthException);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::ExpiredToken);
}

TEST(OAuthDeviceFlowTest, PollForToken_UnknownError_Throws) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        return makeErrorJson("server_error", "Internal server error");
    });

    OAuthDeviceFlow::PollStatus status;
    EXPECT_THROW(flow.pollForToken("dc", status), AuthException);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::Error);
}

TEST(OAuthDeviceFlowTest, PollForToken_MissingAccessToken_Throws) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) {
        // Missing access_token
        json j;
        j["token_type"]  = "Bearer";
        j["expires_in"]  = 3600;
        return j.dump();
    });

    OAuthDeviceFlow::PollStatus status;
    EXPECT_THROW(flow.pollForToken("dc", status), AuthException);
}

TEST(OAuthDeviceFlowTest, PollForToken_TransportError_SetsErrorStatus) {
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([](const std::string&, const std::string&) -> std::string {
        throw std::runtime_error("Network unreachable");
    });

    OAuthDeviceFlow::PollStatus status;
    flow.pollForToken("dc", status);
    EXPECT_EQ(status, OAuthDeviceFlow::PollStatus::Error);
}

// ===========================================================================
// validateIdToken()
// ===========================================================================

TEST(OAuthDeviceFlowTest, ValidateIdToken_EmptyIdToken_Throws) {
    OAuthDeviceFlow flow(makeConfig());

    OAuthDeviceFlow::TokenResponse token;
    token.access_token = "at";
    // id_token intentionally empty

    EXPECT_THROW(flow.validateIdToken(token), AuthException);
}

TEST(OAuthDeviceFlowTest, ValidateIdToken_MissingJwksUrl_Throws) {
    OAuthDeviceFlow::Config cfg = makeConfig();
    cfg.jwks_url.clear();
    OAuthDeviceFlow flow(cfg);

    OAuthDeviceFlow::TokenResponse token;
    token.id_token = "header.payload.signature";

    EXPECT_THROW(flow.validateIdToken(token), AuthException);
}

// ===========================================================================
// authenticate() – progress callback and scope encoding
// ===========================================================================

TEST(OAuthDeviceFlowTest, Authenticate_ProgressCallbackInvoked) {
    // Simulate: 1 authorization_pending poll, then authorized
    int call_count = 0;
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([&call_count](const std::string& url,
                                             const std::string& /*body*/) {
        if (url.find("device_authorization") != std::string::npos) {
            // Use interval=1 so the test completes quickly
            return makeDeviceCodeJson(1);
        }
        // Token endpoint: first call pending, second authorized
        ++call_count;
        if (call_count == 1) {
            return makeErrorJson("authorization_pending");
        }
        // Return a token without id_token; validateIdToken should throw (no jwks_url).
        // We only test the callback side here.
        return makeTokenJson();
    });

    bool callback_invoked = false;
    OAuthDeviceFlow::DeviceCodeResponse captured_resp;

    // authenticate() will throw because id_token is missing / jwks_url unset,
    // but the progress callback must be called before the first poll.
    EXPECT_THROW(
        flow.authenticate([&](const OAuthDeviceFlow::DeviceCodeResponse& r) {
            callback_invoked = true;
            captured_resp    = r;
        }),
        AuthException
    );

    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(captured_resp.user_code, "BDWP-HQMF");
}

TEST(OAuthDeviceFlowTest, RequestDeviceCode_ScopesEncodedInBody) {
    std::string captured_body;
    OAuthDeviceFlow flow(makeConfig());
    flow.setHttpPostForTesting([&captured_body](const std::string&,
                                                const std::string& body) {
        captured_body = body;
        return makeDeviceCodeJson();
    });

    flow.requestDeviceCode();

    EXPECT_NE(captured_body.find("scope="), std::string::npos);
    // "openid email" should appear URL-encoded
    EXPECT_NE(captured_body.find("openid"), std::string::npos);
    EXPECT_NE(captured_body.find("email"),  std::string::npos);
}

TEST(OAuthDeviceFlowTest, PollForToken_ClientSecretIncludedWhenSet) {
    OAuthDeviceFlow::Config cfg = makeConfig();
    cfg.client_secret = "my-secret";
    std::string captured_body;

    OAuthDeviceFlow flow(cfg);
    flow.setHttpPostForTesting([&captured_body](const std::string& url,
                                                const std::string& body) {
        if (url.find("device_authorization") != std::string::npos) {
            return makeDeviceCodeJson();
        }
        captured_body = body;
        return makeTokenJson();
    });

    OAuthDeviceFlow::PollStatus status;
    flow.pollForToken("dc", status);

    EXPECT_NE(captured_body.find("client_secret"), std::string::npos);
    EXPECT_NE(captured_body.find("my-secret"),     std::string::npos);
}
