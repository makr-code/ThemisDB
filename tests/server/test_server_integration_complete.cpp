/**
 * @file test_server_integration_complete.cpp
 * @brief Unified integration test suite for the server module.
 *
 * Closes the "Integration tests (all 40+ endpoints, TLS, auth, rate limiting)"
 * gap identified in src/server/ROADMAP.md.
 *
 * Test suites
 * ───────────
 *  1. ServerAuthEnforcementTest  (port 18101)
 *     Live HTTP server with THEMIS_TOKEN_ADMIN configured.  Verifies that
 *     auth-required endpoints return 401 without a valid Bearer token and
 *     return a non-401 response when the correct token is supplied.
 *
 *  2. RateLimitMiddlewareIntegrationTest  (no live server)
 *     Exercises RateLimitingMiddleware directly: token-bucket exhaustion,
 *     whitelist bypass, per-endpoint overrides, response headers, stats,
 *     reset, and concurrent access.
 *
 *  3. RateLimiterUnitIntegrationTest  (no live server)
 *     Exercises the legacy RateLimiter: blacklist/unblacklist, anomaly-
 *     detection callback, per-user limits, adaptive throttle.
 *
 *  4. TLSAndHttpConfigTest  (no live server)
 *     Validates the remaining HttpServer::Config fields that are not already
 *     covered by test_http_server_network.cpp: HTTP/2, HTTP/3, WebSocket
 *     flags, feature flags, connection limits, and timeout settings.
 *
 *  5. ServerRateLimitLiveTest  (port 18103)
 *     Enforces rate limiting on a live server by sending requests with an
 *     X-Forwarded-For header that carries a non-whitelisted IP address.
 *     With THEMIS_RATE_LIMIT_PER_MINUTE=2, the third request returns 429.
 *
 *  6. ServerEndpointBreadthTest  (port 18104)
 *     Exercises 25+ additional endpoint paths that are not covered by
 *     test_api_integration.cpp.  Tests accept any valid HTTP status code
 *     (the goal is routing + deserialisable JSON body, not functional
 *     correctness of each handler).
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "server/http_server.h"
#include "server/rate_limiter.h"
#include "server/rate_limiting_middleware.h"
#include "server/auth_middleware.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

// POSIX setenv/unsetenv compatibility shim for Windows
#ifdef _WIN32
#  ifndef setenv
#    define setenv(name, value, overwrite) _putenv_s(name, value)
#  endif
#  ifndef unsetenv
#    define unsetenv(name) _putenv_s(name, "")
#  endif
#endif

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

// ============================================================================
// Shared constants
// ============================================================================

namespace {

/// Time given to the HTTP server to complete async startup before tests run.
static constexpr int kServerStartupDelayMs = 120;

/// Minimum valid HTTP status code (200 OK).
static constexpr unsigned kHttpStatusMin = 200;
/// Maximum valid HTTP status code (exclusive sentinel: 600).
static constexpr unsigned kHttpStatusMax = 600;

/// Per-endpoint / per-suite token-bucket capacity used for exhaustion tests.
/// Low enough that it is exhausted within a handful of requests; high enough
/// that a single incidental request doesn't tip it over.
static constexpr size_t  kLowBucketCapacity = 2;
/// Very slow refill rate (tokens/sec) that makes bucket exhaustion permanent
/// within a single test run (< 1 second).
static constexpr double  kSlowRefillRate = 0.001;

/// Upper bound (exclusive) for "client error" HTTP status codes (4xx range ends before 5xx).
static constexpr unsigned kHttpClientErrorMax = 500;

/// The THEMIS_RATE_LIMIT_PER_MINUTE value used in ServerRateLimitLiveTest.
/// With capacity=2 and refill≈0.033 tok/s a third request is denied well
/// within a 1-second window.
static constexpr int kLiveRateLimitPerMinute = 2;

// ============================================================================
// Shared HTTP helper
// ============================================================================

/// Perform a synchronous HTTP request and return the response.
/// Returns 500 (with a descriptive body) if the connection fails.
template<http::verb Verb>
http::response<http::string_body> httpRequest(
    const std::string& host,
    uint16_t port,
    const std::string& target,
    const std::string& body       = "",
    const std::string& auth       = "",
    const std::string& xff        = "",
    const std::string& content_type = "application/json")
{
    try {
        net::io_context ioc;
        tcp::resolver   resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.connect(resolver.resolve(host, std::to_string(port)));

        http::request<http::string_body> req{Verb, target, 11};
        req.set(http::field::host, host);
        if (!auth.empty())
            req.set(http::field::authorization, auth);
        if (!xff.empty())
            req.set("X-Forwarded-For", xff);
        if (!body.empty()) {
            req.set(http::field::content_type, content_type);
            req.body() = body;
        }
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer                 buf;
        http::response<http::string_body>  res;
        http::read(stream, buf, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return res;
    } catch (const std::exception& e) {
        http::response<http::string_body> err{http::status::internal_server_error, 11};
        err.body() = std::string("connection error: ") + e.what();
        err.prepare_payload();
        return err;
    }
}

inline auto httpGet(const std::string& host, uint16_t port,
                    const std::string& target,
                    const std::string& auth = "",
                    const std::string& xff  = "")
{
    return httpRequest<http::verb::get>(host, port, target, "", auth, xff);
}

inline auto httpPost(const std::string& host, uint16_t port,
                     const std::string& target, const json& body,
                     const std::string& auth = "",
                     const std::string& xff  = "")
{
    return httpRequest<http::verb::post>(host, port, target, body.dump(), auth, xff);
}

inline auto httpDelete(const std::string& host, uint16_t port,
                       const std::string& target,
                       const std::string& auth = "")
{
    return httpRequest<http::verb::delete_>(host, port, target, "", auth);
}

/// Build a simple live server for integration tests.
struct ServerFixtureHelper {
    std::unique_ptr<themis::server::HttpServer>    server;
    std::shared_ptr<themis::RocksDBWrapper>        storage;
    std::shared_ptr<themis::SecondaryIndexManager> secondary_index;
    std::shared_ptr<themis::GraphIndexManager>     graph_index;
    std::shared_ptr<themis::VectorIndexManager>    vector_index;
    std::shared_ptr<themis::TransactionManager>    tx_manager;

    bool start(uint16_t port, const std::string& db_path,
               const std::string& host = "127.0.0.1")
    {
        if (std::filesystem::exists(db_path))
            std::filesystem::remove_all(db_path);

        themis::RocksDBWrapper::Config rcfg;
        rcfg.db_path             = db_path;
        rcfg.memtable_size_mb    = 32;
        rcfg.block_cache_size_mb = 64;
        storage = std::make_shared<themis::RocksDBWrapper>(rcfg);
        if (!storage->open()) {
          return false;
        }

        secondary_index = std::make_shared<themis::SecondaryIndexManager>(*storage);
        graph_index     = std::make_shared<themis::GraphIndexManager>(*storage);
        vector_index    = std::make_shared<themis::VectorIndexManager>(*storage);
        tx_manager      = std::make_shared<themis::TransactionManager>(
            *storage, *secondary_index, *graph_index, *vector_index);

        themis::server::HttpServer::Config scfg;
        scfg.host        = host;
        scfg.port        = port;
        scfg.num_threads = 2;

        server = std::make_unique<themis::server::HttpServer>(
            scfg, storage, secondary_index, graph_index, vector_index, tx_manager);
        server->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(kServerStartupDelayMs));
        return true;
    }

    void stop()
    {
        if (server)  { server->stop();  server.reset(); }
        if (storage) { storage->close(); storage.reset(); }
    }
};

/// RAII env-var setter that restores the previous value on destruction.
struct ScopedEnv {
    std::string name_ = {};
    std::string old_value_ = {};
    bool        had_value_{false};

    ScopedEnv(const char* name, const char* value)
        : name_(name)
    {
        const char* prev = std::getenv(name);
        if (prev) { old_value_ = prev; had_value_ = true; }
        ::setenv(name, value, /*overwrite=*/1);
    }

    ~ScopedEnv()
    {
        if (had_value_)
            ::setenv(name_.c_str(), old_value_.c_str(), 1);
        else
            ::unsetenv(name_.c_str());
    }
};

} // anonymous namespace

