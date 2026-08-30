/**
 * @file test_llm_w1s06_exception_boundaries.cpp
 * @brief Focused verification of W1-S06 exception boundary changes.
 *
 * Acceptance criteria for GitHub issue #5359 (W1-S06):
 *   - handleRequest() top-level catch blocks exceptions and returns HTTP 500
 *     instead of propagating (EX-01).
 *   - validateBearerToken() returns false (fail-closed) when no auth header is
 *     present (EX-02), when a Bearer token is present but no JWT validator is
 *     configured (EX-03), and when JWT validation throws (EX-04).
 *   - handleOpenAIListModels() bypasses JWT gate and returns HTTP 200 with an
 *     empty list even when the plugin manager is absent (EX-05).
 *   - Unknown routes beyond the OpenAI compat prefix return HTTP 404 without
 *     raising an exception (EX-06).
 *
 * Http3Handler lifecycle changes (running_ gate, armCleanupTimer) require a
 * live io_context + UDP socket and are verified by code review; they are
 * documented as inspected in src/server/MODULE_GAPS.md (W1-S06 follow-up).
 */

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_LLM

#include "server/llm_api_handler.h"
#include "auth/jwt_validator.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <optional>
#include <string>

namespace http = boost::beast::http;
using json = nlohmann::json;
using namespace themis::server;
using namespace themis::auth;

// ---------------------------------------------------------------------------
// Helper: build a minimal HTTP/1.1 request
// ---------------------------------------------------------------------------
static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& auth_header = "",
    const std::string& body = "")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    req.body() = body;
    req.prepare_payload();
    return req;
}

// ---------------------------------------------------------------------------
// Fixture: LLMApiHandler with no plugin manager and no JWT config
// ---------------------------------------------------------------------------
class W1S06NullHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // nullptr plugin_manager + nullopt jwt_config → minimal handler
        handler_ = std::make_unique<LLMApiHandler>(nullptr, std::nullopt);
    }

    std::unique_ptr<LLMApiHandler> handler_;
};

// EX-01: top-level catch in handleRequest() prevents exception propagation.
// A POST to an authenticated route with no JWT config returns HTTP 401 (not a
// thrown exception). We additionally verify that the response is always a
// well-formed HTTP response object (not garbage from an unhandled exception).
TEST_F(W1S06NullHandlerTest, EX01_HandleRequestDoesNotPropagateExceptions) {
    auto req = makeRequest(http::verb::post, "/api/v1/llm/inference",
                           "", R"({"prompt":"test"})");
    // Should return 401 cleanly (no JWT configured → validateBearerToken false)
    // and must NOT throw.
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleRequest(req));
    // 401 because validateBearerToken() returns false for every request when
    // jwt_validator_ is null.
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

// EX-02: No Authorization header → validateBearerToken returns false → 401.
TEST_F(W1S06NullHandlerTest, EX02_NoAuthHeader_Returns401) {
    auto req = makeRequest(http::verb::get, "/api/v1/llm/stats");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleRequest(req));
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

// EX-03: Bearer token present but jwt_validator_ is null → fail-closed → 401.
TEST_F(W1S06NullHandlerTest, EX03_BearerTokenNoJWTConfig_FailClosed) {
    auto req = makeRequest(http::verb::get, "/api/v1/llm/models",
                           "Bearer some.random.token");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleRequest(req));
    // Fail-closed: no validator configured → deny.
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

// EX-06: Completely unknown route returns HTTP 404, not an exception.
TEST_F(W1S06NullHandlerTest, EX06_UnknownRoute_Returns404) {
    auto req = makeRequest(http::verb::get, "/api/v1/llm/does_not_exist");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleRequest(req));
    EXPECT_EQ(res.result(), http::status::not_found);
}

// ---------------------------------------------------------------------------
// Fixture: LLMApiHandler with a JWT config that has a bogus JWKS URL.
// This lets us exercise the validateBearerToken() exception-catch path
// without any real network or crypto setup.
// ---------------------------------------------------------------------------
class W1S06JWTHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        JWTValidatorConfig cfg;
        // Bogus URL: JWKS fetch will never succeed, but the 2-part token
        // below will throw *before* any network call (format check fires
        // first in parseAndValidate()).
        cfg.jwks_url = "http://localhost:1/bogus-jwks";
        cfg.require_issuer_validation  = false;
        cfg.require_audience_validation = false;
        handler_ = std::make_unique<LLMApiHandler>(nullptr, cfg);
    }

    std::unique_ptr<LLMApiHandler> handler_;
};

// EX-04: Malformed JWT (only 2 parts) → parseAndValidate() throws
// std::runtime_error("Invalid JWT format (expected 3 parts)") which is caught
// by validateBearerToken() → returns false → handler returns 401.
TEST_F(W1S06JWTHandlerTest, EX04_MalformedJWT_ExceptionCaught_Returns401) {
    // "bad.token" has only 2 dot-separated parts — guaranteed format error.
    auto req = makeRequest(http::verb::get, "/api/v1/llm/stats",
                           "Bearer bad.token");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleRequest(req));
    EXPECT_EQ(res.result(), http::status::unauthorized);
    // Response body should be valid JSON
    EXPECT_NO_THROW({
        [[maybe_unused]] const auto parsed = json::parse(res.body());
    });
}

// ---------------------------------------------------------------------------
// EX-05: OpenAI /v1/models bypasses JWT gate and returns HTTP 200 with an
// empty model list when no plugins are registered.
// ---------------------------------------------------------------------------
TEST(W1S06OpenAICompatTest, EX05_ListModels_NoJWT_Returns200EmptyList) {
    LLMApiHandler handler(nullptr, std::nullopt);
    auto req = makeRequest(http::verb::get, "/v1/models");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler.handleRequest(req));
    EXPECT_EQ(res.result(), http::status::ok);
    json body;
    ASSERT_NO_THROW(body = json::parse(res.body()));
    EXPECT_EQ(body.value("object", ""), "list");
    EXPECT_TRUE(body.contains("data"));
    EXPECT_TRUE(body["data"].is_array());
}

// EX-07: OpenAI chat-completions invalid request shape must fail cleanly with
// a deterministic HTTP response (no exception propagation).
TEST(W1S06OpenAICompatTest, EX07_ChatCompletionsInvalidMessages_Returns400) {
    LLMApiHandler handler(nullptr, std::nullopt);
    auto req = makeRequest(
        http::verb::post,
        "/v1/chat/completions",
        "",
        R"({"model":"demo","messages":"not-an-array"})");

    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler.handleRequest(req));
    EXPECT_EQ(res.result(), http::status::bad_request);

    json body;
    ASSERT_NO_THROW(body = json::parse(res.body()));
    EXPECT_TRUE(body.contains("error"));
}

#else // !THEMIS_ENABLE_LLM

// Placeholder when the LLM feature is disabled at build time.
TEST(W1S06ExceptionBoundary, SkippedWithoutLLM) {
    GTEST_SKIP() << "W1-S06 exception boundary tests require THEMIS_ENABLE_LLM";
}

#endif // THEMIS_ENABLE_LLM
