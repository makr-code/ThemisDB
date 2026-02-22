#include <gtest/gtest.h>
#include "server/cache_admin_api_handler.h"
#include "cache/adaptive_query_cache.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace http = boost::beast::http;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& body = "")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

// Minimal base64 encode helper (mirrors the decode in the handler)
static std::string base64Encode(const std::string& input) {
    static const char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class CacheAdminApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "/tmp/themis_test_cache_admin_" +
                   std::to_string(std::chrono::system_clock::now()
                                      .time_since_epoch()
                                      .count());

        themis::AdaptiveQueryCache::Config cfg;
        cfg.l3_db_path = db_path_;
        cfg.enable_circuit_breaker = true;
        cfg.enable_tenant_isolation = true;
        cfg.l1_max_entries = 50;
        cfg.l2_max_entries = 100;

        cache_ = std::make_shared<themis::AdaptiveQueryCache>(cfg);
        // auth = nullptr → auth disabled (dev mode)
        handler_ = std::make_unique<themis::server::CacheAdminApiHandler>(
            cache_, nullptr);
    }

    void TearDown() override {
        handler_.reset();
        cache_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<themis::AdaptiveQueryCache> cache_;
    std::unique_ptr<themis::server::CacheAdminApiHandler> handler_;
};

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/stats
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, StatsReturns200WithJsonBody) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");

    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("stats"));
    EXPECT_TRUE(body.contains("circuit_breaker"));
    EXPECT_TRUE(body.contains("health"));
}

TEST_F(CacheAdminApiHandlerTest, StatsCircuitBreakerInitiallyClosed) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_->handleStats(req);

    json body = json::parse(res.body());
    ASSERT_TRUE(body.contains("circuit_breaker"));
    EXPECT_EQ(body["circuit_breaker"]["state"], "CLOSED");
}

// ---------------------------------------------------------------------------
// Tests: GET /v1/admin/cache/circuit-breaker
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, CircuitBreakerStatusReturns200) {
    auto req = makeRequest(http::verb::get, "/v1/admin/cache/circuit-breaker");
    auto res = handler_->handleCircuitBreakerStatus(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_TRUE(body.contains("state"));
    EXPECT_TRUE(body.contains("enabled"));
    EXPECT_EQ(body["state"], "CLOSED");
    EXPECT_TRUE(body["enabled"].get<bool>());
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/circuit-breaker/reset
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, CircuitBreakerResetReturns200) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/circuit-breaker/reset");
    auto res = handler_->handleCircuitBreakerReset(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ok");
    EXPECT_EQ(body["circuit_breaker"]["state"], "CLOSED");
}

// ---------------------------------------------------------------------------
// Tests: DELETE /v1/admin/cache/key/{encoded_key}
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns200ForValidKey) {
    // Pre-populate cache
    std::string fp = cache_->generateFingerprint("SELECT 1", {}, "");
    bool inserted = cache_->put(fp, {}, json::object(), "");
    ASSERT_TRUE(inserted) << "put() must succeed for this test to be meaningful";
    // Verify the entry is actually present before evicting.
    ASSERT_TRUE(cache_->get(fp, "").has_value()) << "entry must be retrievable after put()";

    std::string encoded = base64Encode(fp);
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/" + encoded);
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["key"], fp);
    // At least one entry (L1) must have been evicted.
    EXPECT_GE(body["evicted"].get<int>(), 1);
}

TEST_F(CacheAdminApiHandlerTest, EvictKeyReturns400ForMissingKey) {
    // Target ends exactly at the prefix with no trailing segment
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/");
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// Regression test: evicting by fingerprint must also evict tenant-prefixed
// entries stored under "tenant:{id}:{fingerprint}" in L1/L2.
TEST_F(CacheAdminApiHandlerTest, EvictKeyAlsoEvictsTenantPrefixedEntry) {
    const std::string tenant = "acme";
    std::string fp = cache_->generateFingerprint("SELECT 2", {}, tenant);
    // With enable_tenant_isolation=true the L1 key is "tenant:acme:<fp>".
    bool inserted = cache_->put(fp, {}, json::object(), tenant);
    ASSERT_TRUE(inserted);
    // Verify the entry is actually present before evicting.
    ASSERT_TRUE(cache_->get(fp, tenant).has_value());

    std::string encoded = base64Encode(fp);
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/key/" + encoded);
    auto res = handler_->handleEvictKey(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    // The tenant-scoped L1 entry "tenant:acme:<fp>" must have been evicted.
    EXPECT_GE(body["evicted"].get<int>(), 1);
}

// ---------------------------------------------------------------------------
// Tests: DELETE /v1/admin/cache/tenant/{tenant_id}
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns200) {
    // Pre-populate with a tenant entry
    std::string fp = cache_->generateFingerprint("SELECT 1", {}, "acme");
    cache_->put(fp, {}, json::object(), "acme");

    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/acme");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["tenant_id"], "acme");
    EXPECT_GE(body["evicted"].get<int>(), 0);
}

TEST_F(CacheAdminApiHandlerTest, EvictTenantReturns400ForMissingTenantId) {
    auto req = makeRequest(http::verb::delete_, "/v1/admin/cache/tenant/");
    auto res = handler_->handleEvictTenant(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Tests: null cache guard
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, Returns503WhenCacheIsNull) {
    auto handler_no_cache =
        std::make_unique<themis::server::CacheAdminApiHandler>(nullptr, nullptr);

    auto req = makeRequest(http::verb::get, "/v1/admin/cache/stats");
    auto res = handler_no_cache->handleStats(req);
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/warmup
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForMissingLogPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup", "{}");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns400ForEmptyLogPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":""})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, WarmupReturns500ForNonExistentFile) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":"/tmp/no_such_file_xyz_abc.ndjson"})");
    auto res = handler_->handleWarmup(req);
    EXPECT_EQ(res.result(), http::status::internal_server_error);
}