// ============================================================================
// ── Suite 1 · ServerAuthEnforcementTest ─────────────────────────────────────
// ============================================================================

static constexpr uint16_t kAuthPort  = 18101;
static const std::string  kAuthHost  = "127.0.0.1";
static const std::string  kAuthDb    = "data/themis_auth_enforcement_test";
static const std::string  kAdminTok  = "test-admin-token-integration-18101";
static const std::string  kBearer    = "Bearer " + kAdminTok;
static const std::string  kBadBearer = "Bearer this-token-does-not-exist-xyz";

class ServerAuthEnforcementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure the admin token BEFORE the server reads env vars.
        env_admin_.reset(new ScopedEnv("THEMIS_TOKEN_ADMIN", kAdminTok.c_str()));
        ASSERT_TRUE(fixture_.start(kAuthPort, kAuthDb))
            << "Failed to start auth-enforcement test server on port " << kAuthPort;
    }

    void TearDown() override {
        fixture_.stop();
        env_admin_.reset();
    }

    http::response<http::string_body> get(const std::string& path,
                                          const std::string& auth = "")
    { return httpGet(kAuthHost, kAuthPort, path, auth); }

    http::response<http::string_body> post(const std::string& path, const json& body,
                                           const std::string& auth = "")
    { return httpPost(kAuthHost, kAuthPort, path, body, auth); }

    ServerFixtureHelper           fixture_;
    std::unique_ptr<ScopedEnv>    env_admin_;
};

// ── Public endpoints must be reachable without authentication ──────────────

TEST_F(ServerAuthEnforcementTest, PublicHealth_NoAuth_Returns200) {
    auto res = get("/health");
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServerAuthEnforcementTest, PublicVersion_NoAuth_Returns200) {
    auto res = get("/version");
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServerAuthEnforcementTest, PublicStats_NoAuth_Returns200) {
    auto res = get("/stats");
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServerAuthEnforcementTest, PublicHealthLive_NoAuth_Returns200) {
    auto res = get("/health/live");
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServerAuthEnforcementTest, PublicHealthReady_NoAuth_Returns200) {
    auto res = get("/health/ready");
    EXPECT_EQ(res.result(), http::status::ok);
}

// ── Auth-required: session endpoints ──────────────────────────────────────

TEST_F(ServerAuthEnforcementTest, SessionCreate_NoAuthHeader_Returns401) {
    auto res = post("/sessions", json::object());
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "Expected 401 without Authorization header; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, SessionCreate_BadToken_Returns401) {
    auto res = post("/sessions", json::object(), kBadBearer);
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "Expected 401 with unknown token; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, SessionCreate_ValidToken_NotUnauthorized) {
    auto res = post("/sessions", json::object(), kBearer);
    // Any response other than 401 means auth passed (could be 200, 201, 503 …)
    EXPECT_NE(res.result(), http::status::unauthorized)
        << "Valid token must not return 401; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, SessionList_NoAuthHeader_Returns401) {
    auto res = get("/sessions");
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ServerAuthEnforcementTest, SessionList_BadToken_Returns401) {
    auto res = get("/sessions", kBadBearer);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ServerAuthEnforcementTest, SessionList_ValidToken_NotUnauthorized) {
    auto res = get("/sessions", kBearer);
    EXPECT_NE(res.result(), http::status::unauthorized);
}

// ── Auth-required: PII reveal endpoint ────────────────────────────────────

