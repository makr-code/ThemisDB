#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "api/ws_handler.h"
#include "server/auth_middleware.h"
#include "server/websocket_session.h"
#include <nlohmann/json.hpp>

using namespace themis::api;
namespace http = boost::beast::http;
using json     = nlohmann::json;

// Helper: build a minimal WebSocket upgrade request for a given target URL.
static http::request<http::string_body>
make_upgrade_request(const std::string& target,
                     const std::string& auth_header = "")
{
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host,                  "localhost");
    req.set(http::field::upgrade,               "websocket");
    req.set(http::field::connection,            "Upgrade");
    req.set(http::field::sec_websocket_key,     "dGhlIHNhbXBsZSBub25jZQ==");
    req.set(http::field::sec_websocket_version, "13");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    return req;
}

// ============================================================================
// isChangeStreamPath()
// ============================================================================

TEST(WsChangeHandlerTest, IsChangeStreamPath_Exact) {
    EXPECT_TRUE(WsChangeHandler::isChangeStreamPath("/v2/changes"));
}

TEST(WsChangeHandlerTest, IsChangeStreamPath_CdcStream) {
    EXPECT_TRUE(WsChangeHandler::isChangeStreamPath("/v2/cdc/stream"));
}

TEST(WsChangeHandlerTest, IsChangeStreamPath_WrongPath) {
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath("/v2/change"));
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath("/changes"));
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath("/v1/changes"));
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath("/"));
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath(""));
}

TEST(WsChangeHandlerTest, IsChangeStreamPath_WithTrailingSlash) {
    // Trailing slash is a different resource – should not match.
    EXPECT_FALSE(WsChangeHandler::isChangeStreamPath("/v2/changes/"));
}

// ============================================================================
// validate() – path checks (no auth middleware)
// ============================================================================

TEST(WsChangeHandlerTest, ValidateRejectsWrongPath) {
    WsChangeHandler handler(nullptr);   // no auth
    auto req      = make_upgrade_request("/v2/other");
    auto decision = handler.validate(req);

    EXPECT_FALSE(decision.should_upgrade);
    EXPECT_EQ(decision.reject_status, http::status::not_found);
}

TEST(WsChangeHandlerTest, ValidateAcceptsCorrectPath_NoAuth) {
    WsChangeHandler handler(nullptr);   // auth bypassed (nullptr)
    auto req      = make_upgrade_request("/v2/changes");
    auto decision = handler.validate(req);

    EXPECT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 0u);
    EXPECT_TRUE(decision.key_prefix.empty());
}

TEST(WsChangeHandlerTest, ValidateAcceptsCdcStreamPath_NoAuth) {
    WsChangeHandler handler(nullptr);   // auth bypassed (nullptr)
    auto req      = make_upgrade_request("/v2/cdc/stream");
    auto decision = handler.validate(req);

    EXPECT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 0u);
    EXPECT_TRUE(decision.key_prefix.empty());
}

// ============================================================================
// validate() – query string parameter extraction
// ============================================================================

TEST(WsChangeHandlerTest, ValidateExtractsFromSequence) {
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?from_sequence=42");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 42u);
}

TEST(WsChangeHandlerTest, ValidateExtractsKeyPrefix) {
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=user:");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "user:");
}

TEST(WsChangeHandlerTest, ValidateExtractsBothParams) {
    WsChangeHandler handler(nullptr);
    auto req = make_upgrade_request(
        "/v2/changes?from_sequence=99&key_prefix=orders:");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 99u);
    EXPECT_EQ(decision.key_prefix, "orders:");
}

TEST(WsChangeHandlerTest, ValidateIgnoresInvalidFromSequence) {
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?from_sequence=notanumber");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    // Invalid value → default 0
    EXPECT_EQ(decision.from_sequence, 0u);
}

TEST(WsChangeHandlerTest, ValidateZeroFromSequenceIsValid) {
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?from_sequence=0");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 0u);
}

// ============================================================================
// validate() – URL-encoded query string parameters
// ============================================================================

TEST(WsChangeHandlerTest, ValidateDecodesPercentEncodedKeyPrefix) {
    // %3A is the percent-encoding of ':'
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders%3A");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders:");
}

TEST(WsChangeHandlerTest, ValidateDecodesPercentEncodedKeyPrefixWithSlash) {
    // %2F is the percent-encoding of '/'
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=tenant%2Forders%3A");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "tenant/orders:");
}

TEST(WsChangeHandlerTest, ValidateDecodesPercentEncodedBothParams) {
    // key_prefix=orders%3A with a numeric from_sequence
    WsChangeHandler handler(nullptr);
    auto req = make_upgrade_request(
        "/v2/changes?from_sequence=7&key_prefix=orders%3A");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.from_sequence, 7u);
    EXPECT_EQ(decision.key_prefix, "orders:");
}

TEST(WsChangeHandlerTest, ValidateLiteralColonStillWorks) {
    // Plain unencoded ':' should continue to work as before.
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders:");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders:");
}

TEST(WsChangeHandlerTest, ValidateMalformedBarePercentPassesThrough) {
    // A bare '%' at the end of the value is not a valid escape – pass through.
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders%");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders%");
}

TEST(WsChangeHandlerTest, ValidateMalformedTruncatedPercentPassesThrough) {
    // '%3' has only one hex digit – not a valid escape, pass through.
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders%3");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders%3");
}

TEST(WsChangeHandlerTest, ValidateMalformedNonHexPercentPassesThrough) {
    // '%ZZ' has non-hex digits – pass through unchanged.
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders%ZZ");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders%ZZ");
}

TEST(WsChangeHandlerTest, ValidatePlusSignIsNotDecodedToSpace) {
    // '+' in a raw URL query string is a literal '+', not a space.
    // (Space is encoded as %20 in RFC 3986 query strings.)
    WsChangeHandler handler(nullptr);
    auto req      = make_upgrade_request("/v2/changes?key_prefix=orders+items");
    auto decision = handler.validate(req);

    ASSERT_TRUE(decision.should_upgrade);
    EXPECT_EQ(decision.key_prefix, "orders+items");
}

// ============================================================================
// validate() – authentication (with default-constructed AuthMiddleware)
// ============================================================================

TEST(WsChangeHandlerTest, ValidateRejectsWhenNoTokenWithAuth) {
    // A default-constructed AuthMiddleware has no registered tokens and JWT
    // disabled, so any token (including empty) will be rejected.
    auto auth = std::make_shared<themis::AuthMiddleware>();
    WsChangeHandler handler(auth);
    auto req      = make_upgrade_request("/v2/changes");  // no auth header
    auto decision = handler.validate(req);

    EXPECT_FALSE(decision.should_upgrade);
    EXPECT_EQ(decision.reject_status, http::status::unauthorized);
}

// ============================================================================
// Back-pressure constant
// ============================================================================

TEST(WsChangeHandlerTest, WsSessionBackpressureConstantIsDefined) {
    // Verify the back-pressure limit is defined on WebSocketSession.
    // Access it via the public typedef exposed by the header.
    using S = themis::server::WebSocketSession;
    // kMaxQueueDepth should be exactly 1000 per the design spec.
    EXPECT_EQ(S::kMaxQueueDepth, 1000u);
}

#endif // THEMIS_ENABLE_WEBSOCKET

// Placeholder when WebSocket is disabled
#ifndef THEMIS_ENABLE_WEBSOCKET
TEST(WsChangeHandlerTest, DisabledByDefault) {
    GTEST_SKIP() << "WebSocket is disabled. Build with "
                    "-DTHEMIS_ENABLE_WEBSOCKET=ON to enable.";
}
#endif