TEST_F(CacheAdminApiHandlerTest, WarmupLoadsValidEntries) {
    // Write a minimal NDJSON warmup log
    const std::string log_path = db_path_ + "_warmup.ndjson";

    // Use a valid fingerprint (64 hex chars) and a base64-encoded JSON value
    const std::string fp = "a1b2c3d4e5f60718293a4b5c6d7e8f9001234567890abcdef1234567890abcd";
    const std::string value = R"({"rows":[]})";
    // base64("{"rows":[]}") — use our helper
    const std::string value_b64 = base64Encode(value);

    {
        std::ofstream f(log_path);
        ASSERT_TRUE(f.is_open());
        f << R"({"key":")" << fp << R"(","value_b64":")" << value_b64
          << R"(","ttl_remaining_s":300,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ok");
    EXPECT_GE(body["entries_loaded"].get<int>(), 1);
    EXPECT_EQ(body["entries_total"].get<int>(), 1);
}

TEST_F(CacheAdminApiHandlerTest, WarmupSkipsMalformedLines) {
    const std::string log_path = db_path_ + "_warmup_bad.ndjson";
    {
        std::ofstream f(log_path);
        f << "not-json\n";
        f << R"({"key":"tooshort","value_b64":"dGVzdA==","ttl_remaining_s":300})" << '\n';
        // Valid entry
        const std::string fp = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
        f << R"({"key":")" << fp << R"(","value_b64":")" << base64Encode(R"({"x":1})")
          << R"(","ttl_remaining_s":300,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["entries_total"].get<int>(), 3);
    EXPECT_EQ(body["entries_loaded"].get<int>(), 1);
    EXPECT_EQ(body["entries_skipped"].get<int>(), 2);
}

TEST_F(CacheAdminApiHandlerTest, WarmupSkipsExpiredEntries) {
    const std::string log_path = db_path_ + "_warmup_expired.ndjson";
    const std::string fp = "cafebabecafebabecafebabecafebabecafebabecafebabecafebabecafebabe";
    {
        std::ofstream f(log_path);
        // ttl_remaining_s = 0 → already expired
        f << R"({"key":")" << fp << R"(","value_b64":")" << base64Encode(R"({})")
          << R"(","ttl_remaining_s":0,"tenant":""})" << '\n';
    }

    auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                           R"({"log_path":")" + log_path + R"("})");
    auto res = handler_->handleWarmup(req);
    std::filesystem::remove(log_path);

    EXPECT_EQ(res.result(), http::status::ok);
    json body = json::parse(res.body());
    EXPECT_EQ(body["entries_loaded"].get<int>(), 0);
    EXPECT_EQ(body["entries_skipped"].get<int>(), 1);
}

// ---------------------------------------------------------------------------
// Tests: POST /v1/admin/cache/snapshot
// ---------------------------------------------------------------------------

TEST_F(CacheAdminApiHandlerTest, SnapshotReturns400ForMissingOutPath) {
    auto req = makeRequest(http::verb::post, "/v1/admin/cache/snapshot", "{}");
    auto res = handler_->handleSnapshot(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(CacheAdminApiHandlerTest, SnapshotExportsAndReloads) {
    // Insert two entries into the cache
    const std::string fp1 = cache_->generateFingerprint("SELECT 10", {}, "");
    const std::string fp2 = cache_->generateFingerprint("SELECT 11", {}, "");
    ASSERT_TRUE(cache_->put(fp1, {}, json::parse(R"({"result":"r1"})"), ""));
    ASSERT_TRUE(cache_->put(fp2, {}, json::parse(R"({"result":"r2"})"), ""));

    const std::string snap_path = db_path_ + "_snapshot.ndjson";

    // Export
    {
        auto req = makeRequest(http::verb::post, "/v1/admin/cache/snapshot",
                               R"({"out_path":")" + snap_path + R"("})");
        auto res = handler_->handleSnapshot(req);
        EXPECT_EQ(res.result(), http::status::ok);
        json body = json::parse(res.body());
        EXPECT_EQ(body["status"], "ok");
        EXPECT_GE(body["entries_exported"].get<int>(), 2);
    }

    // Clear cache and reload from snapshot
    cache_->clear();
    EXPECT_FALSE(cache_->get(fp1, "").has_value());

    {
        auto req = makeRequest(http::verb::post, "/v1/admin/cache/warmup",
                               R"({"log_path":")" + snap_path + R"("})");
        auto res = handler_->handleWarmup(req);
        EXPECT_EQ(res.result(), http::status::ok);
        json body = json::parse(res.body());
        EXPECT_GE(body["entries_loaded"].get<int>(), 2);
    }

    // Verify entries are back
    EXPECT_TRUE(cache_->get(fp1, "").has_value());
    EXPECT_TRUE(cache_->get(fp2, "").has_value());

    std::filesystem::remove(snap_path);
}