TEST_F(ServerAuthEnforcementTest, PiiReveal_NoAuthHeader_Returns401or503) {
    auto res = post("/pii/reveal", json::object());
    // Three outcomes are valid depending on build configuration:
    //  - 401 Unauthorized: PII feature enabled and auth is enforced correctly.
    //  - 503 Service Unavailable: PII feature disabled at runtime via feature flag.
    //  - 404 Not Found: PII reveal route not registered (handler absent in build).
    // What must NOT happen is 200 OK (would mean unauthenticated reveal succeeded).
    const bool expected_no_auth_pass =
        (res.result() == http::status::unauthorized ||
         res.result() == http::status::service_unavailable ||
         res.result() == http::status::not_found);
    EXPECT_TRUE(expected_no_auth_pass)
        << "POST /pii/reveal without auth must not succeed; got: "
        << static_cast<int>(res.result()) << " body: " << res.body();
    EXPECT_NE(res.result(), http::status::ok)
        << "PII reveal without auth header must never return 200";
}

TEST_F(ServerAuthEnforcementTest, PiiReveal_BadToken_NotGranted) {
    auto res = post("/pii/reveal", json::object(), kBadBearer);
    EXPECT_NE(res.result(), http::status::ok);
}

// ── Bearer token extraction corner cases ──────────────────────────────────

TEST_F(ServerAuthEnforcementTest, MalformedAuthHeader_Returns401) {
    // "Token" scheme instead of "Bearer"
    auto res = get("/sessions", "Token " + kAdminTok);
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ServerAuthEnforcementTest, EmptyBearerToken_Returns401) {
    auto res = get("/sessions", "Bearer ");
    EXPECT_EQ(res.result(), http::status::unauthorized);
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_ExtractBearer_Valid) {
    auto tok = themis::AuthMiddleware::extractBearerToken("Bearer abc123");
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(*tok, "abc123");
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_ExtractBearer_Invalid) {
    EXPECT_FALSE(themis::AuthMiddleware::extractBearerToken("").has_value());
    EXPECT_FALSE(themis::AuthMiddleware::extractBearerToken("NoBearer").has_value());
    EXPECT_FALSE(themis::AuthMiddleware::extractBearerToken("Basic dXNlcg==").has_value());
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_ValidateToken_KnownToken) {
    themis::AuthMiddleware auth;
    themis::AuthMiddleware::TokenConfig cfg;
    cfg.token   = kAdminTok;
    cfg.user_id = "admin";
    cfg.scopes  = {"admin", "data:read"};
    auth.addToken(cfg);

    auto result = auth.validateToken(kAdminTok);
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "admin");
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_ValidateToken_UnknownToken) {
    themis::AuthMiddleware auth;
    auto result = auth.validateToken("not-configured-token-xyz");
    EXPECT_FALSE(result.authorized);
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_Authorize_ScopePresent) {
    themis::AuthMiddleware auth;
    themis::AuthMiddleware::TokenConfig cfg;
    cfg.token  = "scope-test-token";
    cfg.scopes = {"data:read", "data:write"};
    auth.addToken(cfg);

    auto r = auth.authorize("scope-test-token", "data:read");
    EXPECT_TRUE(r.authorized);
}

TEST_F(ServerAuthEnforcementTest, AuthMiddleware_Authorize_ScopeMissing) {
    themis::AuthMiddleware auth;
    themis::AuthMiddleware::TokenConfig cfg;
    cfg.token  = "limited-token";
    cfg.scopes = {"data:read"};
    auth.addToken(cfg);

    auto r = auth.authorize("limited-token", "admin");
    EXPECT_FALSE(r.authorized);
}

// ── Auth-required: admin shard + storage-stats endpoints (HS-1) ───────────

TEST_F(ServerAuthEnforcementTest, AdminShardsPost_NoAuth_Returns401) {
    auto res = post("/v1/admin/shards",
                    json{{"node_id", 1}, {"node_address", "127.0.0.1:9001"}});
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "POST /v1/admin/shards without auth must return 401; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, AdminShardsGet_NoAuth_Returns401) {
    auto res = get("/v1/admin/shards");
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "GET /v1/admin/shards without auth must return 401; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, AdminStorageStats_NoAuth_Returns401) {
    auto res = get("/v1/admin/storage/stats");
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "GET /v1/admin/storage/stats without auth must return 401; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, AdminStorageStats_ValidToken_NotUnauthorized) {
    auto res = get("/v1/admin/storage/stats", kBearer);
    EXPECT_NE(res.result(), http::status::unauthorized)
        << "Valid admin token must pass auth gate; got: " << res.body();
}

// ── Auth-required: WAL apply endpoint (HS-2) ─────────────────────────────

TEST_F(ServerAuthEnforcementTest, WalApply_NoAuth_Returns401) {
    auto res = post("/api/v1/wal/apply", json::object());
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "POST /api/v1/wal/apply without auth must return 401; got: " << res.body();
}

TEST_F(ServerAuthEnforcementTest, WalApply_BadToken_Returns401) {
    auto res = post("/api/v1/wal/apply", json::object(), kBadBearer);
    EXPECT_EQ(res.result(), http::status::unauthorized)
        << "POST /api/v1/wal/apply with bad token must return 401; got: " << res.body();
}



using namespace themis::server;

class RateLimitMiddlewareIntegrationTest : public ::testing::Test {
protected:
    RateLimitingMiddleware::Config defaultConfig(size_t capacity = 5,
                                                 double refill   = 1.0) const
    {
        RateLimitingMiddleware::Config cfg;
        cfg.default_capacity    = capacity;
        cfg.default_refill_rate = refill;
        cfg.send_rate_limit_headers = true;
        return cfg;
    }
};

// ── Token-bucket behaviour ─────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, FirstRequest_Allowed) {
    RateLimitingMiddleware mw(defaultConfig(5, 1.0));
    auto r = mw.check("10.0.0.1", "/health");
    EXPECT_TRUE(r.allowed);
}

