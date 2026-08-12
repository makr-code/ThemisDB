/*
 * @file test_cdc_subscription_auth.cpp
 * @brief Security audit: CDC subscription authorization and data leakage tests.
 *
 * Verifies that:
 *   1. Every CDC endpoint enforces token-based authorization (HTTP 401/403 when
 *      no valid token with the required scope is present).
 *   2. Read endpoints require the "cdc:read" scope; admin/mutating endpoints
 *      require the "cdc:admin" scope.
 *   3. The key_prefix subscription filter prevents data from outside the
 *      subscribed prefix from leaking to the caller.
 *
 * Tests use ChangefeedApiHandler directly with a configured AuthMiddleware
 * so that no external HTTP server is required.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <filesystem>
#include <memory>
#include <string>

#include "server/changefeed_api_handler.h"
#include "server/auth_middleware.h"
#include "server/sse_connection_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http  = beast::http;

#ifdef TOKEN_READ
#undef TOKEN_READ
#endif
#ifdef TOKEN_ADMIN
#undef TOKEN_ADMIN
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Shared tokens used throughout this suite
// ─────────────────────────────────────────────────────────────────────────────
namespace {
    constexpr const char* TOKEN_READ   = "test-cdc-read-token";
    constexpr const char* TOKEN_ADMIN  = "test-cdc-admin-token";
    constexpr const char* TOKEN_NOSCOPE= "test-no-cdc-scope-token";
    constexpr const char* TOKEN_INVALID= "this-token-does-not-exist";
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a minimal GET request
// ─────────────────────────────────────────────────────────────────────────────
static http::request<http::string_body> makeGet(
    const std::string& target,
    const std::string& auth_header = "")
{
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "127.0.0.1");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    return req;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a minimal POST request
// ─────────────────────────────────────────────────────────────────────────────
static http::request<http::string_body> makePost(
    const std::string& target,
    const json& body,
    const std::string& auth_header = "")
{
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, "127.0.0.1");
    req.set(http::field::content_type, "application/json");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    req.body() = body.dump();
    req.prepare_payload();
    return req;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a minimal PUT request
// ─────────────────────────────────────────────────────────────────────────────
static http::request<http::string_body> makePut(
    const std::string& target,
    const json& body,
    const std::string& auth_header = "")
{
    http::request<http::string_body> req{http::verb::put, target, 11};
    req.set(http::field::host, "127.0.0.1");
    req.set(http::field::content_type, "application/json");
    if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
    }
    req.body() = body.dump();
    req.prepare_payload();
    return req;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────
class CdcSubscriptionAuthTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC subscription-auth focused tests on Windows due to fixture crash in current runtime.";
#endif
        db_path_ = (std::filesystem::temp_directory_path()
                    / ("test_cdc_sub_auth_" + std::to_string(
                           std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path     = db_path_;
        cfg.memtable_size_mb     = 16;
        cfg.block_cache_size_mb  = 32;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open()) << "Failed to open test RocksDB";

        themis::Changefeed::RetentionPolicy rp;
        rp.enabled = false;
        changefeed_ = std::make_shared<themis::Changefeed>(
            storage_->getRawDB(), nullptr, rp);

        // Auth middleware with three tokens:
        //   TOKEN_READ   – has cdc:read only
        //   TOKEN_ADMIN  – has cdc:read + cdc:admin
        //   TOKEN_NOSCOPE– has data:read only (no CDC scopes)
        auth_ = std::make_shared<themis::AuthMiddleware>();

        themis::AuthMiddleware::TokenConfig read_cfg;
        read_cfg.token   = TOKEN_READ;
        read_cfg.user_id = "cdc-reader";
        read_cfg.scopes  = {"cdc:read"};
        auth_->addToken(read_cfg);

        themis::AuthMiddleware::TokenConfig admin_cfg;
        admin_cfg.token   = TOKEN_ADMIN;
        admin_cfg.user_id = "cdc-admin";
        admin_cfg.scopes  = {"cdc:read", "cdc:admin"};
        auth_->addToken(admin_cfg);

        themis::AuthMiddleware::TokenConfig noscope_cfg;
        noscope_cfg.token   = TOKEN_NOSCOPE;
        noscope_cfg.user_id = "other-user";
        noscope_cfg.scopes  = {"data:read"};
        auth_->addToken(noscope_cfg);

        // SseConnectionManager is optional; pass nullptr for unit tests
        handler_ = std::make_unique<themis::server::ChangefeedApiHandler>(
            storage_, changefeed_, /*sse_manager=*/nullptr, auth_, /*feature_cdc=*/true);
    }

    void TearDown() override {
        handler_.reset();
        changefeed_.reset();
        if (storage_) {
            storage_->close();
        }
        std::filesystem::remove_all(db_path_);
    }

    // Convenience: bearer header value for a token
    static std::string bearer(const char* token) {
        return std::string("Bearer ") + token;
    }

    // Insert a raw CDC event so that subscription filter tests have data to work with
    void recordEvent(const std::string& key, const std::string& value = "val") {
        themis::Changefeed::ChangeEvent ev;
        ev.key   = key;
        ev.value = value;
        ev.type  = themis::Changefeed::ChangeEventType::EVENT_PUT;
        changefeed_->recordEvent(ev);
    }

    std::string                                             db_path_;
    std::shared_ptr<themis::RocksDBWrapper>                 storage_;
    std::shared_ptr<themis::Changefeed>                     changefeed_;
    std::shared_ptr<themis::AuthMiddleware>                 auth_;
    std::unique_ptr<themis::server::ChangefeedApiHandler>   handler_;
};