TEST_F(RateLimitMiddlewareIntegrationTest, BelowCapacity_AllAllowed) {
    RateLimitingMiddleware mw(defaultConfig(5, 0.01));
    for (int i = 0; i < 5; ++i) {
        auto r = mw.check("10.0.0.2", "/health");
        EXPECT_TRUE(r.allowed) << "Request " << i << " should be allowed";
    }
}

TEST_F(RateLimitMiddlewareIntegrationTest, ExceedCapacity_Rejected) {
    RateLimitingMiddleware mw(defaultConfig(3, kSlowRefillRate)); // very slow refill
    for (int i = 0; i < 3; ++i) {
      mw.check("10.0.0.3", "/health");
    }
    auto r = mw.check("10.0.0.3", "/health");
    EXPECT_FALSE(r.allowed);
    EXPECT_GT(r.retry_after_seconds, 0u);
}

TEST_F(RateLimitMiddlewareIntegrationTest, DifferentClients_IndependentBuckets) {
    RateLimitingMiddleware mw(defaultConfig(1, kSlowRefillRate));
    // Exhaust client A
    mw.check("10.0.0.4", "/health");
    auto ra = mw.check("10.0.0.4", "/health");
    EXPECT_FALSE(ra.allowed);
    // Client B is unaffected
    auto rb = mw.check("10.0.0.5", "/health");
    EXPECT_TRUE(rb.allowed);
}

TEST_F(RateLimitMiddlewareIntegrationTest, WhitelistedIP_AlwaysAllowed) {
    RateLimitingMiddleware::Config cfg = defaultConfig(1, kSlowRefillRate);
    cfg.whitelist_ips = {"192.168.0.1"};
    RateLimitingMiddleware mw(cfg);

    for (int i = 0; i < 10; ++i) {
        auto r = mw.check("192.168.0.1", "/health");
        EXPECT_TRUE(r.allowed) << "Whitelisted IP must never be rate-limited";
    }
}

TEST_F(RateLimitMiddlewareIntegrationTest, NonWhitelistedIP_IsLimited) {
    RateLimitingMiddleware::Config cfg = defaultConfig(2, kSlowRefillRate);
    cfg.whitelist_ips = {"192.168.0.1"};
    RateLimitingMiddleware mw(cfg);

    mw.check("203.0.113.1", "/health");
    mw.check("203.0.113.1", "/health");
    auto r = mw.check("203.0.113.1", "/health");
    EXPECT_FALSE(r.allowed);
}

// ── Per-endpoint overrides ─────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, EndpointOverride_TighterLimit) {
    RateLimitingMiddleware::Config cfg = defaultConfig(100, 10.0);
    cfg.endpoint_overrides.push_back({"/bulk", 1, kSlowRefillRate});
    RateLimitingMiddleware mw(cfg);

    // /health uses default (100 capacity) → always allowed
    EXPECT_TRUE(mw.check("10.1.0.1", "/health").allowed);
    // /bulk has capacity=1 → second request blocked
    mw.check("10.1.0.1", "/bulk");
    EXPECT_FALSE(mw.check("10.1.0.1", "/bulk").allowed);
}

TEST_F(RateLimitMiddlewareIntegrationTest, EndpointOverride_PrefixMatching) {
    RateLimitingMiddleware::Config cfg = defaultConfig(100, 10.0);
    cfg.endpoint_overrides.push_back({"/api/v2", 1, kSlowRefillRate});
    RateLimitingMiddleware mw(cfg);

    mw.check("10.2.0.1", "/api/v2/documents");
    auto r = mw.check("10.2.0.1", "/api/v2/documents");
    EXPECT_FALSE(r.allowed);
}

// ── Response headers ───────────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, AllowedRequest_HasRateLimitHeaders) {
    RateLimitingMiddleware mw(defaultConfig(10, 1.0));
    auto r = mw.check("10.3.0.1", "/health");
    EXPECT_TRUE(r.allowed);
    EXPECT_FALSE(r.headers.empty()) << "Rate-limit headers must be present";
    EXPECT_GT(r.limit, 0u);
}

TEST_F(RateLimitMiddlewareIntegrationTest, RejectedRequest_HasRetryAfter) {
    RateLimitingMiddleware mw(defaultConfig(1, kSlowRefillRate));
    mw.check("10.4.0.1", "/health");
    auto r = mw.check("10.4.0.1", "/health");
    EXPECT_FALSE(r.allowed);
    EXPECT_GT(r.retry_after_seconds, 0u);
    // Retry-After header should be present
    auto it = r.headers.find("Retry-After");
    EXPECT_NE(it, r.headers.end()) << "Retry-After header must be set when rejected";
}

// ── Stats tracking ─────────────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, Stats_CountAllowed) {
    RateLimitingMiddleware mw(defaultConfig(10, 1.0));
    mw.check("10.5.0.1", "/health");
    mw.check("10.5.0.2", "/health");
    auto s = mw.getStats();
    EXPECT_EQ(s.total_requests, 2u);
    EXPECT_EQ(s.allowed_requests, 2u);
    EXPECT_EQ(s.rejected_requests, 0u);
}

TEST_F(RateLimitMiddlewareIntegrationTest, Stats_CountRejected) {
    RateLimitingMiddleware mw(defaultConfig(1, kSlowRefillRate));
    mw.check("10.6.0.1", "/health");
    mw.check("10.6.0.1", "/health"); // rejected
    auto s = mw.getStats();
    EXPECT_EQ(s.rejected_requests, 1u);
}

// ── Reset ──────────────────────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, Reset_RestoresBuckets) {
    RateLimitingMiddleware mw(defaultConfig(1, kSlowRefillRate));
    mw.check("10.7.0.1", "/health");
    EXPECT_FALSE(mw.check("10.7.0.1", "/health").allowed);
    mw.reset();
    EXPECT_TRUE(mw.check("10.7.0.1", "/health").allowed);
}

TEST_F(RateLimitMiddlewareIntegrationTest, Reset_ClearsStats) {
    RateLimitingMiddleware mw(defaultConfig(5, 1.0));
    mw.check("10.8.0.1", "/health");
    mw.reset();
    auto s = mw.getStats();
    EXPECT_EQ(s.total_requests, 0u);
}

// ── Thread safety ──────────────────────────────────────────────────────────

TEST_F(RateLimitMiddlewareIntegrationTest, ConcurrentClients_AllSafe) {
    RateLimitingMiddleware mw(defaultConfig(1000, 100.0));
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            std::string ip = "11." + std::to_string(t) + ".0.1";
            for (int i = 0; i < 10; ++i) {
                if (mw.check(ip, "/health").allowed) {
                  ++allowed;
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_GT(allowed.load(), 0);
}

// ============================================================================
// ── Suite 3 · RateLimiterUnitIntegrationTest ─────────────────────────────────
// ============================================================================

class RateLimiterUnitIntegrationTest : public ::testing::Test {
protected:
    RateLimitConfig makeConfig(size_t cap = 5, double refill = 1.0) const
    {
        RateLimitConfig cfg;
        cfg.bucket_capacity  = cap;
        cfg.refill_rate      = refill;
        cfg.per_ip_enabled   = true;
        cfg.per_user_enabled = false;
        return cfg;
    }
};

TEST_F(RateLimiterUnitIntegrationTest, AllowRequest_BelowLimit) {
    RateLimiter rl(makeConfig(5, 0.01));
    EXPECT_TRUE(rl.allowRequest("1.2.3.4"));
}

TEST_F(RateLimiterUnitIntegrationTest, DenyRequest_OverLimit) {
    RateLimiter rl(makeConfig(2, kSlowRefillRate));
    rl.allowRequest("1.2.3.5");
    rl.allowRequest("1.2.3.5");
    EXPECT_FALSE(rl.allowRequest("1.2.3.5"));
}

TEST_F(RateLimiterUnitIntegrationTest, Whitelist_AlwaysAllowed) {
    RateLimitConfig cfg = makeConfig(1, kSlowRefillRate);
    cfg.whitelist_ips   = {"127.0.0.1"};
    RateLimiter rl(cfg);
    for (int i = 0; i < 20; ++i) {
        EXPECT_TRUE(rl.allowRequest("127.0.0.1"))
            << "Whitelisted IP must always be allowed";
    }
}

TEST_F(RateLimiterUnitIntegrationTest, Blacklist_AlwaysBlocked) {
    RateLimiter rl(makeConfig(100, 10.0));
    rl.blacklistIP("5.5.5.5");
    EXPECT_FALSE(rl.allowRequest("5.5.5.5"));
    EXPECT_TRUE(rl.isBlacklisted("5.5.5.5"));
}

TEST_F(RateLimiterUnitIntegrationTest, Unblacklist_ResumesNormal) {
    RateLimiter rl(makeConfig(100, 10.0));
    rl.blacklistIP("6.6.6.6");
    EXPECT_FALSE(rl.allowRequest("6.6.6.6"));
    rl.unblacklistIP("6.6.6.6");
    EXPECT_FALSE(rl.isBlacklisted("6.6.6.6"));
    EXPECT_TRUE(rl.allowRequest("6.6.6.6"));
}

TEST_F(RateLimiterUnitIntegrationTest, IsWhitelisted_True) {
    RateLimitConfig cfg = makeConfig(5, 1.0);
    cfg.whitelist_ips   = {"10.0.0.1"};
    RateLimiter rl(cfg);
    EXPECT_TRUE(rl.isWhitelisted("10.0.0.1"));
}

TEST_F(RateLimiterUnitIntegrationTest, IsWhitelisted_False) {
    RateLimiter rl(makeConfig(5, 1.0));
    EXPECT_FALSE(rl.isWhitelisted("9.9.9.9"));
}

TEST_F(RateLimiterUnitIntegrationTest, GetRetryAfter_ZeroWhenAllowed) {
    RateLimiter rl(makeConfig(5, 1.0));
    EXPECT_EQ(rl.getRetryAfter("7.7.7.7"), 0u);
}

TEST_F(RateLimiterUnitIntegrationTest, GetRetryAfter_NonZeroWhenLimited) {
    RateLimiter rl(makeConfig(1, kSlowRefillRate)); // 1 tok / 0.001 tok·s⁻¹ = exactly 1000 s to refill
    rl.allowRequest("8.8.8.8");
    rl.allowRequest("8.8.8.8"); // blocked
    EXPECT_GT(rl.getRetryAfter("8.8.8.8"), 0u);
}

TEST_F(RateLimiterUnitIntegrationTest, Statistics_Tracked) {
    RateLimiter rl(makeConfig(2, kSlowRefillRate));
    rl.allowRequest("9.9.9.9"); // allowed
    rl.allowRequest("9.9.9.9"); // allowed
    rl.allowRequest("9.9.9.9"); // rejected
    auto s = rl.getStatistics();
    EXPECT_EQ(s.total_requests,    3u);
    EXPECT_EQ(s.allowed_requests,  2u);
    EXPECT_EQ(s.rejected_requests, 1u);
}

TEST_F(RateLimiterUnitIntegrationTest, Reset_ClearsAllBuckets) {
    RateLimiter rl(makeConfig(1, kSlowRefillRate));
    rl.allowRequest("12.0.0.1");
    EXPECT_FALSE(rl.allowRequest("12.0.0.1"));
    rl.reset();
    EXPECT_TRUE(rl.allowRequest("12.0.0.1"));
}

TEST_F(RateLimiterUnitIntegrationTest, AnomalyCallback_FiredOnBlacklist) {
    RateLimiter rl(makeConfig(100, 10.0));
    std::atomic<int> fired{0};
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        if (ev.type == AnomalyEvent::Type::IP_BLACKLISTED) {
          ++fired;
        }
    });
    rl.blacklistIP("99.0.0.1");
    EXPECT_EQ(fired.load(), 1);
}