// ═════════════════════════════════════════════════════════════════════════════
// Section 1: Authorization enforcement on GET /changefeed
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, Get_NoAuthHeader_Returns401) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10");
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
    // WWW-Authenticate challenge must be present
    EXPECT_NE(res.find(http::field::www_authenticate), res.end());
}

TEST_F(CdcSubscriptionAuthTest, Get_InvalidToken_Returns403) {
    // An unknown token is present as a valid "Bearer <token>" header.
    // extractBearerToken succeeds, authorize() returns Denied → 403 Forbidden.
    auto req = makeGet("/changefeed?from_seq=0&limit=10", bearer(TOKEN_INVALID));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, Get_TokenMissingCdcReadScope_Returns403) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10", bearer(TOKEN_NOSCOPE));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
    json body = json::parse(res.body());
    EXPECT_EQ(body.value("error", std::string{}), "insufficient_scope");
}

TEST_F(CdcSubscriptionAuthTest, Get_CdcReadToken_Returns200) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10", bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(CdcSubscriptionAuthTest, Get_CdcAdminToken_Returns200) {
    // Admin token also has cdc:read → should succeed on read endpoint
    auto req = makeGet("/changefeed?from_seq=0&limit=10", bearer(TOKEN_ADMIN));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(CdcSubscriptionAuthTest, Get_InvalidKeyPrefix_Returns400) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10&key_prefix=../orders:", bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 2: Authorization enforcement on GET /changefeed/stream (SSE)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, Stream_NoAuthHeader_Returns401) {
    auto req = makeGet("/changefeed/stream?from_seq=0");
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, Stream_InvalidToken_Returns403) {
    // "Bearer TOKEN_INVALID" → extractBearerToken succeeds, authorize() denies → 403.
    auto req = makeGet("/changefeed/stream?from_seq=0", bearer(TOKEN_INVALID));
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, Stream_TokenMissingCdcReadScope_Returns403) {
    auto req = makeGet("/changefeed/stream?from_seq=0", bearer(TOKEN_NOSCOPE));
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, Stream_CdcReadToken_Returns200) {
    // max_seconds=1 so the handler terminates quickly in test
    auto req = makeGet(
        "/changefeed/stream?from_seq=0&keep_alive=false&max_seconds=1",
        bearer(TOKEN_READ));
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(CdcSubscriptionAuthTest, Stream_InvalidConsumerId_Returns400) {
    auto req = makeGet(
        "/changefeed/stream?from_seq=0&keep_alive=false&max_seconds=1&consumer_id=../bad",
        bearer(TOKEN_READ));
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 3: Authorization enforcement on GET /changefeed/stats (admin)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, Stats_NoAuthHeader_Returns401) {
    auto req = makeGet("/changefeed/stats");
    auto res = handler_->handleStats(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, Stats_CdcReadOnlyToken_Returns403) {
    // cdc:read is not sufficient for stats; cdc:admin is required
    auto req = makeGet("/changefeed/stats", bearer(TOKEN_READ));
    auto res = handler_->handleStats(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
    json body = json::parse(res.body());
    EXPECT_EQ(body.value("error", std::string{}), "insufficient_scope");
}

TEST_F(CdcSubscriptionAuthTest, Stats_CdcAdminToken_Returns200) {
    auto req = makeGet("/changefeed/stats", bearer(TOKEN_ADMIN));
    auto res = handler_->handleStats(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4: Authorization enforcement on POST /changefeed/retention (admin)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, RetentionPost_NoAuthHeader_Returns401) {
    auto req = makePost("/changefeed/retention", {{"before_sequence", 0}});
    auto res = handler_->handleRetention(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, RetentionPost_CdcReadOnlyToken_Returns403) {
    auto req = makePost("/changefeed/retention", {{"before_sequence", 0}},
                        bearer(TOKEN_READ));
    auto res = handler_->handleRetention(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, RetentionPost_CdcAdminToken_Returns200) {
    auto req = makePost("/changefeed/retention", {{"before_sequence", 0}},
                        bearer(TOKEN_ADMIN));
    auto res = handler_->handleRetention(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 5: Authorization enforcement on GET /changefeed/retention
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, RetentionGet_NoAuthHeader_Returns401) {
    auto req = makeGet("/changefeed/retention");
    auto res = handler_->handleRetentionGet(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, RetentionGet_CdcReadToken_Returns200) {
    // GET /changefeed/retention only requires cdc:read
    auto req = makeGet("/changefeed/retention", bearer(TOKEN_READ));
    auto res = handler_->handleRetentionGet(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 6: Authorization enforcement on PUT /changefeed/retention (admin)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, RetentionPut_NoAuthHeader_Returns401) {
    auto req = makePut("/changefeed/retention", {{"enabled", false}});
    auto res = handler_->handleRetentionPut(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, RetentionPut_CdcReadOnlyToken_Returns403) {
    auto req = makePut("/changefeed/retention", {{"enabled", false}},
                       bearer(TOKEN_READ));
    auto res = handler_->handleRetentionPut(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, RetentionPut_CdcAdminToken_Returns200) {
    auto req = makePut("/changefeed/retention", {{"enabled", false}},
                       bearer(TOKEN_ADMIN));
    auto res = handler_->handleRetentionPut(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(CdcSubscriptionAuthTest, StreamAck_InvalidConsumerId_Returns400) {
    auto req = makePost("/changefeed/stream/ack",
                        {{"consumer_id", "../bad"}, {"up_to_sequence", 1}},
                        bearer(TOKEN_READ));
    auto res = handler_->handleStreamAck(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 7: Authorization enforcement on POST /changefeed/compact (admin)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, Compact_NoAuthHeader_Returns401) {
    auto req = makePost("/changefeed/compact", {});
    auto res = handler_->handleCompact(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, Compact_CdcReadOnlyToken_Returns403) {
    auto req = makePost("/changefeed/compact", {}, bearer(TOKEN_READ));
    auto res = handler_->handleCompact(req);
    EXPECT_EQ(res.result(), http::status::forbidden);
}

TEST_F(CdcSubscriptionAuthTest, Compact_CdcAdminToken_Returns200) {
    auto req = makePost("/changefeed/compact", {}, bearer(TOKEN_ADMIN));
    auto res = handler_->handleCompact(req);
    EXPECT_EQ(res.result(), http::status::ok);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 8: Malformed Authorization header
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, MalformedAuthHeader_NotBearer_Returns401) {
    // "Basic" scheme instead of "Bearer" should be treated as missing/invalid
    auto req = makeGet("/changefeed?from_seq=0&limit=10", "Basic dXNlcjpwYXNz");
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcSubscriptionAuthTest, EmptyBearerToken_Returns401) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10", "Bearer ");
    auto res = handler_->handleGet(req);
    // Empty token string after stripping whitespace → extractBearerToken returns nullopt → 401
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 9: Auth-before-feature-flag ordering
//
// When feature_cdc is disabled the handler must still enforce auth FIRST.
// Unauthenticated requests must get 401, NOT 404.  Revealing that a feature
// is disabled to an unauthenticated caller would be an information-disclosure
// vulnerability.
// ═════════════════════════════════════════════════════════════════════════════

class CdcFeatureDisabledAuthTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC subscription-auth focused tests on Windows due to fixture crash in current runtime.";
#endif
        db_path_ = (std::filesystem::temp_directory_path()
                    / ("test_cdc_feat_dis_" + std::to_string(
                           std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 16;
        cfg.block_cache_size_mb = 32;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        themis::Changefeed::RetentionPolicy rp;
        rp.enabled  = false;
        changefeed_ = std::make_shared<themis::Changefeed>(
            storage_->getRawDB(), nullptr, rp);

        auth_ = std::make_shared<themis::AuthMiddleware>();
        themis::AuthMiddleware::TokenConfig admin_cfg;
        admin_cfg.token   = "test-admin-feat-disabled";
        admin_cfg.user_id = "admin";
        admin_cfg.scopes  = {"cdc:read", "cdc:admin"};
        auth_->addToken(admin_cfg);

        // feature_cdc = FALSE
        handler_ = std::make_unique<themis::server::ChangefeedApiHandler>(
            storage_, changefeed_, nullptr, auth_, /*feature_cdc=*/false);
    }

    void TearDown() override {
        handler_.reset();
        changefeed_.reset();
        if (storage_) {
            storage_->close();
        }
        std::filesystem::remove_all(db_path_);
    }

    std::string                                             db_path_;
    std::shared_ptr<themis::RocksDBWrapper>                 storage_;
    std::shared_ptr<themis::Changefeed>                     changefeed_;
    std::shared_ptr<themis::AuthMiddleware>                 auth_;
    std::unique_ptr<themis::server::ChangefeedApiHandler>   handler_;
};

TEST_F(CdcFeatureDisabledAuthTest, Get_FeatureDisabled_NoAuth_Returns401NotFound) {
    // Auth MUST be enforced before the feature-flag check.
    // An unauthenticated request must get 401, not 404.
    auto req = makeGet("/changefeed?from_seq=0");
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "Expected 401 (auth enforced first) but got "
        << static_cast<int>(res.result_int())
        << " — feature flag must not be checked before auth";
}

TEST_F(CdcFeatureDisabledAuthTest, Stream_FeatureDisabled_NoAuth_Returns401NotFound) {
    auto req = makeGet("/changefeed/stream?from_seq=0");
    auto res = handler_->handleStreamSse(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcFeatureDisabledAuthTest, Stats_FeatureDisabled_NoAuth_Returns401NotFound) {
    auto req = makeGet("/changefeed/stats");
    auto res = handler_->handleStats(req);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(CdcFeatureDisabledAuthTest, Get_FeatureDisabled_AuthorizedUser_Returns404) {
    // Authorized user sees 404 when feature is disabled — that is correct and expected.
    auto req = makeGet("/changefeed?from_seq=0",
                       std::string("Bearer test-admin-feat-disabled"));
    auto res = handler_->handleGet(req);
    EXPECT_EQ(res.result(), http::status::not_found);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 10: Data leakage — key_prefix subscription filter
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_OnlyReturnsMatchingEvents) {
    // Insert events under two different key namespaces
    recordEvent("orders:001");
    recordEvent("orders:002");
    recordEvent("users:alice");
    recordEvent("users:bob");

    // Subscribe to "orders:" prefix only
    auto req = makeGet("/changefeed?from_seq=0&limit=100&key_prefix=orders:",
                       bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::ok);

    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("events"));
    const auto& events = body["events"];
    EXPECT_FALSE(events.empty());

    // Every returned event must start with "orders:"
    for (const auto& ev : events) {
        std::string key = ev.value("key", std::string{});
        EXPECT_EQ(key.rfind("orders:", 0), 0u)
            << "Data leakage: event with key '" << key
            << "' outside subscribed prefix 'orders:' was returned";
    }
}

TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_NoEventsOutsidePrefix) {
    // Insert events under two different key namespaces
    recordEvent("invoices:100");
    recordEvent("invoices:101");
    recordEvent("accounts:A1");

    // Subscribe to "invoices:" prefix
    auto req = makeGet("/changefeed?from_seq=0&limit=100&key_prefix=invoices:",
                       bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::ok);

    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("events"));

    for (const auto& ev : body["events"]) {
        std::string key = ev.value("key", std::string{});
        EXPECT_NE(key.rfind("accounts:", 0), 0u)
            << "Data leakage: 'accounts:' event leaked through 'invoices:' subscription";
    }
}

TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_EmptyPrefix_ReturnsAllEvents) {
    recordEvent("ns1:key1");
    recordEvent("ns2:key2");

    // No key_prefix filter → all events returned
    auto req = makeGet("/changefeed?from_seq=0&limit=100", bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::ok);

    json body = json::parse(res.body());
    EXPECT_GE(body["events"].size(), 2u);
}

TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_NonMatchingPrefix_ReturnsEmpty) {
    recordEvent("product:001");
    recordEvent("product:002");

    // Subscribe to a prefix that has no events
    auto req = makeGet("/changefeed?from_seq=0&limit=100&key_prefix=cart:",
                       bearer(TOKEN_READ));
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::ok);

    json body = json::parse(res.body());
    EXPECT_TRUE(body["events"].empty())
        << "Expected no events for 'cart:' prefix but got: " << body["events"].dump();
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 11: Verify response body on auth-denied requests
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(CdcSubscriptionAuthTest, AuthDenied_ResponseBody_ContainsErrorField) {
    // A well-formed error body must be returned so clients can diagnose
    auto req = makeGet("/changefeed?from_seq=0&limit=10", bearer(TOKEN_NOSCOPE));
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::forbidden);

    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("error"))
        << "403 response must include 'error' field; got: " << res.body();
    EXPECT_TRUE(body.contains("message"))
        << "403 response must include 'message' field; got: " << res.body();
}

TEST_F(CdcSubscriptionAuthTest, Unauthorized_ResponseBody_ContainsErrorField) {
    auto req = makeGet("/changefeed?from_seq=0&limit=10");
    auto res = handler_->handleGet(req);
    ASSERT_EQ(res.result(), http::status::unauthorized);

    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("error"))
        << "401 response must include 'error' field; got: " << res.body();
}