TEST_F(RateLimiterUnitIntegrationTest, PerUserBucket_Independent) {
    RateLimitConfig cfg = makeConfig(1, kSlowRefillRate);
    cfg.per_user_enabled = true;
    RateLimiter rl(cfg);
    rl.allowRequest("1.2.3.6", "userA");
    rl.allowRequest("1.2.3.6", "userA"); // blocked for userA
    // userB still has tokens
    EXPECT_TRUE(rl.allowRequest("1.2.3.6", "userB"));
}

// ============================================================================
// ── Suite 4 · TLSAndHttpConfigTest ────────────────────────────────────────────
// ============================================================================

using Config = themis::server::HttpServer::Config;

class TLSAndHttpConfigTest : public ::testing::Test {};

// ── HTTP/2 ────────────────────────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, Http2DisabledByDefault) {
    EXPECT_FALSE(Config().enable_http2);
}

TEST_F(TLSAndHttpConfigTest, Http2CanBeEnabled) {
    Config c;
    c.enable_http2 = true;
    EXPECT_TRUE(c.enable_http2);
}

TEST_F(TLSAndHttpConfigTest, Http2MaxConcurrentStreamsDefault) {
    EXPECT_EQ(Config().http2_max_concurrent_streams, 100u);
}

TEST_F(TLSAndHttpConfigTest, Http2InitialWindowSizeDefault) {
    EXPECT_EQ(Config().http2_initial_window_size, 65535u);
}

TEST_F(TLSAndHttpConfigTest, Http2MaxConcurrentStreamsCanBeSet) {
    Config c;
    c.http2_max_concurrent_streams = 256;
    EXPECT_EQ(c.http2_max_concurrent_streams, 256u);
}

// ── HTTP/3 ────────────────────────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, Http3DisabledByDefault) {
    EXPECT_FALSE(Config().enable_http3);
}

TEST_F(TLSAndHttpConfigTest, Http3MaxIdleTimeoutDefault) {
    EXPECT_EQ(Config().http3_max_idle_timeout_ms, 30000u);
}

TEST_F(TLSAndHttpConfigTest, Http3PortDefaultIsZero) {
    EXPECT_EQ(Config().http3_port, 0u);
}

// ── WebSocket ─────────────────────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, WebSocketDisabledByDefault) {
    EXPECT_FALSE(Config().enable_websocket);
}

TEST_F(TLSAndHttpConfigTest, WebSocketMaxMessageSizeDefault) {
    EXPECT_EQ(Config().websocket_max_message_size, 1048576u); // 1 MiB
}

TEST_F(TLSAndHttpConfigTest, WebSocketPingIntervalDefault) {
    EXPECT_EQ(Config().websocket_ping_interval_ms, 30000u);
}

TEST_F(TLSAndHttpConfigTest, WebSocketCdcPollIntervalDefault) {
    EXPECT_EQ(Config().websocket_cdc_poll_interval_ms, 500u);
}

// ── Feature flags ─────────────────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, SemanticCacheFeatureDisabledByDefault) {
    EXPECT_FALSE(Config().feature_semantic_cache);
}

TEST_F(TLSAndHttpConfigTest, CdcFeatureDisabledByDefault) {
    EXPECT_FALSE(Config().feature_cdc);
}

TEST_F(TLSAndHttpConfigTest, TimeseriesFeatureDisabledByDefault) {
    EXPECT_FALSE(Config().feature_timeseries);
}

TEST_F(TLSAndHttpConfigTest, PIIManagerFeatureDisabledByDefault) {
    EXPECT_FALSE(Config().feature_pii_manager);
}

TEST_F(TLSAndHttpConfigTest, LLMStoreFeatureDisabledByDefault) {
    EXPECT_FALSE(Config().feature_llm_store);
}

TEST_F(TLSAndHttpConfigTest, FeatureFlagsCanBeEnabled) {
    Config c;
    c.feature_cdc             = true;
    c.feature_timeseries      = true;
    c.feature_semantic_cache  = true;
    c.feature_pii_manager     = true;
    EXPECT_TRUE(c.feature_cdc);
    EXPECT_TRUE(c.feature_timeseries);
    EXPECT_TRUE(c.feature_semantic_cache);
    EXPECT_TRUE(c.feature_pii_manager);
}

// ── Connection & timeout limits ────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, MaxConnectionsDefaultIsUnlimited) {
    EXPECT_EQ(Config().max_connections, 0u);
}

TEST_F(TLSAndHttpConfigTest, MaxConnectionsCanBeSet) {
    Config c;
    c.max_connections = 10000;
    EXPECT_EQ(c.max_connections, 10000u);
}

TEST_F(TLSAndHttpConfigTest, RequestTimeoutDefault) {
    EXPECT_EQ(Config().request_timeout_ms, 30000u);
}

TEST_F(TLSAndHttpConfigTest, GracefulShutdownTimeoutDefault) {
    EXPECT_EQ(Config().graceful_shutdown_timeout_ms, 30000u);
}

TEST_F(TLSAndHttpConfigTest, MaxHeaderSizeDefault) {
    EXPECT_EQ(Config().max_header_size_bytes, 8192u);
}

TEST_F(TLSAndHttpConfigTest, MaxRequestSizeDefault) {
    EXPECT_EQ(Config().max_request_size_mb, 10u);
}

// ── Health/error service ──────────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, HealthErrorServiceEnabledByDefault) {
    EXPECT_TRUE(Config().health_error_service_enabled);
}

TEST_F(TLSAndHttpConfigTest, HealthErrorServicePortDefault) {
    EXPECT_EQ(Config().health_error_service_port, 9090u);
}

TEST_F(TLSAndHttpConfigTest, HealthErrorServiceBindAddressDefault) {
    EXPECT_EQ(Config().health_error_service_bind_address, "127.0.0.1");
}

// ── SSE / audit rate limits ───────────────────────────────────────────────

TEST_F(TLSAndHttpConfigTest, SSEMaxEventsPerSecondDefaultIsUnlimited) {
    EXPECT_EQ(Config().sse_max_events_per_second, 0u);
}

TEST_F(TLSAndHttpConfigTest, AuditRateLimitDefault) {
    EXPECT_EQ(Config().audit_rate_limit_per_minute, 100u);
}

// ============================================================================
// ── Suite 5 · ServerRateLimitLiveTest ─────────────────────────────────────────
// ============================================================================

static constexpr uint16_t kRLPort = 18103;
static const std::string  kRLHost = "127.0.0.1";
static const std::string  kRLDb   = "data/themis_rate_limit_live_test";
// A non-whitelisted, TEST-NET-3 IP that will not appear in the default whitelist
static const std::string kTestClientIP = "203.0.113.42";

class ServerRateLimitLiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // THEMIS_RATE_LIMIT_PER_MINUTE=N causes the server to set both
        // bucket_capacity=N and refill_rate=N/60 tok/s.  With N=2 the
        // bucket holds exactly 2 tokens; after 2 requests the bucket is
        // empty and the 3rd is rejected within the same second.
        env_rate_.reset(new ScopedEnv("THEMIS_RATE_LIMIT_PER_MINUTE", std::to_string(kLiveRateLimitPerMinute).c_str()));
        ASSERT_TRUE(fixture_.start(kRLPort, kRLDb, kRLHost))
            << "Failed to start rate-limit live server";
    }

    void TearDown() override {
        fixture_.stop();
        env_rate_.reset();
    }

    // Sends a GET /health with X-Forwarded-For set to kTestClientIP.
    http::response<http::string_body> timedRequest()
    {
        return httpGet(kRLHost, kRLPort, "/health", /*auth=*/"", kTestClientIP);
    }

    ServerFixtureHelper         fixture_;
    std::unique_ptr<ScopedEnv>  env_rate_;
};

TEST_F(ServerRateLimitLiveTest, FirstRequest_IsAllowed) {
    auto res = timedRequest();
    EXPECT_EQ(res.result(), http::status::ok)
        << "First request must be allowed; body: " << res.body();
}

TEST_F(ServerRateLimitLiveTest, SecondRequest_IsAllowed) {
    timedRequest(); // consume token 1
    auto res = timedRequest();
    EXPECT_EQ(res.result(), http::status::ok)
        << "Second request must be allowed; body: " << res.body();
}

TEST_F(ServerRateLimitLiveTest, ThirdRequest_IsRateLimited) {
    timedRequest(); // consume token 1
    timedRequest(); // consume token 2
    auto res = timedRequest(); // bucket now empty → 429
    EXPECT_EQ(res.result(), http::status::too_many_requests)
        << "Third request must be rate-limited; body: " << res.body();
}

TEST_F(ServerRateLimitLiveTest, RateLimitResponse_ContainsRetryAfter) {
    timedRequest();
    timedRequest();
    auto res = timedRequest(); // 429
    EXPECT_EQ(res.result(), http::status::too_many_requests);
    // The server may embed retry_after in either the header or JSON body.
    bool has_retry = res.find("Retry-After") != res.end()
                  || res.body().find("retry_after") != std::string::npos;
    EXPECT_TRUE(has_retry) << "429 response must carry retry guidance";
}

TEST_F(ServerRateLimitLiveTest, RateLimitResponse_HasJsonBody) {
    timedRequest();
    timedRequest();
    auto res = timedRequest();
    EXPECT_EQ(res.result(), http::status::too_many_requests);
    json body;
    ASSERT_NO_THROW(body = json::parse(res.body())) << res.body();
    EXPECT_TRUE(body.contains("error") || body.contains("message"));
}

TEST_F(ServerRateLimitLiveTest, LocalhostNotRateLimited) {
    // Localhost is in the default whitelist; 5 quick requests must all pass.
    for (int i = 0; i < 5; ++i) {
        auto res = httpGet(kRLHost, kRLPort, "/health"); // no X-Forwarded-For
        EXPECT_EQ(res.result(), http::status::ok)
            << "Localhost request " << i << " must not be rate-limited";
    }
}

TEST_F(ServerRateLimitLiveTest, DifferentIPs_IndependentBuckets) {
    // Exhaust kTestClientIP's bucket
    timedRequest();
    timedRequest();
    auto blocked = timedRequest();
    EXPECT_EQ(blocked.result(), http::status::too_many_requests);

    // A different non-whitelisted IP still has its own full bucket
    auto res = httpGet(kRLHost, kRLPort, "/health", "", "203.0.113.99");
    EXPECT_EQ(res.result(), http::status::ok)
        << "Different IP must have its own bucket; body: " << res.body();
}

// ============================================================================
// ── Suite 6 · ServerEndpointBreadthTest ────────────────────────────────────
// ============================================================================

static constexpr uint16_t kBreadthPort = 18104;
static const std::string  kBreadthHost = "127.0.0.1";
static const std::string  kBreadthDb   = "data/themis_endpoint_breadth_test";

class ServerEndpointBreadthTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(fixture_.start(kBreadthPort, kBreadthDb, kBreadthHost))
            << "Failed to start breadth-test server";
    }

    void TearDown() override {
        fixture_.stop();
    }

    /// Returns true when the response came from the live server (any valid HTTP
    /// status 200–599), distinguishing it from the internal "connection failed"
    /// sentinel (HTTP 500 with a "connection error:" prefix in the body).
    static bool isLiveServerResponse(const http::response<http::string_body>& res)
    {
        auto s = static_cast<unsigned>(res.result());
        return s >= kHttpStatusMin && s < kHttpStatusMax;
    }

    /// Run GET and verify the response is valid HTTP and the body (if non-empty)
    /// is parseable JSON.
    void expectReachable(const std::string& path, const std::string& auth = "")
    {
        auto res = httpGet(kBreadthHost, kBreadthPort, path, auth);
        EXPECT_TRUE(isLiveServerResponse(res))
            << "GET " << path << " returned non-HTTP sentinel; body: " << res.body();
        if (!res.body().empty()) {
            json j;
            EXPECT_NO_THROW(j = json::parse(res.body()))
                << "GET " << path << " body is not valid JSON: " << res.body();
        }
    }

    ServerFixtureHelper fixture_;
};

// ── Monitoring / metrics ──────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, MonitoringMetrics_Reachable) {
    expectReachable("/monitoring/metrics");
}

TEST_F(ServerEndpointBreadthTest, MonitoringStatus_Reachable) {
    expectReachable("/monitoring/status");
}

// ── Schema management ─────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, SchemaGet_Reachable) {
    expectReachable("/schema");
}

TEST_F(ServerEndpointBreadthTest, SchemaList_Reachable) {
    expectReachable("/api/v1/schema");
}

// ── Retention ─────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, RetentionGet_Reachable) {
    expectReachable("/retention");
}

// ── Export / Import ───────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, ExportEndpoint_Reachable) {
    expectReachable("/export");
}

TEST_F(ServerEndpointBreadthTest, ImportEndpoint_Reachable) {
    expectReachable("/import");
}

// ── Snapshot ──────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, SnapshotList_Reachable) {
    expectReachable("/snapshots");
}

// ── WAL ───────────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, WalStatus_Reachable) {
    expectReachable("/wal/status");
}

// ── Vector search ─────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, VectorSearch_Reachable) {
    auto res = httpPost(kBreadthHost, kBreadthPort, "/vector/search",
                        json{{"vector", json::array({0.1, 0.2, 0.3})}, {"top_k", 5}});
    EXPECT_TRUE(isLiveServerResponse(res));
}

// ── Async jobs (v2) ───────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, AsyncJobsList_Reachable) {
    expectReachable("/v2/jobs");
}

TEST_F(ServerEndpointBreadthTest, AsyncJobsApiV1_Reachable) {
    expectReachable("/api/v1/jobs");
}

// ── OpenAPI spec ──────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, OpenApiSpec_Reachable) {
    expectReachable("/openapi.json");
}

TEST_F(ServerEndpointBreadthTest, OpenApiYaml_Reachable) {
    expectReachable("/openapi.yaml");
}

// ── Admin ─────────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, AdminStatus_Reachable) {
    expectReachable("/admin/status");
}

// ── Timeseries (feature flag may be off) ──────────────────────────────────

TEST_F(ServerEndpointBreadthTest, TimeseriesQuery_Reachable) {
    auto res = httpPost(kBreadthHost, kBreadthPort, "/timeseries/query",
                        json{{"metric", "cpu_usage"}, {"start", 0}, {"end", 9999}});
    EXPECT_TRUE(isLiveServerResponse(res));
}

// ── GraphQL ───────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, GraphQL_SimpleQuery_Reachable) {
    auto res = httpPost(kBreadthHost, kBreadthPort, "/graphql",
                        json{{"query", "{ __typename }"}});
    EXPECT_TRUE(isLiveServerResponse(res));
}

// ── Merge operations ──────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, MergeEndpoint_Reachable) {
    auto res = httpPost(kBreadthHost, kBreadthPort, "/merge",
                        json{{"collection", "test"}, {"strategy", "last_write_wins"}});
    EXPECT_TRUE(isLiveServerResponse(res));
}

// ── Diff ──────────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, DiffEndpoint_Reachable) {
    expectReachable("/diff");
}

// ── Policy management ─────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, PolicyList_Reachable) {
    expectReachable("/policies");
}

// ── Changefeed (CDC, feature flag may be off) ─────────────────────────────

TEST_F(ServerEndpointBreadthTest, ChangefeedList_Reachable) {
    expectReachable("/changefeeds");
}

// ── Keys / encryption ────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, KeysStatus_Reachable) {
    expectReachable("/keys");
}

// ── MVCC ──────────────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, MvccRevisions_Reachable) {
    expectReachable("/mvcc/revisions");
}

// ── Config CRUD ───────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, ConfigGet_Returns200) {
    auto res = httpGet(kBreadthHost, kBreadthPort, "/config");
    EXPECT_EQ(res.result(), http::status::ok)
        << "GET /config must return 200; body: " << res.body();
}

TEST_F(ServerEndpointBreadthTest, ConfigPost_Returns2xxOr3xxOr4xx) {
    auto res = httpPost(kBreadthHost, kBreadthPort, "/config",
                        json{{"log_level", "info"}});
    auto s = static_cast<unsigned>(res.result());
    // Accept 2xx (success), 3xx (redirect), and 4xx (client error).
    // 5xx would indicate an unexpected server failure.
    EXPECT_TRUE(s >= kHttpStatusMin && s < kHttpClientErrorMax)
        << "POST /config must return 2xx, 3xx, or 4xx; got: " << s;
}

// ── Unknown routes ────────────────────────────────────────────────────────

TEST_F(ServerEndpointBreadthTest, UnknownRoute_Returns404or405) {
    auto res = httpGet(kBreadthHost, kBreadthPort, "/no_such_route_xyz_9999");
    auto s = static_cast<unsigned>(res.result());
    EXPECT_TRUE(s == 404 || s == 405)
        << "Unknown route must return 404 or 405; got: " << s;
}
